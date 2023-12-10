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
#include <gtest/gtest.h>

using namespace std::string_view_literals;

TEST (ClParser, SimpleString) {
  using pstore::command_line::parser;

  std::optional<std::string> r = parser<std::string> () ("hello");
  EXPECT_TRUE (r.has_value ());
  EXPECT_EQ (r.value (), "hello");
}

TEST (ClParser, StringFromSet) {
  using pstore::command_line::parser;

  enum class values { a = 31, b = 37 };
  parser<values> p;
  p.add_literal ("a", values::a, "description a");
  p.add_literal ("b", values::b, "description b");

  {
    std::optional<values> r1 = p ("hello");
    EXPECT_FALSE (r1.has_value ());
  }
  {
    std::optional<values> r2 = p ("a");
    ASSERT_TRUE (r2.has_value ());
    EXPECT_EQ (r2.value (), values::a);
  }
  {
    std::optional<values> r3 = p ("b");
    ASSERT_TRUE (r3.has_value ());
    EXPECT_EQ (r3.value (), values::b);
  }
}

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
  std::optional<color> const r1 = parser_ ("red");
  ASSERT_TRUE (r1.has_value ());
  EXPECT_EQ (r1.value (), color::red);
}
TEST_F (ClParserEnum, Green) {
  std::optional<color> const r1 = parser_ ("green");
  ASSERT_TRUE (r1.has_value ());
  EXPECT_EQ (r1.value (), color::green);
}
TEST_F (ClParserEnum, Blue) {
  std::optional<color> const r1 = parser_ ("blue");
  ASSERT_TRUE (r1.has_value ());
  EXPECT_EQ (r1.value (), color::blue);
}
TEST_F (ClParserEnum, Bad) {
  std::optional<color> const r4 = parser_ ("bad");
  EXPECT_FALSE (r4.has_value ());
}
TEST_F (ClParserEnum, Empty) {
  std::optional<color> const r4 = parser_ ("");
  EXPECT_FALSE (r4.has_value ());
}
