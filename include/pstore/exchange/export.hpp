//===- include/pstore/exchange/export.hpp -----------------*- mode: C++ -*-===//
//*                             _    *
//*   _____  ___ __   ___  _ __| |_  *
//*  / _ \ \/ / '_ \ / _ \| '__| __| *
//* |  __/>  <| |_) | (_) | |  | |_  *
//*  \___/_/\_\ .__/ \___/|_|   \__| *
//*           |_|                    *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file export.hpp
/// \brief The top level entry point for exporting a pstore database.

#ifndef PSTORE_EXCHANGE_EXPORT_HPP
#define PSTORE_EXCHANGE_EXPORT_HPP

#include "pstore/exchange/export_ostream.hpp"

namespace pstore::exchange::export_ns {

  // Note that I'd really like to call this namespace "export", but this is a keyword
  // in C++ meaning that we're not allowed to do so. The "_ns" suffix is an abbreviation of
  // "namespace" to work around this restriction.
  void emit_database (database & db, ostream & os, bool comments);

} // end namespace pstore::exchange::export_ns

#endif // PSTORE_EXCHANGE_EXPORT_HPP
