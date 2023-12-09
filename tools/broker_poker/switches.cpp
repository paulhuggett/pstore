//===- tools/broker_poker/switches.cpp ------------------------------------===//
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

#include <cstdlib>

#include "pstore/command_line/command_line.hpp"
#include "pstore/support/maybe.hpp"

using namespace pstore::command_line;

namespace {

  pstore::maybe<std::string> path_option (std::string const & p) {
    if (p.length () > 0) {
      return pstore::just (p);
    }
    return pstore::nothing<std::string> ();
  }

} // end anonymous namespace

std::pair<switches, int> get_switches (int argc, tchar * argv[]) {
  argument_parser args;
  auto & pipe_path = args.add<string_opt> (
    name{"pipe-path"}, desc{"Overrides the FIFO path to which messages are written."},
    init{std::string_view{""}});

  auto & flood = args.add<unsigned_opt> (
    name ("flood"), desc ("Flood the broker with a number of ECHO messages."), init (0U));
  args.add<alias> (name ("m"), desc ("Alias for --flood"), aliasopt (flood));

  auto & retry_timeout = args.add<opt<std::chrono::milliseconds::rep>> (
    name ("retry-timeout"), desc ("The timeout for connection retries to the broker (ms)."),
    init (switches{}.retry_timeout.count ()));

  auto & kill = args.add<bool_opt> (
    name ("kill"), desc ("Ask the broker to quit after commands have been processed."));
  args.add<alias> (name ("k"), desc ("Alias for --kill"), aliasopt (kill));

  auto & verb = args.add<string_opt> (positional, optional, usage ("[verb]"));
  auto & path = args.add<string_opt> (positional, optional, usage ("[path]"));

  args.parse_args (argc, argv, "pstore broker poker\n");

  switches result;
  result.verb = verb.get ();
  result.path = path.get ();
  result.retry_timeout = std::chrono::milliseconds (retry_timeout.get ());
  result.flood = flood.get ();
  result.kill = kill.get ();
  result.pipe_path = path_option (pipe_path.get ());
  return {result, EXIT_SUCCESS};
}
