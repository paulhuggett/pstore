//===- unittests/command_line/test_command_line.cpp -----------------------===//
//*                                                _   _ _             *
//*   ___ ___  _ __ ___  _ __ ___   __ _ _ __   __| | | (_)_ __   ___  *
//*  / __/ _ \| '_ ` _ \| '_ ` _ \ / _` | '_ \ / _` | | | | '_ \ / _ \ *
//* | (_| (_) | | | | | | | | | | | (_| | | | | (_| | | | | | | |  __/ *
//*  \___\___/|_| |_| |_|_| |_| |_|\__,_|_| |_|\__,_| |_|_|_| |_|\___| *
//*                                                                    *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/command_line/command_line.hpp"

// Standard library includes
#include <algorithm>
#include <cstring>
#include <iterator>
#include <list>
#include <vector>

// #rd party includes
#include <gmock/gmock.h>

using namespace pstore::command_line;
using namespace std::string_view_literals;

namespace {

  class ClCommandLine : public ::testing::Test {
  public:
  protected:
#if defined(_WIN32) && defined(_UNICODE)
    using string_stream = std::wostringstream;
#else
    using string_stream = std::ostringstream;
#endif

    void add () {}
    template <typename... Strs>
    void add (std::string const & s, Strs... strs) {
      strings_.push_back (s);
      this->add (strs...);
    }

    bool parse_command_line_options (options_container & all, string_stream & output,
                                     string_stream & errors) {
      return details::parse_command_line_options (all, std::begin (strings_), std::end (strings_),
                                                  "overview", output, errors);
    }

  private:
    std::list<std::string> strings_;
  };

} // end anonymous namespace


TEST_F (ClCommandLine, SingleLetterStringOption) {
  options_container all;
  auto & option = all.add<string_opt> ("S"sv);
  this->add ("progname", "-Svalue");

  string_stream output;
  string_stream errors;
  bool const res = this->parse_command_line_options (all, output, errors);
  EXPECT_TRUE (res);

  EXPECT_EQ (errors.str ().length (), 0U);
  EXPECT_EQ (output.str ().length (), 0U);

  EXPECT_EQ (option.get (), "value");
  EXPECT_EQ (option.get_num_occurrences (), 1U);
}

TEST_F (ClCommandLine, SingleLetterStringOptionSeparateValue) {
  options_container all;
  auto & option = all.add<string_opt> ("S"sv);
  this->add ("progname", "-S", "value");

  string_stream output;
  string_stream errors;
  bool const res = this->parse_command_line_options (all, output, errors);
  EXPECT_TRUE (res);

  EXPECT_EQ (errors.str ().length (), 0U);
  EXPECT_EQ (output.str ().length (), 0U);

  EXPECT_EQ (option.get (), "value");
  EXPECT_EQ (option.get_num_occurrences (), 1U);
}

TEST_F (ClCommandLine, BooleanOption) {
  options_container all;
  auto & option = all.add<bool_opt> ("arg"sv);
  EXPECT_EQ (option.get (), false);

  this->add ("progname", "--arg");

  string_stream output;
  string_stream errors;
  bool const res = this->parse_command_line_options (all, output, errors);
  EXPECT_TRUE (res);

  EXPECT_EQ (errors.str ().length (), 0U);
  EXPECT_EQ (output.str ().length (), 0U);

  EXPECT_EQ (option.get (), true);
  EXPECT_EQ (option.get_num_occurrences (), 1U);
}

TEST_F (ClCommandLine, SingleLetterBooleanOptions) {
  options_container all;
  auto & opt_a = all.add<bool_opt> ("a"sv);
  auto & opt_b = all.add<bool_opt> ("b"sv);
  auto & opt_c = all.add<bool_opt> ("c"sv);
  EXPECT_EQ (opt_a.get (), false);
  EXPECT_EQ (opt_b.get (), false);
  EXPECT_EQ (opt_c.get (), false);

  this->add ("progname", "-ab");

  string_stream output;
  string_stream errors;
  bool const res = this->parse_command_line_options (all, output, errors);
  EXPECT_TRUE (res);
  EXPECT_EQ (errors.str ().length (), 0U);
  EXPECT_EQ (output.str ().length (), 0U);

  EXPECT_EQ (opt_a.get (), true);
  EXPECT_EQ (opt_a.get_num_occurrences (), 1U);

  EXPECT_EQ (opt_b.get (), true);
  EXPECT_EQ (opt_b.get_num_occurrences (), 1U);

  EXPECT_EQ (opt_c.get (), false);
  EXPECT_EQ (opt_c.get_num_occurrences (), 0U);
}

