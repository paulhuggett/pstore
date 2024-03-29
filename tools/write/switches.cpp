//===- tools/write/switches.cpp -------------------------------------------===//
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
#include "pstore/support/error.hpp"
#include "pstore/support/gsl.hpp"
#include "pstore/support/utf.hpp"

#include "error.hpp"
#include "to_value_pair.hpp"

using namespace pstore::command_line;
using namespace std::string_view_literals;

namespace {

  pstore::database::vacuum_mode to_vacuum_mode (std::string const & opt) {
    if (opt == "disabled") {
      return pstore::database::vacuum_mode::disabled;
    }
    if (opt == "immediate") {
      return pstore::database::vacuum_mode::immediate;
    }
    if (opt == "background") {
      return pstore::database::vacuum_mode::background;
    }
    pstore::raise_error_code (make_error_code (write_error_code::unrecognized_compaction_mode));
  }

} // end anonymous namespace

std::pair<switches, int> get_switches (int argc, tchar * argv[]) {
  argument_parser args;
  auto & add = args.add<list<std::string>> (
    "add"sv, desc ("Add key with corresponding string value. Specified as 'key,value'."
                   " May be repeated to add several keys."));
  args.add<alias> ("a"sv, desc ("Alias for --add"), aliasopt (add));

  auto & add_string = args.add<list<std::string>> (
    "add-string"sv, desc ("Add key to string set. May be repeated to add several strings."));
  args.add<alias> ("s"sv, desc ("Alias for --add-string"), aliasopt (add_string));

  auto & add_file = args.add<list<std::string>> (
    "add-file"sv, desc ("Add key with the named file's contents as the corresponding value."
                        " Specified as 'key,filename'. May be repeated to add several files."));
  args.add<alias> ("f"sv, desc ("Alias for --add-file"), aliasopt (add_file));


  auto & db_path =
    args.add<string_opt> (positional, usage ("repository"),
                          desc ("Path of the pstore repository to be written"), required);
  auto & files = args.add<list<std::string>> (positional, usage ("[filename]..."));

  auto & vacuum_mode = args.add<string_opt> ("compact"sv, optional,
                                             desc ("Set the compaction mode. Argument must one of: "
                                                   "'disabled', 'immediate', 'background'."));
  args.add<alias> ("c"sv, desc ("Alias for --compact"), aliasopt (vacuum_mode));

  args.parse_args (argc, argv, "pstore write utility\n");

  auto const make_value_pair = [] (std::string const & arg) { return to_value_pair (arg); };

  switches result;

  result.db_path = db_path.get ();
  if (!vacuum_mode.empty ()) {
    result.vmode = to_vacuum_mode (vacuum_mode.get ());
  }

  std::transform (std::begin (add), std::end (add), std::back_inserter (result.add),
                  make_value_pair);
  std::copy (std::begin (add_string), std::end (add_string), std::back_inserter (result.strings));

  std::transform (std::begin (add_file), std::end (add_file), std::back_inserter (result.files),
                  make_value_pair);
  std::transform (std::begin (files), std::end (files), std::back_inserter (result.files),
                  [] (std::string const & path) { return std::make_pair (path, path); });

  return {result, EXIT_SUCCESS};
}
