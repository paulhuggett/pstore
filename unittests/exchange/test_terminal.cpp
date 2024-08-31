//===- unittests/exchange/test_terminal.cpp -------------------------------===//
//*  _                      _             _  *
//* | |_ ___ _ __ _ __ ___ (_)_ __   __ _| | *
//* | __/ _ \ '__| '_ ` _ \| | '_ \ / _` | | *
//* | ||  __/ |  | | | | | | | | | | (_| | | *
//*  \__\___|_|  |_| |_| |_|_|_| |_|\__,_|_| *
//*                                          *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/exchange/import_terminals.hpp"

// 3rd party includes
#include <gmock/gmock.h>
#include "peejay/json.hpp"

// pstore includes
#include "pstore/exchange/import_error.hpp"

// Local includes
#include "empty_store.hpp"

using namespace pstore::exchange::import_ns;
using namespace std::string_view_literals;

namespace {

  class RuleTest : public testing::Test {
  public:
    RuleTest ()
            : db_storage_{}
            , db_{db_storage_.file ()} {
      db_.set_vacuum_mode (pstore::database::vacuum_mode::disabled);
    }

  private:
    in_memory_store db_storage_;

  protected:
    pstore::database db_;

    template <typename Parser>
    Parser & parse (Parser & parser, std::string_view sv) {
      auto first = reinterpret_cast<std::byte const *> (sv.data ());
      parser.input (first, first + sv.length ()).eof ();
      return parser;
    }
  };

  class ImportBool : public RuleTest {
  public:
    ImportBool () = default;

    decltype (auto) make_json_bool_parser (pstore::gsl::not_null<bool *> v) {
      return peejay::make_parser (callbacks::make<bool_rule> (&db_, v));
    }
  };

} // end anonymous namespace

TEST_F (ImportBool, True) {
  bool v = false;
  auto parser = make_json_bool_parser (&v);
  this->parse (parser, "true"sv);
  parser.eof ();

  // Check the result.
  ASSERT_FALSE (parser.has_error ()) << "JSON error was: " << parser.last_error ().message ();
  EXPECT_EQ (v, true);
}

TEST_F (ImportBool, False) {
  bool v = true;
  auto parser = make_json_bool_parser (&v);
  this->parse (parser, "false"sv);
  parser.eof ();

  // Check the result.
  ASSERT_FALSE (parser.has_error ()) << "JSON error was: " << parser.last_error ().message ();
  EXPECT_EQ (v, false);
}

namespace {

  class ImportInt64 : public RuleTest {
  public:
    ImportInt64 () = default;
    decltype (auto) make_json_int64_parser (pstore::gsl::not_null<std::int64_t *> v) {
      return peejay::make_parser (callbacks::make<integer_rule> (&db_, v));
    }
  };

} // end anonymous namespace

TEST_F (ImportInt64, Zero) {
  auto v = std::int64_t{0};
  auto parser = make_json_int64_parser (&v);
  this->parse (parser, "0"sv);
  parser.eof ();

  // Check the result.
  ASSERT_FALSE (parser.has_error ()) << "JSON error was: " << parser.last_error ().message ();
  EXPECT_EQ (v, std::int64_t{0});
}

TEST_F (ImportInt64, One) {
  auto v = std::int64_t{0};
  auto parser = make_json_int64_parser (&v);
  this->parse (parser, "1"sv);
  parser.eof ();

  // Check the result.
  ASSERT_FALSE (parser.has_error ()) << "JSON error was: " << parser.last_error ().message ();
  EXPECT_EQ (v, std::int64_t{1});
}

TEST_F (ImportInt64, NegativeOne) {
  auto v = std::int64_t{0};
  auto parser = make_json_int64_parser (&v);
  this->parse (parser, "-1"sv);
  parser.eof ();

  // Check the result.
  ASSERT_FALSE (parser.has_error ()) << "JSON error was: " << parser.last_error ().message ();
  EXPECT_EQ (v, std::int64_t{-1});
}

TEST_F (ImportInt64, Min) {
  auto v = std::int64_t{0};
  auto parser = make_json_int64_parser (&v);
  auto const expected = std::numeric_limits<std::int64_t>::min ();
  this->parse (parser, std::to_string (expected));
  parser.eof ();

  // Check the result.
  ASSERT_FALSE (parser.has_error ()) << "JSON error was: " << parser.last_error ().message ();
  EXPECT_EQ (v, expected);
}

TEST_F (ImportInt64, Max) {
  auto v = std::int64_t{0};
  auto parser = make_json_int64_parser (&v);
  auto const expected = std::numeric_limits<std::int64_t>::max ();
  this->parse (parser, std::to_string (expected));
  parser.eof ();

  // Check the result.
  ASSERT_FALSE (parser.has_error ()) << "JSON error was: " << parser.last_error ().message ();
  EXPECT_EQ (v, expected);
}
