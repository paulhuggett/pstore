//===- unittests/command_line/test_str_to_revision.cpp --------------------===//
//*      _          _                         _     _              *
//*  ___| |_ _ __  | |_ ___    _ __ _____   _(_)___(_) ___  _ __   *
//* / __| __| '__| | __/ _ \  | '__/ _ \ \ / / / __| |/ _ \| '_ \  *
//* \__ \ |_| |    | || (_) | | | |  __/\ V /| \__ \ | (_) | | | | *
//* |___/\__|_|     \__\___/  |_|  \___| \_/ |_|___/_|\___/|_| |_| *
//*                                                                *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "pstore/command_line/str_to_revision.hpp"

// Standard Library
#include <utility>

// 3rd party
#include <gmock/gmock.h>

// pstore
#include "pstore/support/head_revision.hpp"

using pstore::command_line::str_to_revision;

TEST (StrToRevision, SingleCharacterNumber) {
  EXPECT_EQ (str_to_revision ("1"), std::optional<unsigned>{1U});
}

TEST (StrToRevision, MultiCharacterNumber) {
  EXPECT_EQ (str_to_revision ("200000"), std::optional<unsigned>{200000U});
}

TEST (StrToRevision, NumberLeadingWS) {
  EXPECT_EQ (str_to_revision ("    200000"), std::optional<unsigned>{200000U});
}

TEST (StrToRevision, NumberTrailingWS) {
  EXPECT_EQ (str_to_revision ("12345   "), std::optional<unsigned>{12345U});
}

TEST (StrToRevision, Empty) {
  EXPECT_EQ (str_to_revision (""), std::optional<unsigned>{});
}

TEST (StrToRevision, JustWhitespace) {
  EXPECT_EQ (str_to_revision ("  \t"), std::optional<unsigned>{});
}

TEST (StrToRevision, Zero) {
  EXPECT_EQ (str_to_revision ("0"), std::optional<unsigned>{0U});
}

TEST (StrToRevision, HeadLowerCase) {
  EXPECT_EQ (str_to_revision ("head"), std::optional<unsigned>{pstore::head_revision});
}

TEST (StrToRevision, HeadMixedCase) {
  EXPECT_EQ (str_to_revision ("HeAd"), std::optional<unsigned>{pstore::head_revision});
}

TEST (StrToRevision, HeadLeadingWhitespace) {
  EXPECT_EQ (str_to_revision ("  HEAD"), std::optional<unsigned>{pstore::head_revision});
}

TEST (StrToRevision, HeadTraingWhitespace) {
  EXPECT_EQ (str_to_revision ("HEAD  "), std::optional<unsigned>{pstore::head_revision});
}

TEST (StrToRevision, BadString) {
  EXPECT_EQ (str_to_revision ("bad"), std::optional<unsigned>{});
}

TEST (StrToRevision, NumberFollowedByString) {
  EXPECT_EQ (str_to_revision ("123Bad"), std::optional<unsigned>{});
}

TEST (StrToRevision, PositiveOverflow) {
  std::ostringstream str;
  str << std::numeric_limits<unsigned>::max () + 1ULL;
  EXPECT_EQ (str_to_revision (str.str ()), std::optional<unsigned>{});
}

TEST (StrToRevision, Negative) {
  EXPECT_EQ (str_to_revision ("-2"), std::optional<unsigned>{});
}

TEST (StrToRevision, Hex) {
  EXPECT_EQ (str_to_revision ("0x23"), std::optional<unsigned>{});
}
