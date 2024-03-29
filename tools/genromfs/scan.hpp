//===- tools/genromfs/scan.hpp ----------------------------*- mode: C++ -*-===//
//*                       *
//*  ___  ___ __ _ _ __   *
//* / __|/ __/ _` | '_ \  *
//* \__ \ (_| (_| | | | | *
//* |___/\___\__,_|_| |_| *
//*                       *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_GENROMFS_SCAN_HPP
#define PSTORE_GENROMFS_SCAN_HPP

#include <string>

#include "directory_entry.hpp"

unsigned scan (directory_container & directory, std::string const & path, unsigned count);

#endif // PSTORE_GENROMFS_SCAN_HPP
