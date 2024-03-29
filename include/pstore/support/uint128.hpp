//===- include/pstore/support/uint128.hpp -----------------*- mode: C++ -*-===//
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
/// \file uint128.hpp
/// \brief Declares a portable 128-bit integer type.

#ifndef PSTORE_SUPPORT_UINT128_HPP
#define PSTORE_SUPPORT_UINT128_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

#include "pstore/support/assert.hpp"

namespace pstore {
  class uint128;
} // end namespace pstore

namespace std {

  template <>
  struct is_unsigned<pstore::uint128> : true_type {};
  template <>
  struct is_signed<pstore::uint128> : false_type {};

  // Provide the is_(un)signed<__uint128_t> if we have support for the type in the compiler but
  // not in the standard library.
#if defined(PSTORE_HAVE_UINT128_T) && !defined(PSTORE_HAVE_UINT128_TRAITS_SUPPORT)
  template <>
  struct is_unsigned<__uint128_t> : true_type {};
  template <>
  struct is_signed<__uint128_t> : false_type {};
#endif // PSTORE_HAVE_UINT128_T && !PSTORE_HAVE_UINT128_TRAITS_SUPPORT

} // namespace std

namespace pstore {

  namespace details {

    /// A function object which can extract the high 64-bits from an integer type. If the
    /// compiler doesn't support __uint128_t then this value is always 0.
    template <typename T, typename Enable = void>
    class high {
    public:
      constexpr std::uint64_t operator() (T const t) const noexcept {
        (void) t;
        return 0;
      }
    };

    /// A function object which can extract the high 64-bits from an integer type. This partial
    /// specialization of the template covers the case where the compiler has native support for
    /// the __uint128_t type. Note that we can't use PSTORE_HAVE_UINT128_T to determine this
    /// because we enable this flag to be disabled for testing.
    template <typename T>
    class high<T, std::enable_if_t<(sizeof (T) > sizeof (std::uint64_t))>> {
    public:
      constexpr std::uint64_t operator() (T v) const noexcept {
        return (v >> 64U) & ((T{1} << 64) - 1U);
      }
    };

  } // end namespace details


  class alignas (16) uint128 {
  public:
    constexpr uint128 () noexcept = default;
    constexpr uint128 (std::nullptr_t) noexcept = delete;
#ifdef PSTORE_HAVE_UINT128_T
    constexpr uint128 (std::uint64_t const high, std::uint64_t const low) noexcept
            : v_{__uint128_t{high} << 64U | __uint128_t{low}} {}

    /// Construct from an unsigned integer that's 128-bits wide or fewer.
    template <typename IntType,
              typename = std::enable_if_t<std::is_unsigned_v<IntType>>>
    constexpr uint128 (IntType v) noexcept // NOLINT(hicpp-explicit-conversions)
            : v_{v} {}

    /// \param bytes Points to an array of 16 bytes whose contents represent a 128 bit
    /// value.
    explicit constexpr uint128 (std::uint8_t const * const bytes) noexcept
            : v_{bytes_to_uint128 (bytes)} {}
#else
    constexpr uint128 (std::uint64_t const high, std::uint64_t const low) noexcept
            : low_{low}
            , high_{high} {}

    /// Construct from an unsigned integer that's 64-bits wide or fewer.
    template <typename IntType, typename = std::enable_if_t<std::is_unsigned_v<IntType>>>
    constexpr uint128 (IntType const v) noexcept
            : low_{v} {}

    /// \param bytes Points to an array of 16 bytes whose contents represent a 128 bit
    /// value.
    explicit constexpr uint128 (std::uint8_t const * const bytes) noexcept
            : low_{bytes_to_uint64 (&bytes[8])}
            , high_{bytes_to_uint64 (&bytes[0])} {}
#endif // PSTORE_HAVE_UINT128_T

    explicit uint128 (std::array<std::uint8_t, 16> const & bytes) noexcept
            : uint128 (bytes.data ()) {}

    constexpr uint128 (uint128 const &) = default;
    constexpr uint128 (uint128 &&) noexcept = default;
    ~uint128 () noexcept = default;

    uint128 & operator= (uint128 const &) = default;
    uint128 & operator= (uint128 &&) noexcept = default;

    template <typename T>
    constexpr bool operator== (T rhs) const noexcept;
    template <typename T>
    constexpr bool operator!= (T rhs) const noexcept {
      return !operator== (rhs);
    }

#ifdef PSTORE_HAVE_UINT128_T
    constexpr std::uint64_t high () const noexcept {
      return static_cast<std::uint64_t> (v_ >> 64U);
    }
    constexpr std::uint64_t low () const noexcept {
      return static_cast<std::uint64_t> (v_ & max64);
    }
#else
    constexpr std::uint64_t high () const noexcept { return high_; }
    constexpr std::uint64_t low () const noexcept { return low_; }
#endif // PSTORE_HAVE_UINT128_T

