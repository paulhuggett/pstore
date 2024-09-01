//===- unittests/exchange/test_root.cpp -----------------------------------===//
//*                  _    *
//*  _ __ ___   ___ | |_  *
//* | '__/ _ \ / _ \| __| *
//* | | | (_) | (_) | |_  *
//* |_|  \___/ \___/ \__| *
//*                       *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/exchange/import_root.hpp"

// 3rd party includes
#include <gtest/gtest.h>
#include "peejay/json.hpp"

// pstore includes
#include "pstore/core/database.hpp"
#include "pstore/exchange/import_root.hpp"

// Local includes
#include "empty_store.hpp"
#include "json_error.hpp"

using namespace std::string_view_literals;

namespace {

  class ExchangeRoot : public testing::Test {
  public:
    ExchangeRoot ()
            : import_db_{import_store_.file ()} {
      import_db_.set_vacuum_mode (pstore::database::vacuum_mode::disabled);
    }

    in_memory_store import_store_;
    pstore::database import_db_;
  };

} // end anonymous namespace

TEST_F (ExchangeRoot, ImportId) {
  using namespace pstore::exchange;

  static constexpr auto json =
    R"({ "version":1, "id":"7a73d64e-5873-439c-ac8f-2b3a68aebe53", "transactions":[] })"sv;

  peejay::parser<import_ns::callbacks> parser = import_ns::create_parser (import_db_);
  auto first = reinterpret_cast<std::byte const *> (json.data ());
  parser.input (first, first + json.length ()).eof ();
  ASSERT_FALSE (parser.has_error ()) << json_error (parser) << json;

  EXPECT_EQ (import_db_.get_header ().id (), pstore::uuid{"7a73d64e-5873-439c-ac8f-2b3a68aebe53"})
    << "The file UUID was not preserved by import";
  EXPECT_TRUE (import_db_.get_header ().is_valid ()) << "The file header was not valid";
}