TEST_F (ClCommandLine, DoubleDashStringOption) {
  options_container all;
  auto & option = all.add<string_opt> ("arg"sv);
  this->add ("progname", "--arg", "value");

  string_stream output;
  string_stream errors;
  bool const res = this->parse_command_line_options (all, output, errors);
  EXPECT_TRUE (res);
  EXPECT_EQ (errors.str ().length (), 0U);
  EXPECT_EQ (output.str ().length (), 0U);

  EXPECT_EQ (option.get (), "value");
  EXPECT_EQ (option.get_num_occurrences (), 1U);
}

TEST_F (ClCommandLine, DoubleDashStringOptionWithSingleDash) {
  options_container all;
  all.add<bool_opt> ("arg"sv);
  this->add ("progname", "-arg");

  string_stream output;
  string_stream errors;
  bool const res = this->parse_command_line_options (all, output, errors);
  EXPECT_FALSE (res);
  EXPECT_THAT (errors.str (),
               testing::HasSubstr (PSTORE_NATIVE_TEXT ("Unknown command line argument")));
  EXPECT_THAT (errors.str (), testing::HasSubstr (PSTORE_NATIVE_TEXT ("'--arg'")));
  EXPECT_EQ (output.str ().length (), 0U);
}

TEST_F (ClCommandLine, StringOptionEquals) {
  options_container all;
  auto & option = all.add<string_opt> ("arg"sv);
  this->add ("progname", "--arg=value");

  string_stream output;
  string_stream errors;
  EXPECT_TRUE (this->parse_command_line_options (all, output, errors));

  EXPECT_EQ (errors.str ().length (), 0U);
  EXPECT_EQ (output.str ().length (), 0U);

  EXPECT_EQ (option.get (), "value");
  EXPECT_EQ (option.get_num_occurrences (), 1U);
}

TEST_F (ClCommandLine, UnknownArgument) {
  options_container all;
  this->add ("progname", "--arg");

  string_stream output;
  string_stream errors;
  EXPECT_FALSE (this->parse_command_line_options (all, output, errors));
  EXPECT_THAT (errors.str (),
               testing::HasSubstr (PSTORE_NATIVE_TEXT ("Unknown command line argument")));
  EXPECT_EQ (output.str ().length (), 0U);
}

TEST_F (ClCommandLine, NearestName) {
  options_container all;
  all.add<string_opt> ("aa"sv);
  all.add<string_opt> ("xx"sv);
  all.add<string_opt> ("yy"sv);
  this->add ("progname", "--xxx=value");

  string_stream output;
  string_stream errors;
  EXPECT_FALSE (this->parse_command_line_options (all, output, errors));
  EXPECT_THAT (errors.str (),
               testing::HasSubstr (PSTORE_NATIVE_TEXT ("Did you mean '--xx=value'")));
  EXPECT_EQ (output.str ().length (), 0U);
}

TEST_F (ClCommandLine, MissingOptionName) {
  options_container all;
  this->add ("progname", "--=a");

  string_stream output;
  string_stream errors;
  EXPECT_FALSE (this->parse_command_line_options (all, output, errors));
  EXPECT_THAT (errors.str (),
               testing::HasSubstr (PSTORE_NATIVE_TEXT ("Unknown command line argument")));
  EXPECT_EQ (output.str ().length (), 0U);
}

TEST_F (ClCommandLine, StringPositional) {
  options_container all;
  auto & option = all.add<string_opt> ("arg"sv, positional);
  EXPECT_EQ (option.get (), "") << "Expected inital string value to be empty";

  this->add ("progname", "hello");

  string_stream output;
  string_stream errors;
  bool const res = this->parse_command_line_options (all, output, errors);
  EXPECT_TRUE (res);

  EXPECT_EQ (errors.str ().length (), 0U);
  EXPECT_EQ (output.str ().length (), 0U);

  EXPECT_EQ (option.get (), "hello");
}

TEST_F (ClCommandLine, RequiredStringPositional) {
  options_container all;
  auto & option = all.add<string_opt> ("arg"sv, positional, required);

  this->add ("progname");

  string_stream output;
  string_stream errors;
  bool const res = this->parse_command_line_options (all, output, errors);
  EXPECT_FALSE (res);

  EXPECT_THAT (errors.str (),
               testing::HasSubstr (PSTORE_NATIVE_TEXT ("a positional argument was missing")));
  EXPECT_EQ (output.str ().length (), 0U);

  EXPECT_EQ (option.get (), "");
}

