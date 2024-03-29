//===- lib/broker/quit.cpp ------------------------------------------------===//
//*              _ _    *
//*   __ _ _   _(_) |_  *
//*  / _` | | | | | __| *
//* | (_| | |_| | | |_  *
//*  \__, |\__,_|_|\__| *
//*     |_|             *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file broker/quit.cpp

#include "pstore/broker/quit.hpp"

// Standard includes
#include <iostream>

// Platform includes
#include <fcntl.h>
#ifndef _WIN32
#  include <sys/socket.h>
#endif
#include <signal.h>

// pstore includes
#include "pstore/broker/command.hpp"
#include "pstore/broker/gc.hpp"
#include "pstore/broker/globals.hpp"
#include "pstore/broker/internal_commands.hpp"
#include "pstore/broker/scavenger.hpp"
#include "pstore/http/quit.hpp"
#include "pstore/http/server.hpp"
#include "pstore/os/signal_helpers.hpp"

namespace {

  using priority = pstore::logger::priority;
  template <typename T>
  using not_null = pstore::gsl::not_null<T>;

  // push
  // ~~~~
  /// Push a simple message onto the command queue.
  void push (not_null<pstore::broker::command_processor *> const cp, std::string const & message) {
    static std::atomic<std::uint32_t> mid{0};

    log (priority::info, "push command ", pstore::logger::quoted{message.c_str ()});

    PSTORE_ASSERT (message.length () <= pstore::brokerface::message_type::payload_chars);
    auto msg = std::make_unique<pstore::brokerface::message_type> (mid++, std::uint16_t{0},
                                                                   std::uint16_t{1}, message);
    cp->push_command (std::move (msg), nullptr);
  }


} // end anonymous namespace

namespace pstore {
  namespace broker {

    // shutdown
    // ~~~~~~~~
    void shutdown (command_processor * const cp, scavenger * const scav, int const signum,
                   unsigned const num_read_threads,
                   not_null<std::optional<pstore::http::server_status> *> const http_status,
                   not_null<std::atomic<bool> *> const uptime_done) {

      // Set the global "done" flag unless we're already shutting down. The latter condition
      // happens if a "SUICIDE" command is received and the quit thread is woken in response.
      bool expected = false;
      if (done.compare_exchange_strong (expected, true)) {
        std::cerr << "pstore broker is exiting.\n";
        log (priority::info, "performing shutdown");

        // Tell the gcwatcher thread to exit.
        broker::gc_sigint (signum);

        if (scav != nullptr) {
          scav->shutdown ();
        }

        if (cp != nullptr) {
          // Ask the read-loop threads to quit
          for (auto ctr = 0U; ctr < num_read_threads; ++ctr) {
            push (cp, read_loop_quit_command);
          }
          // Finally ask the command loop thread to exit.
          push (cp, command_loop_quit_command);
        }

        pstore::http::quit (http_status);
        *uptime_done = true;

        log (priority::info, "shutdown requests complete");
      }
    }

  } // namespace broker
} // namespace pstore

#define COMMON_SIGNALS                                                                             \
  X (SIGABRT)                                                                                      \
  X (SIGFPE)                                                                                       \
  X (SIGILL)                                                                                       \
  X (SIGINT)                                                                                       \
  X (SIGSEGV)                                                                                      \
  X (SIGTERM)                                                                                      \
  X (sig_self_quit)

#ifdef _WIN32
#  define SIGNALS COMMON_SIGNALS X (SIGBREAK)
#else
#  define SIGNALS                                                                                  \
    COMMON_SIGNALS                                                                                 \
    X (SIGALRM)                                                                                    \
    X (SIGBUS)                                                                                     \
    X (SIGCHLD)                                                                                    \
    X (SIGCONT)                                                                                    \
    X (SIGHUP)                                                                                     \
    X (SIGPIPE)                                                                                    \
    X (SIGQUIT)                                                                                    \
    X (SIGSTOP)                                                                                    \
    X (SIGTSTP)                                                                                    \
    X (SIGTTIN)                                                                                    \
    X (SIGTTOU)                                                                                    \
    X (SIGUSR1)                                                                                    \
    X (SIGUSR2)                                                                                    \
    X (SIGSYS)                                                                                     \
    X (SIGTRAP)                                                                                    \
    X (SIGURG)                                                                                     \
    X (SIGVTALRM)                                                                                  \
    X (SIGXCPU)                                                                                    \
    X (SIGXFSZ)
