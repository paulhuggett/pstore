//===- lib/romfs/romfs.cpp ------------------------------------------------===//
//*                       __      *
//*  _ __ ___  _ __ ___  / _|___  *
//* | '__/ _ \| '_ ` _ \| |_/ __| *
//* | | | (_) | | | | | |  _\__ \ *
//* |_|  \___/|_| |_| |_|_| |___/ *
//*                               *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/romfs/romfs.hpp"

#include <algorithm>
#include <cstring>
#include <string_view>

#include "pstore/support/unsigned_cast.hpp"

using pstore::gsl::czstring;
using pstore::gsl::not_null;

namespace {

  /// Split the pathname \p path into a pair, (head, tail) where head is the
  /// first path component and tail is everything after that.
  std::pair<std::string_view, std::string_view> path_component (std::string_view path) {
    auto const pos = std::min (path.find_first_of ('/'), path.size ());
    auto tail = path;
    tail.remove_prefix (pos);
    return {std::string_view{path.data (), pos}, tail};
  }

  // Skip over any leading path-separator characters in \p path.
  std::string_view next_component (std::string_view path) {
    path.remove_prefix (std::min (path.find_first_not_of ('/'), path.size ()));
    return path;
  }

} // end anonymous namespace

namespace pstore::romfs {

  //*                                _                          *
  //*  ___ _ _ _ _ ___ _ _   __ __ _| |_ ___ __ _ ___ _ _ _  _  *
  //* / -_) '_| '_/ _ \ '_| / _/ _` |  _/ -_) _` / _ \ '_| || | *
  //* \___|_| |_| \___/_|   \__\__,_|\__\___\__, \___/_|  \_, | *
  //*                                       |___/         |__/  *
  char const * error_category::name () const noexcept {
    return "pstore-romfs category";
  }

  std::string error_category::message (int const error) const {
    gsl::czstring result = "unknown error";
    switch (static_cast<error_code> (error)) {
    case error_code::einval: result = "There was an invalid operation"; break;
    case error_code::enoent: result = "The path was not found"; break;
    case error_code::enotdir:
      result = "Cannot apply a directory operation to a non-directory path";
      break;
    }
    return result;
  }

  std::error_code make_error_code (error_code const e) {
    static_assert (std::is_same_v<std::underlying_type_t<decltype (e)>, int>,
                   "base type of error_code must be int to permit safe static cast");
    return {static_cast<int> (e), category};
  }

  //*                        __ _ _      *
  //*  ___ _ __  ___ _ _    / _(_) |___  *
  //* / _ \ '_ \/ -_) ' \  |  _| | / -_) *
  //* \___/ .__/\___|_||_| |_| |_|_\___| *
  //*     |_|                            *
  class open_file {
  public:
    explicit open_file (dirent const & d)
            : dir_{d} {}
    open_file (open_file const &) = delete;
    open_file (open_file &&) = delete;
    ~open_file () noexcept = default;

    open_file & operator= (open_file const &) = delete;
    open_file & operator= (open_file &&) = delete;

    error_or<std::size_t> seek (off_t offset, seek_mode whence);
    std::size_t read (not_null<void *> buffer, std::size_t size, std::size_t count);
    struct stat stat () const { return dir_.stat (); }

  private:
    dirent const & dir_;
    std::size_t pos_ = 0;
  };

  // read
  // ~~~~
  std::size_t open_file::read (not_null<void *> const b, std::size_t const size,
                               std::size_t const count) {
    if (size == 0 || count == 0) {
      return 0;
    }
    auto const file_size =
      std::make_unsigned_t<off_t> (std::max (dir_.stat ().size, std::size_t{0}));
    auto num_read = std::size_t{0};
    auto const * start = reinterpret_cast<std::uint8_t const *> (dir_.contents ().get ()) + pos_;
    void * buffer = b;
    for (; num_read < count; ++num_read) {
      if (pos_ + size > file_size) {
        return num_read;
      }
      std::memcpy (buffer, start, size);
      start += size;
      buffer = reinterpret_cast<std::uint8_t *> (buffer) + size;
      pos_ += size;
    }
    return num_read;
  }

