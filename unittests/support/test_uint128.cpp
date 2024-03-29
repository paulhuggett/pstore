//===- unittests/support/test_uint128.cpp ---------------------------------===//
//*        _       _   _ ____  ___   *
//*  _   _(_)_ __ | |_/ |___ \( _ )  *
//* | | | | | '_ \| __| | __) / _ \  *
//* | |_| | | | | | |_| |/ __/ (_) | *
//*  \__,_|_|_| |_|\__|_|_____\___/  *
//*                                  *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "pstore/support/uint128.hpp"
#include <array>
#include <gtest/gtest.h>
#include "pstore/support/bit_count.hpp"

namespace {

  class UInt128 : public ::testing::Test {
  protected:
    static constexpr auto max64 = std::numeric_limits<std::uint64_t>::max ();
  };

} // end anonymous namespace

TEST_F (UInt128, DefaultCtor) {
  pstore::uint128 v;
  EXPECT_EQ (v.high (), 0U);
  EXPECT_EQ (v.low (), 0U);
}

TEST_F (UInt128, ExplicitCtor) {
  constexpr auto high = std::uint64_t{7};
  constexpr auto low = std::uint64_t{5};
  {
    pstore::uint128 v1{high, low};
    EXPECT_EQ (v1.high (), high);
    EXPECT_EQ (v1.low (), low);
  }
  {
    pstore::uint128 v2{low};
    EXPECT_EQ (v2.high (), 0U);
    EXPECT_EQ (v2.low (), low);
  }
}

TEST_F (UInt128, Equal) {
  constexpr auto high = std::uint64_t{7};
  constexpr auto low = std::uint64_t{5};

  pstore::uint128 v1{high, low};
  pstore::uint128 v2{high, low};
  pstore::uint128 v3{high, low + 1};
  pstore::uint128 v4{high + 1, low};

  EXPECT_TRUE (v1 == v2);
  EXPECT_FALSE (v1 != v2);
  EXPECT_FALSE (v1 == v3);
  EXPECT_TRUE (v1 != v3);
  EXPECT_FALSE (v1 == v4);
  EXPECT_TRUE (v1 != v4);

  EXPECT_TRUE (pstore::uint128{5U} == 5U);
  EXPECT_TRUE (pstore::uint128{5U} != 6U);
}

TEST_F (UInt128, Gt) {
  EXPECT_TRUE (pstore::uint128 (0, 1) > pstore::uint128 (0, 0));
  EXPECT_TRUE (pstore::uint128 (0, 1) >= pstore::uint128 (0, 0));
  EXPECT_TRUE (pstore::uint128 (2, 1) > pstore::uint128 (1, 2));
  EXPECT_TRUE (pstore::uint128 (2, 1) >= pstore::uint128 (1, 2));
  EXPECT_TRUE (pstore::uint128 (1, 1) >= pstore::uint128 (1, 1));

  EXPECT_TRUE (pstore::uint128{6U} > 5U);
  EXPECT_TRUE (pstore::uint128{6U} >= 6U);
}

TEST_F (UInt128, Lt) {
  EXPECT_TRUE (pstore::uint128 (0, 0) < pstore::uint128 (0, 1));
  EXPECT_TRUE (pstore::uint128 (0, 0) <= pstore::uint128 (0, 1));
  EXPECT_TRUE (pstore::uint128 (1, 2) < pstore::uint128 (2, 1));
  EXPECT_TRUE (pstore::uint128 (1, 2) <= pstore::uint128 (2, 1));
  EXPECT_TRUE (pstore::uint128 (1, 1) <= pstore::uint128 (1, 1));
}

TEST_F (UInt128, UnaryMinus) {
  EXPECT_EQ (-pstore::uint128{0U}, pstore::uint128 (0U));
  EXPECT_EQ (-pstore::uint128{1U}, pstore::uint128 (max64, -std::uint64_t{1}));
  EXPECT_EQ (-pstore::uint128{2U}, pstore::uint128 (max64, -std::uint64_t{2}));
}

