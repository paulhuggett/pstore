//===- tools/index_structure/switches.cpp ---------------------------------===//
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

// Standard library
#include <cstdlib>
#include <iostream>
#include <sstream>

// pstore
#include "pstore/command_line/command_line.hpp"
#include "pstore/command_line/revision_opt.hpp"
#include "pstore/command_line/str_to_revision.hpp"
#include "pstore/command_line/tchar.hpp"
#include "pstore/support/gsl.hpp"
#include "pstore/support/utf.hpp"

using namespace pstore::command_line;

namespace {


  std::string usage_help (list<pstore::trailer::indices> const & index_names_opt) {
    std::ostringstream usage;
    usage << "pstore index structure\n\n"
             "Dumps the internal structure of one of more pstore indexes. index-name may be "
             "any of: ";
    pstore::gsl::czstring separator = "";
    for (literal const & lit : *index_names_opt.get_parser ()) {
      usage << separator << '\'' << lit.name << '\'';
      separator = ", ";
    }
    return usage.str ();
  }

} // end anonymous namespace

std::pair<switches, int> get_switches (int argc, tchar * argv[]) {

  options_container all;

  auto & revision = all.add<opt<pstore::command_line::revision_opt, parser<std::string>>> (
    "revision", desc ("The starting revision number (or 'HEAD')"));
  all.add<alias> ("r", desc ("Alias for --revision"), aliasopt (revision));
  auto & db_path =
    all.add<string_opt> (positional, required, usage ("repository"), desc ("Database path"));

#define X(a) literal (#a, static_cast<int> (pstore::trailer::indices::a), #a),
  auto & index_names_opt = all.add<list<pstore::trailer::indices>> (
    positional, optional, one_or_more, usage ("[index-name...]"), values ({PSTORE_INDICES}));
#undef X

  parse_command_line_options (all, argc, argv, usage_help (index_names_opt));

  switches sw;
  sw.revision = static_cast<unsigned> (revision.get ());
  sw.db_path = db_path.get ();
  for (pstore::trailer::indices idx : index_names_opt) {
    sw.selected.set (static_cast<std::underlying_type_t<pstore::trailer::indices>> (idx));
  }
  return {sw, EXIT_SUCCESS};
}
