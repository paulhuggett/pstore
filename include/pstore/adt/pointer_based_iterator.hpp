//===- include/pstore/adt/pointer_based_iterator.hpp ------*- mode: C++ -*-===//
//*              _       _              _                        _  *
//*  _ __   ___ (_)_ __ | |_ ___ _ __  | |__   __ _ ___  ___  __| | *
//* | '_ \ / _ \| | '_ \| __/ _ \ '__| | '_ \ / _` / __|/ _ \/ _` | *
//* | |_) | (_) | | | | | ||  __/ |    | |_) | (_| \__ \  __/ (_| | *
//* | .__/ \___/|_|_| |_|\__\___|_|    |_.__/ \__,_|___/\___|\__,_| *
//* |_|                                                             *
//*  _ _                 _              *
//* (_) |_ ___ _ __ __ _| |_ ___  _ __  *
//* | | __/ _ \ '__/ _` | __/ _ \| '__| *
//* | | ||  __/ | | (_| | || (_) | |    *
//* |_|\__\___|_|  \__,_|\__\___/|_|    *
//*                                     *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file pointer_based_iterator.hpp
/// \brief Provides pointer_based_iterator<> an iterator wrapper for pointers.

#ifndef PSTORE_ADT_POINTER_BASED_ITERATOR_HPP
#define PSTORE_ADT_POINTER_BASED_ITERATOR_HPP

#include <cstddef>
#include <iterator>
#include <type_traits>

#include "peejay/pointer_based_iterator.hpp"

namespace pstore {

  using peejay::pointer_based_iterator;

} // end namespace pstore

#endif // PSTORE_ADT_POINTER_BASED_ITERATOR_HPP