TEST_F (UInt128, CompoundAdd) {
  {
    pstore::uint128 a;
    a += pstore::uint128{};
    EXPECT_EQ (a, (pstore::uint128{0U}));
  }
  {
    pstore::uint128 b;
    b += pstore::uint128{1U};
    EXPECT_EQ (b, (pstore::uint128{1U}));
  }
  {
    pstore::uint128 t2a{max64, 0xff184469d7ac50c0};
    t2a += pstore::uint128{0xffffffff90843100U};
    EXPECT_EQ (t2a, pstore::uint128{0xff184469683081c0U});
  }
  {
    pstore::uint128 t3a{0x010000000, 0x00};
    t3a += pstore::uint128{};
    EXPECT_EQ (t3a, (pstore::uint128{0x010000000, 0x00}));
  }
  {
    pstore::uint128 t4l{UINT64_C (0xff00000000000000)};
    pstore::uint128 t4r{UINT64_C (0x0100000000000000)};
    t4l += t4r;
    EXPECT_EQ (t4l, (pstore::uint128{1U, 0U}));
  }
}

TEST_F (UInt128, PreIncrement) {
  {
    pstore::uint128 a{};
    pstore::uint128 const ra = ++a;
    EXPECT_EQ (a, pstore::uint128{1U});
    EXPECT_EQ (ra, pstore::uint128{1U});
  }
  {
    pstore::uint128 b{max64};
    pstore::uint128 const rb = ++b;
    EXPECT_EQ (b, pstore::uint128 (1U, 0U));
    EXPECT_EQ (rb, pstore::uint128 (1U, 0U));
  }
  {
    pstore::uint128 c{max64, max64};
    pstore::uint128 const rc = ++c;
    EXPECT_EQ (c, (pstore::uint128{}));
    EXPECT_EQ (rc, (pstore::uint128{}));
  }
  {
    pstore::uint128 d{0x0101010101010101U};
    pstore::uint128 const rd = ++d;
    EXPECT_EQ (d, (pstore::uint128{0x0101010101010102U}));
    EXPECT_EQ (rd, (pstore::uint128{0x0101010101010102U}));
  }
  {
    pstore::uint128 e{0x0101010101010101, 0U};
    pstore::uint128 re = ++e;
    EXPECT_EQ (re, (pstore::uint128{0x0101010101010101, 1U}));
    EXPECT_EQ (e, (pstore::uint128{0x0101010101010101, 1U}));
  }
}

TEST_F (UInt128, PostIncrement) {
  {
    pstore::uint128 a{0U};
    pstore::uint128 ra = a++;
    EXPECT_EQ (ra, pstore::uint128{});
    EXPECT_EQ (a, pstore::uint128{1U});
  }
  {
    pstore::uint128 b{max64};
    pstore::uint128 rb = b++;
    EXPECT_EQ (rb, pstore::uint128 (max64));
    EXPECT_EQ (b, pstore::uint128 (1U, 0U));
  }
  {
    pstore::uint128 c{max64, max64};
    pstore::uint128 rc = c++;
    EXPECT_EQ (rc, pstore::uint128 (max64, max64));
    EXPECT_EQ (c, (pstore::uint128{}));
  }
}

TEST_F (UInt128, PreDecrement) {
  {
    pstore::uint128 a{0U, 1U};
    EXPECT_EQ (--a, pstore::uint128 (0U, 0U));
    EXPECT_EQ (a, pstore::uint128 (0U, 0U));
  }
  {
    pstore::uint128 b{1U, 0};
    EXPECT_EQ (--b, pstore::uint128 (max64));
    EXPECT_EQ (b, pstore::uint128 (max64));
  }
  {
    pstore::uint128 c{0U};
    EXPECT_EQ (--c, pstore::uint128 (max64, max64));
    EXPECT_EQ (c, pstore::uint128 (max64, max64));
  }
}

