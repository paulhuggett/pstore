//===- unittests/core/test_transaction.cpp --------------------------------===//
//*  _                                  _   _              *
//* | |_ _ __ __ _ _ __  ___  __ _  ___| |_(_) ___  _ __   *
//* | __| '__/ _` | '_ \/ __|/ _` |/ __| __| |/ _ \| '_ \  *
//* | |_| | | (_| | | | \__ \ (_| | (__| |_| | (_) | | | | *
//*  \__|_|  \__,_|_| |_|___/\__,_|\___|\__|_|\___/|_| |_| *
//*                                                        *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/core/transaction.hpp"

// Standard library includes
#include <mutex>
#include <numeric>

// 3rd party includes
#include <gmock/gmock.h>

// Local includes
#include "empty_store.hpp"

namespace {

  using mock_lock = std::unique_lock<mock_mutex>;

  void append_int (pstore::transaction<mock_lock> & transaction, int v) {
    *(transaction.alloc_rw<int> ().first) = v;
  }

  //*                _        _      _        _                   *
  //*  _ __  ___  __| |__  __| |__ _| |_ __ _| |__  __ _ ___ ___  *
  //* | '  \/ _ \/ _| / / / _` / _` |  _/ _` | '_ \/ _` (_-</ -_) *
  //* |_|_|_\___/\__|_\_\ \__,_\__,_|\__\__,_|_.__/\__,_/__/\___| *
  //*                                                             *
  class mock_database : public pstore::database {
  public:
    explicit mock_database (std::shared_ptr<pstore::file::in_memory> const & file)
            : pstore::database (file) {

      this->set_vacuum_mode (pstore::database::vacuum_mode::disabled);
    }

    MOCK_METHOD (std::shared_ptr<void const>, get, (pstore::address, std::size_t, bool),
                 (const, override));
    MOCK_METHOD (std::shared_ptr<void>, get, (pstore::address, std::size_t, bool), (override));

    MOCK_METHOD (pstore::address, allocate, (std::uint64_t, unsigned), (override));

    // The next two methods simply allow the mocks to call through to the base class's
    // implementation of allocate() and get().
    pstore::address base_allocate (std::uint64_t bytes, unsigned align) {
      return pstore::database::allocate (bytes, align);
    }
    auto base_get_ro (pstore::address const & addr, std::size_t size, bool is_initialized) const
      -> std::shared_ptr<void const> {
      return pstore::database::get (addr, size, is_initialized);
    }
    auto base_get_rw (pstore::address const & addr, std::size_t size, bool is_initialized)
      -> std::shared_ptr<void> {
      return pstore::database::get (addr, size, is_initialized);
    }
  };

  //*  _____                          _   _           *
  //* |_   _| _ __ _ _ _  ___ __ _ __| |_(_)___ _ _   *
  //*   | || '_/ _` | ' \(_-</ _` / _|  _| / _ \ ' \  *
  //*   |_||_| \__,_|_||_/__/\__,_\__|\__|_\___/_||_| *
  //*                                                 *
  class Transaction : public testing::Test {
  public:
    Transaction ();
    void SetUp () override;

  protected:
    pstore::header const * get_header () {
      return reinterpret_cast<pstore::header const *> (store_.buffer ().get ());
    }

    in_memory_store store_;
    mock_database db_;
  };

  Transaction::Transaction ()
          : db_{store_.file ()} {
    db_.set_vacuum_mode (pstore::database::vacuum_mode::disabled);
  }

  void Transaction::SetUp () {
    using testing::_;
    using testing::Const;
    using testing::Invoke;

    // Pass the mocked calls through to their original implementations.
    // I'm simply using the mocking framework to observe that the
    // correct calls are made. The precise division of labor between
    // the database and transaction classes is enforced or determined here.

    EXPECT_CALL (Const (db_), get (_, _, _))
      .WillRepeatedly (Invoke (&db_, &mock_database::base_get_ro));
    EXPECT_CALL (db_, get (_, _, _)).WillRepeatedly (Invoke (&db_, &mock_database::base_get_rw));
    EXPECT_CALL (db_, allocate (_, _))
      .WillRepeatedly (Invoke (&db_, &mock_database::base_allocate));
  }

} // end anonymous namespace

