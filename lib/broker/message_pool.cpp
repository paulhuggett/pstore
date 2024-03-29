//===- lib/broker/message_pool.cpp ----------------------------------------===//
//*                                                               _  *
//*  _ __ ___   ___  ___ ___  __ _  __ _  ___   _ __   ___   ___ | | *
//* | '_ ` _ \ / _ \/ __/ __|/ _` |/ _` |/ _ \ | '_ \ / _ \ / _ \| | *
//* | | | | | |  __/\__ \__ \ (_| | (_| |  __/ | |_) | (_) | (_) | | *
//* |_| |_| |_|\___||___/___/\__,_|\__, |\___| | .__/ \___/ \___/|_| *
//*                                |___/       |_|                   *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file message_pool.cpp

#include "pstore/broker/message_pool.hpp"

namespace pstore::broker {

  message_pool pool;

} // namespace pstore::broker
