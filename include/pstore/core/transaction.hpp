//===- include/pstore/core/transaction.hpp ----------------*- mode: C++ -*-===//
//*  _                                  _   _              *
//* | |_ _ __ __ _ _ __  ___  __ _  ___| |_(_) ___  _ __   *
//* | __| '__/ _` | '_ \/ __|/ _` |/ __| __| |/ _ \| '_ \  *
//* | |_| | | (_| | | | \__ \ (_| | (__| |_| | (_) | | | | *
//*  \__|_|  \__,_|_| |_|___/\__,_|\___|\__|_|\___/|_| |_| *
//*                                                        *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file transaction.hpp
/// \brief The data store transaction class.

#ifndef PSTORE_CORE_TRANSACTION_HPP
#define PSTORE_CORE_TRANSACTION_HPP

#include <mutex>
#include <type_traits>

#include "pstore/core/address.hpp"
#include "pstore/core/database.hpp"
#include "pstore/core/time.hpp"

namespace pstore {
  /// \brief The database transaction class.
  ///
  /// When a transaction object is instantiated, a transation begins. Every subsequent
  /// operation can be potentially undone if the rollback() method is called. The commit()
  /// method commits the work performed by all operations since the start of the
  /// transaction.
  ///
  /// Similarly, the rollback() method command undoes all of the work performed by all
  /// operations since the start of the transaction. If neither the commit() nor rollback()
  /// methods are called before the object is destroyed, a commit() is performed by the
  /// destructor (unless an exception is being unwound). A transaction is a scope in which
  /// operations are performed together and committed, or completely reversed.
  class transaction_base {
  public:
    virtual ~transaction_base () noexcept = default;

    transaction_base (transaction_base const &) = delete;
    transaction_base (transaction_base && rhs) noexcept;

    transaction_base & operator= (transaction_base const &) = delete;
    transaction_base & operator= (transaction_base && rhs) noexcept = delete;

    database & db () noexcept { return db_; }
    database const & db () const noexcept { return db_; }

    /// Returns true if data has been added to this transaction, but not yet committed. In
    /// other words, if it returns false, calls to commit() or rollback() are noops.
    bool is_open () const noexcept { return first_ != address::null (); }

    /// Commits all modifications made to the data store as part of this transaction.
    /// Modifications are visible to other processes when the commit is complete.
    transaction_base & commit ();

    /// Discards all modifications made to the data store as part of this transaction.
    transaction_base & rollback () noexcept;


    ///@{
    std::shared_ptr<void const> getro (address const addr, std::size_t const size) {
      PSTORE_ASSERT (addr >= first_ && addr + size <= first_ + size_);
      return db ().getro (addr, size);
    }

    template <typename T, typename = typename std::enable_if_t<std::is_standard_layout_v<T>>>
    std::shared_ptr<T const> getro (extent<T> const & ex) {
      return this->getro (ex.addr, ex.size);
    }

    template <typename T, typename = typename std::enable_if_t<std::is_standard_layout_v<T>>>
    std::shared_ptr<T const> getro (typed_address<T> const addr, std::size_t const elements = 1) {
      PSTORE_ASSERT (addr.to_address () >= first_ &&
                     (addr.to_address () + elements * sizeof (T)) <= first_ + size_);
      return db_.getro (addr, elements);
    }
    ///@}


    ///@{
    std::shared_ptr<void> getrw (address const addr, std::size_t const size) {
      PSTORE_ASSERT (addr >= first_ && addr + size <= first_ + size_);
      return db_.getrw (addr, size);
    }

    template <typename T, typename = typename std::enable_if_t<std::is_standard_layout_v<T>>>
    std::shared_ptr<T> getrw (extent<T> const & ex) {
      return db_.getrw (ex);
    }

