//===- lib/exchange/import_strings.cpp ------------------------------------===//
//*  _                            _         _        _                  *
//* (_)_ __ ___  _ __   ___  _ __| |_   ___| |_ _ __(_)_ __   __ _ ___  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| / __| __| '__| | '_ \ / _` / __| *
//* | | | | | | | |_) | (_) | |  | |_  \__ \ |_| |  | | | | | (_| \__ \ *
//* |_|_| |_| |_| .__/ \___/|_|   \__| |___/\__|_|  |_|_| |_|\__, |___/ *
//*             |_|                                          |___/      *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file import_names.cpp
/// \brief Implements the class which maps from string indexes to their store address.

#include "pstore/exchange/import_strings.hpp"

namespace pstore::exchange::import_ns {

  // lookup
  // ~~~~~~
  error_or<typed_address<indirect_string>>
  string_mapping::lookup (std::uint64_t const index) const {
    using result_type = error_or<typed_address<indirect_string>>;

    auto const pos = lookup_.find (index);
    return pos != std::end (lookup_) ? result_type{pos->second} : result_type{error::no_such_name};
  }

  // add string
  // ~~~~~~~~~~
  std::error_code string_mapping::add_string (not_null<transaction_base *> const transaction,
                                              peejay::u8string_view str) {
    strings_.push_back (std::string{str});
    std::string const & x = strings_.back ();

    views_.emplace_back (make_sstring_view (x));
    auto & s = views_.back ();

    std::shared_ptr<index::name_index> const names_index =
      index::get_index<trailer::indices::name> (transaction->db ());
    std::pair<index::name_index::iterator, bool> const add_res =
      adder_.add (*transaction, names_index, &s);
    if (!add_res.second) {
      return error::duplicate_name;
    }

    lookup_.emplace (lookup_.size (),
                     typed_address<indirect_string>::make (add_res.first.get_address ()));
    return {};
  }

  // flush
  // ~~~~~
  void string_mapping::flush (not_null<transaction_base *> const transaction) {
    adder_.flush (*transaction);
  }

} // namespace pstore::exchange::import_ns
