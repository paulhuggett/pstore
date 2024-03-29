//===- include/pstore/exchange/export_paths.hpp -----------*- mode: C++ -*-===//
//*                             _                 _   _          *
//*   _____  ___ __   ___  _ __| |_   _ __   __ _| |_| |__  ___  *
//*  / _ \ \/ / '_ \ / _ \| '__| __| | '_ \ / _` | __| '_ \/ __| *
//* |  __/>  <| |_) | (_) | |  | |_  | |_) | (_| | |_| | | \__ \ *
//*  \___/_/\_\ .__/ \___/|_|   \__| | .__/ \__,_|\__|_| |_|___/ *
//*           |_|                    |_|                         *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file export_paths.hpp
/// \brief  Exporting the members of the paths index.

#ifndef PSTORE_EXCHANGE_EXPORT_PATHS_HPP
#define PSTORE_EXCHANGE_EXPORT_PATHS_HPP

#include "pstore/exchange/export_strings.hpp"

namespace pstore::exchange::export_ns {

  void emit_paths (ostream_base & os, indent ind, database const & db, unsigned generation,
                   gsl::not_null<string_mapping *> string_table);

} // namespace pstore::exchange::export_ns

#endif // PSTORE_EXCHANGE_EXPORT_PATHS_HPP
