//===- lib/command_line/tchar.cpp -----------------------------------------===//
//*  _       _                 *
//* | |_ ___| |__   __ _ _ __  *
//* | __/ __| '_ \ / _` | '__| *
//* | || (__| | | | (_| | |    *
//*  \__\___|_| |_|\__,_|_|    *
//*                            *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/command_line/tchar.hpp"

namespace pstore::command_line {

#if defined(_WIN32) && defined(_UNICODE)
  std::wostream & error_stream = std::wcerr;
  std::wostream & out_stream = std::wcout;
#else
  std::ostream & error_stream = std::cerr;
  std::ostream & out_stream = std::cout;
#endif

} // end namespace pstore::command_line
