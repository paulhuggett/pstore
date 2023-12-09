//===- include/pstore/command_line/literal.hpp ------------*- mode: C++ -*-===//
//*  _ _ _                 _  *
//* | (_) |_ ___ _ __ __ _| | *
//* | | | __/ _ \ '__/ _` | | *
//* | | | ||  __/ | | (_| | | *
//* |_|_|\__\___|_|  \__,_|_| *
//*                           *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_COMMAND_LINE_LITERAL_HPP
#define PSTORE_COMMAND_LINE_LITERAL_HPP

#include <string>

namespace pstore::command_line {

  // This represents a single enum value, using "int" as the underlying type.
  struct literal {
    literal () = default;
    literal (std::string const & n, int const v, std::string const & d)
            : name{n}
            , value{v}
            , description{d} {}
    literal (std::string const & n, int const v)
            : literal (n, v, n) {}
    std::string name;
    int value = 0;
    std::string description;
  };

} // end namespace pstore::command_line

#endif // PSTORE_COMMAND_LINE_LITERAL_HPP
