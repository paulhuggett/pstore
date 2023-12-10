//===- include/pstore/command_line/parser.hpp -------------*- mode: C++ -*-===//
//*                                  *
//*  _ __   __ _ _ __ ___  ___ _ __  *
//* | '_ \ / _` | '__/ __|/ _ \ '__| *
//* | |_) | (_| | |  \__ \  __/ |    *
//* | .__/ \__,_|_|  |___/\___|_|    *
//* |_|                              *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_COMMAND_LINE_PARSER_HPP
#define PSTORE_COMMAND_LINE_PARSER_HPP

#include <algorithm>
#include <charconv>
#include <cerrno>
#include <cstdlib>
#include <optional>

#include "pstore/adt/small_vector.hpp"
#include "pstore/support/gsl.hpp"

namespace pstore::command_line {

  //*                              *
  //*  _ __  __ _ _ _ ___ ___ _ _  *
  //* | '_ \/ _` | '_(_-</ -_) '_| *
  //* | .__/\__,_|_| /__/\___|_|   *
  //* |_|                          *
  template <typename T, typename = void>
  class parser {
  public:
    std::optional<T> operator() (std::string const & v) const;
  };

  template <typename T>
  class parser<T, typename std::enable_if_t<std::is_enum_v<T>>> {
  public:
    struct value_type {
      value_type (std::string_view n, T v, std::string_view d)
              : name{n}
              , value{v}
              , description{d} {}
      std::string name;
      T value;
      std::string description;
    };

    void add_literal (std::string_view name, T const value, std::string_view description) {
      literals_.emplace_back (name, value, description);
    }

    std::optional<T> operator() (std::string_view v) const {
      auto const end = literals_.end ();
      if (auto const pos = std::find_if (literals_.begin (), end,
                                         [&v] (value_type const & lit) { return v == lit.name; });
          pos != end) {
        return std::optional<T>{pos->value};
      }
      return std::nullopt;
    }

    auto begin () const { return std::begin (literals_); }
    auto end () const { return std::end (literals_); }

  private:
    small_vector<value_type, 4> literals_;
  };

  template <typename T>
  class parser<T, typename std::enable_if_t<std::is_integral_v<T>>> {
  public:
    std::optional<T> operator() (std::string_view v) const {
      auto const * const first = v.data ();
      auto const * const last = first + v.size ();
      T result{};
      auto [ptr, ec] = std::from_chars (first, last, result);
      if (ec != std::errc{} || ptr != last) {
        return std::nullopt;
      }
      return result;
    }
  };

  template <>
  class parser<std::string> {
  public:
    std::optional<std::string> operator() (std::string_view v) const {
      return std::optional<std::string>{v};
    }
  };

} // end namespace pstore::command_line

#endif // PSTORE_COMMAND_LINE_PARSER_HPP