  // seek
  // ~~~~
  error_or<std::size_t> open_file::seek (off_t const offset, seek_mode const whence) {
    auto make_error = [] (error_code const erc) { return error_or<std::size_t> (erc); };
    using uoff_type = std::make_unsigned_t<off_t>;
    std::size_t new_pos = 0;
    std::size_t const file_size = dir_.stat ().size;
    switch (whence) {
    case seek_mode::set:
      if (offset < 0) {
        return make_error (error_code::einval);
      }
      new_pos = static_cast<uoff_type> (offset);
      break;
    case seek_mode::cur:
      if (offset < 0) {
        auto const positive_offset = static_cast<uoff_type> (-offset);
        if (positive_offset > pos_) {
          return make_error (error_code::einval);
        }
        new_pos = pos_ - positive_offset;
      } else {
        new_pos = pos_ + static_cast<uoff_type> (offset);
      }
      break;
    case seek_mode::end:
      if (offset < 0 && static_cast<std::size_t> (-offset) > file_size) {
        return make_error (error_code::einval);
      }
      new_pos = (offset >= 0) ? (file_size + static_cast<uoff_type> (offset))
                              : (file_size - static_cast<uoff_type> (-offset));
      break;
    }

    pos_ = new_pos;
    return error_or<std::size_t>{new_pos};
  }


  //*                          _ _            _                 *
  //*  ___ _ __  ___ _ _    __| (_)_ _ ___ __| |_ ___ _ _ _  _  *
  //* / _ \ '_ \/ -_) ' \  / _` | | '_/ -_) _|  _/ _ \ '_| || | *
  //* \___/ .__/\___|_||_| \__,_|_|_| \___\__|\__\___/_|  \_, | *
  //*     |_|                                             |__/  *
  class open_directory {
  public:
    explicit open_directory (directory const & dir) noexcept
            : dir_{dir} {}
    open_directory (open_directory const &) = delete;
    open_directory (open_directory &&) = delete;
    ~open_directory () noexcept = default;

    open_directory & operator= (open_directory const &) = delete;
    open_directory & operator= (open_directory &&) = delete;

    void rewind () noexcept { index_ = 0U; }
    dirent const * read ();

  private:
    directory const & dir_;
    unsigned index_ = 0;
  };

  auto open_directory::read () -> dirent const * {
    if (index_ >= dir_.size ()) {
      return nullptr;
    }
    return &(dir_[index_++]);
  }


  //*     _                _      _            *
  //*  __| |___ ___ __ _ _(_)_ __| |_ ___ _ _  *
  //* / _` / -_|_-</ _| '_| | '_ \  _/ _ \ '_| *
  //* \__,_\___/__/\__|_| |_| .__/\__\___/_|   *
  //*                       |_|                *
  std::size_t descriptor::read (not_null<void *> const buffer, std::size_t const size,
                                std::size_t const count) const {
    return f_->read (buffer, size, count);
  }
  error_or<std::size_t> descriptor::seek (off_t const offset, seek_mode const whence) const {
    return f_->seek (offset, whence);
  }
  auto descriptor::stat () const -> struct stat {
    return f_->stat ();
  }


  //*     _ _             _        _                _      _            *
  //*  __| (_)_ _ ___ _ _| |_   __| |___ ___ __ _ _(_)_ __| |_ ___ _ _  *
  //* / _` | | '_/ -_) ' \  _| / _` / -_|_-</ _| '_| | '_ \  _/ _ \ '_| *
  //* \__,_|_|_| \___|_||_\__| \__,_\___/__/\__|_| |_| .__/\__\___/_|   *
  //*                                                |_|                *
  dirent const *
  dirent_descriptor::read () const {
    return f_->read ();
  }
  void dirent_descriptor::rewind () const {
    f_->rewind ();
  }


  //*                 __     *
  //*  _ _ ___ _ __  / _|___ *
  //* | '_/ _ \ '  \|  _(_-< *
  //* |_| \___/_|_|_|_| /__/ *
  //*                        *
  // (ctor)
  // ~~~~~~
  romfs::romfs (not_null<directory const *> const root)
          : root_{root}
          , cwd_{root} {
    PSTORE_ASSERT (this->fsck ());
  }

  // open
  // ~~~~
  auto romfs::open (std::string_view const & path) const -> error_or<descriptor> {
    return this->parse_path (path) >>= [] (dirent_ptr const de) {
      auto const file = std::make_shared<open_file> (*de);
      return error_or<descriptor>{descriptor{file}};
    };
  }