TEST_F (ClCommandLine, TwoPositionals) {
  options_container all;
  auto & opt1 = all.add<string_opt> ("opt1"sv, positional);
  auto & opt2 = all.add<string_opt> ("opt2"sv, positional);

  this->add ("progname", "arg1", "arg2");

  string_stream output;
  string_stream errors;
  bool const res = this->parse_command_line_options (all, output, errors);
  EXPECT_TRUE (res);

  EXPECT_EQ (errors.str ().length (), 0U);
  EXPECT_EQ (output.str ().length (), 0U);

  EXPECT_EQ (opt1.get (), "arg1");
  EXPECT_EQ (opt2.get (), "arg2");
}

TEST_F (ClCommandLine, List) {
  options_container all;
  auto & opt = all.add<list<std::string>> ("opt"sv);

  this->add ("progname", "--opt", "foo", "--opt", "bar");

  string_stream output;
  string_stream errors;
  bool const res = this->parse_command_line_options (all, output, errors);
  EXPECT_TRUE (res);

  EXPECT_EQ (errors.str ().length (), 0U);
  EXPECT_EQ (output.str ().length (), 0U);

  EXPECT_THAT (opt, ::testing::ElementsAre ("foo", "bar"));
}

namespace {

  enum class enumeration { a, b, c };

  std::ostream & operator<< (std::ostream & os, enumeration e) {
    auto str = "";
    switch (e) {
    case enumeration::a: str = "a"; break;
    case enumeration::b: str = "b"; break;
    case enumeration::c: str = "c"; break;
    }
    return os << str;
  }

} // namespace

TEST_F (ClCommandLine, ListOfEnums) {
  options_container all;
  auto & opt = all.add<list<enumeration>> (
    "opt"sv, values (literal{"a", static_cast<int> (enumeration::a), "a description"},
                     literal{"b", static_cast<int> (enumeration::b), "b description"},
                     literal{"c", static_cast<int> (enumeration::c), "c description"}));
  this->add ("progname", "--opt", "a", "--opt", "b");

  string_stream output;
  string_stream errors;
  bool const res = this->parse_command_line_options (all, output, errors);
  EXPECT_TRUE (res);

  EXPECT_EQ (errors.str ().length (), 0U);
  EXPECT_EQ (output.str ().length (), 0U);

  EXPECT_THAT (opt, ::testing::ElementsAre (enumeration::a, enumeration::b));
}

TEST_F (ClCommandLine, ListSingleDash) {
  options_container all;
  auto & opt = all.add<list<std::string>> ("o"sv);

  this->add ("progname", "-oa", "-o", "b", "-oc");

  string_stream output;
  string_stream errors;
  bool const res = this->parse_command_line_options (all, output, errors);
  EXPECT_TRUE (res);

  EXPECT_EQ (errors.str ().length (), 0U);
  EXPECT_EQ (output.str ().length (), 0U);

  EXPECT_THAT (opt, testing::ElementsAre ("a", "b", "c"));
}

TEST_F (ClCommandLine, ListPositional) {
  options_container all;
  auto & opt = all.add<list<std::string>> ("opt"sv, positional);

  this->add ("progname", "foo", "bar");

  string_stream output;
  string_stream errors;
  bool const res = this->parse_command_line_options (all, output, errors);
  EXPECT_TRUE (res);

  EXPECT_EQ (errors.str ().length (), 0U);
  EXPECT_EQ (output.str ().length (), 0U);

  EXPECT_THAT (opt, ::testing::ElementsAre ("foo", "bar"));
}

TEST_F (ClCommandLine, ListCsvEnabled) {
  options_container all;
  auto & opt = all.add<list<std::string>> ("opt"sv, positional, comma_separated);

  this->add ("progname", "a,b", "c,d");

  string_stream output;
  string_stream errors;
  bool const res = this->parse_command_line_options (all, output, errors);
  EXPECT_TRUE (res);

  EXPECT_EQ (errors.str ().length (), 0U);
  EXPECT_EQ (output.str ().length (), 0U);

  EXPECT_THAT (opt, ::testing::ElementsAre ("a", "b", "c", "d"));
}

