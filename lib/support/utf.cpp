//===- lib/support/utf.cpp ------------------------------------------------===//
//*        _    __  *
//*  _   _| |_ / _| *
//* | | | | __| |_  *
//* | |_| | |_|  _| *
//*  \__,_|\__|_|   *
//*                 *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file support/utf.cpp
/// \brief Implementation of functions for processing UTF-8 strings.

#include "pstore/support/utf.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <type_traits>

#include "pstore/support/assert.hpp"