TEST_F (Transaction, CommitEmptyDoesNothing) {
  pstore::database db{store_.file ()};
  db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);

  // A quick check of the initial state.
  auto header = this->get_header ();
  ASSERT_EQ (pstore::leader_size, header->footer_pos.load ().absolute ());

  {
    mock_mutex mutex;
    auto transaction = begin (db, std::unique_lock<mock_mutex>{mutex});
    transaction.commit ();
  }

  EXPECT_EQ (pstore::leader_size, header->footer_pos.load ().absolute ());
}

TEST_F (Transaction, CommitInt) {
  pstore::database db{store_.file ()};
  db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);

  auto header = this->get_header ();
  std::uint64_t const r0footer_offset = header->footer_pos.load ().absolute ();

  int const data_value = 32749;

  // Scope for the single transaction that we'll commit for the test.
  {
    mock_mutex mutex;
    auto transaction = begin (db, std::unique_lock<mock_mutex>{mutex});
    {
      // Write an integer to the store.
      // If rw is a spanning pointer, it will only be saved to the store when it is deleted.
      // TODO: write a large vector (>4K) so the 'protect' function, which is called by the
      // transaction commit function, has an effect.
      std::pair<std::shared_ptr<int>, pstore::typed_address<int>> rw = transaction.alloc_rw<int> ();
      ASSERT_EQ (0U, rw.second.absolute () % alignof (int))
        << "The address must be suitably aligned for int";
      *(rw.first) = data_value;
    }

    transaction.commit ();
  }

  std::uint64_t new_header_offset = pstore::leader_size;
  new_header_offset += sizeof (pstore::trailer);
  new_header_offset += pstore::calc_alignment (new_header_offset, alignof (int));
  std::uint64_t const r1contents_offset = new_header_offset;
  new_header_offset += sizeof (int);
  new_header_offset += pstore::calc_alignment (new_header_offset, alignof (pstore::trailer));

  std::uint64_t const g1footer_offset = header->footer_pos.load ().absolute ();
  ASSERT_EQ (new_header_offset, g1footer_offset)
    << "Expected offset of r1 footer to be " << new_header_offset;


  // Header checks.
  EXPECT_THAT (pstore::header::file_signature1, ::testing::ContainerEq (header->a.signature1))
    << "File header was missing";
  EXPECT_EQ (g1footer_offset, header->footer_pos.load ().absolute ());

  // Check the two footers.
  {
    auto r0footer =
      reinterpret_cast<pstore::trailer const *> (store_.buffer ().get () + r0footer_offset);
    EXPECT_THAT (pstore::trailer::default_signature1,
                 ::testing::ContainerEq (r0footer->a.signature1))
      << "Did not find the r0 footer signature1";
    EXPECT_EQ (0U, r0footer->a.generation) << "r0 footer generation number must be 0";
    EXPECT_EQ (0U, r0footer->a.size) << "expected the r0 footer size value to be 0";
    EXPECT_EQ (pstore::typed_address<pstore::trailer>::null (), r0footer->a.prev_generation)
      << "The r0 footer should not point to a previous generation";
    EXPECT_THAT (pstore::trailer::default_signature2, ::testing::ContainerEq (r0footer->signature2))
      << "Did not find r0 footer signature2";

    auto r1footer =
      reinterpret_cast<pstore::trailer const *> (store_.buffer ().get () + g1footer_offset);
    EXPECT_THAT (pstore::trailer::default_signature1,
                 ::testing::ContainerEq (r1footer->a.signature1))
      << "Did not find the r1 footer signature1";
    EXPECT_EQ (1U, r1footer->a.generation) << "r1 footer generation number must be 1";
    EXPECT_GE (r1footer->a.size, sizeof (int)) << "r1 footer size must be at least sizeof (int";
    EXPECT_EQ (pstore::typed_address<pstore::trailer>::make (pstore::leader_size),
               r1footer->a.prev_generation)
      << "r1 previous pointer must point to r0 footer";
    EXPECT_THAT (pstore::trailer::default_signature2, ::testing::ContainerEq (r1footer->signature2))
      << "Did not find r1 footer signature2";

    EXPECT_GE (r1footer->a.time, r0footer->a.time) << "r1 time must not be earlier than r0 time";
  }

  // Finally check the r1 contents
  {
    auto r1data = reinterpret_cast<int const *> (store_.buffer ().get () + r1contents_offset);
    EXPECT_EQ (data_value, *r1data);
  }
}

