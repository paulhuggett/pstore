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
#include "pstore/command_line/literal.hpp"

namespace pstore::command_line {

  //*                               _                   *
  //*  _ __  __ _ _ _ ___ ___ _ _  | |__  __ _ ___ ___  *
  //* | '_ \/ _` | '_(_-</ -_) '_| | '_ \/ _` (_-</ -_) *
  //* | .__/\__,_|_| /__/\___|_|   |_.__/\__,_/__/\___| *
  //* |_|                                               *
  class parser_base {
  public:
    virtual ~parser_base () noexcept;
    void add_literal_option (std::string const & name, int value, std::string const & description);

    auto begin () const { return std::begin (literals_); }
    auto end () const { return std::end (literals_); }

  private:
    // TODO: this stuff should not be in the base class!
    using container = small_vector<literal, 4>;
    container literals_;
  };


  //*                              *
  //*  _ __  __ _ _ _ ___ ___ _ _  *
  //* | '_ \/ _` | '_(_-</ -_) '_| *
  //* | .__/\__,_|_| /__/\___|_|   *
  //* |_|                          *
  template <typename T, typename = void>
  class parser final : public parser_base {
  public:
    std::optional<T> operator() (std::string const & v) const;
  };

  template <typename T>
  class parser<T, typename std::enable_if_t<std::is_enum_v<T>>> final : public parser_base {
  public:
    std::optional<T> operator() (std::string_view v) const {
      auto const end = this->end ();
      auto const it =
        std::find_if (this->begin (), end, [&v] (literal const & lit) { return v == lit.name; });
      if (it == end) {
        return {};
      }
      return std::optional<T>{std::in_place, static_cast<T> (it->value)};
    }
  };

  template <typename T>
  class parser<T, typename std::enable_if_t<std::is_integral_v<T>>> final : public parser_base {
  public:
    std::optional<T> operator() (std::string_view v) const {
      PSTORE_ASSERT (std::distance (this->begin (), this->end ()) == 0 &&
                     "Don't specify literal values for an integral option!");
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
  class parser<std::string> final : public parser_base {
  public:
    std::optional<std::string> operator() (std::string_view v) const;
  };

} // end namespace pstore::command_line

#endif // PSTORE_COMMAND_LINE_PARSER_HPP