#endif //_WIN32

namespace {

  using pstore::broker::sig_self_quit;

  template <ssize_t Size, typename = std::enable_if_t<(Size > 0)>>
  char const * signal_name (int signo, pstore::gsl::span<char, Size> buffer) {
#define X(sig)                                                                                     \
  case sig: return #sig;
    switch (signo) { SIGNALS }
#undef X
    constexpr auto size = buffer.size ();
    std::snprintf (buffer.data (), size, "#%d", signo);
    buffer[size - 1] = '\0';
    return buffer.data ();
  }

} // end anonymous namespace

#undef SIGNALS

namespace {

  pstore::signal_cv quit_info;

  //***************
  //* quit thread *
  //***************
  void quit_thread (std::weak_ptr<pstore::broker::command_processor> const cp,
                    std::weak_ptr<pstore::broker::scavenger> const scav,
                    unsigned const num_read_threads,
                    not_null<std::optional<pstore::http::server_status> *> const http_status,
                    not_null<std::atomic<bool> *> const uptime_done) {
    try {
      pstore::threads::set_name ("quit");
      pstore::create_log_stream ("broker.quit");

      // Wait for to be told that we are in the process of shutting down. This
      // call will block until signal_handler() [below] is called by, for example,
      // the user typing Control-C.
      quit_info.wait ();

      std::array<char, 16> buffer;
      log (priority::info, "Signal received: shutting down. Signal: ",
           signal_name (quit_info.signal (), pstore::gsl::make_span (buffer)));

      auto const cp_sptr = cp.lock ();
      // If the command processor is alive, clear the queue.
      // TODO: prevent the queue from accepting commands other than the ones we're about
      // to send?
      if (cp_sptr) {
        cp_sptr->clear_queue ();
      }

      auto const scav_sptr = scav.lock ();
      shutdown (cp_sptr.get (), scav_sptr.get (), quit_info.signal (), num_read_threads,
                http_status, uptime_done);
    } catch (std::exception const & ex) {
      log (priority::error, "error:", ex.what ());
    } catch (...) {
      log (priority::error, "unknown exception");
    }

    log (priority::info, "quit thread exiting");
  }


  // signal handler
  // ~~~~~~~~~~~~~~
  /// A signal handler entry point.
  void signal_handler (int const sig) {
    pstore::broker::exit_code = sig;
    pstore::errno_saver const saver;
    quit_info.notify_all (sig);
  }

} // end anonymous namespace


namespace pstore::broker {

  // notify quit thread
  // ~~~~~~~~~~~~~~~~~~
  void notify_quit_thread () {
    quit_info.notify_all (sig_self_quit);
  }

  // create quit thread
  // ~~~~~~~~~~~~~~~~~~
  std::thread
  create_quit_thread (std::weak_ptr<command_processor> cp, std::weak_ptr<scavenger> scav,
                      unsigned num_read_threads,
                      gsl::not_null<std::optional<pstore::http::server_status> *> http_status,
                      gsl::not_null<std::atomic<bool> *> uptime_done) {
    std::thread quit (quit_thread, std::move (cp), std::move (scav), num_read_threads, http_status,
                      uptime_done);

    register_signal_handler (SIGINT, signal_handler);
    register_signal_handler (SIGTERM, signal_handler);
#ifdef _WIN32
      register_signal_handler (SIGBREAK, signal_handler); // Ctrl-Break sequence
#else
      signal (SIGPIPE, SIG_IGN);
#endif
      return quit;
  }

} // end namespace pstore::broker
