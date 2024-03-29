//===- lib/broker/spawn_win32.cpp -----------------------------------------===//
//*                                  *
//*  ___ _ __   __ ___      ___ __   *
//* / __| '_ \ / _` \ \ /\ / / '_ \  *
//* \__ \ |_) | (_| |\ V  V /| | | | *
//* |___/ .__/ \__,_| \_/\_/ |_| |_| *
//*     |_|                          *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file spawn_win32.cpp

#include "pstore/broker/spawn.hpp"

#ifdef _WIN32

// standard includes
#  include <cstring>

// pstore includes
#  include "pstore/os/descriptor.hpp"
#  include "pstore/support/error.hpp"
#  include "pstore/support/utf.hpp"


namespace pstore::broker {
  namespace win32 {
    //*                                          _      *
    //*  _ __ _ _ ___  __ ___ ______  _ __  __ _(_)_ _  *
    //* | '_ \ '_/ _ \/ _/ -_|_-<_-< | '_ \/ _` | | '_| *
    //* | .__/_| \___/\__\___/__/__/ | .__/\__,_|_|_|   *
    //* |_|                          |_|                *
    process_pair::process_pair (HANDLE p, DWORD g) noexcept
            : process_{p, &close_handle}
            , group_{g} {}

    void process_pair::close_handle (HANDLE h) noexcept {
      if (h != nullptr) {
        ::CloseHandle (h);
      }
    }

    // argv quote
    // ~~~~~~~~~~
    /// Given an individual command-line argument, returns it with all necessary quoting and
    /// escaping for use on the Windows command-line.
    /// \param in_arg  The argument string to be quoted.
    /// \param force  If true, the resulting string will always be quoted; otherwise it is
    ///  quoted when necessary. Setting to true may avoid problems with process that don't
    ///  parse quotes properly.
    /// \returns The quoted command-line string.
    std::string argv_quote (std::string const & in_arg, bool force) {
      if (!force && !in_arg.empty () && in_arg.find_first_of (" \t\n\v\"\\") == in_arg.npos) {
        return in_arg;
      }

      std::string res;
      res.reserve (in_arg.length () + 2);
      res.push_back ('"');

      for (auto it = std::begin (in_arg), end = std::end (in_arg); it != end; ++it) {
        auto num_backslashes = std::size_t{0};

        // Count the number of sequential backslashes.
        while (it != end && *it == '\\') {
          ++it;
          ++num_backslashes;
        }

        if (it == end) {
          // Escape all backslashes, but let the terminating
          // double quotation mark we add below be interpreted
          // as a metacharacter.
          res.append (num_backslashes * 2U, '\\');
          break;
        }

        if (*it == '"') {
          // Escape all backslashes and the following
          // double quotation mark.
          res.append (num_backslashes * 2U + 1U, '\\');
        } else {
          // Backslashes aren't special here.
          res.append (num_backslashes, '\\');
        }
        res.push_back (*it);
      }

      res.push_back ('"');
      return res;
    }

    // build command line
    // ~~~~~~~~~~~~~~~~~~
    /// Given a null-terminated array of strings, returns a quoted Windows command-line
    /// string.
    std::string build_command_line (gsl::not_null<gsl::czstring const *> argv) {
      std::string command_line;
      auto separator = "";
      for (auto arg = argv.get (); *arg != nullptr; ++arg) {
        command_line.append (separator);
        command_line.append (argv_quote (*arg, false));
        separator = " ";
      }
      return command_line;
    }

  } // end namespace win32

  process_identifier spawn (gsl::czstring exe_path, gsl::not_null<gsl::czstring const *> argv) {
    auto const exe_path_utf16 = utf::win32::to16 (exe_path);
    auto const command_line = utf::win32::to16 (win32::build_command_line (argv));

    STARTUPINFOW startup_info;
    std::memset (&startup_info, 0, sizeof (startup_info));
    startup_info.cb = sizeof (startup_info);

    PROCESS_INFORMATION process_information;
    std::memset (&process_information, 0, sizeof (process_information));

    if (!::CreateProcessW (const_cast<wchar_t *> (exe_path_utf16.c_str ()),
                           const_cast<wchar_t *> (command_line.c_str ()),
                           static_cast<SECURITY_ATTRIBUTES *> (nullptr), // process attributes
                           static_cast<SECURITY_ATTRIBUTES *> (nullptr), // thread attributes
                           false,                                        // inherit handles
                           BELOW_NORMAL_PRIORITY_CLASS | CREATE_NEW_PROCESS_GROUP |
                             CREATE_NO_WINDOW,                           // creation flags
                           nullptr,                                      // environment
                           nullptr, // working directory for the new process
                           &startup_info, &process_information)) {

      raise (win32_erc (::GetLastError ()), "CreateProcessW");
    }

    // Close the main-thread handle. The process handle is needed to provide a robust
    // reference to the process later on.
    ::CloseHandle (process_information.hThread);
    process_information.hThread = nullptr;

    return std::make_shared<process_identifier::element_type> (process_information.hProcess,
                                                               process_information.dwProcessId);
  }

} // end namespace pstore::broker

#endif //_WIN32
