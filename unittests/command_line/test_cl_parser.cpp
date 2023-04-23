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

TEST (ClParser, SimpleString) {
  using pstore::command_line::parser;

  std::optional<std::string> r = parser<std::string> () ("hello");
  EXPECT_TRUE (r.has_value ());
  EXPECT_EQ (r.value (), "hello");
}

TEST (ClParser, StringFromSet) {
  using pstore::command_line::parser;

  parser<std::string> p;
  p.add_literal_option ("a", 31, "description a");
  p.add_literal_option ("b", 37, "description b");

  {
    std::optional<std::string> r1 = p ("hello");
    EXPECT_FALSE (r1.has_value ());
  }
  {
    std::optional<std::string> r2 = p ("a");
    EXPECT_TRUE (r2.has_value ());
    EXPECT_EQ (r2.value (), "a");
  }
  {
    std::optional<std::string> r3 = p ("b");
    EXPECT_TRUE (r3.has_value ());
    EXPECT_EQ (r3.value (), "b");
  }
}

TEST (ClParser, Int) {
  using pstore::command_line::parser;
  {
    std::optional<int> r1 = parser<int> () ("43");
    EXPECT_TRUE (r1.has_value ());
    EXPECT_EQ (r1.value (), 43);
  }
  {
    parser<int> p;
    std::optional<int> r2 = p ("");
    EXPECT_FALSE (r2.has_value ());
  }
  {
    std::optional<int> r3 = parser<int> () ("bad");
    EXPECT_FALSE (r3.has_value ());
  }
  {
    std::optional<int> r4 = parser<int> () ("42bad");
    EXPECT_FALSE (r4.has_value ());
  }
}

TEST (ClParser, Enum) {
  using pstore::command_line::parser;

  enum color { red, blue, green };
  parser<color> p;
  p.add_literal_option ("red", red, "description red");
  p.add_literal_option ("blue", blue, "description blue");
  p.add_literal_option ("green", green, "description green");
  {
    std::optional<color> const r1 = p ("red");
    EXPECT_TRUE (r1.has_value ());
    EXPECT_EQ (r1.value (), red);
  }
  {
    std::optional<color> const r2 = p ("blue");
    EXPECT_TRUE (r2.has_value ());
    EXPECT_EQ (r2.value (), blue);
  }
  {
    std::optional<color> const r3 = p ("green");
    EXPECT_TRUE (r3.has_value ());
    EXPECT_EQ (r3.value (), green);
  }
  {
    std::optional<color> const r4 = p ("bad");
    EXPECT_FALSE (r4.has_value ());
  }
  {
    std::optional<color> const r5 = p ("");
    EXPECT_FALSE (r5.has_value ());
  }
}

TEST (ClParser, Modifiers) {
  using namespace pstore::command_line;
  EXPECT_EQ (opt<int> ().get_num_occurrences_flag (), num_occurrences_flag::optional);
  EXPECT_EQ (opt<int>{optional}.get_num_occurrences_flag (), num_occurrences_flag::optional);
  EXPECT_EQ (opt<int>{required}.get_num_occurrences_flag (), num_occurrences_flag::required);
  EXPECT_EQ (opt<int>{one_or_more}.get_num_occurrences_flag (), num_occurrences_flag::zero_or_more);
  EXPECT_EQ (opt<int> (required, one_or_more).get_num_occurrences_flag (),
             num_occurrences_flag::one_or_more);
  EXPECT_EQ (opt<int> (optional, one_or_more).get_num_occurrences_flag (),
             num_occurrences_flag::zero_or_more);

  EXPECT_EQ (opt<int> ().name (), "");
  EXPECT_EQ (opt<int>{"name"}.name (), "name");

  EXPECT_EQ (opt<int>{}.description (), "");
  EXPECT_EQ (opt<int>{desc ("description")}.description (), "description");
}
