//===- include/pstore/support/utf.hpp ---------------------*- mode: C++ -*-===//
//*        _    __  *
//*  _   _| |_ / _| *
//* | | | | __| |_  *
//* | |_| | |_|  _| *
//*  \__,_|\__|_|   *
//*                 *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file support/utf.hpp
/// \brief Functionality for processing UTF-8 strings. On Windows, provides an
/// additional set of functions to convert UTF-8 strings to and from UTF-16.

#ifndef PSTORE_SUPPORT_UTF_HPP
#define PSTORE_SUPPORT_UTF_HPP

#include <algorithm>
#include <cstddef>
#include <iosfwd>
#include <string_view>

#include "pstore/support/gsl.hpp"
#include "pstore/support/maybe.hpp"

#if defined(_WIN32)

#  include <tchar.h>

namespace pstore {
  namespace utf {
    namespace win32 {

      /// Converts an array of UTF-16 encoded wchar_t to UTF-8.
      /// \param wstr    The buffer start address.
      /// \param length  The number of bytes in the buffer.
      /// \return A UTF-8 encoded string equivalent to the string given by the buffer starting
      ///  at wstr.
      std::string to8 (wchar_t const * const wstr, std::size_t length);

      /// Converts a NUL-terminated array of wchar_t from UTF-16 to UTF-8.
      std::string to8 (wchar_t const * const wstr);

      /// Converts a UTF-16 encoded wstring to UTF-8.
      inline std::string to8 (std::wstring const & wstr) {
        return to8 (wstr.data (), wstr.length ());
      }

      /// Converts an array of UTF-8 encoded chars to UTF-16.
      /// \param str     The buffer start address.
      /// \param length  The number of bytes in the buffer.
      /// \return A UTF-16 encoded string equivalent to the string given by the buffer
      /// starting at str.
      std::wstring to16 (char const * str, std::size_t length);

      /// Converts a null-terminated string of UTF-8 encoded chars to UTF-16.
      std::wstring to16 (char const * str);

      inline std::wstring to16 (std::string const & str) {
        return to16 (str.data (), str.length ());
      }

      ///@{
      /// Converts a UTF-8 encoded char sequence to a "multi-byte" string using
      /// the system default Windows ANSI code page.
      /// \param length The number of bytes in the 'str' buffer.
      std::string to_mbcs (char const * str, std::size_t length);
      inline std::string to_mbcs (std::string_view str) {
        return to_mbcs (str.data (), str.length ());
      }
      std::string to_mbcs (wchar_t const * wstr, std::size_t length);
      inline std::string to_mbcs (std::wstring const & str) {
        return to_mbcs (str.data (), str.length ());
      }
      ///@}

      ///@{
      /// Converts a Windows "multi-byte" character string using the system default
      /// ANSI code page to UTF-8. Note that the system code page can vary from one
      /// machine to another, so such strings should only ever be temporary.

      /// \param mbcs A buffer containing an array of "multi-byte" characters to be converted
      ///             to UTF-8. If must contain the number of bytes given by the 'length'
      ///             parameter.
      /// \param length The number of bytes in the 'mbcs' buffer.
      std::string mbcs_to8 (char const * mbcs, std::size_t length);

      /// \param str A null-terminating string of "multi-byte" characters to be converted
      ///            to UTF-8.
      inline std::string mbcs_to8 (char const * str) {
        PSTORE_ASSERT (str != nullptr);
        return mbcs_to8 (str, std::strlen (str));
      }

      /// \param str  A string of "multi-byte" characters to be converted to UTF-8.
      inline std::string mbcs_to8 (std::string const & str) {
        return mbcs_to8 (str.data (), str.length ());
      }
      ///@}
    } // end namespace win32
  }   // end namespace utf
} // end namespace pstore

#else //_WIN32

// The type follows the Microsoft convention for its naming.
// NOLINTNEXTLINE(readability-identifier-naming)
using TCHAR = char;

#endif // !defined(_WIN32)


namespace pstore {
  namespace utf {

    using utf8_string = std::basic_string<std::uint8_t>;
    using utf16_string = std::basic_string<char16_t>;

    auto operator<< (std::ostream & os, utf8_string const & s) -> std::ostream &;

  } // end namespace utf
} // end namespace pstore



namespace pstore {
  namespace utf {

    /// If the top two bits are 0b10, then this is a UTF-8 continuation byte
    /// and is skipped; other patterns in these top two bits represent the
    /// start of a character.
    template <typename CharType>
    constexpr auto is_utf_char_start (CharType c) noexcept -> bool {
      using uchar_type = typename std::make_unsigned_t<CharType>;
      return (static_cast<uchar_type> (c) & 0xC0U) != 0x80U;
    }

    ///@{
    /// Returns the number of UTF-8 code-points in a sequence.

    template <typename Iterator>
    constexpr auto length (Iterator first, Iterator last) -> std::size_t {
      auto const result =
        std::count_if (first, last, [] (char const c) { return is_utf_char_start (c); });
      PSTORE_ASSERT (result >= 0);
      using utype = typename std::make_unsigned_t<decltype (result)>;
      static_assert (std::numeric_limits<utype>::max () <= std::numeric_limits<std::size_t>::max (),
                     "std::size_t cannot hold result of count_if");
      return static_cast<std::size_t> (result);
    }

    template <typename ElementType, std::ptrdiff_t Extent>
    constexpr auto length (gsl::span<ElementType, Extent> span) -> std::size_t {
      return length (span.begin (), span.end ());
    }

    /// Returns the number of UTF-8 code points in the buffer given by \p str.
    ///
    /// \param str  The string buffer.
    /// \return The number of UTF-8 code points in the buffer.
    constexpr auto length (std::string_view str) -> std::size_t {
      return length (std::begin (str), std::end (str));
    }

    constexpr auto length (std::nullptr_t) noexcept -> std::size_t {
      return 0;
    }
    ///@}

    using native_string = std::basic_string<TCHAR>;
    using native_ostringstream = std::basic_ostringstream<TCHAR>;

#if defined(_WIN32)
#  if defined(_UNICODE)
    inline auto to_native_string (std::string const & str) -> std::wstring {
      return utf::win32::to16 (str);
    }
    inline auto to_native_string (gsl::czstring const str) -> std::wstring {
      return utf::win32::to16 (str);
    }
    inline auto from_native_string (std::wstring const & str) -> std::string {
      return utf::win32::to8 (str);
    }
    inline auto from_native_string (gsl::cwzstring const str) -> std::string {
      return utf::win32::to8 (str);
    }
#  else
    // This is Windows in "Multibyte character set" mode.
    inline auto to_native_string (std::string const & str) -> std::string {
      return win32::to_mbcs (str);
    }
    inline auto to_native_string (gsl::czstring const str) -> std::string {
      return win32::to_mbcs (str);
    }
#  endif //_UNICODE
    inline auto from_native_string (std::string const & str) -> std::string {
      return win32::mbcs_to8 (str);
    }
    inline auto from_native_string (gsl::czstring const str) -> std::string {
      return win32::mbcs_to8 (str);
    }
#else //_WIN32
    constexpr auto to_native_string (std::string const & str) noexcept -> std::string const & {
      return str;
    }
    constexpr auto from_native_string (std::string const & str) noexcept -> std::string const & {
      return str;
    }
#endif
  } // end namespace utf
} // end namespace pstore

#endif // PSTORE_SUPPORT_UTF_HPP
