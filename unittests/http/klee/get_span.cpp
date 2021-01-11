//*             _                            *
//*   __ _  ___| |_   ___ _ __   __ _ _ __   *
//*  / _` |/ _ \ __| / __| '_ \ / _` | '_ \  *
//* | (_| |  __/ |_  \__ \ |_) | (_| | | | | *
//*  \__, |\___|\__| |___/ .__/ \__,_|_| |_| *
//*  |___/               |_|                 *
//===- unittests/http/klee/get_span.cpp -----------------------------------===//
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
#include <cstring>
#include <vector>

#include <klee/klee.h>

#include "pstore/http/buffered_reader.hpp"

using IO = int;
using byte_span = pstore::gsl::span<std::uint8_t>;


int main (int argc, char * argv[]) {

    auto refill = [] (IO io, byte_span const & sp) {
        std::memset (sp.data (), 0, sp.size ());
        return pstore::error_or_n<IO, byte_span::iterator>{pstore::in_place, io, sp.end ()};
    };

    std::size_t buffer_size;
    klee_make_symbolic (&buffer_size, sizeof (buffer_size), "buffer_size");
    std::size_t requested_size;
    klee_make_symbolic (&requested_size, sizeof (requested_size), "requested_size");

    klee_assume (buffer_size < 5);
    klee_assume (requested_size < 5);

    std::vector<std::uint8_t> v;
    v.resize (requested_size);

    auto io = IO{};
    auto br = pstore::httpd::make_buffered_reader<decltype (io)> (refill, buffer_size);
    br.get_span (io, pstore::gsl::make_span (v));
}