TEST_F (UInt128, PostDecrement) {
  {
    pstore::uint128 a{1U};
    EXPECT_EQ (a--, (pstore::uint128{1U}));
    EXPECT_EQ (a, (pstore::uint128{0U}));
  }
  {
    pstore::uint128 b{1U, 0U};
    EXPECT_EQ (b--, pstore::uint128 (1U, 0U));
    EXPECT_EQ (b, pstore::uint128 (max64));
  }
  {
    pstore::uint128 c{0U};
    EXPECT_EQ (c--, pstore::uint128 (0U));
    EXPECT_EQ (c, pstore::uint128 (max64, max64));
  }
}

TEST_F (UInt128, ShiftLeft) {
  EXPECT_EQ ((pstore::uint128{0x01U} << 0U), (pstore::uint128{0x01U}));
  EXPECT_EQ (pstore::uint128 (0x8000000000000000, 0x00) << 0U,
             pstore::uint128 (0x8000000000000000, 0x00));
  EXPECT_EQ ((pstore::uint128{0x01U} << 1U), (pstore::uint128{0x02U}));
  EXPECT_EQ (pstore::uint128 (0x4000000000000000, 0x00) << 1U,
             pstore::uint128 (0x8000000000000000, 0x00));
  EXPECT_EQ (pstore::uint128 (0x01, std::uint64_t{0x01} << 63) << 1U, pstore::uint128 (0x03, 0x00));
  EXPECT_EQ ((pstore::uint128{max64} << 64U), pstore::uint128 (max64, 0x00));
  EXPECT_EQ ((pstore::uint128{0x01U} << 127U), pstore::uint128 (std::uint64_t{0x01} << 63, 0x00));
}

TEST (Uint128, ShiftRightAssign) {
  static constexpr std::uint64_t top_bit = std::uint64_t{1} << 63;
  {
    pstore::uint128 a{0x01U};
    a >>= 0U;
    EXPECT_EQ (a, (pstore::uint128{0x01U}));
  }
  {
    pstore::uint128 b{0x01U};
    b >>= 1U;
    EXPECT_EQ (b, (pstore::uint128{0x00U}));
  }
  {
    pstore::uint128 c{0x01, top_bit};
    c >>= 1U;
    EXPECT_EQ (c, (pstore::uint128{top_bit | (top_bit >> 1)}));
  }
  {
    pstore::uint128 d{top_bit, 0x00};
    d >>= 1U;
    EXPECT_EQ (d, (pstore::uint128{top_bit >> 1, 0x00}));
  }
  {
    pstore::uint128 e{top_bit, top_bit};
    e >>= 64U;
    EXPECT_EQ (e, (pstore::uint128{top_bit}));
  }
  {
    pstore::uint128 f{top_bit, 0x00};
    f >>= 127U;
    EXPECT_EQ (f, (pstore::uint128{0x01U}));
  }
}

TEST_F (UInt128, BitwiseAnd) {
  EXPECT_EQ ((pstore::uint128{max64, max64} & 0x01U), (pstore::uint128{0x00, 0x01}));
  EXPECT_EQ ((pstore::uint128{0x00, max64} & 0x01U), (pstore::uint128{0x00, 0x01}));
  EXPECT_EQ ((pstore::uint128{max64, 0x00} & 0x01U), (pstore::uint128{0x00, 0x00}));

  EXPECT_EQ ((pstore::uint128{max64, max64} & pstore::uint128{0x01U}), (pstore::uint128{0x01U}));
  EXPECT_EQ ((pstore::uint128{max64} & pstore::uint128{0x01U}), (pstore::uint128{0x01U}));
  EXPECT_EQ ((pstore::uint128{max64, 0x00} & pstore::uint128{0x01U}), (pstore::uint128{0x00U}));

  EXPECT_EQ ((pstore::uint128{max64, max64} & pstore::uint128{0x01, 0x01}),
             (pstore::uint128{0x01, 0x01}));
  EXPECT_EQ ((pstore::uint128{max64} & pstore::uint128{0x01, 0x01}), (pstore::uint128{0x01U}));
  EXPECT_EQ ((pstore::uint128{max64, 0x00} & pstore::uint128{0x01, 0x01}),
             (pstore::uint128{0x01, 0x00}));
  EXPECT_EQ ((pstore::uint128{max64, max64} & pstore::uint128{max64, max64}),
             (pstore::uint128{max64, max64}));
}

