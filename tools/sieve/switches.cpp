//===- tools/sieve/switches.cpp -------------------------------------------===//
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

using namespace pstore::command_line;

namespace {} // end anonymous namespace

user_options user_options::get (int argc, tchar * argv[]) {
  options_container all;
  auto & endian_opt = all.add<opt<endian>> (
    "endian", desc ("The endian-ness of the output data"),
    values (
      literal{"big", static_cast<int> (endian::big), "Big-endian"},
      literal{"little", static_cast<int> (endian::little), "Little-endian"},
      literal{"native", static_cast<int> (endian::native), "The endian-ness of the host machine"}),
    init (endian::native));
  all.add<alias> ("e", desc ("Alias for --endian"), aliasopt (endian_opt));


  auto & maximum_opt =
    all.add<opt<unsigned>> ("maximum", desc ("The maximum prime value"), init (100U));
  all.add<alias> ("m", desc ("Alias for --maximum"), aliasopt (maximum_opt));


  auto & output_opt =
    all.add<string_opt> ("output", desc ("Output file name. (Default: standard-out)"), init ("-"));
  all.add<alias> ("o", desc ("Alias for --output"), aliasopt (output_opt));

  parse_command_line_options (all, argc, argv, "pstore prime number generator\n");

  user_options result;
  result.output = output_opt.get ();
  result.endianness = endian_opt.get ();
  result.maximum = maximum_opt.get ();
  return result;
}
