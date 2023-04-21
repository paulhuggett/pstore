//===- lib/core/index_types.cpp -------------------------------------------===//
//*  _           _             _                          *
//* (_)_ __   __| | _____  __ | |_ _   _ _ __   ___  ___  *
//* | | '_ \ / _` |/ _ \ \/ / | __| | | | '_ \ / _ \/ __| *
//* | | | | | (_| |  __/>  <  | |_| |_| | |_) |  __/\__ \ *
//* |_|_| |_|\__,_|\___/_/\_\  \__|\__, | .__/ \___||___/ *
//*                                |___/|_|               *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "pstore/core/index_types.hpp"

#include "pstore/core/hamt_set.hpp"

namespace {

  constexpr auto index_integral (pstore::trailer::indices const idx) noexcept
    -> std::underlying_type_t<pstore::trailer::indices> {
    return static_cast<std::underlying_type_t<pstore::trailer::indices>> (idx);
  }

  template <pstore::trailer::indices Index>
  void flush_index (pstore::transaction_base & transaction,
                    pstore::trailer::index_records_array * const locations,
                    unsigned const generation) {
    pstore::database & db = transaction.db ();
    if (auto const index = pstore::index::get_index<Index> (db, false /*create*/)) {
      (*locations)[index_integral (Index)] = index->flush (transaction, generation);
    }
  }

} // end anonymous namespace

namespace pstore::index {

  // flush indices
  // ~~~~~~~~~~~~~
  void flush_indices (transaction_base & transaction,
                      trailer::index_records_array * const locations, unsigned const generation) {
#define X(k)                                                                                       \
  case trailer::indices::k:                                                                        \
    flush_index<trailer::indices::k> (transaction, locations, generation);                         \
    break;

    for (auto ctr = std::underlying_type_t<trailer::indices>{0};
         ctr <= index_integral (trailer::indices::last); ++ctr) {
      switch (static_cast<trailer::indices> (ctr)) {
        PSTORE_INDICES
      case trailer::indices::last: break;
      }
    }
#undef X
    PSTORE_ASSERT (locations->size () == index_integral (trailer::indices::last));
  }

} // end namespace pstore::index
