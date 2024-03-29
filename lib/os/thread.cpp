//===- lib/os/thread.cpp --------------------------------------------------===//
//*  _   _                        _  *
//* | |_| |__  _ __ ___  __ _  __| | *
//* | __| '_ \| '__/ _ \/ _` |/ _` | *
//* | |_| | | | | |  __/ (_| | (_| | *
//*  \__|_| |_|_|  \___|\__,_|\__,_| *
//*                                  *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file thread.cpp

#include "pstore/os/thread.hpp"

#include <array>

namespace pstore::threads {

  std::string get_name () {
    std::array<char, name_size> buffer;
    return {get_name (gsl::make_span (buffer))};
  }

} // end namespace pstore::threads
