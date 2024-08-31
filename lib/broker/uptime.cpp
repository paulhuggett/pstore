//===- lib/broker/uptime.cpp ----------------------------------------------===//
//*              _   _                 *
//*  _   _ _ __ | |_(_)_ __ ___   ___  *
//* | | | | '_ \| __| | '_ ` _ \ / _ \ *
//* | |_| | |_) | |_| | | | | | |  __/ *
//*  \__,_| .__/ \__|_|_| |_| |_|\___| *
//*       |_|                          *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/broker/uptime.hpp"

// standard library
#include <sstream>
#include <thread>

// 3rd party
#include "peejay/json.hpp"
#include "peejay/null.hpp"

// pstore
#include "pstore/os/logging.hpp"
#include "pstore/os/thread.hpp"

namespace {

#ifndef NDEBUG
  bool is_valid_json (std::string_view str) {
    peejay::parser<peejay::null> parser{};
    auto first = reinterpret_cast<std::byte const *> (str.data ());
    parser.input (first, first + str.length ()).eof ();
    return !parser.has_error ();
  }
#endif

} // end anonymous namespace

namespace pstore::broker {

  descriptor_condition_variable uptime_cv;
  brokerface::channel<descriptor_condition_variable> uptime_channel (&uptime_cv);

  void uptime (gsl::not_null<std::atomic<bool> *> const done) {
    log (logger::priority::info, "uptime 1 second tick starting");

    auto seconds = std::uint64_t{0};
    auto until = std::chrono::system_clock::now ();
    while (!*done) {
      until += std::chrono::seconds{1};
      std::this_thread::sleep_until (until);
      ++seconds;

      uptime_channel.publish ([seconds] () {
        std::ostringstream os;
        os << "{ \"uptime\": " << seconds << " }";
        std::string const & str = os.str ();
        PSTORE_ASSERT (is_valid_json (str));
        return str;
      });
    }

    log (logger::priority::info, "uptime thread exiting");
  }

} // end namespace pstore::broker
