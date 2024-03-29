//===- lib/core/database.cpp ----------------------------------------------===//
//*      _       _        _                     *
//*   __| | __ _| |_ __ _| |__   __ _ ___  ___  *
//*  / _` |/ _` | __/ _` | '_ \ / _` / __|/ _ \ *
//* | (_| | (_| | || (_| | |_) | (_| \__ \  __/ *
//*  \__,_|\__,_|\__\__,_|_.__/ \__,_|___/\___| *
//*                                             *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file database.cpp
#include "pstore/core/database.hpp"

#include "pstore/core/start_vacuum.hpp"
#include "pstore/core/time.hpp"
#include "pstore/os/path.hpp"

#include "base32.hpp"

namespace {

  pstore::file::range_lock get_vacuum_range_lock (pstore::file::file_base * const file,
                                                  pstore::file::file_base::lock_kind const kind) {
    return {file, sizeof (pstore::header) + offsetof (pstore::lock_block, vacuum_lock),
            sizeof (pstore::lock_block::vacuum_lock), kind};
  }

} // end anonymous namespace

namespace pstore {

  // crc checks enabled
  // ~~~~~~~~~~~~~~~~~~
  bool database::crc_checks_enabled () {
#if PSTORE_CRC_CHECKS_ENABLED
    return true;
#else
    return false;
#endif
  }

  // (ctor)
  // ~~~~~~
  database::database (std::string const & path, access_mode const am,
                      bool const access_tick_enabled)
          : storage_{database::open (path, am)}
          , size_{database::get_footer_pos (*this->file ())} {

    this->finish_init (access_tick_enabled);
  }

  // (dtor)
  // ~~~~~~
  database::~database () noexcept {
    no_ex_escape ([this] () { this->close (); });
  }

  // finish init
  // ~~~~~~~~~~~
  void database::finish_init (bool const access_tick_enabled) {
    (void) access_tick_enabled;

    PSTORE_ASSERT (file ()->is_open ());

    // Build the initial segment address table.
    storage_.update_master_pointers (0);

    trailer::validate (*this, size_.footer_pos ());
    this->protect (address{sizeof (header)}, address{size_.logical_size ()});

    header_ = storage_.address_to_pointer (typed_address<header>::null ());
    sync_name_ = database::build_sync_name (*header_);

    // Put a shared-read lock on the lock_block strcut in the file. We're not going to modify
    // these bytes.
    range_lock_ = get_vacuum_range_lock (this->file (), file::file_handle::lock_kind::shared_read);
    lock_ = std::unique_lock<file::range_lock> (range_lock_);
  }

  // close
  // ~~~~~
  void database::close () {
    if (!closed_) {
      if (modified_ && vacuum_mode_ != vacuum_mode::disabled) {
        start_vacuum (*this);
      }
      closed_ = true;
    }
  }

  // upgrade to write lock
  // ~~~~~~~~~~~~~~~~~~~~~
  std::unique_lock<file::range_lock> * database::upgrade_to_write_lock () {
    // TODO: look at exception-safety in this function.
    lock_.unlock ();
    lock_.release ();
    range_lock_ =
      get_vacuum_range_lock (this->file (), file::file_handle::lock_kind::exclusive_write);
    lock_ = std::unique_lock<file::range_lock> (range_lock_, std::defer_lock);
    return &lock_;
  }

  // clear index cache
  // ~~~~~~~~~~~~~~~~~
  void database::clear_index_cache () {
    for (std::shared_ptr<index::index_base> & index : indices_) {
      index.reset ();
    }
  }

  // older revision footer pos
  // ~~~~~~~~~~~~~~~~~~~~~~~~~
  typed_address<trailer> database::older_revision_footer_pos (unsigned const revision) const {
    if (revision == pstore::head_revision || revision > this->get_current_revision ()) {
      raise (pstore::error_code::unknown_revision);
    }

    // Walk backwards down the linked list of revisions to find it.
    typed_address<trailer> footer_pos = size_.footer_pos ();
    for (;;) {
      auto const tail = this->getro (footer_pos);
      unsigned int const tail_revision = tail->a.generation;
      if (revision > tail_revision) {
        raise (pstore::error_code::unknown_revision);
      }
      if (tail_revision == revision) {
        break;
      }

      // TODO: check that footer_pos and generation number are getting smaller; time must
      // not be getting larger.
      footer_pos = tail->a.prev_generation;
      trailer::validate (*this, footer_pos);
    }

    return footer_pos;
  }