TEST_F (Transaction, RollbackAfterAppendingInt) {
  pstore::database db{store_.file ()};
  db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);

  // A quick check of the initial state.
  auto header = this->get_header ();
  ASSERT_EQ (pstore::leader_size, header->footer_pos.load ().absolute ());

  {
    mock_mutex mutex;
    auto transaction = begin (db, std::unique_lock<mock_mutex>{mutex});

    // Write an integer to the store.
    *(transaction.alloc_rw<int> ().first) = 42;

    // Abandon the transaction.
    transaction.rollback ();
  }

  // Header checks
  EXPECT_THAT (pstore::header::file_signature1, ::testing::ContainerEq (header->a.signature1))
    << "File header was missing";
  EXPECT_EQ (pstore::leader_size, header->footer_pos.load ().absolute ())
    << "Expected the file header footer_pos to point to r0 header";

  {
    auto r0footer =
      reinterpret_cast<pstore::trailer const *> (store_.buffer ().get () + pstore::leader_size);
    EXPECT_THAT (pstore::trailer::default_signature1,
                 ::testing::ContainerEq (r0footer->a.signature1))
      << "Did not find r0 footer signature1";
    EXPECT_EQ (0U, r0footer->a.generation) << "r0 footer generation number must be 0";
    EXPECT_EQ (0U, r0footer->a.size);
    EXPECT_EQ (pstore::typed_address<pstore::trailer>::null (), r0footer->a.prev_generation);
    EXPECT_THAT (pstore::trailer::default_signature2, ::testing::ContainerEq (r0footer->signature2))
      << "Did not find r0 footer signature2";
  }
}

TEST_F (Transaction, CommitAfterAppending4Mb) {
  pstore::database db{store_.file ()};
  db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);
  {
    mock_mutex mutex;
    auto transaction = begin (db, std::unique_lock<mock_mutex>{mutex});

    std::size_t const elements = (4U * 1024U * 1024U) / sizeof (int);
    transaction.allocate (elements * sizeof (int), 1 /*align*/);
    transaction.commit ();
  }

  // Check the two footers.
  {
    pstore::header const * const header = this->get_header ();
    pstore::typed_address<pstore::trailer> const r1_footer_offset = header->footer_pos;

    auto r1_footer = reinterpret_cast<pstore::trailer const *> (store_.buffer ().get () +
                                                                r1_footer_offset.absolute ());
    EXPECT_THAT (pstore::trailer::default_signature1,
                 ::testing::ContainerEq (r1_footer->a.signature1))
      << "Did not find r1 footer signature1";
    EXPECT_EQ (1U, r1_footer->a.generation) << "r1 footer generation number must be 1";
    EXPECT_EQ (4194304U, r1_footer->a.size);
    EXPECT_THAT (pstore::trailer::default_signature2,
                 ::testing::ContainerEq (r1_footer->signature2))
      << "Did not find r1 footer signature2";

    pstore::typed_address<pstore::trailer> const r0_footer_offset = r1_footer->a.prev_generation;

    auto r0_footer = reinterpret_cast<pstore::trailer const *> (store_.buffer ().get () +
                                                                r0_footer_offset.absolute ());
    EXPECT_THAT (pstore::trailer::default_signature1,
                 ::testing::ContainerEq (r0_footer->a.signature1))
      << "Did not find r0 footer signature1";
    EXPECT_EQ (0U, r0_footer->a.generation) << "r0 footer generation number must be 0";
    EXPECT_EQ (0U, r0_footer->a.size) << "expected the r0 footer size value to be 0";
    EXPECT_EQ (pstore::typed_address<pstore::trailer>::null (), r0_footer->a.prev_generation)
      << "The r0 footer should not point to a previous generation";
    EXPECT_THAT (pstore::trailer::default_signature2,
                 ::testing::ContainerEq (r0_footer->signature2))
      << "Did not find r0 footer signature2";

    EXPECT_GE (r1_footer->a.time, r0_footer->a.time) << "r1 time must not be earlier than r0 time";
  }
}