    uint128 operator- () const noexcept;
    constexpr bool operator!() const noexcept;
    uint128 operator~() const noexcept;

    uint128 & operator++ () noexcept;
    uint128 operator++ (int) noexcept;
    uint128 & operator-- () noexcept;
    uint128 operator-- (int) noexcept;

    uint128 & operator+= (uint128 rhs) noexcept;
    uint128 & operator-= (uint128 const b) noexcept { return *this += -b; }

    template <typename T>
    constexpr uint128 operator& (T rhs) const noexcept;
    template <typename T>
    constexpr uint128 operator| (T rhs) const noexcept;

    template <typename T>
    uint128 operator<< (T n) const noexcept;
    uint128 & operator>>= (unsigned n) noexcept;

    static constexpr std::size_t hex_string_length = std::size_t{32};

    template <typename OutputIterator>
    OutputIterator to_hex (OutputIterator out) const noexcept;
    std::string to_hex_string () const;

    static std::optional<uint128> from_hex_string (std::string_view str);

  private:
    static constexpr std::uint64_t max64 = std::numeric_limits<std::uint64_t>::max ();
#ifdef PSTORE_HAVE_UINT128_T
    __uint128_t v_ = 0U;
    static constexpr __uint128_t bytes_to_uint128 (std::uint8_t const * bytes) noexcept;
#else
    std::uint64_t low_ = 0U;
    std::uint64_t high_ = 0U;
    static constexpr std::uint64_t bytes_to_uint64 (std::uint8_t const * bytes) noexcept;
#endif // PSTORE_HAVE_UINT128_T

    static constexpr char digit_to_hex (unsigned const v) noexcept {
      PSTORE_ASSERT (v < 0x10);
      return static_cast<char> (v + ((v < 10) ? '0' : 'a' - 10));
    }
  };

  PSTORE_STATIC_ASSERT (sizeof (uint128) == 16);
  PSTORE_STATIC_ASSERT (alignof (uint128) == 16);
  PSTORE_STATIC_ASSERT (std::is_standard_layout<uint128>::value);

  // operator==
  // ~~~~~~~~~~
  template <typename T>
  constexpr bool uint128::operator== (T rhs) const noexcept {
#ifdef PSTORE_HAVE_UINT128_T
    return v_ == rhs;
#else
    return this->high () == details::high<T>{}(rhs) && this->low () == (rhs & max64);
#endif
  }

  template <>
  constexpr bool uint128::operator==<uint128> (uint128 const rhs) const noexcept {
#ifdef PSTORE_HAVE_UINT128_T
    return v_ == rhs.v_;
#else
    return this->high () == rhs.high () && this->low () == rhs.low ();
#endif
  }

  // operator++
  // ~~~~~~~~~~
  inline uint128 & uint128::operator++ () noexcept {
#ifdef PSTORE_HAVE_UINT128_T
    ++v_;
#else
    if (++low_ == 0U) {
      ++high_;
    }
#endif
    return *this;
  }
  inline uint128 uint128::operator++ (int) noexcept {
    auto const prev = *this;
    ++(*this);
    return prev;
  }

  // operator--
  // ~~~~~~~~~~
  inline uint128 & uint128::operator-- () noexcept {
#ifdef PSTORE_HAVE_UINT128_T
    --v_;
#else
    if (low_ == 0U) {
      low_ = max64;
      --high_;
    } else {
      --low_;
    }
#endif
    return *this;
  }

  inline uint128 uint128::operator-- (int) noexcept {
    auto const prev = *this;
    --(*this);
    return prev;
  }

  // operator+=
  // ~~~~~~~~~~
  inline uint128 & uint128::operator+= (uint128 const rhs) noexcept {
#ifdef PSTORE_HAVE_UINT128_T
    v_ += rhs.v_;
#else
    auto const old_low = low_;
    low_ += rhs.low_;
    high_ += rhs.high_;
    if (low_ < old_low) {
      ++high_;
    }
#endif
    return *this;
  }

  // operator-() (unary negation)
  // ~~~~~~~~~~~
  inline uint128 uint128::operator- () const noexcept {
#ifdef PSTORE_HAVE_UINT128_T
    return {-v_};
#else
    auto r = ~(*this);
    return ++r;
#endif
  }

  // operator!
  // ~~~~~~~~~
  constexpr bool uint128::operator!() const noexcept {
#ifdef PSTORE_HAVE_UINT128_T
    return !v_; // NOLINT(readability-implicit-bool-conversion)
#else
    return !(high_ != 0 || low_ != 0);
#endif
  }

