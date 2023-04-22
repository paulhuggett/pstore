//===- include/pstore/support/round2.hpp ------------------*- mode: C++ -*-===//
//*                            _ ____   *
//*  _ __ ___  _   _ _ __   __| |___ \  *
//* | '__/ _ \| | | | '_ \ / _` | __) | *
//* | | | (_) | |_| | | | | (_| |/ __/  *
//* |_|  \___/ \__,_|_| |_|\__,_|_____| *
//*                                     *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file round2.hpp
/// \brief Defines a function which rounds up to the next highest power of 2.
///
/// The implementation of these functions is based on code published in the "Bit Twiddling Hacks"
/// web page by Sean Eron Anderson (seander@cs.stanford.edu) found at
/// <https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2>. The original code is
/// public domain.

#ifndef PSTORE_SUPPORT_ROUND2_HPP
#define PSTORE_SUPPORT_ROUND2_HPP

#include <cstdint>

namespace pstore {

  namespace details {

    template <unsigned Shift, typename T>
    constexpr T round (T v) noexcept {
      if constexpr (Shift == 0U) {
        return v;
      }
      v = round<Shift / 2> (v);
      return v | (v >> Shift);
    }

  } // end namespace details

  constexpr std::uint8_t round_to_power_of_2 (std::uint8_t v) noexcept {
    --v;
    v = details::round<4, std::uint8_t> (v);
    ++v;
    return v;
  }

  constexpr std::uint16_t round_to_power_of_2 (std::uint16_t v) noexcept {
    --v;
    v = details::round<8, std::uint16_t> (v);
    ++v;
    return v;
  }

  constexpr std::uint32_t round_to_power_of_2 (std::uint32_t v) noexcept {
    --v;
    v = details::round<16, std::uint32_t> (v);
    ++v;
    return v;
  }

  constexpr std::uint64_t round_to_power_of_2 (std::uint64_t v) noexcept {
    --v;
    v = details::round<32, std::uint64_t> (v);
    ++v;
    return v;
  }

} // end namespace pstore

#endif // PSTORE_SUPPORT_ROUND2_HPP