  // sync
  // ~~~~
  void database::sync (unsigned const revision) {
    // If revision <= current revision then we don't need to start at head! We do so if the
    // revision is later than the current region (that's what is_newer is about with footer_pos
    // tracking the current footer as it moves backwards).
    bool is_newer = false;
    typed_address<trailer> footer_pos = size_.footer_pos ();

    if (revision == head_revision) {
      // If we're asked for the head, then we always need a full check to see if there's
      // a newer revision.
      is_newer = true;
    } else {
      unsigned const current_revision = this->get_current_revision ();
      // An early out if the user requests the same revision that we already have.
      if (revision == current_revision) {
        return;
      }
      if (revision > current_revision) {
        is_newer = true;
      }
    }

    // The transactions form a singly linked list with the newest revision at its head.
    // If we're asked for a revision _newer_ that the currently synced number then we need
    // to hunt backwards starting at the head. Syncing to an older revision is simpler because
    // we can work back from the current revision.
    PSTORE_ASSERT (is_newer || footer_pos != typed_address<trailer>::null ());
    if (is_newer) {
      // This atomic read of footer_pos fixes our view of the head-revision. Any transactions
      // after this point won't be seen by this process.
      auto const new_footer_pos = this->getro (typed_address<header>::null ())->footer_pos.load ();

      if (revision == head_revision && new_footer_pos == footer_pos) {
        // We were asked for the head revision but the head turns out to the same
        // as the one to which we're currently synced. The previous early out code didn't
        // have sufficient context to catch this case, but here we do. Nothing more to do.
        return;
      }

      footer_pos = new_footer_pos;

      // Always check file size after loading from the footer.
      auto const file_size = storage_.file ()->size ();

      // Perform a proper footer validity check.
      if (footer_pos.absolute () + sizeof (trailer) > file_size) {
        raise (error_code::footer_corrupt, storage_.file ()->path ());
      }

      // We may need to map additional data from the file. If another process has added
      // data to the store since we opened our connection, then a sync may want to access
      // that new data. Deal with that possibility here.
      storage_.map_bytes (size_.logical_size (), footer_pos.absolute () + sizeof (trailer));

      size_.update_footer_pos (footer_pos);
      trailer::validate (*this, footer_pos);
    }

    // The code above moves to the head revision. If that's what was requested, then we're done.
    // If a specific numbered revision is needed, then we need to search for it.
    if (revision != head_revision) {
      footer_pos = this->older_revision_footer_pos (revision);
    }

    // We must clear the index cache because the current revision has changed.
    this->clear_index_cache ();
    size_.update_footer_pos (footer_pos);
  }

  // build new store [static]
  // ~~~~~~~~~~~~~~~
  void database::build_new_store (file::file_base & file) {
    // Write the inital header, lock block, and footer to the file.
    {
      file.seek (0);

      header header{};
      header.footer_pos = typed_address<trailer>::make (leader_size);
      file.write (header);

      lock_block const lb{};
      file.write (lb);

      PSTORE_ASSERT (header.footer_pos.load () == typed_address<trailer>::make (file.tell ()));

      trailer t{};
      std::fill (std::begin (t.a.index_records), std::end (t.a.index_records),
                 typed_address<index::header_block>::null ());
      t.a.time = milliseconds_since_epoch ();
      t.crc = t.get_crc ();
      file.write (t);
    }
    // Make sure that the file is at least large enough for the minimum region size.
    {
      PSTORE_ASSERT (file.size () == leader_size + sizeof (trailer));
      // TODO: ask the region factory what the min region size is.
      if (file.size () < storage::min_region_size && !database::small_files_enabled ()) {
        file.truncate (storage::min_region_size);
      }
    }
  }

  // build sync name [static]
  // ~~~~~~~~~~~~~~~
  std::string database::build_sync_name (header const & header) {
    auto name = base32::convert (uint128{header.a.id.array ()});
    name.erase (std::min (sync_name_length, name.length ()));
    return name;
  }

