//*  _   _      _        _    *
//* | |_(_) ___| | _____| |_  *
//* | __| |/ __| |/ / _ \ __| *
//* | |_| | (__|   <  __/ |_  *
//*  \__|_|\___|_|\_\___|\__| *
//*                           *
//===- unittests/pstore_mcrepo/test_ticket.cpp ----------------------------===//
// Copyright (c) 2017 by Sony Interactive Entertainment, Inc.
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

#include "pstore_mcrepo/ticket.hpp"
#include "transaction.hpp"

using namespace pstore::repo;


namespace {
    class TicketTest : public ::testing::Test {
    protected:
        transaction transaction_;
    };
} // namespace

TEST_F (TicketTest, Empty) {
    std::vector<ticket_member> m;
    pstore::record record = ticket::alloc (transaction_, pstore::address{}, m);
    auto t = reinterpret_cast<ticket const *> (record.addr.absolute ());

    assert (transaction_.get_storage ().begin ()->first ==
            reinterpret_cast<std::uint8_t const *> (t));
    assert (transaction_.get_storage ().begin ()->second.get () ==
            reinterpret_cast<std::uint8_t const *> (t));
    EXPECT_EQ (0U, t->size ());
    EXPECT_TRUE (t->empty ());
}

TEST_F (TicketTest, SingleMember) {
    constexpr auto output_file_path = pstore::address{64U};
    constexpr auto digest = pstore::index::digest{28U};
    constexpr auto name = pstore::address{32U};
    constexpr std::uint8_t linkage = 2U;
    constexpr bool is_comdat = true;

    ticket_member sm{digest, name, linkage, is_comdat};

    std::vector<ticket_member> v{sm};
    ticket::alloc (transaction_, output_file_path, v);

    auto t = reinterpret_cast<ticket const *> (transaction_.get_storage ().begin ()->first);

    EXPECT_EQ (1U, t->size ());
    EXPECT_FALSE (t->empty ());
    EXPECT_EQ (output_file_path, t->path ());
    EXPECT_EQ (sizeof (ticket), t->size_bytes ());
    EXPECT_EQ (digest, (*t)[0].digest.low ());
    EXPECT_EQ (name, (*t)[0].name);
    EXPECT_EQ (linkage, (*t)[0].linkage);
    EXPECT_EQ (is_comdat, (*t)[0].comdat);
}

TEST_F (TicketTest, MultipleMembers) {
    constexpr auto output_file_path = pstore::address{32U};
    constexpr auto digest1 = pstore::index::digest{128U};
    constexpr auto digest2 = pstore::index::digest{144U};
    constexpr auto name = pstore::address{16U};
    constexpr std::uint8_t linkage = 2U;
    constexpr bool is_comdat = false;

    ticket_member mm1{digest1, name, linkage, is_comdat};
    ticket_member mm2{digest2, name + 24U, linkage, !is_comdat};

    std::vector<ticket_member> v{mm1, mm2};
    ticket::alloc (transaction_, output_file_path, v);

    auto t = reinterpret_cast<ticket const *> (transaction_.get_storage ().begin ()->first);

    EXPECT_EQ (2U, t->size ());
    EXPECT_FALSE (t->empty ());
    EXPECT_EQ (80U, t->size_bytes ());
    for (auto const & m : *t) {
        EXPECT_EQ (linkage, m.linkage);
    }
}

// eof: unittests/pstore_mcrepo/test_ticket.cpp
