//===- lib/romfs/directory.cpp --------------------------------------------===//
//*      _ _               _                    *
//*   __| (_)_ __ ___  ___| |_ ___  _ __ _   _  *
//*  / _` | | '__/ _ \/ __| __/ _ \| '__| | | | *
//* | (_| | | | |  __/ (__| || (_) | |  | |_| | *
//*  \__,_|_|_|  \___|\___|\__\___/|_|   \__, | *
//*                                      |___/  *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/romfs/directory.hpp"

#include <algorithm>
#include <cstring>

#include "pstore/romfs/dirent.hpp"

using pstore::romfs::directory;
using pstore::romfs::dirent;
template <typename T>
using not_null = pstore::gsl::not_null<T>;

namespace {

  // Returns true if the directory entry given by 'd' is a directory which references the
  // directory structure 'expected'.
  bool is_expected_dir (dirent const & d, not_null<directory const *> const expected) {
    auto const od = d.opendir ();
    return od && *od == expected;
  }

} // end anonymous namespace

namespace pstore::romfs {

  //*     _ _            _                 *
  //*  __| (_)_ _ ___ __| |_ ___ _ _ _  _  *
  //* / _` | | '_/ -_) _|  _/ _ \ '_| || | *
  //* \__,_|_|_| \___\__|\__\___/_|  \_, | *
  //*                                |__/  *
  // operator[]
  // ~~~~~~~~~~
  auto directory::operator[] (std::size_t const pos) const noexcept -> dirent const & {
    using index_type = decltype (members_)::index_type;
    PSTORE_ASSERT (pos <= static_cast<std::make_unsigned_t<index_type>> (
                            std::numeric_limits<index_type>::max ()));
    return members_[static_cast<index_type> (pos)];
  }

  // find
  // ~~~~
  auto directory::find (gsl::not_null<directory const *> const d) const -> iterator {
    // This is a straightforward linear search. Could limit performance in the future.
    return std::find_if (this->begin (), this->end (),
                         [d] (dirent const & de) { return is_expected_dir (de, d); });
  }

  auto directory::find (std::string_view name) const -> iterator {
    // Directories are sorted by name: we can use a binary search here.
    auto const end = this->end ();
    auto const pos =
      std::lower_bound (begin (), end, name, [] (dirent const & a, std::string_view const & b) {
        return a.name ().get () < b;
      });
    if (pos != end && pos->name ().get () == name) {
      return pos;
    }
    return end;
  }

  // check
  // ~~~~~
  bool directory::check (gsl::not_null<directory const *> const parent,
                         check_stack_entry const * const visited) const {
    auto const end = this->end ();

    // This stops us from looping forever if we follow a directory pointer to a directory
    // that we have already visited.
    for (auto const * v = visited; v != nullptr; v = v->prev) {
      if (v->d == parent) {
        return true;
      }
    }

    // Check that the directory entries are sorted by name.
    if (!std::is_sorted (begin (), end, [] (dirent const & a, dirent const & b) {
          return std::strcmp (a.name (), b.name ()) < 0;
        })) {
      return false;
    }

    // Look for the '.' and '..' entries and check that they point where we expect.
    iterator const dot = this->find (".");
    iterator const dot_dot = this->find ("..");
    if (dot == end || dot_dot == end) {
      return false;
    }
    if (!is_expected_dir (*dot, this) || !is_expected_dir (*dot_dot, parent)) {
      return false;
    }

    // Recursively check any directories contained by this one.
    return std::all_of (std::begin (*this), std::end (*this), [&visited, this] (dirent const & de) {
      if (!de.is_directory ()) {
        return true;
      }
      if (auto const od = de.opendir ()) {
        check_stack_entry const me{*od, visited};
        return (*od)->check (this, &me);
      }
      // Error: this entry claimed to be a directory but opendir() failed.
      return false;
    });
  }

  bool directory::check () const {
    return this->check (this, nullptr);
  }

} // end namespace pstore::romfs
