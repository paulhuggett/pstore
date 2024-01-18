//===- include/pstore/command_line/type_description.hpp ---*- mode: C++ -*-===//
//*  _                          _                     _       _   _              *
//* | |_ _   _ _ __   ___    __| | ___  ___  ___ _ __(_)_ __ | |_(_) ___  _ __   *
//* | __| | | | '_ \ / _ \  / _` |/ _ \/ __|/ __| '__| | '_ \| __| |/ _ \| '_ \  *
//* | |_| |_| | |_) |  __/ | (_| |  __/\__ \ (__| |  | | |_) | |_| | (_) | | | | *
//*  \__|\__, | .__/ \___|  \__,_|\___||___/\___|_|  |_| .__/ \__|_|\___/|_| |_| *
//*      |___/|_|                                      |_|                       *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_COMMAND_LINE_TYPE_DESCRIPTION_HPP
#define PSTORE_COMMAND_LINE_TYPE_DESCRIPTION_HPP

#include <string>
#include <type_traits>

namespace pstore::command_line {

  template <typename T, typename = void>
  struct type_description {};
  template <>
  struct type_description<std::string> {
    static inline auto value = "str";
  };
  template <>
  struct type_description<int> {
    static inline auto value = "int";
  };
  template <>
  struct type_description<long> {
    static inline auto value = "int";
  };
  template <>
  struct type_description<long long> {
    static inline auto value = "int";
  };
  template <>
  struct type_description<unsigned short> {
    static inline auto value = "uint";
  };
  template <>
  struct type_description<unsigned int> {
    static inline auto value = "uint";
  };
  template <>
  struct type_description<unsigned long> {
    static inline auto value = "uint";
  };
  template <>
  struct type_description<unsigned long long> {
    static inline auto value = "uint";
  };
  template <typename T>
  struct type_description<T, std::enable_if_t<std::is_enum_v<T>>> {
    static inline auto value = "enum";
  };

} // end namespace pstore::command_line

#endif // PSTORE_COMMAND_LINE_TYPE_DESCRIPTION_HPP
