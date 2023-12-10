//===- unittests/command_line/test_modifiers.cpp --------------------------===//
//*                      _ _  __ _                *
//*  _ __ ___   ___   __| (_)/ _(_) ___ _ __ ___  *
//* | '_ ` _ \ / _ \ / _` | | |_| |/ _ \ '__/ __| *
//* | | | | | | (_) | (_| | |  _| |  __/ |  \__ \ *
//* |_| |_| |_|\___/ \__,_|_|_| |_|\___|_|  |___/ *
//*                                               *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/command_line/modifiers.hpp"

#include <sstream>
#include <string>
#include <vector>

#include <gmock/gmock.h>

#include "pstore/command_line/command_line.hpp"
#include "pstore/command_line/option.hpp"

using namespace pstore::command_line;
using namespace std::string_view_literals;
using testing::HasSubstr;
using testing::Not;

namespace {

  enum class enumeration : int { a, b, c };

#if defined(_WIN32) && defined(_UNICODE)
  using string_stream = std::wostringstream;
#else
  using string_stream = std::ostringstream;
#endif

  class Modifiers : public testing::Test {
  public:
    ~Modifiers () override = default;
  };

} // end anonymous namespace

TEST_F (Modifiers, DefaultConstruction) {
  opt<enumeration> opt;
  EXPECT_EQ (opt.get (), enumeration::a);
}

TEST_F (Modifiers, Init) {
  // init() allows the initial (default) value of the option to be described.
  opt<enumeration> opt_a{init (enumeration::a)};

  EXPECT_EQ (opt_a.get (), enumeration::a);

  opt<enumeration> opt_b{init (enumeration::b)};
  EXPECT_EQ (opt_b.get (), enumeration::b);
}


namespace {

  class EnumerationParse : public testing::Test {
  public:
    EnumerationParse () = default;
    ~EnumerationParse () override = default;
  };

} // end anonymous namespace

TEST_F (EnumerationParse, SetA) {
  argument_parser args;
  auto & enum_opt = args.add<opt<enumeration>> (
    "enumeration"sv, values (literal{"a", enumeration::a, "a description"},
                             literal{"b", enumeration::b, "b description"},
                             literal{"c", enumeration::c, "c description"}));

  std::vector<std::string> argv{"progname", "--enumeration=a"};
  string_stream output;
  string_stream errors;
  bool ok = args.parse_args (std::begin (argv), std::end (argv), "overview", output, errors);
  ASSERT_TRUE (ok);
  ASSERT_EQ (enum_opt.get (), enumeration::a);
}

TEST_F (EnumerationParse, SetC) {
  argument_parser args;
  auto & enum_opt = args.add<opt<enumeration>> (
    "enumeration"sv, values (literal{"a", enumeration::a, "a description"},
                             literal{"b", enumeration::b, "b description"},
                             literal{"c", enumeration::c, "c description"}));

  std::vector<std::string> argv{"progname", "--enumeration=c"};
  string_stream output;
  string_stream errors;
  bool ok = args.parse_args (std::begin (argv), std::end (argv), "overview", output, errors);
  ASSERT_TRUE (ok);
  ASSERT_EQ (enum_opt.get (), enumeration::c);
}

TEST_F (EnumerationParse, ErrorBadValue) {
  argument_parser args;
  args.add<opt<enumeration>> ("enumeration"sv,
                              values (literal{"a", enumeration::a, "a description"},
                                      literal{"b", enumeration::b, "b description"},
                                      literal{"c", enumeration::c, "c description"}));

  std::vector<std::string> argv{"progname", "--enumeration=bad"};
  string_stream output;
  string_stream errors;
  bool ok = args.parse_args (std::begin (argv), std::end (argv), "overview", output, errors);
  ASSERT_FALSE (ok);
  EXPECT_THAT (errors.str (), HasSubstr (PSTORE_NATIVE_TEXT ("'bad'")));
}

TEST_F (EnumerationParse, GoodValueAfterError) {
  argument_parser args;
  args.add<opt<enumeration>> ("enumeration"sv,
                              values (literal{"a", enumeration::a, "a description"},
                                      literal{"b", enumeration::b, "b description"},
                                      literal{"c", enumeration::c, "c description"}));

  std::vector<std::string> argv{"progname", "--unknown", "--enumeration=a"};
  string_stream output;
  string_stream errors;
  bool ok = args.parse_args (std::begin (argv), std::end (argv), "overview", output, errors);
  ASSERT_FALSE (ok);
  EXPECT_THAT (errors.str (), Not (HasSubstr (PSTORE_NATIVE_TEXT ("'a'"))));
}
