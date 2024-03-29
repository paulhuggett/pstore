//===- lib/adt/sstring_view.cpp -------------------------------------------===//
//*          _        _                     _                *
//*  ___ ___| |_ _ __(_)_ __   __ _  __   _(_) _____      __ *
//* / __/ __| __| '__| | '_ \ / _` | \ \ / / |/ _ \ \ /\ / / *
//* \__ \__ \ |_| |  | | | | | (_| |  \ V /| |  __/\ V  V /  *
//* |___/___/\__|_|  |_|_| |_|\__, |   \_/ |_|\___| \_/\_/   *
//*                           |___/                          *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/adt/sstring_view.hpp"

#include <algorithm>

namespace pstore {

  sstring_view<std::shared_ptr<char const>> make_shared_sstring_view (std::string_view str) {
    auto const length = str.length ();
    auto const ptr =
      std::shared_ptr<char> (new char[length], [] (gsl::czstring const p) { delete[] p; });
    std::copy (std::begin (str), std::end (str), ptr.get ());
    return {std::static_pointer_cast<char const> (ptr), length};
  }

} // end namespace pstore
