//===- include/pstore/exchange/import_terminals.hpp -------*- mode: C++ -*-===//
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
/// \file import_terminals.hpp
/// \brief Declares rules for the handling of terminals in the grammer (e.g. integers and
/// strings).
#ifndef PSTORE_EXCHANGE_IMPORT_TERMINALS_HPP
#define PSTORE_EXCHANGE_IMPORT_TERMINALS_HPP

#include "pstore/exchange/import_rule.hpp"

namespace pstore::exchange::import_ns {

  class bool_rule final : public rule {
  public:
    bool_rule (not_null<context *> const ctxt, not_null<bool *> const v) noexcept
            : rule (ctxt)
            , v_{v} {}
    std::error_code boolean_value (bool v) override;
    gsl::czstring name () const noexcept override;

  private:
    not_null<bool *> const v_;
  };

  class integer_rule final : public rule {
  public:
    integer_rule (not_null<context *> const ctxt, not_null<std::int64_t *> const v) noexcept
            : rule (ctxt)
            , v_{v} {}
    std::error_code integer_value (std::int64_t v) override;
    gsl::czstring name () const noexcept override;

  private:
    not_null<std::int64_t *> const v_;
  };

  class uinteger_rule final : public rule {
  public:
    uinteger_rule (not_null<context *> const ctxt, not_null<std::uint64_t *> const v) noexcept
            : rule (ctxt)
            , v_{v} {}
    std::error_code integer_value (std::int64_t v) override;
    gsl::czstring name () const noexcept override;

  private:
    not_null<std::uint64_t *> const v_;
  };

  class string_rule final : public rule {
  public:
    string_rule (not_null<context *> const ctxt, not_null<std::string *> const v) noexcept
            : rule (ctxt)
            , v_{v} {}
    std::error_code string_value (peejay::u8string_view v) override;
    gsl::czstring name () const noexcept override;

  private:
    not_null<std::string *> const v_;
  };

} // end namespace pstore::exchange::import_ns

#endif // PSTORE_EXCHANGE_IMPORT_TERMINALS_HPP
