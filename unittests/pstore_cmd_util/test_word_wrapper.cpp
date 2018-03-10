//*                        _                                             *
//* __      _____  _ __ __| | __      ___ __ __ _ _ __  _ __   ___ _ __  *
//* \ \ /\ / / _ \| '__/ _` | \ \ /\ / / '__/ _` | '_ \| '_ \ / _ \ '__| *
//*  \ V  V / (_) | | | (_| |  \ V  V /| | | (_| | |_) | |_) |  __/ |    *
//*   \_/\_/ \___/|_|  \__,_|   \_/\_/ |_|  \__,_| .__/| .__/ \___|_|    *
//*                                              |_|   |_|               *
//===- unittests/pstore_cmd_util/test_word_wrapper.cpp --------------------===//
// Copyright (c) 2017-2018 by Sony Interactive Entertainment, Inc.
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
#include "pstore/cmd_util/cl/word_wrapper.hpp"
#include <vector>
#include "gmock/gmock.h"

using namespace pstore::cmd_util::cl;

TEST (WordWrapper, NoWrap) {
    std::string const text = "text";
    word_wrapper wr (text, text.length ());
    EXPECT_EQ (*wr, "text");
}

TEST (WordWrapper, TwoLines) {
    std::string const text = "one two";

    std::vector<std::string> lines;
    // Note this loop it hand-written to ensure that the test operations don't vary according to the
    // implementation of standard library functions.
    auto it = word_wrapper (text, 4);
    auto end = word_wrapper::end (text, 4);
    while (it != end) {
        std::string const & s = *it;
        lines.push_back (s);
        ++it;
    }
    EXPECT_THAT (lines, ::testing::ElementsAre ("one", "two"));
}

TEST (WordWrapper, LongWordShortLine) {
    std::string const text = "antidisestablishmentarianism is along word";

    std::vector<std::string> lines;
    // Note this loop it hand-written to ensure that the test operations don't vary according to the
    // implementation of standard library functions.
    auto it = word_wrapper (text, 4);
    std::string const & s = *it;
    EXPECT_EQ (s, "antidisestablishmentarianism");
}

// eof: unittests/pstore_cmd_util/test_word_wrapper.cpp
