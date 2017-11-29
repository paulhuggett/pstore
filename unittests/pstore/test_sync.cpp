//*                        *
//*  ___ _   _ _ __   ___  *
//* / __| | | | '_ \ / __| *
//* \__ \ |_| | | | | (__  *
//* |___/\__, |_| |_|\___| *
//*      |___/             *
//===- unittests/pstore/test_sync.cpp -------------------------------------===//
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

#include "pstore/transaction.hpp"

#include <limits>
#include <memory>
#include <mutex>
#include <vector>

// 3rd party includes
#include "gmock/gmock.h"

// local includes
#include "check_for_error.hpp"
#include "empty_store.hpp"

namespace {
    class mock_mutex {
    public:
        void lock () {}
        void unlock () {}
    };


    class SyncFixture : public EmptyStore {
    public:
        void SetUp () override;
        void TearDown () override;

        using lock_guard = std::unique_lock<mock_mutex>;
        using transaction_type = pstore::transaction<lock_guard>;

        void add (transaction_type & transaction, std::string const & key,
                  std::string const & value);
        bool is_found (std::string const & key);
        void read (std::string const & key, std::string * const value_out);

    protected:
        mock_mutex mutex_;
        std::unique_ptr<pstore::database> db_;
    };

    // SetUp
    // ~~~~~
    void SyncFixture::SetUp () {
        EmptyStore::SetUp ();
        db_.reset (new pstore::database (file_));
        db_->set_vacuum_mode (pstore::database::vacuum_mode::disabled);
    }

    // TearDown
    // ~~~~~~~~
    void SyncFixture::TearDown () {
        db_.reset ();
        EmptyStore::TearDown ();
    }

    // add
    // ~~~
    void SyncFixture::add (transaction_type & transaction, std::string const & key,
                           std::string const & value) {

        auto where = pstore::address::null ();
        {
            // Allocate storage for string 'value' and copy the data into it.
            std::shared_ptr<char> ptr;
            std::tie (ptr, where) = transaction.alloc_rw<char> (value.length ());
            std::copy (std::begin (value), std::end (value), ptr.get ());
        }

        auto index = db_->get_write_index ();
        index->insert_or_assign (transaction, key, pstore::record{where, value.length ()});
    }

    // find
    // ~~~~
    bool SyncFixture::is_found (std::string const & key) {
        auto index = db_->get_write_index ();
        return index->find (key) != index->cend ();
    }

    // read
    // ~~~~
    void SyncFixture::read (std::string const & key, std::string * value_out) {
        auto index = db_->get_write_index ();
        auto const it = index->find (key);
        ASSERT_NE (it, index->cend ());

        pstore::record const & r = it->second;
        auto value = std::static_pointer_cast<char const> (db_->getro (r));
        *value_out = std::string{value.get (), r.size};
    }
} // (anonymous namespace)


TEST_F (SyncFixture, SyncBetweenVersions) {

    {
        transaction_type t1 = pstore::begin (*db_, lock_guard{mutex_});
        this->add (t1, "key0", "doesn't change");
        this->add (t1, "key1", "first value");
        t1.commit ();
    }
    {
        transaction_type t2 = pstore::begin (*db_, lock_guard{mutex_});
        this->add (t2, "key1", "second value");
        t2.commit ();
    }

    std::string value;

    this->read ("key1", &value);
    EXPECT_THAT (value, ::testing::StrEq ("second value"));
    this->read ("key0", &value);
    EXPECT_THAT (value, ::testing::StrEq ("doesn't change"));

    db_->sync (0);
    EXPECT_FALSE (this->is_found ("key0")) << "key0 should not be present at revision 0";
    EXPECT_FALSE (this->is_found ("key1")) << "key1 should not be present at revision 0";

    db_->sync (1);
    this->read ("key1", &value);
    EXPECT_THAT (value, ::testing::StrEq ("first value"));
    this->read ("key0", &value);
    EXPECT_THAT (value, ::testing::StrEq ("doesn't change"));

    db_->sync (2);
    this->read ("key1", &value);
    EXPECT_THAT (value, ::testing::StrEq ("second value"));
    this->read ("key0", &value);
    EXPECT_THAT (value, ::testing::StrEq ("doesn't change"));

    db_->sync (1);
    this->read ("key1", &value);
    EXPECT_THAT (value, ::testing::StrEq ("first value"));
    this->read ("key0", &value);
    EXPECT_THAT (value, ::testing::StrEq ("doesn't change"));
}

TEST_F (SyncFixture, SyncToBadVersions) {

    check_for_error ([this]() { db_->sync (1); }, pstore::error_code::unknown_revision);

    {
        transaction_type t1 = pstore::begin (*db_, lock_guard{mutex_});
        this->add (t1, "a", "first value");
        t1.commit ();
    }
    db_->sync (1);
    {
        transaction_type t2 = pstore::begin (*db_, lock_guard{mutex_});
        this->add (t2, "b", "second value");
        t2.commit ();
    }

    check_for_error ([this]() { db_->sync (3); }, pstore::error_code::unknown_revision);
}
// eof: unittests/pstore/test_sync.cpp
