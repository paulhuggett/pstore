//===- include/pstore/command_line/str_to_revision.hpp ----*- mode: C++ -*-===//
//*      _          _                         _     _              *
//*  ___| |_ _ __  | |_ ___    _ __ _____   _(_)___(_) ___  _ __   *
//* / __| __| '__| | __/ _ \  | '__/ _ \ \ / / / __| |/ _ \| '_ \  *
//* \__ \ |_| |    | || (_) | | | |  __/\ V /| \__ \ | (_) | | | | *
//* |___/\__|_|     \__\___/  |_|  \___| \_/ |_|___/_|\___/|_| |_| *
//*                                                                *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file command_line/str_to_revision.hpp
/// \brief Converts a string to a pair containing a revision number and boolean indicating whether
/// the conversion was successful.

#ifndef PSTORE_COMMAND_LINE_STR_TO_REVISION_HPP
#define PSTORE_COMMAND_LINE_STR_TO_REVISION_HPP

#include <optional>
#include <string>
#include <utility>

namespace pstore::command_line {

  /// Converts a string to a revision number. Leading and trailing whitespace is ignored, the text
  /// "head" (regardless of case) will become pstore::database::head_revision.
  ///
  /// \param str  The string to be converted to a revision number.
  /// \returns Either the converted revision number or nothing.

  std::optional<unsigned> str_to_revision (std::string const & str);

} // end namespace pstore::command_line

#endif // PSTORE_COMMAND_LINE_STR_TO_REVISION_HPP
