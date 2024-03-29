//===- unittests/dump/test_array.cpp --------------------------------------===//
//*                              *
//*   __ _ _ __ _ __ __ _ _   _  *
//*  / _` | '__| '__/ _` | | | | *
//* | (_| | |  | | | (_| | |_| | *
//*  \__,_|_|  |_|  \__,_|\__, | *
//*                       |___/  *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/dump/value.hpp"

// Standard library includes
#include <memory>

// 3rd party includes
#include <gtest/gtest.h>

// Local includes
#include "convert.hpp"

namespace {

  template <typename CharType>
  class Array : public testing::Test {
  protected:
    std::basic_ostringstream<CharType> out_;
  };

  using CharacterTypes = testing::Types<char, wchar_t>;

} // end anonymous namespace

#ifdef PSTORE_IS_INSIDE_LLVM
TYPED_TEST_CASE (Array, CharacterTypes);
#else
TYPED_TEST_SUITE (Array, CharacterTypes, );
#endif


TYPED_TEST (Array, Empty) {
  pstore::dump::array arr;
  arr.write (this->out_);
  EXPECT_EQ (convert<TypeParam> ("[ ]"), this->out_.str ());
}

TYPED_TEST (Array, TwoNumbers) {
  using namespace ::pstore::dump;
  array arr ({make_number (3), make_number (5)});
  arr.write (this->out_);
  EXPECT_EQ (convert<TypeParam> ("[ 0x3, 0x5 ]"), this->out_.str ());
}

TYPED_TEST (Array, TwoStrings) {
  using namespace ::pstore::dump;
  array arr ({
    make_value ("Hello"),
    make_value ("World"),
  });
  arr.write (this->out_);
  auto const expected = convert<TypeParam> ("\n"
                                            "- Hello\n"
                                            "- World");
  EXPECT_EQ (expected, this->out_.str ());
}
