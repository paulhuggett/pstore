//===- tools/brokerd/switches.cpp -----------------------------------------===//
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

#include <iostream>
#include <vector>

#include "pstore/command_line/command_line.hpp"
#include "pstore/support/utf.hpp"

using namespace pstore::command_line;

namespace {

  std::optional<std::string> path_option (string_opt const & path) {
    if (path.get_num_occurrences () == 0U) {
      return std::nullopt;
    }
    return path.get ();
  }

} // end anonymous namespace


std::pair<switches, int> get_switches (int const argc, tchar * argv[]) {
  options_container all;
  auto & record_path =
    all.add<string_opt> ("record", desc{"Record received messages in the named output file"});
  all.add<alias> ("r", desc{"Alias for --record"}, aliasopt{record_path});

  auto & playback_path =
    all.add<string_opt> ("playback", desc{"Play back messages from the named file"});
  all.add<alias> ("p", desc{"Alias for --playback"}, aliasopt{playback_path});

  auto & pipe_path = all.add<string_opt> (
    "pipe-path", desc{"Overrides the path of the FIFO from which commands will be read"});

  auto & num_read_threads =
    all.add<opt<unsigned>> ("read-threads", desc{"The number of pipe reading threads"}, init (2U));

  auto & http_port = all.add<opt<std::uint16_t>> (
    "http-port", desc{"The port on which to listen for HTTP connections"}, init (in_port_t{8080}));
  auto & disable_http =
    all.add<bool_opt> ("disable-http", desc{"Disable the HTTP server"}, init (false));

  auto & announce_http_port =
    all.add<bool_opt> ("announce-http-port",
                       desc{"Display a message when the HTTP server is available"}, init (false));

  auto & scavenge_time =
    all.add<opt<unsigned>> ("scavenge-time",
                            desc{"The time in seconds that a message will spend in the command "
                                 "queue before being removed by the scavenger"},
                            init (4U * 60U * 60U));

  parse_command_line_options (all, argc, argv, "pstore broker agent");

  switches result;
  result.playback_path = path_option (playback_path);
  result.record_path = path_option (record_path);
  result.pipe_path = path_option (pipe_path);
  result.num_read_threads = num_read_threads.get ();
  result.announce_http_port = announce_http_port.get ();
  result.http_port = disable_http ? std::optional<in_port_t> (std::nullopt)
                                  : std::optional<in_port_t> (http_port.get ());
  result.scavenge_time = std::chrono::seconds{scavenge_time.get ()};
  return {std::move (result), EXIT_SUCCESS};
}
