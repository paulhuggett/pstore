//===- unittests/exchange/json_error.hpp ------------------*- mode: C++ -*-===//
//*    _                                             *
//*   (_)___  ___  _ __     ___ _ __ _ __ ___  _ __  *
//*   | / __|/ _ \| '_ \   / _ \ '__| '__/ _ \| '__| *
//*   | \__ \ (_) | | | | |  __/ |  | | | (_) | |    *
//*  _/ |___/\___/|_| |_|  \___|_|  |_|  \___/|_|    *
//* |__/                                             *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_UNITTESTS_EXCHANGE_JSON_ERROR_HPP
#define PSTORE_UNITTESTS_EXCHANGE_JSON_ERROR_HPP

#include <ostream>

#include "peejay/json.hpp"

template <typename Parser>
class json_error {
public:
  json_error (Parser const & p)
          : parser_{p} {}
  std::ostream & write (std::ostream & os) const {
    auto const pos = parser_.input_pos ();
    using namespace peejay;
    return os << "JSON error was: " << parser_.last_error ().message () << ' '
              << static_cast<unsigned> (line{pos}) << ':' << static_cast<unsigned> (column{pos})
              << '\n';
  }

private:
  Parser const & parser_;
};

template <typename Parser>
json_error (Parser &) -> json_error<Parser>;

template <typename Parser>
inline std::ostream & operator<< (std::ostream & os, json_error<Parser> const & je) {
  return je.write (os);
}

#endif // PSTORE_UNITTESTS_EXCHANGE_JSON_ERROR_HPP