  // operator~ (binary ones complement)
  // ~~~~~~~~~
  inline uint128 uint128::operator~() const noexcept {
#ifdef PSTORE_HAVE_UINT128_T
    return {~v_};
#else
    auto t = *this;
    t.low_ = ~t.low_;
    t.high_ = ~t.high_;
    return t;
#endif
  }

  // operator&
  // ~~~~~~~~~
  template <typename T>
  constexpr uint128 uint128::operator& (T const rhs) const noexcept {
#ifdef PSTORE_HAVE_UINT128_T
    return {v_ & rhs};
#else
    return {0U, low () & rhs};
#endif
  }

  template <>
  constexpr uint128 uint128::operator&<uint128> (uint128 const rhs) const noexcept {
#ifdef PSTORE_HAVE_UINT128_T
    return {v_ & rhs.v_};
#else
    return {high () & rhs.high (), low () & rhs.low ()};
#endif
  }

  // operator|
  // ~~~~~~~~~
  template <typename T>
  constexpr uint128 uint128::operator| (T const rhs) const noexcept {
#ifdef PSTORE_HAVE_UINT128_T
    return {v_ | rhs};
#else
    return {0U, low () | rhs};
#endif
  }

  template <>
  constexpr uint128 uint128::operator|<uint128> (uint128 const rhs) const noexcept {
#ifdef PSTORE_HAVE_UINT128_T
    return {v_ | rhs.v_};
#else
    return {high () | rhs.high (), low () | rhs.low ()};
#endif
  }

  // operator<<
  // ~~~~~~~~~~
  template <typename Other>
  uint128 uint128::operator<< (Other const n) const noexcept {
    PSTORE_ASSERT (n <= 128);
#ifdef PSTORE_HAVE_UINT128_T
    return {v_ << n};
#else
    if (n >= 64U) {
      return {low () << (n - 64U), 0U};
    }
    if (n == 0U) {
      return *this;
    }
    std::uint64_t const mask = (std::uint64_t{1} << n) - 1U;
    auto const dist = 64U - n;
    std::uint64_t const top_of_low_mask = mask << dist;
    std::uint64_t const bottom_of_high = (low () & top_of_low_mask) >> dist;
    return {(high () << n) | bottom_of_high, low () << n};
#endif // PSTORE_HAVE_UINT128_T
  }

  // operator>>=
  // ~~~~~~~~~~~
  inline uint128 & uint128::operator>>= (unsigned const n) noexcept {
    PSTORE_ASSERT (n <= 128);
#ifdef PSTORE_HAVE_UINT128_T
    v_ >>= n;
#else
    if (n >= 64) {
      low_ = high_ >> (n - 64U);
      high_ = 0U;
    } else if (n > 0) {
      std::uint64_t const mask = (std::uint64_t{1} << n) - 1U;
      low_ = (low_ >> n) | ((high_ & mask) << (64U - n));
      high_ >>= n;
    }
#endif // PSTORE_HAVE_UINT128_T
    return *this;
  }

  // bytes to uintXX
  // ~~~~~~~~~~~~~~~
#ifdef PSTORE_HAVE_UINT128_T
  constexpr __uint128_t uint128::bytes_to_uint128 (std::uint8_t const * const bytes) noexcept {
    return __uint128_t{bytes[0]} << 120U | __uint128_t{bytes[1]} << 112U |
           __uint128_t{bytes[2]} << 104U | __uint128_t{bytes[3]} << 96U |
           __uint128_t{bytes[4]} << 88U | __uint128_t{bytes[5]} << 80U |
           __uint128_t{bytes[6]} << 72U | __uint128_t{bytes[7]} << 64U |
           __uint128_t{bytes[8]} << 56U | __uint128_t{bytes[9]} << 48U |
           __uint128_t{bytes[10]} << 40U | __uint128_t{bytes[11]} << 32U |
           __uint128_t{bytes[12]} << 24U | __uint128_t{bytes[13]} << 16U |
           __uint128_t{bytes[14]} << 8U | __uint128_t{bytes[15]};
  }
#else
  constexpr std::uint64_t uint128::bytes_to_uint64 (std::uint8_t const * const bytes) noexcept {
    return std::uint64_t{bytes[0]} << 56 | std::uint64_t{bytes[1]} << 48 |
           std::uint64_t{bytes[2]} << 40 | std::uint64_t{bytes[3]} << 32 |
           std::uint64_t{bytes[4]} << 24 | std::uint64_t{bytes[5]} << 16 |
           std::uint64_t{bytes[6]} << 8 | std::uint64_t{bytes[7]};
  }
#endif // PSTORE_HAVE_UINT128_T

