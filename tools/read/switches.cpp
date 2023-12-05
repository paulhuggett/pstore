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

std::pair<switches, int> get_switches (int argc, tchar * argv[]) {
  options_container all;
  auto & revision = all.add<opt<pstore::command_line::revision_opt, parser<std::string>>> (
    "revision", desc ("The starting revision number (or 'HEAD')"));
  all.add<alias> ("r", desc ("Alias for --revision"), aliasopt (revision));

  auto & db_path = all.add<string_opt> (
    positional, usage ("repository"), desc ("Path of the pstore repository to be read"), required);
  auto & key = all.add<string_opt> (positional, usage ("key"), required);
  auto & string_mode =
    all.add<bool_opt> ("strings", init (false),
                       desc ("Reads from the 'strings' index rather than the 'names' index."));
  all.add<alias> ("s", desc ("Alias for --strings"), aliasopt (string_mode));

  parse_command_line_options (all, argc, argv, "pstore read utility\n");

  switches result;
  result.revision = static_cast<unsigned> (revision.get ());
  result.db_path = db_path.get ();
  result.key = key.get ();
  result.string_mode = string_mode.get ();

  return {result, EXIT_SUCCESS};
}