TEST_F (Transaction, CommitAfterAppendingAndWriting4Mb) {
  static constexpr auto elements = std::size_t{32};
  using element_type = int;

  // A collection of "initial" values which will fill the first region except for (elements/2)
  // integers.
  static constexpr std::size_t initial_elements =
    (pstore::address::segment_size - (pstore::leader_size + sizeof (pstore::trailer)) -
     (elements / 2U) * sizeof (element_type)) /
    sizeof (element_type);

  pstore::database db{store_.file ()};
  db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);

  auto addr = pstore::typed_address<element_type>::null ();
  {
    mock_mutex mutex;
    auto transaction = begin (db, std::unique_lock<mock_mutex>{mutex});
    {
      // A first allocation to fill up most of the first segment.
      transaction.allocate (initial_elements * sizeof (element_type), alignof (element_type));

      // The second allocation should span the end of the first and the start of the second
      // segment.
      addr = pstore::typed_address<element_type>::make (
        transaction.allocate (elements * sizeof (element_type), alignof (element_type)));

      // Fill the second allocation with values,
      std::shared_ptr<element_type> ptr = transaction.getrw (addr, elements);
      std::iota (ptr.get (), ptr.get () + elements, 0);
    }
    transaction.commit ();
  }
  {
    std::vector<int> expected (elements);
    std::iota (std::begin (expected), std::end (expected), 0);

    std::shared_ptr<int const> v = db.getro (addr, elements);
    std::vector<int> actual (v.get (), v.get () + elements);

    EXPECT_THAT (actual, ::testing::ContainerEq (expected));
  }
}

TEST_F (Transaction, RollbackAfterAppending4mb) {
  pstore::storage::region_container const & regions = db_.storage ().regions ();
  pstore::file::file_base * const file = db_.storage ().file ();

  mock_mutex mutex;
  std::uint64_t const original_size = file->size ();
  std::size_t const original_num_regions = regions.size ();
  constexpr auto bytes_to_allocate = 4U * 1024U * 1024U;

  auto transaction = begin (db_, std::unique_lock<mock_mutex>{mutex});

  transaction.allocate (bytes_to_allocate, 1 /*align*/);
  EXPECT_GT (regions.size (), original_num_regions) << "Allocate did not create regions";
  EXPECT_GT (file->size (), original_size) << "The file did not grow!";

  // Abandon the transaction.
  transaction.rollback ();
  EXPECT_EQ (regions.size (), original_num_regions) << "Rollback did not reclaim regions";
  EXPECT_EQ (file->size (), original_size) << "The file was not restored to its original size";
}

TEST_F (Transaction, RollbackAfterFillingRegion) {
  pstore::storage::region_container const & regions = db_.storage ().regions ();
  pstore::file::file_base * const file = db_.storage ().file ();
  std::size_t const original_num_regions = regions.size ();
  constexpr auto align = 1U;

  mock_mutex mutex;
  using guard_type = std::unique_lock<mock_mutex>;

  {
    auto t1 = begin (db_, guard_type{mutex});
    t1.allocate (regions.back ()->end () - db_.size () - sizeof (pstore::trailer), align);
    t1.commit ();
  }
  EXPECT_EQ (regions.size (), original_num_regions)
    << "Expected the first allocate to fill the initial region";

  std::uint64_t const original_size = file->size ();
  {
    auto t2 = begin (db_, guard_type{mutex});
    t2.allocate (sizeof (int), align);
    t2.rollback ();
  }
  EXPECT_EQ (regions.size (), original_num_regions) << "Rollback did not reclaim regions";
  EXPECT_EQ (file->size (), original_size) << "The file was not restored to its original size";
}

