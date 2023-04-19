//===- include/pstore/romfs/directory.hpp -----------------*- mode: C++ -*-===//
//*      _ _               _                    *
//*   __| (_)_ __ ___  ___| |_ ___  _ __ _   _  *
//*  / _` | | '__/ _ \/ __| __/ _ \| '__| | | | *
//* | (_| | | | |  __/ (__| || (_) | |  | |_| | *
//*  \__,_|_|_|  \___|\___|\__\___/|_|   \__, | *
//*                                      |___/  *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_ROMFS_DIRECTORY_HPP
#define PSTORE_ROMFS_DIRECTORY_HPP

#include <cstdlib>

#include "pstore/adt/pointer_based_iterator.hpp"
#include "pstore/support/gsl.hpp"

namespace pstore::romfs {

  class dirent;

  class directory {
  public:
    using iterator = gsl::span<dirent const>::const_iterator;

    constexpr directory (gsl::span<dirent const> members) noexcept
            : members_{members} {
      PSTORE_ASSERT (members_.size () >= 0);
    }
    directory (directory const &) = delete;
    directory (directory &&) noexcept = delete;

    ~directory () noexcept = default;

    directory & operator= (directory const &) = delete;
    directory & operator= (directory &&) noexcept = delete;

    iterator begin () const noexcept { return iterator{members_.begin ()}; }
    iterator end () const noexcept { return iterator{members_.end ()}; }

    std::size_t size () const noexcept { return static_cast<std::size_t> (members_.size ()); }
    dirent const & operator[] (std::size_t pos) const noexcept;

    /// Search the directory for a member whose name equals \p name.
    ///
    /// \param name  The name of the entry to be found.
    /// \returns  An iterator to the directory entry if found, or end if not.
    iterator find (std::string_view name) const;

    /// Searchs the directory for a member which references the directory structure \p d.
    ///
    /// \param d  The directory to be found.
    /// \returns  An iterator to the directory entry if found, or end if not.
    iterator find (gsl::not_null<directory const *> d) const;

    /// Performs basic validity checks on a directory hierarchy.
    bool check () const;

  private:
    struct check_stack_entry {
      gsl::not_null<directory const *> d;
      check_stack_entry const * prev;
    };
    bool check (gsl::not_null<directory const *> parent, check_stack_entry const * visited) const;

    /// The number of entries in the members_ array.
    //    std::size_t const size_;
    /// An array of directory members.
    gsl::span<dirent const> const members_;
  };

} // end namespace pstore::romfs

#endif // PSTORE_ROMFS_DIRECTORY_HPP
