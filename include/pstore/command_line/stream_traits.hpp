//===- include/pstore/command_line/stream_traits.hpp ------*- mode: C++ -*-===//
//*      _                              _             _ _        *
//*  ___| |_ _ __ ___  __ _ _ __ ___   | |_ _ __ __ _(_) |_ ___  *
//* / __| __| '__/ _ \/ _` | '_ ` _ \  | __| '__/ _` | | __/ __| *
//* \__ \ |_| | |  __/ (_| | | | | | | | |_| | | (_| | | |_\__ \ *
//* |___/\__|_|  \___|\__,_|_| |_| |_|  \__|_|  \__,_|_|\__|___/ *
//*                                                              *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_COMMAND_LINE_STREAM_TRAITS_HPP
#define PSTORE_COMMAND_LINE_STREAM_TRAITS_HPP

#include "pstore/support/utf.hpp"

namespace pstore::command_line {

  template <typename T>
  struct stream_trait {};

  template <>
  struct stream_trait<char> {
    static constexpr std::string_view out_string (std::string_view str) noexcept { return str; }
    static constexpr std::string_view out_text (std::string_view str) noexcept { return str; }
  };

#ifdef _WIN32
  template <>
  struct stream_trait<wchar_t> {
    static std::wstring out_string (std::string_view str) { return utf::to_native_string (str); }
    static std::wstring out_text (std::string_view str) { return utf::to_native_string (str); }
  };
#endif // _WIN32

} // end namespace pstore::command_line

#endif // PSTORE_COMMAND_LINE_STREAM_TRAITS_HPP