TEST_F (Transaction, CommitTwoSeparateTransactions) {
  // Append two individual transactions, each containing a single int.
  {
    pstore::database db{store_.file ()};
    db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);
    mock_mutex mutex;
    {
      auto t1 = begin (db, mock_lock (mutex));
      append_int (t1, 1);
      t1.commit ();
    }
    {
      auto t2 = begin (db, mock_lock (mutex));
      append_int (t2, 2);
      t2.commit ();
    }
  }

  std::size_t footer2 = pstore::leader_size;
  footer2 += pstore::calc_alignment (footer2, alignof (pstore::trailer));
  footer2 += sizeof (pstore::trailer);

  footer2 += pstore::calc_alignment (footer2, alignof (int));
  footer2 += sizeof (int);
  footer2 += pstore::calc_alignment (footer2, alignof (pstore::trailer));
  footer2 += sizeof (pstore::trailer);

  footer2 += pstore::calc_alignment (footer2, alignof (int));
  footer2 += sizeof (int);
  footer2 += pstore::calc_alignment (footer2, alignof (pstore::trailer));

  pstore::header const * const header = this->get_header ();
  EXPECT_EQ (pstore::typed_address<pstore::trailer>::make (footer2), header->footer_pos.load ());
}

// Use the alloc_rw<> convenience method to return a pointer to an int that has been freshly
// allocated within the transaction.
TEST_F (Transaction, GetRwInt) {
  // First setup the mock expectations.
  {
    using ::testing::Expectation;
    using ::testing::Ge;
    using ::testing::Invoke;

    Expectation allocate_int = EXPECT_CALL (db_, allocate (sizeof (int), alignof (int)))
                                 .WillOnce (Invoke (&db_, &mock_database::base_allocate));

    // A call to get(). First argument (address) must lie beyond the initial transaction
    // and must request a writable int.
    EXPECT_CALL (db_, get (Ge (pstore::address{pstore::leader_size + sizeof (pstore::trailer)}),
                           sizeof (int), false))
      .After (allocate_int)
      .WillOnce (Invoke (&db_, &mock_database::base_get_rw));
  }
  // Now the real body of the test
  {
    mock_mutex mutex;
    auto transaction = begin (db_, std::unique_lock<mock_mutex>{mutex});
    transaction.alloc_rw<int> ();
    transaction.commit ();
  }
}

// Use the getro<> method to return an address referencing the first int in the store.
TEST_F (Transaction, GetRoInt) {
  using testing::Const;
  using testing::Invoke;

  // First setup the mock expectations.
  EXPECT_CALL (Const (db_), get (pstore::address::null (), sizeof (int), true))
    .WillOnce (Invoke (&db_, &mock_database::base_get_ro));

  // Now the real body of the test
  mock_mutex mutex;
  auto transaction = begin (db_, std::unique_lock<mock_mutex>{mutex});
  db_.getro (pstore::typed_address<int>::null (), 1);
  transaction.commit ();
}

TEST_F (Transaction, GetRwUInt64) {
  std::uint64_t const expected = 1ULL << 40;
  pstore::extent<std::uint64_t> extent;
  {
    mock_mutex mutex;
    auto transaction = begin (db_, std::unique_lock<mock_mutex>{mutex});
    {
      // Allocate the storage
      pstore::address const addr =
        transaction.allocate (sizeof (std::uint64_t), alignof (std::uint64_t));
      extent = make_extent (pstore::typed_address<std::uint64_t> (addr), sizeof (std::uint64_t));
      std::shared_ptr<std::uint64_t> ptr = transaction.getrw (extent);

      // Save the data to the store
      *(ptr) = expected;
    }
    transaction.commit ();
  }
  EXPECT_EQ (expected, *db_.getro (extent));
}
