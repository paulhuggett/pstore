//===- unittests/core/test_hamt_map.cpp -----------------------------------===//
//*  _                     _                            *
//* | |__   __ _ _ __ ___ | |_   _ __ ___   __ _ _ __   *
//* | '_ \ / _` | '_ ` _ \| __| | '_ ` _ \ / _` | '_ \  *
//* | | | | (_| | | | | | | |_  | | | | | | (_| | |_) | *
//* |_| |_|\__,_|_| |_| |_|\__| |_| |_| |_|\__,_| .__/  *
//*                                             |_|     *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/core/hamt_map.hpp"

// Standard library includes
#include <random>
#include <list>

// 3rd party includes
#include <gtest/gtest.h>

// pstore includes
#include "pstore/core/index_types.hpp"
#include "pstore/core/transaction.hpp"

// local includes
#include "check_for_error.hpp"
#include "empty_store.hpp"

using namespace std::string_literals;
using pstore::gsl::not_null;

using index_pointer = pstore::index::details::index_pointer;
using branch = pstore::index::details::branch;
using linear_node = pstore::index::details::linear_node;

// *******************************************
// *                                         *
// *              IndexFixture               *
// *                                         *
// *******************************************

namespace {

  class IndexFixture : public testing::Test {
  public:
    IndexFixture ()
            : db_{store_.file ()} {
      db_.set_vacuum_mode (pstore::database::vacuum_mode::disabled);
    }

    using lock_guard = std::unique_lock<mock_mutex>;
    using transaction_type = pstore::transaction<lock_guard>;

  protected:
    mock_mutex mutex_;
    in_memory_store store_;
    pstore::database db_;
  };

} // end anonymous namespace

// Test initial address index pointer.
TEST_F (IndexFixture, InitAddress) {
  constexpr auto addr = pstore::address{1};
  index_pointer index{addr};
  EXPECT_EQ (pstore::address{1}, index.to_address ());
  EXPECT_FALSE (index.is_heap ());
}

// Test initial pointer index pointer.
TEST_F (IndexFixture, InternalSizeBytes) {
  EXPECT_EQ (24U, branch::size_bytes (1));
  EXPECT_EQ (32U, branch::size_bytes (2));
  EXPECT_EQ (528U, branch::size_bytes (64));
}

namespace {

  class DefaultIndexFixture : public IndexFixture {
  public:
    DefaultIndexFixture ()
            : index_{(new default_index{db_})} {}

  protected:
    using default_index = pstore::index::hamt_map<std::string, std::string>;
    std::unique_ptr<default_index> index_;
  };

} // end anonymous namespace

// Test default constructor.
TEST_F (DefaultIndexFixture, DefaultConstructor) {
  EXPECT_EQ (0U, index_->size ());
  EXPECT_TRUE (index_->empty ());
  EXPECT_EQ (index_->root ().to_address (), pstore::address::null ());
  EXPECT_TRUE (index_->root ().is_empty ());
}

// test iterator: empty index.
TEST_F (DefaultIndexFixture, EmptyBeginEqualsEnd) {
  default_index::const_iterator begin = index_->cbegin (db_);
  default_index::const_iterator end = index_->cend (db_);
  EXPECT_EQ (begin, end);
}

// test insert: index only contains a single leaf node.
TEST_F (DefaultIndexFixture, InsertSingle) {
  transaction_type t1 = begin (db_, lock_guard{mutex_});
  auto const first = std::make_pair ("a"s, "b"s);
  auto const second = std::make_pair ("a"s, "c"s);
  std::pair<default_index::iterator, bool> itp = index_->insert (t1, first);
  std::string const & key = (*itp.first).first;
  EXPECT_EQ ("a", key);
  EXPECT_TRUE (itp.second);
  itp = index_->insert (t1, second);
  std::string & value = (*itp.first).second;
  EXPECT_EQ ("b", value);
  EXPECT_FALSE (itp.second);
}

// test insert_or_assign: index only contains a single leaf node.
TEST_F (DefaultIndexFixture, UpsertSingle) {
  transaction_type t1 = begin (db_, lock_guard{mutex_});
  auto const first = std::make_pair ("a"s, "b"s);
  auto const second = std::make_pair ("a"s, "c"s);
  std::pair<default_index::iterator, bool> itp = index_->insert_or_assign (t1, first);
  std::string const & key = (*itp.first).first;
  EXPECT_EQ ("a", key);
  EXPECT_TRUE (itp.second);
  itp = index_->insert_or_assign (t1, second);
  std::string & value = (*itp.first).second;
  EXPECT_EQ ("c", value);
  EXPECT_FALSE (itp.second);
}

// test iterator: index only contains a single leaf node.
TEST_F (DefaultIndexFixture, InsertSingleIterator) {
  transaction_type t1 = begin (db_, lock_guard{mutex_});
  auto const first = std::make_pair ("a"s, "b"s);
  index_->insert_or_assign (t1, first);

  default_index::iterator begin = index_->begin (db_);
  default_index::iterator end = index_->end (db_);
  EXPECT_NE (begin, end);
  std::string const & v1 = (*begin).first;
  EXPECT_EQ (first.first, v1);
  ++begin;
  EXPECT_EQ (begin, end);
}

// test iterator: index contains an internal heap node.
TEST_F (DefaultIndexFixture, InsertHeap) {
  transaction_type t1 = begin (db_, lock_guard{mutex_});
  auto const first = std::make_pair ("a"s, "b"s);
  auto const second = std::make_pair ("c"s, "d"s);
  index_->insert_or_assign (t1, first);
  index_->insert_or_assign (t1, second);

  default_index::iterator begin = index_->begin (db_);
  default_index::iterator end = index_->end (db_);
  EXPECT_NE (begin, end);
  ++begin;
  EXPECT_NE (begin, end);
  ++begin;
  EXPECT_EQ (begin, end);
}

// test iterator: index only contains a leaf store node.
TEST_F (DefaultIndexFixture, InsertLeafStore) {
  transaction_type t1 = begin (db_, lock_guard{mutex_});
  auto const first = std::make_pair ("a"s, "b"s);
  index_->insert_or_assign (t1, first);
  index_->flush (t1, db_.get_current_revision ());

  default_index::const_iterator begin = index_->cbegin (db_);
  default_index::const_iterator end = index_->cend (db_);
  EXPECT_NE (begin, end);
  std::string const & v1 = (*begin).first;
  EXPECT_EQ (first.first, v1);
  begin++;
  EXPECT_EQ (begin, end);
}

// test iterator: index contains an internal store node.
TEST_F (DefaultIndexFixture, InsertInternalStoreIterator) {
  transaction_type t1 = begin (db_, lock_guard{mutex_});
  index_->insert_or_assign (t1, std::make_pair ("a"s, "b"s));
  index_->insert_or_assign (t1, std::make_pair ("c"s, "d"s));
  index_->flush (t1, db_.get_current_revision ());

  default_index::const_iterator begin = index_->cbegin (db_);
  default_index::const_iterator end = index_->cend (db_);
  EXPECT_NE (begin, end);
  begin++;
  EXPECT_NE (begin, end);
  begin++;
  EXPECT_EQ (begin, end);
}

// test insert: index contains an internal store node.
TEST_F (DefaultIndexFixture, InsertInternalStore) {
  transaction_type t1 = begin (db_, lock_guard{mutex_});
  std::pair<default_index::iterator, bool> itp1 = index_->insert (t1, std::make_pair ("a"s, "b"s));
  std::pair<default_index::iterator, bool> itp2 = index_->insert (t1, std::make_pair ("c"s, "d"s));

  std::string const & key1 = (*itp1.first).first;
  EXPECT_EQ ("a", key1);
  EXPECT_TRUE (itp1.second);
  std::string const & key2 = (*itp2.first).first;
  EXPECT_EQ ("c", key2);
  EXPECT_TRUE (itp2.second);

  index_->flush (t1, db_.get_current_revision ());

  std::pair<default_index::iterator, bool> itp3 = index_->insert (t1, std::make_pair ("c"s, "f"s));
  std::string & value = (*itp3.first).second;
  EXPECT_EQ ("d", value);
  EXPECT_FALSE (itp3.second);
}