  template <typename OutputIterator>
  OutputIterator uint128::to_hex (OutputIterator out) const noexcept {
#ifndef NDEBUG
    auto emitted = 0U;
#endif
#ifdef PSTORE_HAVE_UINT128_T
    for (auto shift = 4U; shift <= 128U; shift += 4U) {
      *(out++) = digit_to_hex ((v_ >> (128U - shift)) & 0x0FU);
#  ifndef NDEBUG
      ++emitted;
#  endif
    }
#else
    for (auto shift = 4U; shift <= 64; shift += 4U) {
      *(out++) = digit_to_hex ((high_ >> (64U - shift)) & 0x0FU);
#  ifndef NDEBUG
      ++emitted;
#  endif
    }
    for (auto shift = 4U; shift <= 64; shift += 4U) {
      *(out++) = digit_to_hex ((low_ >> (64U - shift)) & 0x0FU);
#  ifndef NDEBUG
      ++emitted;
#  endif
    }
#endif
    PSTORE_ASSERT (emitted == hex_string_length);
    return out;
  }

  inline std::ostream & operator<< (std::ostream & os, uint128 const & value) {
    return os << '{' << value.high () << ',' << value.low () << '}';
  }

  constexpr bool operator== (uint128 const & lhs, uint128 const & rhs) noexcept {
    return lhs.operator== (rhs);
  }
  constexpr bool operator!= (uint128 const & lhs, uint128 const & rhs) noexcept {
    return lhs.operator!= (rhs);
  }

  constexpr bool operator<(uint128 const & lhs, uint128 const & rhs) noexcept {
    return lhs.high () < rhs.high () || (!(rhs.high () < lhs.high ()) && lhs.low () < rhs.low ());
  }
  constexpr bool operator> (uint128 const & lhs, uint128 const & rhs) noexcept {
    return rhs < lhs;
  }
  constexpr bool operator>= (uint128 const & lhs, uint128 const & rhs) noexcept {
    return !(lhs < rhs);
  }
  constexpr bool operator<= (uint128 const & lhs, uint128 const & rhs) noexcept {
    return !(rhs < lhs);
  }

} // end namespace pstore

namespace std {
  template <>
  struct hash<pstore::uint128> {
    using argument_type = pstore::uint128;
    using result_type = size_t;

    result_type operator() (argument_type const & s) const {
      return std::hash<std::uint64_t>{}(s.low ()) ^ std::hash<std::uint64_t>{}(s.high ());
    }
  };

  template <>
  class numeric_limits<pstore::uint128> {
    using type = pstore::uint128;

  public:
    static constexpr bool is_specialized = true;
    static constexpr bool is_signed = false;

    static constexpr type min () noexcept { return {0U}; }
    static constexpr type max () noexcept {
      return {std::numeric_limits<std::uint64_t>::max (),
              std::numeric_limits<std::uint64_t>::max ()};
    }
    static constexpr type lowest () noexcept { return type{0U}; }

    static constexpr const int digits = 128;
    static constexpr const int digits10 = 38;
    static constexpr const int max_digits10 = 0;
    static constexpr const bool is_integer = true;
    static constexpr const bool is_exact = true;
    static constexpr const int radix = 2;
    static constexpr type epsilon () noexcept { return {0U}; }
    static constexpr type round_error () noexcept { return {0U}; }

    static constexpr const int min_exponent = 0;
    static constexpr const int min_exponent10 = 0;
    static constexpr const int max_exponent = 0;
    static constexpr const int max_exponent10 = 0;

    static constexpr const bool has_infinity = false;
    static constexpr const bool has_quiet_NaN = false; // NOLINT(readability-identifier-naming)
    // NOLINTNEXTLINE(readability-identifier-naming)
    static constexpr const bool has_signaling_NaN = false;
    static constexpr const float_denorm_style has_denorm = denorm_absent;
    static constexpr const bool has_denorm_loss = false;
    static constexpr type infinity () noexcept { return {0U}; }
    // NOLINTNEXTLINE(readability-identifier-naming)
    static constexpr type quiet_NaN () noexcept { return {0U}; }
    // NOLINTNEXTLINE(readability-identifier-naming)
    static constexpr type signaling_NaN () noexcept { return {0U}; }
    static constexpr type denorm_min () noexcept { return {0U}; }

    static constexpr const bool is_iec559 = false;
    static constexpr const bool is_bounded = true;
    static constexpr const bool is_modulo = true;

    static constexpr const bool traps = true;
    static constexpr const bool tinyness_before = false;
    static constexpr const float_round_style round_style = std::round_toward_zero;
  };

} // end namespace std

#endif // PSTORE_SUPPORT_UINT128_HPP