  // open [static]
  // ~~~~
  auto database::open (std::string const & path, access_mode const am)
    -> std::shared_ptr<file::file_handle> {

    using present_mode = file::file_handle::present_mode;
    auto const create_mode = file::file_handle::create_mode::open_existing;
    auto const write_mode = (am == access_mode::writable || am == access_mode::writeable_no_create)
                              ? file::file_handle::writable_mode::read_write
                              : file::file_handle::writable_mode::read_only;

    auto file = std::make_shared<file::file_handle> (path);
    file->open (create_mode, write_mode, present_mode::allow_not_found);
    if (file->is_open ()) {
      return file;
    }

    if (am != access_mode::writable) {
      raise (std::errc::no_such_file_or_directory, path);
    }

    // We didn't find the file so will have to create a new one.
    //
    // To ensure that this operation is as close to atomic as the host OS allows, we create
    // a temporary file in the same directory but with a random name. (This approach was
    // chosen because this directory most likely has the same permissions and is on the same
    // physical volume as the final file). We then populate that file with the basic data
    // structures and rename it to the final destination.
    {
      auto const temporary_file = std::make_shared<file::file_handle> ();
      temporary_file->open (file::file_handle::unique{}, path::dir_name (path));
      file::deleter temp_file_deleter (temporary_file->path ());
      // Fill the file with its initial contents.
      database::build_new_store (*temporary_file);
      temporary_file->close ();
      temporary_file->rename (path);
      // The temporary file will no longer be found anyway (we just renamed it).
      temp_file_deleter.release ();
    }
    file->open (create_mode, write_mode, file::file_handle::present_mode::must_exist);
    PSTORE_ASSERT (file->is_open () && file->path () == path);
    return file;
  }

  // first writable address
  // ~~~~~~~~~~~~~~~~~~~~~~
  address database::first_writable_address () const {
    return (header_->footer_pos.load () + 1).to_address ();
  }

  // get spanning
  // ~~~~~~~~~~~~
  auto database::get_spanning (address const addr, std::size_t const size, bool const initialized)
    -> std::shared_ptr<void> {
    // The deleter is called when the shared pointer that we're about to return is
    // released.
    auto deleter = [this, addr, size] (std::uint8_t * const p) {
      // Check that this code is not trying to write back to read-only storage. This error
      // can occur if a non-const pointer is being destroyed after the containing
      // transaction has been committed.
      PSTORE_ASSERT (addr >= this->first_writable_address ());

      // If we're returning a writable pointer then we must copy the (potentially
      // modified) contents back to the data store.
      storage_.copy<storage::copy_to_store_traits> (
        addr, size, p,
        [] (std::uint8_t * const dest, std::uint8_t const * const src, std::size_t const n) {
          std::memcpy (dest, src, n);
        });
      delete[] p;
    };

    // Create the memory block that we'll be returning, attaching a suitable deleter
    // which will be responsible for copying the data back to the store (if we're providing
    // a writable pointer).
    std::shared_ptr<std::uint8_t> result{new std::uint8_t[size], deleter};

    if (initialized) {
      // Copy from the data store's regions to the newly allocated memory block.
      storage_.copy<storage::copy_from_store_traits> (
        addr, size, result.get (),
        [] (std::uint8_t const * const src, std::uint8_t * const dest, std::size_t const n) {
          std::memcpy (dest, src, n);
        });
    }
    return std::static_pointer_cast<void> (result);
  }

  auto database::get_spanning (address const addr, std::size_t const size,
                               bool const initialized) const -> std::shared_ptr<void const> {
    // Create the memory block that we'll be returning, attaching a suitable deleter
    // which will be responsible for copying the data back to the store (if we're providing
    // a writable pointer).
    std::shared_ptr<std::uint8_t> const result{new std::uint8_t[size],
                                               [] (std::uint8_t * const p) { delete[] p; }};

    if (initialized) {
      // Copy from the data store's regions to the newly allocated memory block.
      storage_.copy<storage::copy_from_store_traits> (
        addr, size, result.get (),
        [] (std::uint8_t const * const src, std::uint8_t * const dest, std::size_t const n) {
          std::memcpy (dest, src, n);
        });
    }
    return std::static_pointer_cast<void const> (result);
  }