    template <typename T, typename = typename std::enable_if_t<std::is_standard_layout_v<T>>>
    std::shared_ptr<T> getrw (typed_address<T> const addr, std::size_t const elements = 1) {
      PSTORE_ASSERT (addr.to_address () >= first_ &&
                     (addr.to_address () + elements * sizeof (T)) <= first_ + size_);
      return db_.getrw (addr, elements);
    }
    ///@}

    ///@{
    /// Extend the database store ensuring that there's enough room for the requested number
    /// of bytes with any additional padding to statify the alignment requirement.
    ///
    /// \param size   The number of bytes of storages to be allocated.
    /// \param align  The alignment of the allocated storage. Must be a power of 2.
    /// \result       The database address of the new storage.
    /// \note     The newly allocated space is not initialized.
    virtual address allocate (std::uint64_t size, unsigned align);

    /// Extend the database store ensuring that there's enough room for an instance of the
    /// template type.
    /// \result  The database address of the new storage.
    /// \note    The newly allocated space is not initialized.
    template <typename Ty>
    address allocate () {
      return this->allocate (sizeof (Ty), alignof (Ty));
    }
    ///@}

    ///@{

    /// Allocates sufficient space in the transaction for \p size bytes
    /// at an alignment given by \p align and returns both a writable pointer
    /// to the new space and its address.
    ///
    /// \param size  The number of bytes of storage to allocate.
    /// \param align The alignment of the newly allocated storage. Must be a non-zero power
    ///              of two.
    /// \returns  A std::pair which contains a writable pointer to the newly
    ///           allocated space and the address of that space.
    ///
    /// \note     The newly allocated space is not initialized.
    std::pair<std::shared_ptr<void>, address> alloc_rw (std::size_t size, unsigned align);

    /// Allocates sufficient space in the transaction for one or more new instances of
    /// type 'Ty' and returns both a writable pointer to the new space and
    /// its address. Ty must be a "standard layout" type.
    ///
    /// \param num  The number of instances of type Ty for which space should be allocated.
    /// \returns  A std::pair which contains a writable pointer to the newly
    ///           allocated space and the address of that space.
    ///
    /// \note     The newly allocated space it not initialized.
    template <typename Ty, typename = typename std::enable_if_t<std::is_standard_layout_v<Ty>>>
    auto alloc_rw (std::size_t const num = 1) -> std::pair<std::shared_ptr<Ty>, typed_address<Ty>> {
      auto [ptr, addr] = this->alloc_rw (sizeof (Ty) * num, alignof (Ty));
      return {std::static_pointer_cast<Ty> (ptr), typed_address<Ty> (addr)};
    }
    ///@}

    /// Returns the number of bytes allocated in this transaction.
    std::uint64_t size () const noexcept { return size_; }

  protected:
    explicit transaction_base (database & db);

  private:
    database & db_;
    /// The number of bytes allocated in this transaction.
    std::uint64_t size_ = 0;

    /// The size of the db at the creation of this transaction
    std::uint64_t dbsize_ = 0;

    /// The first address occupied by this transaction. 0 if the transaction
    /// has not yet allocated any data.
    address first_ = address::null ();
  };


  //*  _                             _   _           *
  //* | |_ _ _ __ _ _ _  ___ __ _ __| |_(_)___ _ _   *
  //* |  _| '_/ _` | ' \(_-</ _` / _|  _| / _ \ ' \  *
  //*  \__|_| \__,_|_||_/__/\__,_\__|\__|_\___/_||_| *
  //*                                                *
  template <typename LockGuard>
  class transaction : public transaction_base {
  public:
    using lock_type = LockGuard;

    transaction (database & db, lock_type && lock);
    transaction (transaction const & rhs) = delete;
    transaction (transaction && rhs) noexcept = default;

    ~transaction () noexcept override;

    transaction & operator= (transaction const & rhs) = delete;
    transaction & operator= (transaction && rhs) noexcept = delete;

  private:
    lock_type lock_;
  };

  // (ctor)
  // ~~~~~~
  template <typename LockGuard>
  transaction<LockGuard>::transaction (database & db, LockGuard && lock)
          : transaction_base (db)
          , lock_{std::move (lock)} {

    PSTORE_ASSERT (!this->is_open ());
  }

