//===- include/pstore/exchange/import_uuid.hpp ------------*- mode: C++ -*-===//
//*  _                            _                 _     _  *
//* (_)_ __ ___  _ __   ___  _ __| |_   _   _ _   _(_) __| | *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| | | | | | | | |/ _` | *
//* | | | | | | | |_) | (_) | |  | |_  | |_| | |_| | | (_| | *
//* |_|_| |_| |_| .__/ \___/|_|   \__|  \__,_|\__,_|_|\__,_| *
//*             |_|                                          *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_EXCHANGE_IMPORT_UUID_HPP
#define PSTORE_EXCHANGE_IMPORT_UUID_HPP

#include "pstore/core/uuid.hpp"
#include "pstore/exchange/import_rule.hpp"

namespace pstore::exchange::import_ns {

  class uuid_rule final : public rule {
  public:
    uuid_rule (not_null<context *> ctxt, not_null<uuid *> v) noexcept;
    uuid_rule (uuid_rule const &) = delete;
    uuid_rule (uuid_rule &&) = delete;

    ~uuid_rule () noexcept override = default;

    uuid_rule & operator= (uuid_rule const &) = delete;
    uuid_rule & operator= (uuid_rule &&) = delete;

    std::error_code string_value (peejay::u8string_view v) override;
    gsl::czstring name () const noexcept override;

  private:
    not_null<uuid *> const v_;
  };

} // end namespace pstore::exchange::import_ns

#endif // PSTORE_EXCHANGE_IMPORT_UUID_HPP
