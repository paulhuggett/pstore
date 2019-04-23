//*      _ _                _    *
//*   __| (_)_ __ ___ _ __ | |_  *
//*  / _` | | '__/ _ \ '_ \| __| *
//* | (_| | | | |  __/ | | | |_  *
//*  \__,_|_|_|  \___|_| |_|\__| *
//*                              *
//===- include/pstore/romfs/dirent.hpp ------------------------------------===//
// Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
// All rights reserved.
//
// Developed by:
//   Toolchain Team
//   SN Systems, Ltd.
//   www.snsystems.com
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal with the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimers.
//
// - Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimers in the
//   documentation and/or other materials provided with the distribution.
//
// - Neither the names of SN Systems Ltd., Sony Interactive Entertainment,
//   Inc. nor the names of its contributors may be used to endorse or
//   promote products derived from this Software without specific prior
//   written permission.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
//===----------------------------------------------------------------------===//
#ifndef PSTORE_ROMFS_DIRENT_HPP
#define PSTORE_ROMFS_DIRENT_HPP

#include <cassert>
#include <cstdint>
#include <ctime>

#include <sys/types.h>

#include "pstore/support/error_or.hpp"
#include "pstore/support/gsl.hpp"
#include "pstore/support/portab.hpp"

namespace pstore {
    namespace romfs {

        // using mode_t = std::uint8_t;
        enum class mode_t { file, directory };

        struct stat {
            constexpr stat (std::size_t size, std::time_t mtime, mode_t m) noexcept
                    : st_size{size}
                    , st_mtime{mtime}
                    , mode{m} {}

            bool operator== (stat const & rhs) const noexcept {
                return st_size == rhs.st_size && st_mtime == rhs.st_mtime && mode == rhs.mode;
            }
            bool operator!= (stat const & rhs) const noexcept { return !operator== (rhs); }

            // static constexpr auto is_directory_flag = std::uint16_t{0x01};
            std::size_t st_size;  ///< File size in bytes.
            std::time_t st_mtime; ///< Time when file data was last modified.
            mode_t mode;
        };

        class directory;

        class dirent {
        public:
            constexpr dirent (gsl::czstring PSTORE_NONNULL name,
                              void const * PSTORE_NONNULL contents, stat s) noexcept
                    : name_{name}
                    , contents_{contents}
                    , stat_{s} {}
            constexpr dirent (gsl::czstring PSTORE_NONNULL name,
                              directory const * PSTORE_NONNULL dir) noexcept
                    : name_{name}
                    , contents_{dir}
                    , stat_{sizeof (dir), 0 /*time*/, mode_t::directory} {}

            constexpr gsl::czstring PSTORE_NONNULL name () const noexcept { return name_; }
            constexpr bool is_directory () const noexcept {
                return stat_.mode == mode_t::directory;
            }
            constexpr void const * PSTORE_NONNULL contents () const noexcept { return contents_; }

            error_or<class directory const * PSTORE_NONNULL> opendir () const;
            struct stat stat () const noexcept {
                return stat_;
            }

        private:
            gsl::czstring PSTORE_NONNULL name_;
            void const * PSTORE_NONNULL contents_;
            struct stat stat_;
            //            std::size_t size_;
            //            mode_t mode_;
        };

    } // end namespace romfs
} // end namespace pstore

#endif // PSTORE_ROMFS_DIRENT_HPP
