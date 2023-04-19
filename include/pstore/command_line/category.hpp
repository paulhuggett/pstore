//===- include/pstore/command_line/category.hpp -----------*- mode: C++ -*-===//
//*            _                               *
//*   ___ __ _| |_ ___  __ _  ___  _ __ _   _  *
//*  / __/ _` | __/ _ \/ _` |/ _ \| '__| | | | *
//* | (_| (_| | ||  __/ (_| | (_) | |  | |_| | *
//*  \___\__,_|\__\___|\__, |\___/|_|   \__, | *
//*                    |___/            |___/  *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file category.hpp
/// \brief Defines pstore::command_line::option_category; a means to group switches in command-line
/// help text.

#ifndef PSTORE_COMMAND_LINE_CATEGORY_HPP
#define PSTORE_COMMAND_LINE_CATEGORY_HPP

#include <string>

namespace pstore::command_line {

  class option_category {
  public:
    explicit option_category (std::string const & title);
    std::string const & title () const noexcept { return title_; }

  private:
    std::string title_;
  };

} // end namespace pstore::command_line

#endif // PSTORE_COMMAND_LINE_CATEGORY_HPP
