//*            _                               *
//*   ___ __ _| |_ ___  __ _  ___  _ __ _   _  *
//*  / __/ _` | __/ _ \/ _` |/ _ \| '__| | | | *
//* | (_| (_| | ||  __/ (_| | (_) | |  | |_| | *
//*  \___\__,_|\__\___|\__, |\___/|_|   \__, | *
//*                    |___/            |___/  *
//===- include/pstore/command_line/category.hpp ---------------------------===//
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
/// \file category.hpp
/// \brief Defines option_category; a means to group switches in command-line help text.

#ifndef PSTORE_COMMAND_LINE_CATEGORY_HPP
#define PSTORE_COMMAND_LINE_CATEGORY_HPP

#include <string>

namespace pstore {
    namespace command_line {

        class option_category {
        public:
            explicit option_category (std::string const & title);
            std::string const & title () const noexcept { return title_; }

        private:
            std::string title_;
        };

    } // end namespace command_line
} // end namespace pstore

#endif // PSTORE_COMMAND_LINE_CATEGORY_HPP
