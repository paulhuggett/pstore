//===- tools/read/switches.cpp --------------------------------------------===//
//*               _ _       _                *
//*  _____      _(_) |_ ___| |__   ___  ___  *
//* / __\ \ /\ / / | __/ __| '_ \ / _ \/ __| *
//* \__ \\ V  V /| | || (__| | | |  __/\__ \ *
//* |___/ \_/\_/ |_|\__\___|_| |_|\___||___/ *
//*                                          *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "switches.hpp"

#include "pstore/command_line/command_line.hpp"
#include "pstore/command_line/str_to_revision.hpp"
#include "pstore/command_line/revision_opt.hpp"
#include "pstore/support/error.hpp"

using namespace pstore::command_line;
using namespace std::string_view_literals;

std::pair<switches, int> get_switches (int argc, tchar * argv[]) {
  argument_parser args;
  auto & revision = args.add<opt<pstore::command_line::revision_opt, parser<std::string>>> (
    "revision"sv, desc ("The starting revision number (or 'HEAD')"));
  args.add<alias> ("r"sv, desc ("Alias for --revision"), aliasopt (revision));

  auto & db_path = args.add<string_opt> (
    positional, usage ("repository"), desc ("Path of the pstore repository to be read"), required);
  auto & key = args.add<string_opt> (positional, usage ("key"), required);
  auto & string_mode =
    args.add<bool_opt> ("strings"sv, init (false),
                        desc ("Reads from the 'strings' index rather than the 'names' index."));
  args.add<alias> ("s"sv, desc ("Alias for --strings"), aliasopt (string_mode));

  args.parse_args (argc, argv, "pstore read utility\n");

  switches result;
  result.revision = static_cast<unsigned> (revision.get ());
  result.db_path = db_path.get ();
  result.key = key.get ();
  result.string_mode = string_mode.get ();

  return {result, EXIT_SUCCESS};
}