  // get spanningu
  // ~~~~~~~~~~~~~
  auto database::get_spanningu (address const addr, std::size_t const size,
                                bool const initialized) const -> unique_pointer<void const> {
    // Create the memory block that we'll be returning, attaching a suitable deleter
    // which will be responsible for copying the data back to the store (if we're providing
    // a writable pointer).
    unique_pointer<std::uint8_t> result{new std::uint8_t[size], deleter};
    if (initialized) {
      // Copy from the data store's regions to the newly allocated memory block.
      storage_.copy<storage::copy_from_store_traits> (
        addr, size, result.get (),
        [] (std::uint8_t const * const src, std::uint8_t * const dest, std::size_t const n) {
          std::memcpy (dest, src, n);
        });
    }
    return {result.release (), deleter};
  }

  // check get params
  // ~~~~~~~~~~~~~~~~
  void database::check_get_params (address const addr, std::size_t const size,
                                   bool const writable) const {
    if (closed_) {
      raise (pstore::error_code::store_closed);
    }

    if (writable && addr < first_writable_address ()) {
      raise (error_code::read_only_address, "An attempt was made to write to read-only storage");
    }
    std::uint64_t const start = addr.absolute ();
    std::uint64_t const logical_size = size_.logical_size ();
    if (start > logical_size || size > logical_size - start) {
      raise (error_code::bad_address);
    }
  }

  // get
  // ~~~
  auto database::get (address const addr, std::size_t const size, bool const initialized)
    -> std::shared_ptr<void> {
    this->check_get_params (addr, size, true);
    if (storage_.request_spans_regions (addr, size)) {
      return this->get_spanning (addr, size, initialized);
    }
    return storage_.address_to_pointer (addr);
  }
  auto database::get (address const addr, std::size_t const size, bool const initialized) const
    -> std::shared_ptr<void const> {
    this->check_get_params (addr, size, false);
    if (storage_.request_spans_regions (addr, size)) {
      return this->get_spanning (addr, size, initialized);
    }
    return storage_.address_to_pointer (addr);
  }

  // getu
  // ~~~~
  auto database::getu (address addr, std::size_t size, bool initialized) const
    -> unique_pointer<void const> {
    this->check_get_params (addr, size, false);
    if (storage_.request_spans_regions (addr, size)) {
      return this->get_spanningu (addr, size, initialized);
    }
    return {storage_.address_to_raw_pointer (addr), deleter_nop<void const>};
  }

  // allocate
  // ~~~~~~~~
  pstore::address database::allocate (std::uint64_t const bytes, unsigned const align) {
    PSTORE_ASSERT (is_power_of_two (align));
    if (closed_) {
      raise (error_code::store_closed);
    }
    modified_ = true;

    std::uint64_t const old_logical_size = size_.logical_size ();
    PSTORE_ASSERT (old_logical_size >= size_.footer_pos ().absolute () + sizeof (trailer));
    std::uint64_t const extra_for_alignment =
      calc_alignment (old_logical_size, std::uint64_t{align});
    PSTORE_ASSERT (extra_for_alignment < align);
    // Bump 'result' up by the number of alignment bytes that we're adding to ensure
    // that it's properly aligned.
    std::uint64_t const result = old_logical_size + extra_for_alignment;

    // Increase the number of bytes being requested by enough to ensure that result is
    // properly aligned.
    std::uint64_t const new_logical_size = result + bytes;

    // Memory map additional space if necessary.
    storage_.map_bytes (old_logical_size, new_logical_size);

    size_.update_logical_size (new_logical_size);
    if /*constexpr*/ (
      database::small_files_enabled ()) { //! OCLINT(PH - don't warn that this is a constant)
      this->file ()->truncate (new_logical_size);
    }
    return address{result};
  }

  // truncate
  // ~~~~~~~~
  void database::truncate (std::uint64_t const size) {
    if (closed_) {
      raise (error_code::store_closed);
    }
    modified_ = true;

    // Adjust the memory mapped space if necessary.
    storage_.map_bytes (size_.logical_size (), size);

    size_.truncate_logical_size (size);

    if /*constexpr*/ (
      database::small_files_enabled ()) { //! OCLINT(PH - don't warn that this is a constant)
      this->file ()->truncate (size);
    } else {
      storage_.truncate_to_physical_size ();
    }
  }

  // set new footer
  // ~~~~~~~~~~~~~~
  void database::set_new_footer (typed_address<trailer> const new_footer_pos) {
    size_.update_footer_pos (new_footer_pos);

    // Finally (this should be the last thing we do), point the file header at the new
    // footer. Any other threads/processes will now see our new transaction as the state
    // of the database.

    header_->footer_pos = new_footer_pos;
  }

} // end namespace pstore
