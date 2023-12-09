//===- unittests/command_line/test_help.cpp -------------------------------===//
//*  _          _        *
//* | |__   ___| |_ __   *
//* | '_ \ / _ \ | '_ \  *
//* | | | |  __/ | |_) | *
//* |_| |_|\___|_| .__/  *
//*              |_|     *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/command_line/help.hpp"

#include <sstream>

#include <gmock/gmock.h>

#include "pstore/command_line/option.hpp"
#include "pstore/command_line/command_line.hpp"

using string_stream = std::ostringstream;

using namespace pstore::command_line;
using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;

namespace {

  class Help : public testing::Test {
  public:
    bool parse_args (argument_parser & args, string_stream & output, string_stream & errors) {
      std::array<std::string, 2> argv{{"program", "--help"}};
      return args.parse_args (std::begin (argv), std::end (argv), "overview", output, errors);
    }
  };

} // end anonymous namespace

TEST_F (Help, Empty) {
  argument_parser args;
  string_stream output;
  string_stream errors;
  bool res = this->parse_args (args, output, errors);
  EXPECT_FALSE (res);
  EXPECT_EQ (output.str (), "OVERVIEW: overview\nUSAGE: program\n");
  EXPECT_EQ (errors.str (), "");
}

TEST_F (Help, HasSwitches1) {
  argument_parser args;
  auto & option1 = args.add<string_opt> ("arg1"sv, positional);
  args.add<alias> ("alias1"sv, aliasopt{option1});
  EXPECT_FALSE (args.has_switches (nullptr));
}

TEST_F (Help, HasSwitches2) {
  argument_parser args;
  args.add<string_opt> ("arg2"sv);
  EXPECT_TRUE (args.has_switches (nullptr));
}

TEST_F (Help, BuildDefaultCategoryOnly) {
  argument_parser args;
  args.add<string_opt> ("arg1"sv, positional);
  auto & option2 = args.add<string_opt> ("arg2"sv);
  categories_collection const actual = build_categories (nullptr, args);

  ASSERT_EQ (actual.size (), 1U);
  auto const & first = *actual.begin ();
  EXPECT_EQ (first.first, nullptr);
  EXPECT_THAT (first.second, testing::ElementsAre (&option2));
}

TEST_F (Help, BuildTwoCategories) {
  argument_parser args;
  args.add<string_opt> ("arg1"sv, positional);
  auto & option2 = args.add<string_opt> ("arg2"sv);
  option_category category{"category"};
  auto & option3 = args.add<string_opt> ("arg3"sv, cat (category));

  categories_collection const actual = build_categories (nullptr, args);

  ASSERT_EQ (actual.size (), 2U);
  auto it = std::begin (actual);
  EXPECT_EQ (it->first, nullptr);
  EXPECT_THAT (it->second, testing::ElementsAre (&option2));
  ++it;
  EXPECT_EQ (it->first, &category);
  EXPECT_THAT (it->second, testing::ElementsAre (&option3));
}

TEST_F (Help, SwitchStrings) {
  // This option has a name in Katakana to verify that we are counting unicode code-points and not
  // UTF-8 code-units.
  auto const name = "\xE3\x82\xAA"   // KATAKANA LETTER O
                    "\xE3\x83\x97"   // KATAKANA LETTER PU
                    "\xE3\x82\xB7"   // KATAKANA LETTER SI
                    "\xE3\x83\xA7"   // KATAKANA LETTER SMALL YO
                    "\xE3\x83\xB3"s; // KATAKANA LETTER N
  string_opt option1{name};
  options_set options{&option1};
  switch_strings const actual = get_switch_strings (options);

  ASSERT_EQ (actual.size (), 1U);
  auto it = std::begin (actual);
  EXPECT_EQ (it->first, &option1);
  ASSERT_EQ (it->second.size (), 1U);
  EXPECT_EQ (it->second[0], std::make_tuple ("--"s + name + " <str>", std::size_t{13}));
}
