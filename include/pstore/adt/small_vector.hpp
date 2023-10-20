//===- include/pstore/adt/small_vector.hpp ----------------*- mode: C++ -*-===//
//*                      _ _                  _              *
//*  ___ _ __ ___   __ _| | | __   _____  ___| |_ ___  _ __  *
//* / __| '_ ` _ \ / _` | | | \ \ / / _ \/ __| __/ _ \| '__| *
//* \__ \ | | | | | (_| | | |  \ V /  __/ (__| || (_) | |    *
//* |___/_| |_| |_|\__,_|_|_|   \_/ \___|\___|\__\___/|_|    *
//*                                                          *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file small_vector.hpp
/// \brief Provides a small, normally stack allocated, buffer but which can be
/// resized dynamically when necessary.

#ifndef PSTORE_ADT_SMALL_VECTOR_HPP
#define PSTORE_ADT_SMALL_VECTOR_HPP

#include "peejay/small_vector.hpp"

namespace pstore {

  using peejay::small_vector;

} // end namespace pstore

#endif // PSTORE_ADT_SMALL_VECTOR_HPP
