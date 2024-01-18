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
using namespace std::string_view_literals;

namespace {} // end anonymous namespace

user_options user_options::get (int argc, tchar * argv[]) {
  argument_parser args;
  auto & endian_opt = args.add<opt<endian>> (
    "endian"sv, desc ("The endian-ness of the output data"),
    values (literal{"big", endian::big, "Big-endian"},
            literal{"little", endian::little, "Little-endian"},
            literal{"native", endian::native, "The endian-ness of the host machine"}),
    init (endian::native));
  args.add<alias> ("e"sv, desc ("Alias for --endian"), aliasopt (endian_opt));


  auto & maximum_opt =
    args.add<unsigned_opt> ("maximum"sv, desc{"The maximum prime value"}, init{100U});
  args.add<alias> ("m"sv, desc{"Alias for --maximum"}, aliasopt{maximum_opt});


  auto & output_opt = args.add<string_opt> (
    "output"sv, desc ("Output file name. (Default: standard-out)"), init ("-"sv));
  args.add<alias> ("o"sv, desc ("Alias for --output"), aliasopt (output_opt));

  args.parse_args (argc, argv, "pstore prime number generator\n");

  user_options result;
  result.output = output_opt.get ();
  result.endianness = endian_opt.get ();
  result.maximum = maximum_opt.get ();
  return result;
}
