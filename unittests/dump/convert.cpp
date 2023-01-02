//===- unittests/dump/convert.cpp -----------------------------------------===//
//*                                _    *
//*   ___ ___  _ ____   _____ _ __| |_  *
//*  / __/ _ \| '_ \ \ / / _ \ '__| __| *
//* | (_| (_) | | | \ V /  __/ |  | |_  *
//*  \___\___/|_| |_|\_/ \___|_|   \__| *
//*                                     *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "convert.hpp"

#include <vector>

template <>
std::basic_string<char> convert (char const * str) {
  return {str};
}
template <>
std::basic_string<wchar_t> convert (char const * str) {
  std::vector<wchar_t> result (std::strlen (str) + 1, L'\0');
  for (;;) {
    auto const bytes_available = result.size ();
    auto const bytes_written = std::mbstowcs (&result[0], str, bytes_available);
    if (bytes_written < bytes_available) {
      result.resize (bytes_written);
      break;
    }
    result.resize (bytes_available + bytes_available / 2U, L'\0');
  }

  return {std::begin (result), std::end (result)};
}