TEST_F (ClCommandLine, ListCsvDisabled) {
  options_container all;
  auto & opt = all.add<list<std::string>> ("opt"sv, positional);

  this->add ("progname", "a,b");

  string_stream output;
  string_stream errors;
  bool const res = this->parse_command_line_options (all, output, errors);
  EXPECT_TRUE (res);

  EXPECT_EQ (errors.str ().length (), 0U);
  EXPECT_EQ (output.str ().length (), 0U);

  EXPECT_THAT (opt, ::testing::ElementsAre ("a,b"));
}


TEST_F (ClCommandLine, MissingRequired) {
  options_container all;
  auto & opt1 = all.add<string_opt> ("opt"sv, required);

  this->add ("progname");

  string_stream output;
  string_stream errors;
  bool const res = this->parse_command_line_options (all, output, errors);
  EXPECT_FALSE (res);

  EXPECT_THAT (errors.str (),
               testing::HasSubstr (PSTORE_NATIVE_TEXT ("must be specified at least once")));
  EXPECT_EQ (output.str ().length (), 0U);

  EXPECT_EQ (opt1.get_num_occurrences (), 0U);
  EXPECT_EQ (opt1.get (), "");
}

TEST_F (ClCommandLine, MissingValue) {
  options_container all;
  auto & opt1 = all.add<string_opt> ("opt"sv, required);

  this->add ("progname", "--opt");

  string_stream output;
  string_stream errors;
  bool const res = this->parse_command_line_options (all, output, errors);
  EXPECT_FALSE (res);

  EXPECT_THAT (errors.str (), testing::HasSubstr (PSTORE_NATIVE_TEXT ("requires a value")));
  EXPECT_EQ (output.str ().length (), 0U);

  EXPECT_EQ (opt1.get (), "");
}

TEST_F (ClCommandLine, UnwantedValue) {
  options_container all;
  auto & opt1 = all.add<bool_opt> ("opt"sv);

  this->add ("progname", "--opt=true");

  string_stream output;
  string_stream errors;
  EXPECT_FALSE (this->parse_command_line_options (all, output, errors));
  EXPECT_THAT (errors.str (), ::testing::HasSubstr (PSTORE_NATIVE_TEXT ("does not take a value")));
  EXPECT_EQ (output.str ().length (), 0U);
  EXPECT_FALSE (opt1.get ());
}

TEST_F (ClCommandLine, DoubleDashSwitchToPositional) {
  options_container all;
  auto & opt1 = all.add<string_opt> ("opt"sv);
  auto & p = all.add<list<std::string>> ("names"sv, positional);

  this->add ("progname", "--", "-opt", "foo");

  string_stream output;
  string_stream errors;
  bool const res = this->parse_command_line_options (all, output, errors);
  EXPECT_TRUE (res);

  EXPECT_EQ (errors.str ().length (), 0U);
  EXPECT_EQ (output.str ().length (), 0U);

  EXPECT_EQ (opt1.get_num_occurrences (), 0U);
  EXPECT_EQ (opt1.get (), "");
  EXPECT_THAT (p, ::testing::ElementsAre ("-opt", "foo"));
}

TEST_F (ClCommandLine, AliasBool) {
  options_container all;
  auto & opt1 = all.add<bool_opt> ("opt"sv);
  auto & opt2 = all.add<alias> ("o"sv, aliasopt{opt1});

  this->add ("progname", "-o");

  string_stream output;
  string_stream errors;
  bool const res = this->parse_command_line_options (all, output, errors);
  EXPECT_TRUE (res);
  EXPECT_EQ (errors.str ().length (), 0U);
  EXPECT_EQ (output.str ().length (), 0U);

  EXPECT_EQ (opt1.get_num_occurrences (), 1U);
  EXPECT_EQ (opt1.get (), true);
  EXPECT_EQ (opt2.get_num_occurrences (), 1U);
}

TEST_F (ClCommandLine, TwoCallsToParser) {
  options_container all;
  auto & option = all.add<string_opt> ("S"sv);
  this->add ("progname", "-Svalue");

  string_stream output;
  string_stream errors;
  bool const res1 = this->parse_command_line_options (all, output, errors);
  EXPECT_TRUE (res1);
  bool const res2 = this->parse_command_line_options (all, output, errors);
  EXPECT_TRUE (res2);

  EXPECT_EQ (errors.str ().length (), 0U);
  EXPECT_EQ (output.str ().length (), 0U);

  EXPECT_EQ (option.get (), "value");
  // We saw the -S switch twice because the arguments were parsed twice.
  EXPECT_EQ (option.get_num_occurrences (), 2U);
}
