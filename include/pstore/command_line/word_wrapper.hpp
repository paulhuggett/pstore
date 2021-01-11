//*                        _                                             *
//* __      _____  _ __ __| | __      ___ __ __ _ _ __  _ __   ___ _ __  *
//* \ \ /\ / / _ \| '__/ _` | \ \ /\ / / '__/ _` | '_ \| '_ \ / _ \ '__| *
//*  \ V  V / (_) | | | (_| |  \ V  V /| | | (_| | |_) | |_) |  __/ |    *
//*   \_/\_/ \___/|_|  \__,_|   \_/\_/ |_|  \__,_| .__/| .__/ \___|_|    *
//*                                              |_|   |_|               *
//===- include/pstore/command_line/word_wrapper.hpp -----------------------===//
// Copyright (c) 2017-2021 by Sony Interactive Entertainment, Inc.
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
#ifndef PSTORE_COMMAND_LINE_WORD_WRAPPER_HPP
#define PSTORE_COMMAND_LINE_WORD_WRAPPER_HPP

#include <cstdlib>
#include <iterator>
#include <string>

namespace pstore {
    namespace command_line {

        class word_wrapper {
        public:
            using iterator_category = std::input_iterator_tag;
            using value_type = std::string const;
            using difference_type = std::ptrdiff_t;
            using pointer = std::string const *;
            using reference = std::string const &;

            static constexpr std::size_t default_width = 79;

            explicit word_wrapper (std::string const & text,
                                   std::size_t const max_width = default_width)
                    : word_wrapper (text, max_width, 0U) {}
            static word_wrapper end (std::string const & text,
                                     std::size_t max_width = default_width);

            bool operator== (word_wrapper const & rhs) const;
            bool operator!= (word_wrapper const & rhs) const { return !operator== (rhs); }

            reference operator* () const { return substr_; }
            pointer operator-> () const { return &substr_; }

            word_wrapper & operator++ ();
            word_wrapper operator++ (int);

        private:
            word_wrapper (std::string const & text, std::size_t max_width, std::size_t pos);
            void next ();

            std::string const & text_;
            std::size_t const max_width_;
            std::size_t start_pos_;
            std::string substr_;
        };

    } // end namespace command_line
} // end namespace pstore

#endif // PSTORE_COMMAND_LINE_WORD_WRAPPER_HPP
