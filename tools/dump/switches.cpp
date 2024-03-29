//===- tools/dump/switches.cpp --------------------------------------------===//
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

#include <iterator>

#include "pstore/command_line/command_line.hpp"
#include "pstore/command_line/revision_opt.hpp"
#include "pstore/command_line/str_to_revision.hpp"
#include "pstore/dump/digest_opt.hpp"

using namespace std::string_view_literals;

namespace pstore::command_line {

  // parser<digest_opt>
  // ~~~~~~~~~~~~~~~~~~
  template <>
  class parser<dump::digest_opt> {
  public:
    std::optional<dump::digest_opt> operator() (std::string_view v) const {
      if (std::optional<index::digest> const d = uint128::from_hex_string (v)) {
        return std::optional<dump::digest_opt>{*d};
      }
      return std::nullopt;
    }
  };

  // type description<digest_opt>
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  template <>
  struct type_description<dump::digest_opt> {
    static constexpr gsl::czstring value = "digest";
  };

} // end namespace pstore::command_line

using namespace pstore::command_line;

namespace {} // end anonymous namespace

std::pair<switches, int> get_switches (int argc, tchar * argv[]) {
  using digest_opt = list<pstore::dump::digest_opt>;

  argument_parser options;
  option_category what_cat{"Options controlling what is dumped"};

  auto & fragment =
    options.add<digest_opt> ("fragment"sv, desc{"Dump the contents of a specific fragment"},
                             comma_separated, category (what_cat));
  options.add<alias> ("F"sv, aliasopt{fragment});
  auto & all_fragments = options.add<bool_opt> (
    "all-fragments"sv, desc{"Dump the contents of the fragments index"}, category (what_cat));

  auto & compilation =
    options.add<digest_opt> ("compilation"sv, desc{"Dump the contents of a specific compilation"},
                             comma_separated, category (what_cat));
  options.add<alias> ("C"sv, aliasopt{compilation});

  auto & all_compilations = options.add<bool_opt> (
    "all-compilations"sv, desc{"Dump the contents of the compilations index"}, category (what_cat));

  auto & debug_line_header = options.add<digest_opt> (
    "debug-line-header"sv, desc{"Dump the contents of a specific debug line header"},
    comma_separated, category (what_cat));
  auto & all_debug_line_headers = options.add<bool_opt> (
    "all-debug-line-headers"sv, desc{"Dump the contents of the debug line headers index"},
    category (what_cat));

  auto & header =
    options.add<bool_opt> ("header"sv, desc{"Dump the file header"}, category (what_cat));
  options.add<alias> ("h"sv, desc{"Alias for --header"}, aliasopt{header});

  auto & indices =
    options.add<bool_opt> ("indices"sv, desc{"Dump the indices"}, category (what_cat));
  options.add<alias> ("i"sv, aliasopt{indices});

  auto & log_opt =
    options.add<bool_opt> ("log"sv, desc{"List the transactions"}, category (what_cat));
  options.add<alias> ("l"sv, aliasopt{log_opt});

  auto & names_opt =
    options.add<bool_opt> ("names"sv, desc{"Dump the name index"}, category (what_cat));
  options.add<alias> ("n"sv, aliasopt{names_opt});
  auto & paths_opt =
    options.add<bool_opt> ("paths"sv, desc{"Dump the path index"}, category (what_cat));
  options.add<alias> ("p"sv, aliasopt{paths_opt});

  auto & all = options.add<bool_opt> (
    "all"sv,
    desc{"Show store-related output. Equivalent to: --all-compilations "
         "--all-debug-line-headers --all-fragments --header --indices --log --names --paths"},
    category (what_cat));
  options.add<alias> ("a"sv, desc{"Alias for --all"}, aliasopt{all});

  auto & revision = options.add<opt<pstore::command_line::revision_opt, parser<std::string>>> (
    "revision"sv, desc{"The starting revision number (or 'HEAD')"});
  options.add<alias> ("r"sv, desc{"Alias for --revision"}, aliasopt{revision});


  option_category how_cat{"Options controlling how fields are emitted"};

  auto & no_times = options.add<bool_opt> (
    "no-times"sv, desc{"Times are displayed as a fixed value (for testing)"}, category (how_cat));
  auto & hex = options.add<bool_opt> ("hex"sv, desc{"Emit number values in hexadecimal notation"},
                                      category (how_cat));
  options.add<alias> ("x"sv, desc{"Alias for --hex"}, aliasopt{hex});

  auto & expanded_addresses = options.add<bool_opt> (
    "expanded-addresses"sv, desc{"Emit address values as an explicit segment/offset object"},
    category (how_cat));

#ifdef PSTORE_IS_INSIDE_LLVM
  auto & triple = options.add<string_opt> (
    "triple"sv, desc{"The target triple to use for disassembly if one is not known"},
    init ("x86_64-pc-linux-gnu-repo"), cat (how_cat));
  auto & no_disassembly = options.add<bool_opt> (
    "no-disassembly"sv, desc{"Emit executable sections as binary rather than disassembly"},
    cat (how_cat));
#endif // PSTORE_IS_INSIDE_LLVM

  auto & paths = options.add<list<std::string>> (positional, usage{"filename..."});



  options.parse_args (argc, argv, "pstore dump utility\n");

  switches result;

  auto const get_digest_from_opt = [] (pstore::dump::digest_opt const & d) {
    return pstore::index::digest{d};
  };
  std::transform (std::begin (fragment), std::end (fragment), std::back_inserter (result.fragments),
                  get_digest_from_opt);
  result.show_all_fragments = all_fragments.get ();

  std::transform (std::begin (compilation), std::end (compilation),
                  std::back_inserter (result.compilations), get_digest_from_opt);
  result.show_all_compilations = all_compilations.get ();

  std::transform (std::begin (debug_line_header), std::end (debug_line_header),
                  std::back_inserter (result.debug_line_headers), get_digest_from_opt);
  result.show_all_debug_line_headers = all_debug_line_headers.get ();

  result.show_header = header.get ();
  result.show_indices = indices.get ();
  result.show_log = log_opt.get ();

  result.show_names = names_opt.get ();
  result.show_paths = paths_opt.get ();

  if (all) {
    result.show_all_compilations = result.show_all_debug_line_headers = result.show_all_fragments =
      result.show_header = result.show_indices = result.show_log = result.show_names =
        result.show_paths = true;
  }

  result.revision = static_cast<unsigned> (revision.get ());

  result.hex = hex.get ();
  result.no_times = no_times.get ();
  result.expanded_addresses = expanded_addresses.get ();
#ifdef PSTORE_IS_INSIDE_LLVM
  result.triple = triple.get ();
  result.no_disassembly = no_disassembly.get ();
#endif // PSTORE_IS_INSIDE_LLVM
  std::copy (std::begin (paths), std::end (paths), std::back_inserter (result.paths));

  return {result, EXIT_SUCCESS};
}