TEST_F (UInt128, ToHexString) {
  {
    std::string str1;
    pstore::uint128 (0, 0).to_hex (std::back_inserter (str1));
    EXPECT_EQ (str1, "00000000000000000000000000000000");
  }
  {
    std::string str2;
    pstore::uint128 (1, 2).to_hex (std::back_inserter (str2));
    EXPECT_EQ (str2, "00000000000000010000000000000002");
  }
  {
    std::string str3;
    pstore::uint128 (std::numeric_limits<std::uint64_t>::max (),
                     std::numeric_limits<std::uint64_t>::max ())
      .to_hex (std::back_inserter (str3));
    EXPECT_EQ (str3, "ffffffffffffffffffffffffffffffff");
  }
}

TEST_F (UInt128, FromBytes) {
  EXPECT_EQ (pstore::uint128 (0, 0), (pstore::uint128{std::array<std::uint8_t, 16>{
                                       {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}}));
  EXPECT_EQ (pstore::uint128 (0xffffffffffffffff, 0xffffffffffffffff),
             (pstore::uint128{
               std::array<std::uint8_t, 16>{{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                             0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}}}));

  EXPECT_EQ (pstore::uint128 (0, 0xff), (pstore::uint128{std::array<std::uint8_t, 16>{
                                          {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff}}}));
  EXPECT_EQ (pstore::uint128 (0, 0xff00), (pstore::uint128{std::array<std::uint8_t, 16>{
                                            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0}}}));
  EXPECT_EQ (pstore::uint128 (0, 0xff0000),
             (pstore::uint128{
               std::array<std::uint8_t, 16>{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0, 0}}}));
  EXPECT_EQ (pstore::uint128 (0, 0xff000000),
             (pstore::uint128{
               std::array<std::uint8_t, 16>{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0, 0, 0}}}));
  EXPECT_EQ (pstore::uint128 (0, 0xff00000000),
             (pstore::uint128{
               std::array<std::uint8_t, 16>{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0, 0, 0, 0}}}));
  EXPECT_EQ (pstore::uint128 (0, 0xff0000000000),
             (pstore::uint128{
               std::array<std::uint8_t, 16>{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0, 0, 0, 0, 0}}}));
  EXPECT_EQ (pstore::uint128 (0, 0xff000000000000),
             (pstore::uint128{
               std::array<std::uint8_t, 16>{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0, 0, 0, 0, 0, 0}}}));
  EXPECT_EQ (pstore::uint128 (0, 0xff00000000000000),
             (pstore::uint128{
               std::array<std::uint8_t, 16>{{0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0, 0, 0, 0, 0, 0, 0}}}));
  EXPECT_EQ (pstore::uint128 (0xff, 0), (pstore::uint128{std::array<std::uint8_t, 16>{
                                          {0, 0, 0, 0, 0, 0, 0, 0xff, 0, 0, 0, 0, 0, 0, 0, 0}}}));
  EXPECT_EQ (pstore::uint128 (0xff00, 0), (pstore::uint128{std::array<std::uint8_t, 16>{
                                            {0, 0, 0, 0, 0, 0, 0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0}}}));
  EXPECT_EQ (pstore::uint128 (0xff0000, 0),
             (pstore::uint128{
               std::array<std::uint8_t, 16>{{0, 0, 0, 0, 0, 0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}}));
  EXPECT_EQ (pstore::uint128 (0xff000000, 0),
             (pstore::uint128{
               std::array<std::uint8_t, 16>{{0, 0, 0, 0, 0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}}));
  EXPECT_EQ (pstore::uint128 (0xff00000000, 0),
             (pstore::uint128{
               std::array<std::uint8_t, 16>{{0, 0, 0, 0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}}));
  EXPECT_EQ (pstore::uint128 (0xff0000000000, 0),
             (pstore::uint128{
               std::array<std::uint8_t, 16>{{0, 0, 0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}}));
  EXPECT_EQ (pstore::uint128 (0xff000000000000, 0),
             (pstore::uint128{
               std::array<std::uint8_t, 16>{{0, 0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}}));
  EXPECT_EQ (pstore::uint128 (0xff00000000000000, 0),
             (pstore::uint128{
               std::array<std::uint8_t, 16>{{0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}}));
}

TEST_F (UInt128, LimitsMaxMin) {
  pstore::uint128 max = std::numeric_limits<pstore::uint128>::max ();
  ++max;
  EXPECT_EQ (max, 0U);

  pstore::uint128 min = std::numeric_limits<pstore::uint128>::min ();
  EXPECT_EQ (min, 0U);
}

TEST_F (UInt128, PopCount) {
  EXPECT_EQ ((pstore::bit_count::pop_count (pstore::uint128{1U})), 1U);
  EXPECT_EQ ((pstore::bit_count::pop_count (pstore::uint128{1U, 0U})), 1U);
  EXPECT_EQ ((pstore::bit_count::pop_count (pstore::uint128{max64})), 64U);
  EXPECT_EQ ((pstore::bit_count::pop_count (pstore::uint128{max64, 0U})), 64U);
  EXPECT_EQ ((pstore::bit_count::pop_count (pstore::uint128{max64, max64})), 128U);
}

TEST (Uint128FromString, Empty) {
  EXPECT_EQ (pstore::uint128::from_hex_string (""), std::optional<pstore::uint128> ());
}

TEST (Uint128FromString, Bad) {
  EXPECT_EQ (pstore::uint128::from_hex_string ("0000000000000000000000000000000g"),
             std::optional<pstore::uint128> ());
}

TEST (Uint128FromString, Digits) {
  EXPECT_EQ (pstore::uint128::from_hex_string ("00000000000000000000000000000000"),
             std::optional<pstore::uint128> (pstore::uint128{0U}));
  EXPECT_EQ (pstore::uint128::from_hex_string ("00000000000000000000000000000001"),
             std::optional<pstore::uint128> (pstore::uint128{1U}));
  EXPECT_EQ (
    pstore::uint128::from_hex_string ("10000000000000000000000000000001"),
    std::optional<pstore::uint128> (pstore::uint128{0x10000000'00000000U, 0x00000000'00000001U}));
  EXPECT_EQ (
    pstore::uint128::from_hex_string ("99999999999999999999999999999999"),
    std::optional<pstore::uint128> (pstore::uint128{0x99999999'99999999U, 0x99999999'99999999U}));
}
TEST (Uint128FromString, Alpha) {
  EXPECT_EQ (
    pstore::uint128::from_hex_string ("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"),
    std::optional<pstore::uint128> (pstore::uint128{0xFFFFFFFF'FFFFFFFFU, 0xFFFFFFFF'FFFFFFFFU}));
  EXPECT_EQ (
    pstore::uint128::from_hex_string ("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"),
    std::optional<pstore::uint128> (pstore::uint128{0xAAAAAAAA'AAAAAAAAU, 0xAAAAAAAA'AAAAAAAAU}));
  EXPECT_EQ (
    pstore::uint128::from_hex_string ("ffffffffffffffffffffffffffffffff"),
    std::optional<pstore::uint128> (pstore::uint128{0xFFFFFFFF'FFFFFFFFU, 0xFFFFFFFF'FFFFFFFFU}));
  EXPECT_EQ (
    pstore::uint128::from_hex_string ("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"),
    std::optional<pstore::uint128> (pstore::uint128{0xAAAAAAAA'AAAAAAAAU, 0xAAAAAAAA'AAAAAAAAU}));
}