// test insert_or_assign: index contains an internal store node.
TEST_F (DefaultIndexFixture, UpsertInternalStore) {
  transaction_type t1 = begin (db_, lock_guard{mutex_});
  std::pair<default_index::iterator, bool> itp1 =
    index_->insert_or_assign (t1, std::make_pair ("a"s, "b"s));
  std::pair<default_index::iterator, bool> itp2 =
    index_->insert_or_assign (t1, std::make_pair ("c"s, "d"s));

  std::string const & key1 = (*itp1.first).first;
  EXPECT_EQ ("a", key1);
  EXPECT_TRUE (itp1.second);
  std::string const & key2 = (*itp2.first).first;
  EXPECT_EQ ("c", key2);
  EXPECT_TRUE (itp2.second);

  index_->flush (t1, db_.get_current_revision ());

  std::pair<default_index::iterator, bool> itp3 =
    index_->insert_or_assign (t1, std::make_pair ("c"s, "f"s));
  std::string & value = (*itp3.first).second;
  EXPECT_EQ ("f", value);
  EXPECT_FALSE (itp3.second);
}

// *******************************************
// *                                         *
// *             hash_function               *
// *                                         *
// *******************************************

namespace {

  // An implementation of the hash function interface that has a hard-wired map from input
  // string to the corresponding hash value.
  class hash_function {
  public:
    using map = std::map<std::string, std::uint64_t>;
    explicit hash_function (map const & map)
            : map_ (map) {}
    std::uint64_t operator() (std::string const & str) const {
      auto it = map_.find (str);
      PSTORE_ASSERT (it != map_.end ());
      return it->second;
    }

  private:
    map const & map_;
  };

  using test_trie =
    pstore::index::hamt_map<std::string, std::string, hash_function, std::equal_to<std::string>>;

} // end anonymous namespace

// *******************************************
// *                                         *
// *          GenericIndexFixture            *
// *                                         *
// *******************************************

namespace {

  class GenericIndexFixture : public IndexFixture {
  protected:
    // insert or assign a new node into the index.
    static auto insert_or_assign (test_trie & index, transaction_type & transaction,
                                  std::string const & key) -> std::pair<test_trie::iterator, bool>;

    // insert or assign a new node into the index.
    static auto insert_or_assign (test_trie & index, transaction_type & transaction,
                                  std::string const & key, std::string const & value)
      -> std::pair<test_trie::iterator, bool>;

    // Return true if the key in the store, false otherwise.
    bool is_found (test_trie const & index, std::string const & key);

    void check_is_leaf (index_pointer node) const;
    void check_is_heap_branch (index_pointer node) const;
    void check_is_store_branch (index_pointer node) const;
  };

  // insert or assign
  // ~~~~~~~~~~~~~~~~
  auto GenericIndexFixture::insert_or_assign (test_trie & index, transaction_type & transaction,
                                              std::string const & key)
    -> std::pair<test_trie::iterator, bool> {
    return index.insert_or_assign (transaction, std::make_pair (key, "value "s + key));
  }

  auto GenericIndexFixture::insert_or_assign (test_trie & index, transaction_type & transaction,
                                              std::string const & key, std::string const & value)
    -> std::pair<test_trie::iterator, bool> {
    return index.insert_or_assign (transaction, key, value);
  }

  // is found
  // ~~~~~~~~
  bool GenericIndexFixture::is_found (test_trie const & index, std::string const & key) {
    return index.contains (db_, key);
  }

  // check is leaf
  // ~~~~~~~~~~~~~
  void GenericIndexFixture::check_is_leaf (index_pointer node) const {
    EXPECT_TRUE (node.is_address ());
    EXPECT_TRUE (node.is_leaf ());
  }

  // check is heap branch
  // ~~~~~~~~~~~~~~~~~~~~
  void GenericIndexFixture::check_is_heap_branch (index_pointer node) const {
    EXPECT_TRUE (node.is_heap ());
    EXPECT_TRUE (node.is_branch ());
  }

  // check is store branch
  // ~~~~~~~~~~~~~~~~~~~~~
  void GenericIndexFixture::check_is_store_branch (index_pointer node) const {
    EXPECT_TRUE (node.is_address ());
    EXPECT_TRUE (node.is_branch ());
  }

} // end anonymous namespace


namespace {

  class HamtRoundTrip : public testing::Test {
  public:
    HamtRoundTrip ()
            : db_{store_.file ()} {}

  protected:
    using lock_guard = std::unique_lock<mock_mutex>;
    using transaction_type = pstore::transaction<lock_guard>;
    using index_type = pstore::index::hamt_map<std::string, std::string>;

    mock_mutex mutex_;
    in_memory_store store_;
    pstore::database db_;
  };

} // end anonymous namespace

TEST_F (HamtRoundTrip, Empty) {
  pstore::typed_address<pstore::index::header_block> addr;
  index_type index1{db_, pstore::typed_address<pstore::index::header_block>::null ()};
  {
    auto t1 = begin (db_, std::unique_lock<mock_mutex>{mutex_});
    auto const size_before_flush = db_.size ();
    addr = index1.flush (t1, db_.get_current_revision ());
    EXPECT_EQ (db_.size (), size_before_flush);
    t1.commit ();
  }
  index_type index2{db_, addr};
  EXPECT_EQ (index2.size (), 0U);
}

TEST_F (HamtRoundTrip, LeafMember) {
  pstore::typed_address<pstore::index::header_block> addr;
  index_type index1{db_, pstore::typed_address<pstore::index::header_block>::null ()};
  {
    auto t1 = begin (db_, std::unique_lock<mock_mutex>{mutex_});
    index1.insert_or_assign (t1, index_type::value_type{"a", "a"});
    addr = index1.flush (t1, db_.get_current_revision ());
    t1.commit ();
  }

  index_type index2{db_, addr};
  ASSERT_EQ (index2.size (), 1U);
  auto const actual = *index2.begin (db_);
  EXPECT_EQ (actual, (index_type::value_type{"a", "a"}));
}

// ****************
// *              *
// *   OneLevel   *
// *              *
// ****************

namespace {

  class OneLevel : public GenericIndexFixture {
  protected:
    OneLevel ()
            : hash_{hashes_}
            , index_{new test_trie (
                db_, pstore::typed_address<pstore::index::header_block>::null (), hash_)} {}

    void SetUp () final {
      GenericIndexFixture::SetUp ();
      // With a known hash function (see map_) and the insertion order below, we should end
      // up with a trie which looks like:
      //
      // root_->bitmap = 0b1000000010001010
      //            +--------+--------+--------+--------+
      // root_ ->   | 000001 | 000011 | 000111 | 001111 |      (hash bits 0-5)
      //            +--------+--------+--------+--------+
      //                |       |          |       |
      //                v       |          |       |
      //               "b"      v          |       |      (0b000001)
      //                       "a"         v       |      (0b000011)
      //                                  "c"      v      (0b000111)
      //                                          "d"     (0b001111)
      std::uint64_t const first_hash = hash_ ("a");
      std::uint64_t const second_hash = hash_ ("b");
      std::uint64_t const third_hash = hash_ ("c");
      std::uint64_t const fourth_hash = hash_ ("d");
      auto const mask = (1U << 6) - 1U;
      EXPECT_NE (first_hash & mask, second_hash & mask);
      EXPECT_NE (first_hash & mask, third_hash & mask);
      EXPECT_NE (first_hash & mask, fourth_hash & mask);
      EXPECT_NE (second_hash & mask, third_hash & mask);
      EXPECT_NE (second_hash & mask, fourth_hash & mask);
      EXPECT_NE (third_hash & mask, fourth_hash & mask);
    }

    static std::map<std::string, std::uint64_t> const hashes_;

    hash_function hash_;
    std::unique_ptr<test_trie> index_;
  };

  std::map<std::string, std::uint64_t> const OneLevel::hashes_{
    {"a", UINT64_C (0b000011)},
    {"b", UINT64_C (0b000001)},
    {"c", UINT64_C (0b000111)},
    {"d", UINT64_C (0b001111)},
  };

} // end anonymous namespace

// insert_or_assign a single node ("a") into the database.
TEST_F (OneLevel, InsertFirstNode) {
  transaction_type t1 = begin (db_, lock_guard{mutex_});
  EXPECT_FALSE (this->is_found (*index_, "a")); // check the index::empty() function.
  std::pair<test_trie::iterator, bool> itp = this->insert_or_assign (*index_, t1, "a");
  std::string const & key = (*itp.first).first;
  EXPECT_EQ ("a", key);
  EXPECT_TRUE (itp.second);
  EXPECT_EQ (1U, index_->size ());
  EXPECT_TRUE (this->is_found (*index_, "a")) << "key \"a\" should be present in the index";
}

