//===- unittests/command_line/test_cl_parser.cpp --------------------------===//
//*       _                                   *
//*   ___| |  _ __   __ _ _ __ ___  ___ _ __  *
//*  / __| | | '_ \ / _` | '__/ __|/ _ \ '__| *
//* | (__| | | |_) | (_| | |  \__ \  __/ |    *
//*  \___|_| | .__/ \__,_|_|  |___/\___|_|    *
//*          |_|                              *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/command_line/command_line.hpp"

// 3rd party includes
#include "gtest/gtest.h"
#if PSTORE_FUZZTEST
#  include "fuzztest/fuzztest.h"
#endif

using namespace std::string_view_literals;

TEST (ClParser, SimpleString) {
  using pstore::command_line::parser;

  std::optional<std::string> r = parser<std::string> () ("hello");
  ASSERT_TRUE (r.has_value ());
  EXPECT_EQ (r.value (), "hello");
}

#if PSTORE_FUZZTEST
static void StringParseNeverCrashes (std::string const & s) {
  std::optional<std::string> r = pstore::command_line::parser<std::string> () (s);
  ASSERT_TRUE (r.has_value ());
  EXPECT_EQ (r.value (), s);
}
FUZZ_TEST (ClParser, StringParseNeverCrashes);
#endif // PSTORE_FUZZTEST

TEST (ClParser, Int) {
  std::optional<int> r1 = pstore::command_line::parser<int>{}("43");
  EXPECT_EQ (r1.value_or (0), 43);
}

TEST (ClParser, IntEmpty) {
  pstore::command_line::parser<int> p;
  std::optional<int> const r = p ("");
  EXPECT_FALSE (r.has_value ());
}

TEST (ClParser, IntBad) {
  std::optional<int> const r = pstore::command_line::parser<int>{}("bad");
  EXPECT_FALSE (r.has_value ());
}

TEST (ClParser, IntWithBadTail) {
  std::optional<int> const r = pstore::command_line::parser<int>{}("42bad");
  EXPECT_FALSE (r.has_value ());
}

TEST (ClParser, IntNegative) {
  std::optional<int> const r = pstore::command_line::parser<int>{}("-42");
  EXPECT_EQ (r.value_or (0), -42);
}

TEST (ClParser, IntTooLarge) {
  std::optional<std::int8_t> const r = pstore::command_line::parser<std::int8_t>{}("256");
  EXPECT_FALSE (r.has_value ());
}

#if PSTORE_FUZZTEST
static void IntParse (std::string const & arg) {
  int expected = 0;
  auto last = arg.data () + arg.size ();
  auto [ptr, ec] = std::from_chars (arg.data (), last, expected);
  if (ptr != last) {
    ec = std::errc::invalid_argument;
  }

  std::optional<int> r = pstore::command_line::parser<int> () (arg);
  EXPECT_EQ (r.has_value (), ec == std::errc{});
  if (r) {
    EXPECT_EQ (*r, expected);
  }
}
FUZZ_TEST (ClParser, IntParse);
#endif // PSTORE_FUZZTEST

TEST (ClParser, Modifiers) {
  using namespace pstore::command_line;
  EXPECT_EQ (opt<int> ().get_occurrences_flag (), occurrences_flag::optional);
  EXPECT_EQ (opt<int>{optional}.get_occurrences_flag (), occurrences_flag::optional);
  EXPECT_EQ (opt<int>{required}.get_occurrences_flag (), occurrences_flag::required);
  EXPECT_EQ (opt<int>{one_or_more}.get_occurrences_flag (), occurrences_flag::zero_or_more);
  EXPECT_EQ (opt<int> (required, one_or_more).get_occurrences_flag (),
             occurrences_flag::one_or_more);
  EXPECT_EQ (opt<int> (optional, one_or_more).get_occurrences_flag (),
             occurrences_flag::zero_or_more);

  EXPECT_EQ (opt<int> ().name (), "");
  EXPECT_EQ (opt<int>{"name"sv}.name (), "name");
  EXPECT_EQ (opt<int>{name ("name"sv)}.name (), "name");

  EXPECT_EQ (opt<int>{}.description (), "");
  EXPECT_EQ (opt<int>{desc ("description")}.description (), "description");
}

namespace {

  class ClParserEnum : public testing::Test {
  public:
    ClParserEnum () {
      parser_.add_literal ("red", color::red, "description red");
      parser_.add_literal ("green", color::green, "description green");
      parser_.add_literal ("blue", color::blue, "description blue");
    }

  protected:
    enum class color { red, green, blue };
    pstore::command_line::parser<color> parser_;
  };

} // namespace

TEST_F (ClParserEnum, Red) {
  std::optional<color> const r = parser_ ("red");
  ASSERT_TRUE (r.has_value ());
  EXPECT_EQ (r.value (), color::red);
}
TEST_F (ClParserEnum, Green) {
  std::optional<color> const r = parser_ ("green");
  ASSERT_TRUE (r.has_value ());
  EXPECT_EQ (r.value (), color::green);
}
TEST_F (ClParserEnum, Blue) {
  std::optional<color> const r = parser_ ("blue");
  ASSERT_TRUE (r.has_value ());
  EXPECT_EQ (r.value (), color::blue);
}
TEST_F (ClParserEnum, Bad) {
  std::optional<color> const r = parser_ ("bad");
  EXPECT_FALSE (r.has_value ());
}
TEST_F (ClParserEnum, Empty) {
  std::optional<color> const r = parser_ ("");
  EXPECT_FALSE (r.has_value ());
}

#if PSTORE_FUZZTEST
static void EnumParse (std::string const & arg) {
  enum class color { red, green, blue };
  pstore::command_line::parser<color> parser;
  parser.add_literal ("red", color::red, "description red");
  parser.add_literal ("green", color::green, "description green");
  parser.add_literal ("blue", color::blue, "description blue");

  std::optional<color> const r = parser (arg);

  if (arg == "red") {
    ASSERT_TRUE (r.has_value ());
    EXPECT_EQ (*r, color::red);
  } else if (arg == "green") {
    ASSERT_TRUE (r.has_value ());
    EXPECT_EQ (*r, color::green);
  } else if (arg == "blue") {
    ASSERT_TRUE (r.has_value ());
    EXPECT_EQ (*r, color::blue);
  } else {
    EXPECT_FALSE (r.has_value ());
  }
}
FUZZ_TEST (ClParser, EnumParse);
#endif // PSTORE_FUZZTEST
