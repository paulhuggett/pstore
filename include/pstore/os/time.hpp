//===- include/pstore/os/time.hpp -------------------------*- mode: C++ -*-===//
//*  _   _                 *
//* | |_(_)_ __ ___   ___  *
//* | __| | '_ ` _ \ / _ \ *
//* | |_| | | | | | |  __/ *
//*  \__|_|_| |_| |_|\___| *
//*                        *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef PSTORE_OS_TIME_HPP
#define PSTORE_OS_TIME_HPP

#include <ctime>

namespace pstore {

  struct std::tm local_time (std::time_t clock);
  struct std::tm gm_time (std::time_t clock);

} // end namespace pstore

#endif // PSTORE_OS_TIME_HPP