// insert_or_assign the second node ("b") into the existing leaf node ("a").
TEST_F (OneLevel, InsertSecondNode) {
  transaction_type t1 = begin (db_, lock_guard{mutex_});
  this->insert_or_assign (*index_, t1, "a");
  std::pair<test_trie::iterator, bool> itp = this->insert_or_assign (*index_, t1, "b");
  std::string const & key = (*itp.first).first;
  EXPECT_EQ ("b", key);
  EXPECT_TRUE (itp.second);
  EXPECT_EQ (2U, index_->size ());
  EXPECT_TRUE (this->is_found (*index_, "b")) << "key \"b\" should be present in the index";
  {
    // Check that the root is in heap as we expected.
    index_pointer root = index_->root ();
    this->check_is_heap_branch (root);
    branch const * const root_internal = root.untag<branch *> ();
    EXPECT_EQ (root_internal->get_bitmap (), 0b1010U);
    this->check_is_leaf ((*root_internal)[0]);
    this->check_is_leaf ((*root_internal)[1]);
    EXPECT_GT ((*root_internal)[0].to_address (), (*root_internal)[1].to_address ());
    index_->flush (t1, db_.get_current_revision ());
    EXPECT_NE (root, index_->root ());
    this->check_is_store_branch (index_->root ());
  }
}

TEST_F (OneLevel, InsertOfExistingKeyDoesNotResultInHeapNode) {
  transaction_type t1 = begin (db_, lock_guard{mutex_});
  index_->insert (t1, test_trie::value_type{"a", "a"});
  index_->insert (t1, test_trie::value_type{"b", "b"});
  index_->flush (t1, db_.get_current_revision ());

  EXPECT_FALSE (index_->root ().is_heap ());

  index_->insert (t1, test_trie::value_type{"a", "a2"});
  EXPECT_FALSE (index_->root ().is_heap ());
}

// insert_or_assign a new node into the store internal node.
TEST_F (OneLevel, InsertThirdNode) {
  transaction_type t1 = begin (db_, lock_guard{mutex_});
  OneLevel::insert_or_assign (*index_, t1, "a");
  OneLevel::insert_or_assign (*index_, t1, "b");
  EXPECT_FALSE (this->is_found (*index_, "c")); // check "c" is not in the index.
  index_->flush (t1, db_.get_current_revision ());
  // The index root is a store internal node. To insert_or_assign a new node, the store internal
  // node need to be copied into the heap.
  std::pair<test_trie::iterator, bool> const itp = OneLevel::insert_or_assign (*index_, t1, "c");
  std::string const & key = (*itp.first).first;
  EXPECT_EQ ("c", key);
  EXPECT_TRUE (itp.second);

  EXPECT_EQ (3U, index_->size ());
  EXPECT_TRUE (this->is_found (*index_, "c")) << "key \"c\" should be present in the index";
  {
    auto root = index_->root ();
    this->check_is_heap_branch (root);
    branch const * const root_internal = root.untag<branch *> ();
    EXPECT_EQ (root_internal->get_bitmap (), 0b10001010U);
    this->check_is_leaf ((*root_internal)[2]);
    EXPECT_GT ((*root_internal)[2].to_address (), (*root_internal)[1].to_address ());
    EXPECT_GT ((*root_internal)[2].to_address (), (*root_internal)[0].to_address ());
    index_->flush (t1, db_.get_current_revision ());
    EXPECT_NE (root, index_->root ());
    EXPECT_TRUE (this->is_found (*index_, "c")) << "key \"c\" should be present in the index";
  }
}

