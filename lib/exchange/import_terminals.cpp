//===- lib/exchange/import_terminals.cpp ----------------------------------===//
//*  _                            _     _                      _             _      *
//* (_)_ __ ___  _ __   ___  _ __| |_  | |_ ___ _ __ _ __ ___ (_)_ __   __ _| |___  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| | __/ _ \ '__| '_ ` _ \| | '_ \ / _` | / __| *
//* | | | | | | | |_) | (_) | |  | |_  | ||  __/ |  | | | | | | | | | | (_| | \__ \ *
//* |_|_| |_| |_| .__/ \___/|_|   \__|  \__\___|_|  |_| |_| |_|_|_| |_|\__,_|_|___/ *
//*             |_|                                                                 *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file import_terminals.cpp
/// \brief Implements rules for the handling of terminals in the grammer (e.g. integers and
/// strings).
#include "pstore/exchange/import_terminals.hpp"

#include "pstore/exchange/import_error.hpp"

namespace pstore::exchange::import_ns {

  //*  _              _            _      *
  //* | |__  ___  ___| |  _ _ _  _| |___  *
  //* | '_ \/ _ \/ _ \ | | '_| || | / -_) *
  //* |_.__/\___/\___/_| |_|  \_,_|_\___| *
  //*                                     *
  std::error_code bool_rule::boolean_value (bool const v) {
    *v_ = v;
    return pop ();
  }

  gsl::czstring bool_rule::name () const noexcept {
    return "bool";
  }

  //*  _     _                               _      *
  //* (_)_ _| |_ ___ __ _ ___ _ _   _ _ _  _| |___  *
  //* | | ' \  _/ -_) _` / -_) '_| | '_| || | / -_) *
  //* |_|_||_\__\___\__, \___|_|   |_|  \_,_|_\___| *
  //*               |___/                           *
  std::error_code integer_rule::integer_value (std::int64_t const v) {
    *v_ = v;
    return pop ();
  }

  gsl::czstring integer_rule::name () const noexcept {
    return "integer";
  }

  //*       _     _                               _      *
  //*  _  _(_)_ _| |_ ___ __ _ ___ _ _   _ _ _  _| |___  *
  //* | || | | ' \  _/ -_) _` / -_) '_| | '_| || | / -_) *
  //*  \_,_|_|_||_\__\___\__, \___|_|   |_|  \_,_|_\___| *
  //*                    |___/                           *
  std::error_code uinteger_rule::integer_value (std::int64_t const v) {
    if (v < 0) {
      return peejay::error::number_out_of_range;
    }
    *v_ = static_cast<std::uint64_t> (v);
    return pop ();
  }

  gsl::czstring uinteger_rule::name () const noexcept {
    return "uinteger";
  }

  //*     _       _                      _      *
  //*  __| |_ _ _(_)_ _  __ _   _ _ _  _| |___  *
  //* (_-<  _| '_| | ' \/ _` | | '_| || | / -_) *
  //* /__/\__|_| |_|_||_\__, | |_|  \_,_|_\___| *
  //*                   |___/                   *
  std::error_code string_rule::string_value (peejay::u8string_view v) {
    *v_ = v;
    return pop ();
  }

  gsl::czstring string_rule::name () const noexcept {
    return "string";
  }

} // end namespace pstore::exchange::import_ns
