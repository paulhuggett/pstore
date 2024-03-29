//===- unittests/dump/test_object.cpp -------------------------------------===//
//*        _     _           _    *
//*   ___ | |__ (_) ___  ___| |_  *
//*  / _ \| '_ \| |/ _ \/ __| __| *
//* | (_) | |_) | |  __/ (__| |_  *
//*  \___/|_.__// |\___|\___|\__| *
//*           |__/                *
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
#include <sstream>

// 3rd party includes
#include <gmock/gmock.h>

// Local includes
#include "split.hpp"
#include "convert.hpp"

namespace {

  template <typename CharType>
  class Object : public testing::Test {
  protected:
    std::basic_ostringstream<CharType> out_;
  };

  using CharacterTypes = ::testing::Types<char, wchar_t>;

} // end anonymous namespace

#ifdef PSTORE_IS_INSIDE_LLVM
TYPED_TEST_CASE (Object, CharacterTypes);
#else
TYPED_TEST_SUITE (Object, CharacterTypes, );
#endif // PSTORE_IS_INSIDE_LLVM

TYPED_TEST (Object, Empty) {
  pstore::dump::object v;
  v.write (this->out_);
  EXPECT_EQ (this->out_.str (), convert<TypeParam> ("{ }"));
}

TYPED_TEST (Object, SingleNumber) {
  using testing::ElementsAre;
  using namespace pstore::dump;

  object v{object::container{{"key", make_number (42)}}};
  v.write (this->out_);
  EXPECT_THAT (split_tokens (this->out_.str ()),
               ElementsAre (convert<TypeParam> ("key"), convert<TypeParam> (":"),
                            convert<TypeParam> ("0x2a")));
}

TYPED_TEST (Object, TwoNumbers) {
  using testing::ElementsAre;
  using namespace pstore::dump;
  object v{object::container{
    {"k1", make_number (42)},
    {"k2", make_number (43)},
  }};
  v.write (this->out_);

  auto const lines = split_lines (this->out_.str ());
  ASSERT_EQ (2U, lines.size ());
  EXPECT_THAT (
    split_tokens (lines.at (0)),
    ElementsAre (convert<TypeParam> ("k1"), convert<TypeParam> (":"), convert<TypeParam> ("0x2a")));
  EXPECT_THAT (
    split_tokens (lines.at (1)),
    ElementsAre (convert<TypeParam> ("k2"), convert<TypeParam> (":"), convert<TypeParam> ("0x2b")));
}

TYPED_TEST (Object, KeyWithColon) {
  using namespace ::pstore::dump;
  object v{object::container{
    {"k1:k2", make_number (42)},
  }};
  v.write (this->out_);

  EXPECT_THAT (split_tokens (this->out_.str ()),
               ::testing::ElementsAre (convert<TypeParam> ("k1:k2"), convert<TypeParam> (":"),
                                       convert<TypeParam> ("0x2a")));
}

TYPED_TEST (Object, KeyWithColonSpace) {
  using namespace ::pstore::dump;
  object v{{
    {"k1: k2", make_number (42)},
  }};
  v.write (this->out_);
  EXPECT_EQ (this->out_.str (), convert<TypeParam> ("\"k1: k2\" : 0x2a"));
}

TYPED_TEST (Object, KeyNeedingQuoting) {
  using namespace ::pstore::dump;
  object v{{
    {"  k1", make_number (42)},
  }};
  v.write (this->out_);
  EXPECT_EQ (this->out_.str (), convert<TypeParam> ("\"  k1\" : 0x2a"));
}

TYPED_TEST (Object, ValueAlignment) {
  using namespace ::pstore::dump;
  object v{{
    {"short", make_number (42)},
    {"much_longer", make_number (43)},
  }};
  v.write (this->out_);
  auto const actual = this->out_.str ();
  auto const expected = convert<TypeParam> ("short       : 0x2a\n"
                                            "much_longer : 0x2b");
  EXPECT_EQ (expected, actual);
}

TYPED_TEST (Object, Nested) {
  using namespace ::pstore::dump;
  object v{{
    {"k1", make_value (std::string ("value1"))},
    {"k2", make_value (object::container{
             {"ik1", make_value ("iv1")},
             {"ik2", make_value ("iv2")},
           })},
  }};
  v.write (this->out_);
  auto const actual = this->out_.str ();
  auto const expected = convert<TypeParam> ("k1 : value1\n"
                                            "k2 : \n"
                                            "    ik1 : iv1\n"
                                            "    ik2 : iv2");
  EXPECT_EQ (expected, actual);
}

TEST (Object, GetFound) {
  using namespace ::pstore::dump;
  auto v = make_value ("Hello World");
  object object{{{"key", v}}};
  EXPECT_EQ (v, object.get ("key"));
}

TEST (Object, GetNotFound) {
  using namespace ::pstore::dump;
  auto v = make_value ("Hello World");
  object object{{{"key", v}}};
  EXPECT_EQ (std::shared_ptr<value> (nullptr), object.get ("missing"));
}

TEST (Object, BackInserter) {
  using namespace ::pstore::dump;
  object object{{{"k1", make_value ("v1")}}};
}