//  insert_or_assign a new node into the heap internal node.
TEST_F (OneLevel, InsertFourthNode) {
  transaction_type t1 = begin (db_, lock_guard{mutex_});
  this->insert_or_assign (*index_, t1, "a");
  this->insert_or_assign (*index_, t1, "b");
  this->insert_or_assign (*index_, t1, "c");

  // The node "d" is inserted into the internal heap node.
  std::pair<test_trie::iterator, bool> itp = this->insert_or_assign (*index_, t1, "d");
  std::string const & key = (*itp.first).first;
  EXPECT_EQ ("d", key);
  EXPECT_TRUE (itp.second);
  EXPECT_EQ (4U, index_->size ());
  EXPECT_TRUE (this->is_found (*index_, "d")) << "key \"d\" should be present in the index";
  {
    // Check that the trie was laid out as we expected.
    index_pointer root = index_->root ();
    this->check_is_heap_branch (root);
    branch const * const root_internal = root.untag<branch *> ();
    EXPECT_EQ (root_internal->get_bitmap (), 0b10000000'10001010U);
    EXPECT_EQ (4U, pstore::bit_count::pop_count (root_internal->get_bitmap ()));
    this->check_is_leaf ((*root_internal)[3]);
    EXPECT_GT ((*root_internal)[3].to_address (), (*root_internal)[2].to_address ());
    index_->flush (t1, db_.get_current_revision ());
    this->check_is_store_branch (index_->root ());
    EXPECT_TRUE (this->is_found (*index_, "d")) << "key \"d\" should be present in the index";
  }
}

//  Test forward iterator.
TEST_F (OneLevel, ForwardIteration) {
  transaction_type t1 = begin (db_, lock_guard{mutex_});
  this->insert_or_assign (*index_, t1, "a");
  this->insert_or_assign (*index_, t1, "b");
  this->insert_or_assign (*index_, t1, "c");
  this->insert_or_assign (*index_, t1, "d");

  // Check trie iterator in the heap.
  test_trie::iterator begin = index_->begin (db_);
  test_trie::iterator end = index_->end (db_);
  EXPECT_NE (begin, end);

  std::string const v1 = (*begin).first;
  EXPECT_EQ ("b", v1);
  ++begin;
  std::string const & v2 = (*begin).first;
  EXPECT_EQ ("a", v2);
  ++begin;
  std::string const & v3 = (*begin).first;
  EXPECT_EQ ("c", v3);
  ++begin;
  std::string const & v4 = (*begin).first;
  EXPECT_EQ ("d", v4);
  ++begin;
  EXPECT_EQ (begin, end);

  index_->flush (t1, db_.get_current_revision ());
  this->check_is_store_branch (index_->root ());

  // Check trie iterator in the store.
  test_trie::const_iterator cbegin = index_->cbegin (db_);
  test_trie::const_iterator cend = index_->cend (db_);
  EXPECT_NE (cbegin, cend);

  std::string const v5 = cbegin->first;
  EXPECT_EQ ("b", v5);
  ++cbegin;
  std::string const v6 = cbegin->first;
  EXPECT_EQ ("a", v6);
  ++cbegin;
  std::string const v7 = cbegin->first;
  EXPECT_EQ ("c", v7);
  ++cbegin;
  std::string const v8 = cbegin->first;
  EXPECT_EQ ("d", v8);
  ++cbegin;
  EXPECT_EQ (cbegin, cend);
}

TEST_F (OneLevel, UpsertIteration) {
  transaction_type t1 = begin (db_, lock_guard{mutex_});
  this->insert_or_assign (*index_, t1, "a");
  this->insert_or_assign (*index_, t1, "c");
  this->insert_or_assign (*index_, t1, "d");
  std::pair<test_trie::iterator, bool> itp = this->insert_or_assign (*index_, t1, "b");

  // Check trie iterator in the heap.
  test_trie::iterator begin = itp.first;
  test_trie::iterator end = index_->end (db_);
  EXPECT_NE (begin, end);

  std::string const v1 = (*begin).first;
  EXPECT_EQ ("b", v1);
  ++begin;
  std::string const & v2 = (*begin).first;
  EXPECT_EQ ("a", v2);
  ++begin;
  std::string const & v3 = (*begin).first;
  EXPECT_EQ ("c", v3);
  ++begin;
  std::string const & v4 = (*begin).first;
  EXPECT_EQ ("d", v4);
  ++begin;
  EXPECT_EQ (begin, end);

  index_->flush (t1, db_.get_current_revision ());
  itp = this->insert_or_assign (*index_, t1, "b");
  begin = itp.first;

  std::string const v5 = begin->first;
  EXPECT_EQ ("b", v5);
  ++begin;
  std::string const v6 = begin->first;
  EXPECT_EQ ("a", v6);
  ++begin;
  std::string const v7 = begin->first;
  EXPECT_EQ ("c", v7);
  ++begin;
  std::string const v8 = begin->first;
  EXPECT_EQ ("d", v8);
  ++begin;
  EXPECT_EQ (begin, end);
}

// *******************************************
// *                                         *
// *      TwoValuesWithHashCollision         *
// *                                         *
// *******************************************

namespace {

  // Careful choice of input and our knowledge of the hash function's behaviour means that
  // we can choose two inputs whose hashes collide in the bottom 6 bits.
  class TwoValuesWithHashCollision : public GenericIndexFixture {
  protected:
    TwoValuesWithHashCollision ()
            : hash_{hashes_}
            , index_{new test_trie (
                db_, pstore::typed_address<pstore::index::header_block>::null (), hash_)} {}

    void SetUp () final {
      GenericIndexFixture::SetUp ();
      this->check_collision (hash_ ("a"), hash_ ("b"), 1);
      this->check_collision (hash_ ("a"), hash_ ("c"), 1);
      this->check_collision (hash_ ("e"), hash_ ("f"), 10);
    }

    static std::map<std::string, std::uint64_t> const hashes_;

    hash_function hash_;
    std::unique_ptr<test_trie> index_;

    void check_collision (std::uint64_t const first_hash, std::uint64_t const second_hash,
                          unsigned collision_level);
  };

  constexpr auto lower6 = UINT64_C (0b000000);
  constexpr auto lower60 =
    UINT64_C (0b0010'01001000'00011100'01100001'01000100'00001100'00100000'01000000);

  std::map<std::string, std::uint64_t> const TwoValuesWithHashCollision::hashes_{
    //"a" and "b" collide in the lower 6 bits.
    {"a", UINT64_C (0b000000) << 6 | lower6},
    {"b", UINT64_C (0b000001) << 6 | lower6},
    {"c", UINT64_C (0b000010) << 6 | lower6},
    // "e" and "f" collide in lower 60 bits.
    {"e", UINT64_C (0b1100) << 60 | lower60},
    {"f", UINT64_C (0b1111) << 60 | lower60},
    // "g" and "h" collide in all hash bits.
    {"g", 0},
    {"h", 0},
    {"i", 0},
  };

  void TwoValuesWithHashCollision::check_collision (std::uint64_t const first_hash,
                                                    std::uint64_t const second_hash,
                                                    unsigned collision_level) {
    ASSERT_TRUE (collision_level < 11);
    auto const shift = 6U * collision_level;
    auto const mask = (UINT64_C (1) << shift) - 1U;
    EXPECT_EQ (first_hash & mask, second_hash & mask);

    auto const shifted_first_hash = first_hash >> shift;
    auto const shifted_second_hash = second_hash >> shift;
    auto const shifted_mask = (UINT64_C (1) << 6) - 1U;
    EXPECT_NE (shifted_first_hash & shifted_mask, shifted_second_hash & shifted_mask);
  }

} // end anonymous namespace

TEST_F (TwoValuesWithHashCollision, LeafLevelOneCollision) {
  transaction_type t1 = begin (db_, lock_guard{mutex_});

  // First insert_or_assign should be very conventional. The result will be a trie whose root
  // points to an address of the "first" string. Second insert_or_assign should trigger the
  // insertion of an additional internal node in the trie. This unit test checks the
  // insert_into_leaf function.
  //
  //              +--------+
  // root_ ->     | 000000 |       (hash bits 0-5)
  //              +--------+
  //                   |
  //                   v
  //           +--------+--------+
  // level1 -> | 000000 | 000001 |  (hash bits 6-11)
  //           +--------+--------+
  //                |       |
  //                v       |
  //               "a"      v       (0b000000'000000)
  //                       "b"      (0b000001'000000)

  this->insert_or_assign (*index_, t1, "a");
  this->insert_or_assign (*index_, t1, "b");
  EXPECT_EQ (2U, index_->size ());
  EXPECT_TRUE (this->is_found (*index_, "a")) << "key \"a\" should be present in the index";
  EXPECT_TRUE (this->is_found (*index_, "b")) << "key \"b\" should be present in the index";
  {
    // Check that the trie was laid out as we expected in the heap.
    index_pointer root = index_->root ();
    this->check_is_heap_branch (root);
    branch const * const root_internal = root.untag<branch *> ();
    EXPECT_EQ (root_internal->get_bitmap (), 0b1U);

    auto level1 = (*root_internal)[0];
    this->check_is_heap_branch (level1);

    auto level1_internal = level1.untag<branch *> ();
    EXPECT_EQ (level1_internal->get_bitmap (), 0b11U);
    this->check_is_leaf ((*level1_internal)[0]);
    this->check_is_leaf ((*level1_internal)[1]);
    index_->flush (t1, db_.get_current_revision ());
  }
  {
    // Check that the trie was laid out as we expected in the store.
    index_pointer root = index_->root ();
    this->check_is_store_branch (root);
    auto root_address = root.untag_address<branch> ();
    std::shared_ptr<branch const> const root_internal = branch::read_node (db_, root_address);
    EXPECT_EQ (root_internal->get_bitmap (), 0b1U);

    auto level1 = (*root_internal)[0];
    this->check_is_store_branch (level1);
    auto level1_address = level1.untag_address<branch> ();
    auto level1_internal = branch::read_node (db_, level1_address);
    EXPECT_EQ (level1_internal->get_bitmap (), 0b11U);
    this->check_is_leaf ((*level1_internal)[0]);
    this->check_is_leaf ((*level1_internal)[1]);
    EXPECT_TRUE (this->is_found (*index_, "a")) << "key \"a\" should be present in the index";
    EXPECT_TRUE (this->is_found (*index_, "b")) << "key \"b\" should be present in the index";
  }
}

TEST_F (TwoValuesWithHashCollision, InternalCollision) {
  transaction_type t1 = begin (db_, lock_guard{mutex_});

  // After inserting "a" and "b", the index root is an internal node. when inserting "c", this
  // unit test checks the insert_or_assign_node function. With a known hash function and the
  // insertion order below, we should end up with a trie which looks like:
  //
  //              +--------+
  // root_ ->     | 000000 |                      (hash bits 0-5)
  //              +--------+
  //                   |
  //                   v
  //           +--------+--------+--------+
  // level1 -> | 000000 | 000001 | 000010 |       (hash bits 6-11)
  //           +--------+--------+--------+
  //                |       |        |
  //                v       |        |
  //               "a"      v        |            (0b000000'000000)
  //                       "b"       v            (0b000001'000000)
  //                                "c"           (0b000010'000000)
  this->insert_or_assign (*index_, t1, "a");
  this->insert_or_assign (*index_, t1, "b");
  this->insert_or_assign (*index_, t1, "c");
  EXPECT_EQ (3U, index_->size ());
  EXPECT_TRUE (this->is_found (*index_, "c")) << "key \"c\" should be present in the index";
  {
    // Check that the trie was laid out as we expected in the heap.
    index_pointer root = index_->root ();
    this->check_is_heap_branch (root);
    branch const * const root_internal = root.untag<branch *> ();
    EXPECT_EQ (root_internal->get_bitmap (), 0b1U);

    auto level1 = (*root_internal)[0];
    this->check_is_heap_branch (level1);

    auto level1_internal = level1.untag<branch *> ();
    EXPECT_EQ (level1_internal->get_bitmap (), 0b111U);
    this->check_is_leaf ((*level1_internal)[0]);
    this->check_is_leaf ((*level1_internal)[1]);
    this->check_is_leaf ((*level1_internal)[2]);
    index_->flush (t1, db_.get_current_revision ());
  }
  {
    // Check that the trie was laid out as we expected in the store.
    index_pointer root = index_->root ();
    this->check_is_store_branch (root);
    auto root_address = root.untag_address<branch> ();
    std::shared_ptr<branch const> const root_internal = branch::read_node (db_, root_address);
    EXPECT_EQ (root_internal->get_bitmap (), 0b1U);

    auto level1 = (*root_internal)[0];
    this->check_is_store_branch (level1);
    auto level1_address = level1.untag_address<branch> ();
    auto level1_internal = branch::read_node (db_, level1_address);
    EXPECT_EQ (level1_internal->get_bitmap (), 0b111U);
    this->check_is_leaf ((*level1_internal)[0]);
    this->check_is_leaf ((*level1_internal)[1]);
    EXPECT_TRUE (this->is_found (*index_, "a")) << "key \"a\" should be present in the index";
    EXPECT_TRUE (this->is_found (*index_, "b")) << "key \"b\" should be present in the index";
    EXPECT_TRUE (this->is_found (*index_, "c")) << "key \"c\" should be present in the index";
  }
}

TEST_F (TwoValuesWithHashCollision, LevelOneCollisionIterator) {
  transaction_type t1 = begin (db_, lock_guard{mutex_});
  this->insert_or_assign (*index_, t1, "a");
  this->insert_or_assign (*index_, t1, "b");
  this->insert_or_assign (*index_, t1, "c");

  // Check trie iterator in the heap.
  test_trie::iterator begin = index_->begin (db_);
  test_trie::iterator end = index_->end (db_);
  EXPECT_NE (begin, end);

  std::string const & v1 = (*begin).first;
  EXPECT_EQ ("a", v1);
  ++begin;
  std::string const & v2 = (*begin).first;
  EXPECT_EQ ("b", v2);
  ++begin;
  std::string const & v3 = (*begin).first;
  EXPECT_EQ ("c", v3);
  ++begin;
  EXPECT_EQ (begin, end);

  index_->flush (t1, db_.get_current_revision ());
  this->check_is_store_branch (index_->root ());

  // Check trie iterator in the store.
  test_trie::const_iterator cbegin = index_->cbegin (db_);
  test_trie::const_iterator cend = index_->cend (db_);
  EXPECT_NE (cbegin, cend);

  std::string const v5 = cbegin->first;
  EXPECT_EQ ("a", v5);
  ++cbegin;
  std::string const v6 = cbegin->first;
  EXPECT_EQ ("b", v6);
  ++cbegin;
  std::string const v7 = cbegin->first;
  EXPECT_EQ ("c", v7);
  ++cbegin;
  EXPECT_EQ (cbegin, cend);
}

TEST_F (TwoValuesWithHashCollision, LevelOneCollisionUpsertIterator) {
  transaction_type t1 = begin (db_, lock_guard{mutex_});
  this->insert_or_assign (*index_, t1, "b");
  this->insert_or_assign (*index_, t1, "c");
  std::pair<test_trie::iterator, bool> itp = this->insert_or_assign (*index_, t1, "a");

  // Check trie iterator in the heap.
  EXPECT_TRUE (itp.second);
  test_trie::iterator begin = itp.first;
  test_trie::iterator end = index_->end (db_);
  EXPECT_NE (begin, end);

  std::string const & v1 = (*begin).first;
  EXPECT_EQ ("a", v1);
  ++begin;
  std::string const & v2 = (*begin).first;
  EXPECT_EQ ("b", v2);
  ++begin;
  std::string const & v3 = (*begin).first;
  EXPECT_EQ ("c", v3);
  ++begin;
  EXPECT_EQ (begin, end);

  index_->flush (t1, db_.get_current_revision ());

  // Check trie iterator in the store.
  itp = this->insert_or_assign (*index_, t1, "a");
  EXPECT_FALSE (itp.second);
  // Check trie iterator in the heap.
  begin = itp.first;
  EXPECT_NE (begin, end);

  std::string const v5 = begin->first;
  EXPECT_EQ ("a", v5);
  ++begin;
  std::string const v6 = begin->first;
  EXPECT_EQ ("b", v6);
  ++begin;
  std::string const v7 = begin->first;
  EXPECT_EQ ("c", v7);
  ++begin;
  EXPECT_EQ (begin, end);
}

TEST_F (TwoValuesWithHashCollision, LeafLevelTenCollision) {
  transaction_type t1 = begin (db_, lock_guard{mutex_});

  // First insert_or_assign should be very conventional. The result will be a trie whose root
  // points to an address of the "first" string. Second insert_or_assign should trigger the
  // insertion of an additional internal node in the trie. This unit test checks the
  // insert_into_leaf function.
  //
  //              +--------+
  // root_ ->     | 000000 |       (hash bits 0-5)
  //              +--------+
  //                   |
  //                   v
  //              +--------+
  // level1 ->    | 000001 |       (hash bits 6-11)
  //              +--------+
  //                   |
  //                   :
  //                   |
  //                   v
  //              +--------+
  // level9 ->    | 001001 |       (hash bits 54-59)
  //              +--------+
  //                   |
  //                   v
  //           +--------+--------+
  // level10-> | 001100 | 001111 |  (hash bits 60-63)
  //           +--------+--------+
  //                |       |
  //                v       |
  //               "e"      v       (0b1100 001001 001000 000111 000110 000101 000100 000011
  //               000010 000001 000000)
  //                       "f"      (0b1111 001001 001000 000111 000110 000101 000100 000011
  //                       000010 000001 000000)

  this->insert_or_assign (*index_, t1, "e");
  this->insert_or_assign (*index_, t1, "f");

  EXPECT_TRUE (this->is_found (*index_, "e")) << "key \"e\" should be present in the index";
  EXPECT_TRUE (this->is_found (*index_, "f")) << "key \"f\" should be present in the index";
  {
    // Check that the trie was laid out as we expected in the heap.
    index_pointer root = index_->root ();
    this->check_is_heap_branch (root);
    branch const * const root_internal = root.untag<branch *> ();
    EXPECT_EQ (root_internal->get_bitmap (), 0b1U);

    auto const level1_internal = (*root_internal)[0].untag<branch *> ();
    auto const level2_internal = (*level1_internal)[0].untag<branch *> ();
    auto const level3_internal = (*level2_internal)[0].untag<branch *> ();
    auto const level4_internal = (*level3_internal)[0].untag<branch *> ();
    auto const level5_internal = (*level4_internal)[0].untag<branch *> ();
    EXPECT_EQ (level5_internal->get_bitmap (), 0b100000U);

    auto const level6_internal = (*level5_internal)[0].untag<branch *> ();
    auto const level7_internal = (*level6_internal)[0].untag<branch *> ();
    auto const level8_internal = (*level7_internal)[0].untag<branch *> ();
    auto const level9_internal = (*level8_internal)[0].untag<branch *> ();
    auto const level10_internal = (*level9_internal)[0].untag<branch *> ();
    EXPECT_EQ (level10_internal->get_bitmap (), 0b1001000000000000U);

    this->check_is_leaf ((*level10_internal)[0]);
    this->check_is_leaf ((*level10_internal)[1]);
    index_->flush (t1, db_.get_current_revision ());
  }
  {
    // Check that the trie was laid out as we expected in the store.
    index_pointer root = index_->root ();
    this->check_is_store_branch (root);
    auto const root_internal = branch::read_node (db_, root.untag_address<branch> ());
    auto const level1 = (*root_internal)[0];
    auto const level1_internal = branch::read_node (db_, level1.untag_address<branch> ());
    auto const level2 = (*level1_internal)[0];
    auto const level2_internal = branch::read_node (db_, level2.untag_address<branch> ());
    auto const level3 = (*level2_internal)[0];
    auto const level3_internal = branch::read_node (db_, level3.untag_address<branch> ());
    auto const level4 = (*level3_internal)[0];
    auto const level4_internal = branch::read_node (db_, level4.untag_address<branch> ());
    auto const level5 = (*level4_internal)[0];
    auto const level5_internal = branch::read_node (db_, level5.untag_address<branch> ());
    EXPECT_EQ (level5_internal->get_bitmap (), 0b100000U);
    auto const level6 = (*level5_internal)[0];
    auto const level6_internal = branch::read_node (db_, level6.untag_address<branch> ());
    auto const level7 = (*level6_internal)[0];
    auto const level7_internal = branch::read_node (db_, level7.untag_address<branch> ());
    auto const level8 = (*level7_internal)[0];
    auto const level8_internal = branch::read_node (db_, level8.untag_address<branch> ());
    auto const level9 = (*level8_internal)[0];
    auto const level9_internal = branch::read_node (db_, level9.untag_address<branch> ());
    auto const level10 = (*level9_internal)[0];
    auto const level10_internal = branch::read_node (db_, level10.untag_address<branch> ());
    EXPECT_EQ (level10_internal->get_bitmap (), 0b10010000'00000000U);
    this->check_is_leaf ((*level10_internal)[0]);
    this->check_is_leaf ((*level10_internal)[1]);
    EXPECT_TRUE (this->is_found (*index_, "e")) << "key \"e\" should be present in the index";
    EXPECT_TRUE (this->is_found (*index_, "f")) << "key \"f\" should be present in the index";
  }
}

TEST_F (TwoValuesWithHashCollision, LevelTenCollisionIterator) {
  transaction_type t1 = begin (db_, lock_guard{mutex_});
  this->insert_or_assign (*index_, t1, "e");
  this->insert_or_assign (*index_, t1, "f");

  // Check trie iterator in the heap.
  test_trie::iterator begin = index_->begin (db_);
  test_trie::iterator end = index_->end (db_);
  EXPECT_NE (begin, end);

  std::string const & v1 = (*begin).first;
  EXPECT_EQ ("e", v1);
  ++begin;
  std::string const & v2 = (*begin).first;
  EXPECT_EQ ("f", v2);
  ++begin;
  EXPECT_EQ (begin, end);

  index_->flush (t1, db_.get_current_revision ());
  this->check_is_store_branch (index_->root ());

  // Check trie iterator in the store.
  begin = index_->begin (db_);
  EXPECT_NE (begin, end);

  std::string const v3 = begin->first;
  EXPECT_EQ ("e", v3);
  ++begin;
  std::string const v4 = begin->first;
  EXPECT_EQ ("f", v4);
  ++begin;
  EXPECT_EQ (begin, end);
}

TEST_F (TwoValuesWithHashCollision, LevelTenCollisionUpsertIterator) {
  transaction_type t1 = begin (db_, lock_guard{mutex_});
  this->insert_or_assign (*index_, t1, "f");
  std::pair<test_trie::iterator, bool> itp = this->insert_or_assign (*index_, t1, "e");

  // Check trie iterator in the heap.
  EXPECT_TRUE (itp.second);
  test_trie::iterator begin = itp.first;
  test_trie::iterator end = index_->end (db_);
  EXPECT_NE (begin, end);

  std::string const & v1 = (*begin).first;
  EXPECT_EQ ("e", v1);
  ++begin;
  std::string const & v2 = (*begin).first;
  EXPECT_EQ ("f", v2);
  ++begin;
  EXPECT_EQ (begin, end);

  index_->flush (t1, db_.get_current_revision ());
  itp = this->insert_or_assign (*index_, t1, "e");
  EXPECT_FALSE (itp.second);

  // Check trie iterator in the store.
  begin = itp.first;
  EXPECT_NE (begin, end);

  std::string const v3 = begin->first;
  EXPECT_EQ ("e", v3);
  ++begin;
  std::string const v4 = begin->first;
  EXPECT_EQ ("f", v4);
  ++begin;
  EXPECT_EQ (begin, end);
}

TEST_F (TwoValuesWithHashCollision, LevelTenCollisionInsert) {
  transaction_type t1 = begin (db_, lock_guard{mutex_});
  index_->insert (t1, std::make_pair ("f"s, "value f"s));
  std::pair<test_trie::iterator, bool> itp = index_->insert (t1, std::make_pair ("e"s, "value e"s));
  EXPECT_TRUE (itp.second);
  std::string const & v = (*itp.first).second;
  EXPECT_EQ ("value e", v);

  itp = index_->insert (t1, std::make_pair ("e"s, "new value e"s));
  EXPECT_FALSE (itp.second);
  std::string const & v1 = (*itp.first).second;
  EXPECT_EQ ("value e", v1);
}

TEST_F (TwoValuesWithHashCollision, LeafLevelLinearCase) {
  transaction_type t1 = begin (db_, lock_guard{mutex_});

  //
  //              +--------+
  // root_ ->     | 000000 |       (hash bits 0-5)
  //              +--------+
  //                   |
  //                   :
  //                   |
  //                   v
  //              +--------+
  // level10 ->   |  0000  |       (hash bits 60-63)
  //              +--------+
  //                   |
  //                   v
  //            +---+---+---+
  // level11 -> | 0 | 1 | 2 |
  //            +---+---+---+
  //              |   |   |
  //              v   |   |
  //             "g"  v   |          ( 0 )
  //                 "h"  v          ( 0 )
  //                     "i"         ( 0 )

  this->insert_or_assign (*index_, t1, "g");
  this->insert_or_assign (*index_, t1, "h");
  this->insert_or_assign (*index_, t1, "i");
  EXPECT_TRUE (this->is_found (*index_, "g")) << "key 'g' should be present in the index (heap)";
  EXPECT_TRUE (this->is_found (*index_, "h")) << "key 'h' should be present in the index (heap)";
  EXPECT_TRUE (this->is_found (*index_, "i")) << "key 'i' should be present in the index (heap)";
  {
    // Check that the trie was laid out as we expected in the heap.
    index_pointer root = index_->root ();
    this->check_is_heap_branch (root);

    auto const root_internal = root.untag<branch *> ();
    auto const level1_internal = (*root_internal)[0].untag<branch *> ();
    auto const level2_internal = (*level1_internal)[0].untag<branch *> ();
    auto const level3_internal = (*level2_internal)[0].untag<branch *> ();
    auto const level4_internal = (*level3_internal)[0].untag<branch *> ();
    auto const level5_internal = (*level4_internal)[0].untag<branch *> ();
    auto const level6_internal = (*level5_internal)[0].untag<branch *> ();
    auto const level7_internal = (*level6_internal)[0].untag<branch *> ();
    auto const level8_internal = (*level7_internal)[0].untag<branch *> ();
    auto const level9_internal = (*level8_internal)[0].untag<branch *> ();
    auto const level10_internal = (*level9_internal)[0].untag<branch *> ();
    EXPECT_TRUE ((*level10_internal)[0].is_linear ());
    auto const level11_linear = (*level10_internal)[0].untag<linear_node *> ();
    EXPECT_EQ (level11_linear->size (), 3U);
    EXPECT_EQ (level11_linear->size_bytes (), 40U);
    EXPECT_NE ((*level11_linear)[0], pstore::address::null ());
    index_->flush (t1, db_.get_current_revision ());
  }
  {
    // Check that the trie was laid out as we expected in the store.
    index_pointer root = index_->root ();
    this->check_is_store_branch (root);

    auto const root_internal = branch::read_node (db_, root.untag_address<branch> ());
    auto const level1 = (*root_internal)[0];
    auto const level1_internal = branch::read_node (db_, level1.untag_address<branch> ());
    auto const level2 = (*level1_internal)[0];
    auto const level2_internal = branch::read_node (db_, level2.untag_address<branch> ());
    auto const level3 = (*level2_internal)[0];
    auto const level3_internal = branch::read_node (db_, level3.untag_address<branch> ());
    auto const level4 = (*level3_internal)[0];
    auto const level4_internal = branch::read_node (db_, level4.untag_address<branch> ());
    auto const level5 = (*level4_internal)[0];
    auto const level5_internal = branch::read_node (db_, level5.untag_address<branch> ());
    auto const level6 = (*level5_internal)[0];
    auto const level6_internal = branch::read_node (db_, level6.untag_address<branch> ());
    auto const level7 = (*level6_internal)[0];
    auto const level7_internal = branch::read_node (db_, level7.untag_address<branch> ());
    auto const level8 = (*level7_internal)[0];
    auto const level8_internal = branch::read_node (db_, level8.untag_address<branch> ());
    auto const level9 = (*level8_internal)[0];
    auto const level9_internal = branch::read_node (db_, level9.untag_address<branch> ());
    auto const level10 = (*level9_internal)[0];
    auto const level10_internal = branch::read_node (db_, level10.untag_address<branch> ());
    auto const level11 = (*level10_internal)[0];

    std::shared_ptr<linear_node const> sptr;
    linear_node const * level11_linear = nullptr;
    std::tie (sptr, level11_linear) =
      linear_node::get_node (db_, index_pointer{level11.untag_address<linear_node> ()});

    EXPECT_EQ (level11_linear->size (), 3U);
    EXPECT_TRUE (this->is_found (*index_, "g")) << "key 'g' should be present in the index (store)";
    EXPECT_TRUE (this->is_found (*index_, "h")) << "key 'h' should be present in the index (store)";
    EXPECT_TRUE (this->is_found (*index_, "i")) << "key 'i' should be present in the index (store)";
  }
}

TEST_F (TwoValuesWithHashCollision, LeafLevelLinearCaseIterator) {
  transaction_type t1 = begin (db_, lock_guard{mutex_});
  this->insert_or_assign (*index_, t1, "g");
  this->insert_or_assign (*index_, t1, "h");
  this->insert_or_assign (*index_, t1, "i");

  // Check trie iterator in the heap.
  test_trie::iterator begin = index_->begin (db_);
  test_trie::iterator end = index_->end (db_);
  EXPECT_NE (begin, end);

  std::string const & v1 = (*begin).first;
  EXPECT_EQ ("g", v1);
  ++begin;
  std::string const & v2 = (*begin).first;
  EXPECT_EQ ("h", v2);
  ++begin;
  std::string const & v3 = (*begin).first;
  EXPECT_EQ ("i", v3);
  ++begin;
  EXPECT_EQ (begin, end);

  index_->flush (t1, db_.get_current_revision ());
  this->check_is_store_branch (index_->root ());

  // Check trie iterator in the store.
  begin = index_->begin (db_);
  EXPECT_NE (begin, end);

  std::string const v4 = begin->first;
  EXPECT_EQ ("g", v4);
  ++begin;
  std::string const v5 = begin->first;
  EXPECT_EQ ("h", v5);
  ++begin;
  std::string const v6 = begin->first;
  EXPECT_EQ ("i", v6);
  ++begin;
  EXPECT_EQ (begin, end);

  this->insert_or_assign (*index_, t1, "g", "new value g");
  begin = index_->begin (db_);
  std::string const & v7 = (*begin).second;
  EXPECT_EQ ("new value g", v7);

  {
    auto value = "second new g";
    test_trie::iterator it = this->insert_or_assign (*index_, t1, "g", value).first;
    std::string const & v8 = (*it).second;
    EXPECT_EQ (value, v8);
  }
}

TEST_F (TwoValuesWithHashCollision, LeafLevelLinearUpsertIterator) {
  transaction_type t1 = begin (db_, lock_guard{mutex_});
  this->insert_or_assign (*index_, t1, "g");
  {
    std::pair<test_trie::iterator, bool> itp = this->insert_or_assign (*index_, t1, "h");
    EXPECT_TRUE (itp.second);
    EXPECT_EQ ((*itp.first).first, "h"s);
  }
  this->insert_or_assign (*index_, t1, "i");

  // Check trie iterator in the heap.
  test_trie::const_iterator first = index_->find (db_, "h"s);
  test_trie::const_iterator last = index_->end (db_);
  EXPECT_NE (first, last);
  std::string const & v1 = (*first).first;
  EXPECT_EQ ("h", v1);
  ++first;
  std::string const & v2 = (*first).first;
  EXPECT_EQ ("i", v2);
  ++first;
  EXPECT_EQ (first, last);

  index_->flush (t1, db_.get_current_revision ());

  std::pair<test_trie::iterator, bool> itp =
    this->insert_or_assign (*index_, t1, "g", "new value g");
  EXPECT_FALSE (itp.second);
  // Check trie iterator in the store.
  first = itp.first;
  EXPECT_NE (first, last);
  std::string const v4 = first->first;
  EXPECT_EQ ("g", v4);
  ++first;
  std::string const v5 = first->first;
  EXPECT_EQ ("h", v5);
  ++first;
  std::string const v6 = first->first;
  EXPECT_EQ ("i", v6);
  ++first;
  EXPECT_EQ (first, last);

  // Check assigned new value.
  std::string const & v7 = (*itp.first).second;
  EXPECT_EQ ("new value g", v7);
}

TEST_F (TwoValuesWithHashCollision, LeafLevelLinearInsertIterator) {
  transaction_type t1 = begin (db_, lock_guard{mutex_});
  index_->insert (t1, std::make_pair ("g"s, "value g"s));
  index_->insert (t1, std::make_pair ("h"s, "value h"s));
  std::pair<test_trie::iterator, bool> itp = index_->insert (t1, std::make_pair ("i"s, "value i"s));
  EXPECT_TRUE (itp.second);
  index_->flush (t1, db_.get_current_revision ());

  itp = index_->insert (t1, std::make_pair ("g"s, "new value g"s));
  EXPECT_FALSE (itp.second);
  std::string const & v = (*itp.first).second;
  EXPECT_EQ ("value g", v);
}
// *******************************************
// *                                         *
// *         FourNodesOnTwoLevels            *
// *                                         *
// *******************************************

namespace {

  class FourNodesOnTwoLevels : public GenericIndexFixture {
  protected:
    FourNodesOnTwoLevels ()
            : index_{new test_trie (db_,
                                    pstore::typed_address<pstore::index::header_block>::null (),
                                    hash_function{hashes_})} {}

    std::unique_ptr<test_trie> index_;
    static std::map<std::string, std::uint64_t> const hashes_;
  };

  std::map<std::string, std::uint64_t> const FourNodesOnTwoLevels::hashes_{
    {"a", UINT64_C (0b000000'000000)}, // "a" and "b" collide in the lower 6 bits
    {"b", UINT64_C (0b000001'000000)},
    {"c", UINT64_C (0b000000'000001)}, // ... as do "c" and "d".
    {"d", UINT64_C (0b000001'000001)},
  };

} // end anonymous namespace

// With a known hash function (see map_) and the insertion order a to d, we should end
// up with a trie which looks like:
//
//            +--------+--------+
// root_ ->   | 000000 | 000001 |             (hash bits 0-5)
//            +--------+--------+
//                 |       |
//           +-----+       +----+
//           |                  |
//           v                  v
// +--------+--------+     +-------+-------+
// | 000000 | 000001 |     | 00000 | 00001 |  (hash bits 6-11)
// +--------+--------+     +-------+-------+
//     |        |              |       |
//     v        |              |       |
//    "a"       v              |       |      (0b0000000'0000000)
//             "b"             v       |      (0b0000001'0000000)
//                            "c"      v      (0b0000000'0000001)
//                                    "d"     (0b0000001'0000001)
TEST_F (FourNodesOnTwoLevels, ForwardIteration) {
  transaction_type t1 = begin (db_, lock_guard{mutex_});
  this->insert_or_assign (*index_, t1, "a");
  this->insert_or_assign (*index_, t1, "b");
  this->insert_or_assign (*index_, t1, "c");
  this->insert_or_assign (*index_, t1, "d");

  // Check trie iterator in the heap.
  test_trie::iterator begin = index_->begin (db_);
  test_trie::iterator end = index_->end (db_);
  EXPECT_NE (begin, end);

  std::string const & v1 = (*begin).first;
  EXPECT_EQ ("a", v1);
  ++begin;
  std::string const & v2 = (*begin).first;
  EXPECT_EQ ("b", v2);
  ++begin;
  std::string const & v3 = (*begin).first;
  EXPECT_EQ ("c", v3);
  ++begin;
  std::string const & v4 = (*begin).first;
  EXPECT_EQ ("d", v4);
  ++begin;
  EXPECT_EQ (begin, end);

  index_->flush (t1, db_.get_current_revision ());
  this->check_is_store_branch (index_->root ());

  // Check trie iterator in the store.
  test_trie::const_iterator cbegin = index_->cbegin (db_);
  test_trie::const_iterator cend = index_->cend (db_);
  EXPECT_NE (cbegin, cend);

  std::string const v5 = cbegin->first;
  EXPECT_EQ ("a", v5);
  ++cbegin;
  std::string const v6 = cbegin->first;
  EXPECT_EQ ("b", v6);
  ++cbegin;
  std::string const v7 = cbegin->first;
  EXPECT_EQ ("c", v7);
  ++cbegin;
  std::string const v8 = cbegin->first;
  EXPECT_EQ ("d", v8);
  ++cbegin;
  EXPECT_EQ (cbegin, cend);
}

TEST_F (FourNodesOnTwoLevels, UpsertIteration) {
  transaction_type t1 = begin (db_, lock_guard{mutex_});
  this->insert_or_assign (*index_, t1, "b");
  this->insert_or_assign (*index_, t1, "c");
  this->insert_or_assign (*index_, t1, "d");
  std::pair<test_trie::iterator, bool> itp = this->insert_or_assign (*index_, t1, "a");

  // Check trie iterator in the heap.
  test_trie::iterator begin = itp.first;
  test_trie::iterator end = index_->end (db_);
  EXPECT_NE (begin, end);

  std::string const & v1 = (*begin).first;
  EXPECT_EQ ("a", v1);
  ++begin;
  std::string const & v2 = (*begin).first;
  EXPECT_EQ ("b", v2);
  ++begin;
  std::string const & v3 = (*begin).first;
  EXPECT_EQ ("c", v3);
  ++begin;
  std::string const & v4 = (*begin).first;
  EXPECT_EQ ("d", v4);
  ++begin;
  EXPECT_EQ (begin, end);

  index_->flush (t1, db_.get_current_revision ());
  itp = this->insert_or_assign (*index_, t1, "a");

  // Check trie iterator in the store.
  begin = itp.first;
  std::string const v5 = begin->first;
  EXPECT_EQ ("a", v5);
  ++begin;
  std::string const v6 = begin->first;
  EXPECT_EQ ("b", v6);
  ++begin;
  std::string const v7 = begin->first;
  EXPECT_EQ ("c", v7);
  ++begin;
  std::string const v8 = begin->first;
  EXPECT_EQ ("d", v8);
  ++begin;
  EXPECT_EQ (begin, end);
}

// *******************************************
// *                                         *
// *        LeavesAtDifferentLevels          *
// *                                         *
// *******************************************

namespace {

  class LeavesAtDifferentLevels : public GenericIndexFixture {
  public:
    LeavesAtDifferentLevels ()
            : index_{new test_trie (db_,
                                    pstore::typed_address<pstore::index::header_block>::null (),
                                    hash_function{hashes_})} {}

  protected:
    std::unique_ptr<test_trie> index_;
    static std::map<std::string, std::uint64_t> const hashes_;
  };

  std::map<std::string, std::uint64_t> const LeavesAtDifferentLevels::hashes_{
    {"a", UINT64_C (0b0000'00000000)},
    {"b", UINT64_C (0b0000'00000001)},
    {"c", UINT64_C (0b0000'01000001)},
    {"d", UINT64_C (0b0000'00000010)},
  };
} // namespace


TEST_F (LeavesAtDifferentLevels, ForwardIteration) {
  transaction_type t1 = begin (db_, lock_guard{mutex_});
  // With a known hash function and the insertion order below, we should end up with a
  // trie which looks like:
  //
  //          +--------+--------+--------+
  // root_ -> | 000000 | 000001 | 000010 |   (hash bits 0-5)
  //          +--------+--------+--------+
  //              |        |        |
  //              v        |        |
  //             "a"       |        v       (0b000000'000000)
  //                       |       "d"      (0b000000'000010)
  //                       v
  //              +--------+--------+
  //              | 000000 | 000001 |       (hash bits 6-11)
  //              +--------+--------+
  //                  |        |
  //                  v        |
  //                 "b"       v            (0b000000'000001)
  //                          "c"           (0b000001'000001)
  this->insert_or_assign (*index_, t1, "a");
  this->insert_or_assign (*index_, t1, "b");
  this->insert_or_assign (*index_, t1, "c");
  this->insert_or_assign (*index_, t1, "d");

  test_trie::iterator begin = index_->begin (db_);
  test_trie::iterator end = index_->end (db_);
  EXPECT_NE (begin, end);

  std::string const & v1 = (*begin).first;
  EXPECT_EQ ("a", v1);
  ++begin;
  std::string const & v2 = (*begin).first;
  EXPECT_EQ ("b", v2);
  ++begin;
  std::string const & v3 = (*begin).first;
  EXPECT_EQ ("c", v3);
  ++begin;
  std::string const & v4 = (*begin).first;
  EXPECT_EQ ("d", v4);
  ++begin;
  EXPECT_EQ (begin, end);

  index_->flush (t1, db_.get_current_revision ());
  this->check_is_store_branch (index_->root ());

  // Check trie iterator in the store.
  test_trie::const_iterator cbegin = index_->cbegin (db_);
  test_trie::const_iterator cend = index_->cend (db_);
  EXPECT_NE (cbegin, cend);

  std::string const v5 = cbegin->first;
  EXPECT_EQ ("a", v5);
  ++cbegin;
  std::string const v6 = cbegin->first;
  EXPECT_EQ ("b", v6);
  ++cbegin;
  std::string const v7 = cbegin->first;
  EXPECT_EQ ("c", v7);
  ++cbegin;
  std::string const v8 = cbegin->first;
  EXPECT_EQ ("d", v8);
  ++cbegin;
  EXPECT_EQ (cbegin, cend);
}

TEST_F (LeavesAtDifferentLevels, UpsertIteration) {
  transaction_type t1 = begin (db_, lock_guard{mutex_});
  this->insert_or_assign (*index_, t1, "b");
  this->insert_or_assign (*index_, t1, "c");
  this->insert_or_assign (*index_, t1, "d");
  std::pair<test_trie::iterator, bool> itp = this->insert_or_assign (*index_, t1, "a");

  test_trie::iterator begin = itp.first;
  test_trie::iterator end = index_->end (db_);
  EXPECT_NE (begin, end);

  std::string const & v1 = (*begin).first;
  EXPECT_EQ ("a", v1);
  ++begin;
  std::string const & v2 = (*begin).first;
  EXPECT_EQ ("b", v2);
  ++begin;
  std::string const & v3 = (*begin).first;
  EXPECT_EQ ("c", v3);
  ++begin;
  std::string const & v4 = (*begin).first;
  EXPECT_EQ ("d", v4);
  ++begin;
  EXPECT_EQ (begin, end);

  index_->flush (t1, db_.get_current_revision ());

  // Check trie iterator in the store.
  itp = this->insert_or_assign (*index_, t1, "a");
  begin = itp.first;
  std::string const v5 = begin->first;
  EXPECT_EQ ("a", v5);
  ++begin;
  std::string const v6 = begin->first;
  EXPECT_EQ ("b", v6);
  ++begin;
  std::string const v7 = begin->first;
  EXPECT_EQ ("c", v7);
  ++begin;
  std::string const v8 = begin->first;
  EXPECT_EQ ("d", v8);
  ++begin;
  EXPECT_EQ (begin, end);
}


namespace {

  class CorruptInternalNodes : public GenericIndexFixture {
  public:
    CorruptInternalNodes ()
            : index_{new test_trie (db_,
                                    pstore::typed_address<pstore::index::header_block>::null (),
                                    hash_function{hashes_})} {}

  protected:
    void build (transaction_type & transaction);

    void iterate () const;
    void find () const;

    std::shared_ptr<branch> load_branch (index_pointer ptr);
    static constexpr unsigned branch_children = 2U;

    std::unique_ptr<test_trie> index_;
    static std::map<std::string, std::uint64_t> const hashes_;
  };

  std::map<std::string, std::uint64_t> const CorruptInternalNodes::hashes_{
    {"a", UINT64_C (0b0000'00000000)},
    {"b", UINT64_C (0b0000'00000001)},
  };

  // build
  // ~~~~~
  void CorruptInternalNodes::build (transaction_type & transaction) {
    this->insert_or_assign (*index_, transaction, "a");
    this->insert_or_assign (*index_, transaction, "b");
    index_->flush (transaction, transaction.db ().get_current_revision ());
  }

  // iterate
  // ~~~~~~~
  void CorruptInternalNodes::iterate () const {
    test_trie const & index = *index_.get ();
    // Using an iterator.
    check_for_error (
      [this, &index] () {
        std::list<test_trie::value_type> visited;
        std::copy (index.begin (db_), index.end (db_), std::inserter (visited, visited.end ()));
      },
      pstore::error_code::index_corrupt);
  }

  // find
  // ~~~~
  void CorruptInternalNodes::find () const {
    test_trie const & index = *index_.get ();
    check_for_error ([this, &index] () { index.find (db_, "a"s); },
                     pstore::error_code::index_corrupt);
  }

  // load branch
  // ~~~~~~~~~~~
  std::shared_ptr<branch> CorruptInternalNodes::load_branch (index_pointer ptr) {
    PSTORE_ASSERT (ptr.is_branch ());
    return std::static_pointer_cast<branch> (
      db_.getrw (ptr.untag_address<branch> ().to_address (), branch::size_bytes (branch_children)));
  }

} // end anonymous namespace

TEST_F (CorruptInternalNodes, BitmapIsZero) {
  {
    transaction_type t1 = begin (db_, lock_guard{mutex_});
    this->build (t1);

    index_pointer root = index_->root ();
    this->check_is_store_branch (root);

    // Corrupt the bitmap field.
    {
      auto inode = this->load_branch (root);
      inode->set_bitmap (0);
    }

    t1.commit ();
  }
  this->iterate ();
  this->find ();
}

TEST_F (CorruptInternalNodes, ChildPointsToParent) {
  {
    transaction_type t1 = begin (db_, lock_guard{mutex_});
    this->build (t1);

    index_pointer root = index_->root ();
    this->check_is_store_branch (root);

    // Corrupt the first child field such that it points back to the root.
    {
      auto inode = this->load_branch (root);
      (*inode)[0] = root;
    }

    t1.commit ();
  }
  this->iterate ();
  this->find ();
}

TEST_F (CorruptInternalNodes, ChildClaimsToBeOnHeap) {
  {
    transaction_type t1 = begin (db_, lock_guard{mutex_});
    this->build (t1);

    index_pointer root = index_->root ();
    this->check_is_store_branch (root);

    // Corrupt the first child field such it claims to be on the heap.
    {
      std::shared_ptr<branch> inode = this->load_branch (root);
      index_pointer & first = (*inode)[0];
      first = index_pointer{reinterpret_cast<branch *> (first.to_address ().absolute ())};
    }
    t1.commit ();
  }
  this->iterate ();
  this->find ();
}

// *******************************************
// *                                         *
// *              InvalidIndex               *
// *                                         *
// *******************************************

namespace {

  class InvalidIndex : public IndexFixture {};

} // end anonymous namespace

TEST_F (InvalidIndex, InsertIntoIndexAtWrongRevision) {
  {
    transaction_type t1 = begin (db_, lock_guard{mutex_});
    auto r1index = pstore::index::get_index<pstore::trailer::indices::write> (db_);
    r1index->insert_or_assign (t1, "key1"s, pstore::extent<char> ());
    t1.commit ();
  }
  db_.sync (0);
  auto r0index = pstore::index::get_index<pstore::trailer::indices::write> (db_);
  {
    transaction_type t2 = begin (db_, lock_guard{mutex_});

    // We're now synced to revision 1. Trying to insert into the index loaded from
    // r0 should raise an error.
    check_for_error ([&] () { r0index->insert_or_assign (t2, "key2"s, pstore::extent<char> ()); },
                     pstore::error_code::index_not_latest_revision);
  }
}
