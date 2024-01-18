//===- include/pstore/support/scope_guard.hpp -------------*- mode: C++ -*-===//
//*                                                       _  *
//*  ___  ___ ___  _ __   ___    __ _ _   _  __ _ _ __ __| | *
//* / __|/ __/ _ \| '_ \ / _ \  / _` | | | |/ _` | '__/ _` | *
//* \__ \ (_| (_) | |_) |  __/ | (_| | |_| | (_| | | | (_| | *
//* |___/\___\___/| .__/ \___|  \__, |\__,_|\__,_|_|  \__,_| *
//*               |_|           |___/                        *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_SUPPORT_SCOPE_GUARD_HPP
#define PSTORE_SUPPORT_SCOPE_GUARD_HPP

#include <algorithm>
#include <utility>

#include "pstore/support/portab.hpp"

namespace pstore {

  /// An implementation of std::scope_exit<> which is based loosely on P0052r7 "Generic Scope
  /// Guard and RAII Wrapper for the Standard Library".
  template <typename ExitFunction, typename = std::enable_if_t<std::is_invocable_v<ExitFunction>>>
  class scope_exit {
  public:
    template <
      typename OtherExitFunction,
      typename = std::enable_if_t<std::is_invocable_v<OtherExitFunction> &&
                                  !std::is_same_v<scope_exit, remove_cvref_t<OtherExitFunction>>>>
    explicit scope_exit (OtherExitFunction && other)
            : exit_function_{std::forward<OtherExitFunction> (other)} {}

    scope_exit (scope_exit const &) = delete;
    scope_exit (scope_exit && rhs) noexcept
            : execute_on_destruction_{rhs.execute_on_destruction_}
            , exit_function_{std::move (rhs.exit_function_)} {
      rhs.release ();
    }

    scope_exit & operator= (scope_exit const &) = delete;
    scope_exit & operator= (scope_exit &&) = delete;

    ~scope_exit () noexcept {
      if (execute_on_destruction_) {
        no_ex_escape ([this] () { exit_function_ (); });
      }
    }

    void release () noexcept { execute_on_destruction_ = false; }

  private:
    bool execute_on_destruction_ = true;
    ExitFunction exit_function_;
  };

  template <typename ExitFunction>
  scope_exit (ExitFunction &&) -> scope_exit<ExitFunction>;

  template <typename Function>
  scope_exit<std::decay_t<Function>> make_scope_exit (Function && f) {
    return scope_exit<std::decay_t<Function>> (std::forward<Function> (f));
  }

} // namespace pstore

#endif // PSTORE_SUPPORT_SCOPE_GUARD_HPP
