//===- include/pstore/exchange/import_non_terminals.hpp ---*- mode: C++ -*-===//
//*  _                            _                        *
//* (_)_ __ ___  _ __   ___  _ __| |_   _ __   ___  _ __   *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| | '_ \ / _ \| '_ \  *
//* | | | | | | | |_) | (_) | |  | |_  | | | | (_) | | | | *
//* |_|_| |_| |_| .__/ \___/|_|   \__| |_| |_|\___/|_| |_| *
//*             |_|                                        *
//*  _                      _             _      *
//* | |_ ___ _ __ _ __ ___ (_)_ __   __ _| |___  *
//* | __/ _ \ '__| '_ ` _ \| | '_ \ / _` | / __| *
//* | ||  __/ |  | | | | | | | | | | (_| | \__ \ *
//*  \__\___|_|  |_| |_| |_|_|_| |_|\__,_|_|___/ *
//*                                              *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_EXCHANGE_IMPORT_NON_TERMINALS_HPP
#define PSTORE_EXCHANGE_IMPORT_NON_TERMINALS_HPP

#include <tuple>

#include "pstore/exchange/import_rule.hpp"

namespace pstore::exchange::import_ns {

  //*      _     _        _              _      *
  //*  ___| |__ (_)___ __| |_   _ _ _  _| |___  *
  //* / _ \ '_ \| / -_) _|  _| | '_| || | / -_) *
  //* \___/_.__// \___\__|\__| |_|  \_,_|_\___| *
  //*         |__/                              *
  //-MARK: object rule
  template <typename NextState, typename... Args>
  class object_rule final : public rule {
  public:
    explicit object_rule (not_null<context *> const ctxt, Args... args)
            : rule (ctxt)
            , args_{std::forward_as_tuple (args...)} {}
    object_rule (object_rule const &) = delete;
    object_rule (object_rule &&) noexcept = delete;

    ~object_rule () noexcept override = default;

    object_rule & operator= (object_rule const &) = delete;
    object_rule & operator= (object_rule &&) noexcept = delete;

    gsl::czstring name () const noexcept override { return "object rule"; }

    std::error_code begin_object () override {
      std::apply (&object_rule::replace_top<NextState, Args...>,
                  std::tuple_cat (std::make_tuple (this), args_));
      return {};
    }

  private:
    std::tuple<Args...> args_;
  };

  template <typename Next, typename... Args>
  std::error_code push_object_rule (rule * const rule, Args... args) {
    return rule->push<object_rule<Next, Args...>> (args...);
  }


  //*                                    _      *
  //*  __ _ _ _ _ _ __ _ _  _   _ _ _  _| |___  *
  //* / _` | '_| '_/ _` | || | | '_| || | / -_) *
  //* \__,_|_| |_| \__,_|\_, | |_|  \_,_|_\___| *
  //*                    |__/                   *
  //-MARK: array rule
  template <typename NextRule, typename... Args>
  class array_rule final : public rule {
  public:
    explicit array_rule (not_null<context *> const ctxt, Args... args)
            : rule (ctxt)
            , args_{std::forward_as_tuple (args...)} {}
    array_rule (array_rule const &) = delete;
    array_rule (array_rule &&) noexcept = delete;

    ~array_rule () noexcept override = default;

    array_rule & operator= (array_rule const &) = delete;
    array_rule & operator= (array_rule &&) noexcept = delete;

    std::error_code begin_array () override {
      std::apply (&array_rule::replace_top<NextRule, Args...>,
                  std::tuple_cat (std::make_tuple (this), args_));
      return {};
    }

    gsl::czstring name () const noexcept override { return "array rule"; }

  private:
    std::tuple<Args...> args_;
  };

  template <typename NextRule, typename... Args>
  std::error_code push_array_rule (rule * const rule, Args... args) {
    return rule->push<array_rule<NextRule, Args...>> (args...);
  }

} // end namespace pstore::exchange::import_ns

#endif // PSTORE_EXCHANGE_IMPORT_NON_TERMINALS_HPP
