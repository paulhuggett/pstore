//===- tools/diff/switches.cpp --------------------------------------------===//
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

using namespace pstore::command_line;
using namespace std::string_view_literals;

std::pair<switches, int> get_switches (int argc, tchar * argv[]) {
  options_container all;
  auto & db_path = all.add<string_opt> (
    positional, usage ("repository"), desc ("Path of the pstore repository to be read."), required);
  auto & first_revision = all.add<opt<pstore::command_line::revision_opt, parser<std::string>>> (
    positional, usage ("[1st-revision]"), desc ("The first revision number (or 'HEAD')"), optional);
  auto & second_revision = all.add<opt<pstore::command_line::revision_opt, parser<std::string>>> (
    positional, usage ("[2nd-revision]"), desc ("The second revision number (or 'HEAD')"),
    optional);

  option_category how_cat ("Options controlling how fields are emitted");
  auto & hex =
    all.add<bool_opt> ("hex"sv, desc ("Emit numbers in hexadecimal notation"), cat (how_cat));
  all.add<alias> ("x"sv, desc ("Alias for --hex"), aliasopt (hex));

  parse_command_line_options (all, argc, argv, "pstore diff utility\n");

  switches result;
  result.db_path = db_path.get ();
  result.first_revision = static_cast<unsigned> (first_revision.get ());
  result.second_revision = second_revision.get_num_occurrences () > 0
                             ? pstore::just (static_cast<unsigned> (second_revision.get ()))
                             : pstore::nothing<pstore::revision_number> ();
  result.hex = hex.get ();
  return {result, EXIT_SUCCESS};
}