  // opendir
  // ~~~~~~~
  error_or<dirent_descriptor> romfs::opendir (std::string_view const & path) {
    auto get_directory = [] (dirent const * const de) {
      using rett = error_or<directory const *>;
      return de->is_directory () ? rett{de->opendir ()} : rett{error_code::enotdir};
    };

    auto create_descriptor = [] (directory const * const d) {
      auto const directory = std::make_shared<open_directory> (*d);
      return error_or<dirent_descriptor>{dirent_descriptor{directory}};
    };

    return (this->parse_path (path) >>= get_directory) >>= create_descriptor;
  }

  // stat
  // ~~~~
  error_or<struct stat> romfs::stat (std::string_view const & path) const {
    return this->parse_path (path) >>=
           [] (dirent_ptr const de) { return error_or<struct stat>{de->stat ()}; };
  }

  // getcwd
  // ~~~~~~
  error_or<std::string> romfs::getcwd () const {
    return dir_to_string (cwd_);
  }

  // chdir
  // ~~~~~
  std::error_code romfs::chdir (std::string_view const & path) {
    auto const dirent_to_directory = [] (dirent_ptr const de) { return de->opendir (); };

    auto const set_cwd = [this] (directory const * const d) {
      cwd_ = d; // Warning: side-effect!
      return error_or<directory const *> (d);
    };

    auto const eo = (this->parse_path (path) >>= dirent_to_directory) >>= set_cwd;
    return eo.get_error ();
  }

  // directory to dirent [static]
  // ~~~~~~~~~~~~~~~~~~~
  not_null<dirent const *> romfs::directory_to_dirent (not_null<directory const *> const d) {
    directory::iterator const dotdot_pos = d->find ("..");
    PSTORE_ASSERT (dotdot_pos != d->end () && dotdot_pos->is_directory ());
    return &*dotdot_pos;
  }

  // parse path
  // ~~~~~~~~~~
  auto romfs::parse_path (std::string_view const & path, not_null<directory const *> dir) const
    -> error_or<dirent_ptr> {
    if (path.empty () || dir.get () == nullptr) {
      return error_or<dirent_ptr> (error_code::enoent);
    }

    auto p = path;
    if (p[0] == '/') {
      dir = root_;
      p = next_component (path);
    }

    dirent const * current_de = directory_to_dirent (dir);
    PSTORE_ASSERT (current_de->is_directory ());
    while (!p.empty ()) {
      auto [component, tail] = path_component (p);
      p = next_component (tail);
      auto const component_pos = dir->find (component);
      if (component_pos == dir->end ()) {
        // name not found
        return error_or<dirent_ptr>{error_code::enoent};
      }

      current_de = &*component_pos;
      if (current_de->is_directory ()) {
        auto eo_directory = current_de->opendir ();
        if (!eo_directory) {
          return error_or<dirent_ptr>{eo_directory.get_error ()};
        }
        dir = *eo_directory;
      } else if (!p.empty ()) {
        // not a directory and wasn't the last component.
        return error_or<dirent_ptr> (error_code::enotdir);
      }
    }
    return error_or<dirent_ptr>{current_de};
  }

  // dir to string
  // ~~~~~~~~~~~~~
  error_or<std::string> romfs::dir_to_string (not_null<directory const *> const dir) const {
    if (dir == root_) {
      return error_or<std::string>{"/"};
    }
    auto const parent_de = dir->find ("..");
    PSTORE_ASSERT (parent_de != dir->end () && "All directories must contain a '..' entry");
    return parent_de->opendir () >>= [this, dir] (not_null<directory const *> const parent) {
      return this->dir_to_string (parent) >>= [dir, parent] (std::string s) {
        PSTORE_ASSERT (s.length () > 0);
        if (s.back () != '/') {
          s += '/';
        }

        auto const p = parent->find (dir);
        PSTORE_ASSERT (p != dir->end ());
        return error_or<std::string> (s.append (p->name ().get ()));
      };
    };
  }

  // fsck
  // ~~~~
  bool romfs::fsck () const {
    return root_->check ();
  }

} // end namespace pstore::romfs