  // (dtor)
  // ~~~~~~
  template <typename LockGuard>
  transaction<LockGuard>::~transaction () noexcept {
    static_assert (noexcept (rollback ()), "rollback must be noexcept");
    this->rollback ();
  }

  //*  _         _                           _  *
  //* | |___  __| |__  __ _ _  _ __ _ _ _ __| | *
  //* | / _ \/ _| / / / _` | || / _` | '_/ _` | *
  //* |_\___/\__|_\_\ \__, |\_,_\__,_|_| \__,_| *
  //*                 |___/                     *
  /// lock_guard fills a similar role as a type such as std::scoped_lock<> in that it provides
  /// convenient RAII-style mechanism for owning a mutex for the duration of a scoped block. The
  /// major differences are that it manages only a single mutex, and that it assumes ownership of
  /// the mutex.
  template <typename MutexType>
  class lock_guard {
  public:
    explicit lock_guard (MutexType && mut)
            : mut_{std::move (mut)} {
      mut_.lock ();
      owned_ = true;
    }
    lock_guard (lock_guard const & rhs) = delete;
    lock_guard (lock_guard && rhs) noexcept
            : mut_{std::move (rhs.mut_)}
            , owned_{rhs.owned_} {
      rhs.owned_ = false;
    }

    ~lock_guard () {
      if (owned_) {
        mut_.unlock ();
        owned_ = false;
      }
    }

    lock_guard & operator= (lock_guard const & rhs) = delete;
    lock_guard & operator= (lock_guard && rhs) noexcept {
      if (this != &rhs) {
        mut_ = std::move (rhs.mut_);
        owned_ = rhs.owned_;
        rhs.owned_ = false;
      }
      return *this;
    }

  private:
    MutexType mut_;
    bool owned_ = false;
  };


  //*  _                             _   _                      _            *
  //* | |_ _ _ __ _ _ _  ___ __ _ __| |_(_)___ _ _    _ __ _  _| |_ _____ __ *
  //* |  _| '_/ _` | ' \(_-</ _` / _|  _| / _ \ ' \  | '  \ || |  _/ -_) \ / *
  //*  \__|_| \__,_|_||_/__/\__,_\__|\__|_\___/_||_| |_|_|_\_,_|\__\___/_\_\ *
  //*                                                                        *
  /// A mutex which is used to protect a pstore file from being simultaneously written by multiple
  /// threads or processes.
  class transaction_mutex {
  public:
    explicit transaction_mutex (database & db)
            : rl_{
                db.file (),                                                // file
                sizeof (header) + offsetof (lock_block, transaction_lock), // offset
                sizeof (lock_block::transaction_lock),                     // size
                pstore::file::file_base::lock_kind::exclusive_write        // kind
              } {}

    transaction_mutex (transaction_mutex && rhs) noexcept = default;
    transaction_mutex (transaction_mutex const & rhs) = delete;

    transaction_mutex & operator= (transaction_mutex const & rhs) = delete;
    transaction_mutex & operator= (transaction_mutex && rhs) noexcept = default;

    void lock () { rl_.lock (); }
    void unlock () { rl_.unlock (); }

  private:
    file::range_lock rl_;
  };

  using transaction_lock = lock_guard<transaction_mutex>;


  ///@{
  /// \brief Creates a new transaction.
  /// Every operation performed on a transaction instance can be potentially undone. The
  /// object's commit() method commits the work performed by all operations since the start of
  /// the transaction.
  inline transaction<transaction_lock> begin (database & db, transaction_lock & lock) {
    return {db, std::move (lock)};
  }
  inline transaction<transaction_lock> begin (database & db, transaction_lock && lock) {
    return {db, std::move (lock)};
  }
  transaction<transaction_lock> begin (database & db);

  ///@}

} // namespace pstore

#endif // PSTORE_CORE_TRANSACTION_HPP
