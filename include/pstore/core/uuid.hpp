//===- include/pstore/core/uuid.hpp -----------------------*- mode: C++ -*-===//
//*              _     _  *
//*  _   _ _   _(_) __| | *
//* | | | | | | | |/ _` | *
//* | |_| | |_| | | (_| | *
//*  \__,_|\__,_|_|\__,_| *
//*                       *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file uuid.hpp
/// \brief Defines the uuid class which supports RFC4122 UUIDs.

#ifndef PSTORE_CORE_UUID_HPP
#define PSTORE_CORE_UUID_HPP

#include <array>
#include <cstdint>
#include <optional>
#include <string>

#if defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN
#  include <Rpc.h>
#elif defined(__APPLE__)
#  include <uuid/uuid.h>
#endif

#include "pstore/support/assert.hpp"

namespace pstore {

  /// The uuid class is used to represent Universally Unique Identifiers (UUID) as defined
  /// by RFC 4122. Specifically, it will generate version 4 (random) UUIDs but can be used to
  /// record all versions and variants.
  class uuid {
    friend bool operator== (uuid const & lhs, uuid const & rhs) noexcept;
    friend bool operator!= (uuid const & lhs, uuid const & rhs) noexcept;
    friend bool operator<(uuid const & lhs, uuid const & rhs) noexcept;
    friend bool operator<= (uuid const & lhs, uuid const & rhs) noexcept;
    friend bool operator> (uuid const & lhs, uuid const & rhs) noexcept;
    friend bool operator>= (uuid const & lhs, uuid const & rhs) noexcept;

  public:
    static constexpr auto elements = std::size_t{16};

    /// RFC4122 defines the UUID string representation which includes 16 two-digit hex numbers
    /// and 4 hyphens.
    static constexpr auto string_length = elements * 2 + 4;

    using container_type = std::array<std::uint8_t, elements>;
    using value_type = container_type::value_type;
    using size_type = container_type::size_type;
    using difference_type = container_type::difference_type;

    using reference = container_type::reference;
    using const_reference = container_type::const_reference;
    using iterator = container_type::iterator;
    using const_iterator = container_type::const_iterator;

    uuid ();
    /// Converts a string following the convention defined by RFC4122 to a UUID. If the string
    /// is not valid an error is raised.
    explicit uuid (std::string const & s);

    /// A constructor used to construct a specific UUID from its binary value.
    explicit constexpr uuid (container_type const & c)
            : data_{c} {}

#if defined(_WIN32)
    /// Convert from the native Win32 UUID type.
    explicit uuid (UUID const & u);
#elif defined(__APPLE__)
    explicit uuid (uuid_t const & bytes);
#endif

    /// Converts a string to a UUID following the convention defined by RFC4122. If the string
    /// is not valid, returns nothing<uuid>.
    ///
    /// \param str  A string to be converted to a UUID.
    /// \returns  just<uuid>() if the string \p s was valid according to the description in
    ///   RFC4122. If the string was invalid, nothing<uuid>.
    static std::optional<uuid> from_string (std::string_view str);

    iterator begin () noexcept { return std::begin (data_); }
    const_iterator begin () const noexcept { return std::begin (data_); }
    iterator end () noexcept { return std::end (data_); }
    const_iterator end () const noexcept { return std::end (data_); }

    container_type const & array () const noexcept { return data_; }

    enum class variant_type {
      ncs,       // NCS backward compatibility
      rfc_4122,  // Defined by RFC 4122
      microsoft, // Microsoft Corporation backward compatibility
      future     // Future definition
    };
    enum class version_type {
      time_based = 1,
      dce_security = 2,
      name_based_md5 = 3,
      random_number_based = 4,
      name_based_sha1 = 5,
      unknown,
    };

    auto variant () const noexcept -> variant_type;
    auto version () const noexcept -> version_type;
    bool is_null () const noexcept;

    /// Yields a string representation of the UUID following the convention defined by RFC4122.
    std::string str () const;

  private:
    container_type data_{};

    enum {
      version_octet = 6,
      variant_octet = 8,
    };

    /// Get a specific numbered byte from a supplied numeric value of type Ty. The bytes
    /// are numbered with 0 being the least significant and N the most significant (where
    /// N is the number of bytes in Ty).
    template <typename Ty>
    static std::uint8_t get_byte (Ty const t, unsigned const num) noexcept {
      PSTORE_ASSERT (num < sizeof (Ty));
      return static_cast<std::uint8_t> ((t >> (num * 8)) & 0xff);
    }
  };

  PSTORE_STATIC_ASSERT (std::is_standard_layout<uuid>::value);
  PSTORE_STATIC_ASSERT (sizeof (uuid) == 16);

  std::ostream & operator<< (std::ostream & os, uuid::version_type v);
  std::ostream & operator<< (std::ostream & os, uuid::variant_type v);
  std::ostream & operator<< (std::ostream & os, uuid const & m);

  inline bool operator== (uuid const & lhs, uuid const & rhs) noexcept {
    return lhs.data_ == rhs.data_;
  }
  inline bool operator<(uuid const & lhs, uuid const & rhs) noexcept {
    return lhs.data_ < rhs.data_;
  }
  inline bool operator!= (uuid const & lhs, uuid const & rhs) noexcept {
    return lhs.data_ != rhs.data_;
  }
  inline bool operator> (uuid const & lhs, uuid const & rhs) noexcept {
    return lhs.data_ > rhs.data_;
  }
  inline bool operator<= (uuid const & lhs, uuid const & rhs) noexcept {
    return lhs.data_ <= rhs.data_;
  }
  inline bool operator>= (uuid const & lhs, uuid const & rhs) noexcept {
    return lhs.data_ >= rhs.data_;
  }

} // end namespace pstore

#endif // PSTORE_CORE_UUID_HPP
