//===- lib/support/uint128.cpp --------------------------------------------===//
//*        _       _   _ ____  ___   *
//*  _   _(_)_ __ | |_/ |___ \( _ )  *
//* | | | | | '_ \| __| | __) / _ \  *
//* | |_| | | | | | |_| |/ __/ (_) | *
//*  \__,_|_|_| |_|\__|_|_____\___/  *
//*                                  *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/support/uint128.hpp"

#include "pstore/support/maybe.hpp"

namespace {

  std::optional<unsigned> hex_to_digit (char const digit) noexcept {
    if (digit >= 'a' && digit <= 'f') {
      return static_cast<unsigned> (digit) - ('a' - 10);
    }
    if (digit >= 'A' && digit <= 'F') {
      return static_cast<unsigned> (digit) - ('A' - 10);
    }
    if (digit >= '0' && digit <= '9') {
      return static_cast<unsigned> (digit) - '0';
    }
    return {};
  }

  std::optional<std::uint64_t> get64 (std::string_view str, unsigned index) {
    PSTORE_ASSERT (index < str.length ());
    auto result = std::uint64_t{0};
    for (auto shift = 60; shift >= 0; shift -= 4, ++index) {
      auto const digit = hex_to_digit (str[index]);
      if (!digit) {
        return {};
      }
      result |= (static_cast<std::uint64_t> (digit.value ()) << static_cast<unsigned> (shift));
    }
    return result;
  }

} // end anonymous namespace

namespace pstore {

  // to hex string
  // ~~~~~~~~~~~~~
  std::string uint128::to_hex_string () const {
    std::string result;
    result.reserve (hex_string_length);
    this->to_hex (std::back_inserter (result));
    return result;
  }

  // from hex string
  // ~~~~~~~~~~~~~~~
  std::optional<uint128> uint128::from_hex_string (std::string_view str) {
    if (str.length () != 32) {
      return {};
    }
    return get64 (str, 0U) >>= [&] (std::uint64_t const high) {
      return get64 (str, 16U) >>= [&] (std::uint64_t const low) {
        return std::optional<uint128>{std::in_place, high, low};
      };
    };
  }

} // end namespace pstore
