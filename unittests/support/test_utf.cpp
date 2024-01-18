//===- unittests/support/test_utf.cpp -------------------------------------===//
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

#include "pstore/support/utf.hpp"

// Standard library includes
#include <cstddef>
#include <cstdint>
#include <limits>

// 3rd party includes
#include <gmock/gmock.h>

TEST (Utf, LengthOfEmptyNulTerminatedString) {
  ASSERT_EQ (0U, pstore::utf::length (""));
}

namespace {

  struct SimpleAsciiFixture : testing::Test {
    static std::string const str;
  };

  std::string const SimpleAsciiFixture::str{"hello mum"};

} // end anonymous namespace

TEST_F (SimpleAsciiFixture, LengthWithExplicitSize) {
  ASSERT_EQ (9U, pstore::utf::length (str));
}

namespace {

  struct ShortJapaneseStringFixture : testing::Test {
    static std::uint8_t const bytes[];
    static std::string const str;
  };

  std::uint8_t const ShortJapaneseStringFixture::bytes[] = {
    0xE3, 0x81, 0x8A, // HIRAGANA LETTER O
    0xE3, 0x81, 0xAF, // HIRAGANA LETTER HA
    0xE3, 0x82, 0x88, // HIRAGANA LETTER YO
    0xE3, 0x81, 0x86, // HIRAGANA LETTER U
    0xE3, 0x81, 0x94, // HIRAGANA LETTER GO
    0xE3, 0x81, 0x96, // HIRAGANA LETTER ZA
    0xE3, 0x81, 0x84, // HIRAGANA LETTER I
    0xE3, 0x81, 0xBE, // HIRAGANA LETTER MA
    0xE3, 0x81, 0x99, // HIRAGANA LETTER SU
    0x00,             // NULL
  };
  std::string const ShortJapaneseStringFixture::str{reinterpret_cast<char const *> (bytes)};

} // end anonymous namespace

TEST_F (ShortJapaneseStringFixture, Length) {
  ASSERT_EQ (9U, pstore::utf::length (str));
}


namespace {

  struct FourByteUtf8ChineseCharacters : testing::Test {
    static std::uint8_t const bytes[];
    static std::string const str;
  };

  std::uint8_t const FourByteUtf8ChineseCharacters::bytes[] = {
    0xF0, 0xA0, 0x9C, 0x8E, // CJK UNIFIED IDEOGRAPH-2070E
    0xF0, 0xA0, 0x9C, 0xB1, // CJK UNIFIED IDEOGRAPH-20731
    0xF0, 0xA0, 0x9D, 0xB9, // CJK UNIFIED IDEOGRAPH-20779
    0xF0, 0xA0, 0xB1, 0x93, // CJK UNIFIED IDEOGRAPH-20C53
    0x00,                   // NULL
  };
  std::string const FourByteUtf8ChineseCharacters::str{reinterpret_cast<char const *> (bytes)};

} // end anonymous namespace

TEST_F (FourByteUtf8ChineseCharacters, Length) {
  ASSERT_EQ (4U, pstore::utf::length (str));
}


namespace {
  // Test the highest Unicode value that is still resulting in an
  // overlong sequence if represented with the given number of bytes. This
  // is a boundary test for safe UTF-8 decoders. All four characters should
  // be rejected like malformed UTF-8 sequences.
  // Since IETF RFC 3629 modified the UTF-8 definition, any encodings beyond
  // 4 bytes are now illegal
  // (see http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html)

  struct MaxLengthUTFSequence : testing::Test {
    static std::uint8_t const bytes[];
    static char const * str;
  };

  std::uint8_t const MaxLengthUTFSequence::bytes[] = {
    0x7F,                   // U+007F DELETE
    0xDF, 0xBF,             // U+07FF
    0xEF, 0xBF, 0xBF,       // U+FFFF
    0xF4, 0x8F, 0xBF, 0xBF, // U+10FFFF
    0x00,                   // U+0000 NULL
  };

  char const * MaxLengthUTFSequence::str = reinterpret_cast<char const *> (bytes);
} // namespace

TEST_F (MaxLengthUTFSequence, Length) {
  ASSERT_EQ (4U, pstore::utf::length (str));
}
TEST_F (MaxLengthUTFSequence, LengthWithNulTerminatedString) {
  ASSERT_EQ (4U, pstore::utf::length (str));
}
