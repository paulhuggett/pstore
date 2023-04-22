//===- unittests/adt/test_sstring_view.cpp --------------------------------===//
//*          _        _                     _                *
//*  ___ ___| |_ _ __(_)_ __   __ _  __   _(_) _____      __ *
//* / __/ __| __| '__| | '_ \ / _` | \ \ / / |/ _ \ \ /\ / / *
//* \__ \__ \ |_| |  | | | | | (_| |  \ V /| |  __/\ V  V /  *
//* |___/___/\__|_|  |_|_| |_|\__, |   \_/ |_|\___| \_/\_/   *
//*                           |___/                          *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "pstore/adt/sstring_view.hpp"

// Standard library includes
#include <cstring>
#include <sstream>

// 3rd party
#include <gmock/gmock.h>

using namespace std::string_literals;
using namespace std::string_view_literals;

namespace {

  std::shared_ptr<char> new_shared (std::string_view s) {
    auto result = std::shared_ptr<char> (new char[s.size ()], [] (char * p) { delete[] p; });
    std::copy (std::begin (s), std::end (s), result.get ());
    return result;
  }

  template <typename T>
  struct string_maker {};

  template <typename CharType>
  struct string_maker<std::shared_ptr<CharType>> {
    std::shared_ptr<CharType> operator() (std::string const & str) const {
      auto ptr = std::shared_ptr<char> (new char[str.length ()], [] (char * p) { delete[] p; });
      std::copy (std::begin (str), std::end (str), ptr.get ());
      return std::static_pointer_cast<CharType> (ptr);
    }
  };

  template <typename CharType>
  struct string_maker<std::unique_ptr<CharType[]>> {
    std::unique_ptr<CharType[]> operator() (std::string const & str) const {
      auto ptr = std::make_unique<std::remove_const_t<CharType>[]> (str.length ());
      std::copy (std::begin (str), std::end (str), ptr.get ());
      return std::unique_ptr<CharType[]>{ptr.release ()};
    }
  };

  template <>
  struct string_maker<char const *> {
    char const * operator() (std::string const & str) const noexcept { return str.data (); }
  };

  template <typename StringType>
  class SStringViewInit : public testing::Test {};

  using SStringViewInitTypes =
    testing::Types<string_maker<std::shared_ptr<char>>, string_maker<std::shared_ptr<char const>>,
                   string_maker<std::unique_ptr<char[]>>>;

  // The pstore APIs that return shared_ptr<> and unique_ptr<> sstring_views is named
  // make_shared_sstring_view() and make_unique_sstring_view() respectively to try to avoid
  // confusion about the ownership of the memory. However, here, I'd like all of the functions to
  // have the same name.
  template <typename ValueType>
  inline pstore::sstring_view<std::shared_ptr<ValueType>>
  make_sstring_view (std::shared_ptr<ValueType> && ptr, std::size_t length) {
    return pstore::make_shared_sstring_view (std::move (ptr), length);
  }

  template <typename ValueType>
  inline pstore::sstring_view<std::unique_ptr<ValueType>>
  make_sstring_view (std::unique_ptr<ValueType> && ptr, std::size_t length) {
    return pstore::make_unique_sstring_view (std::move (ptr), length);
  }

} // end anonymous namespace

#ifdef PSTORE_IS_INSIDE_LLVM
TYPED_TEST_CASE (SStringViewInit, SStringViewInitTypes);
#else
TYPED_TEST_SUITE (SStringViewInit, SStringViewInitTypes, );
#endif // PSTORE_IS_INSIDE_LLVM

TYPED_TEST (SStringViewInit, Empty) {
  using namespace pstore;
  TypeParam t;
  std::string const src;
  auto ptr = t (src);
  auto sv = make_sstring_view (std::move (ptr), src.length ());
  EXPECT_EQ (sv.size (), 0U);
  EXPECT_EQ (sv.length (), 0U);
  EXPECT_EQ (sv.max_size (), std::numeric_limits<std::size_t>::max ());
  EXPECT_TRUE (sv.empty ());
  EXPECT_EQ (std::distance (std::begin (sv), std::end (sv)), 0);
}

TYPED_TEST (SStringViewInit, Short) {
  using namespace pstore;
  TypeParam t;
  std::string const src{"hello"};
  auto ptr = t (src);
  auto sv = make_sstring_view (std::move (ptr), src.length ());
  EXPECT_EQ (sv.size (), 5U);
  EXPECT_EQ (sv.length (), 5U);
  EXPECT_EQ (sv.max_size (), std::numeric_limits<std::size_t>::max ());
  EXPECT_FALSE (sv.empty ());
  EXPECT_EQ (std::distance (std::begin (sv), std::end (sv)), 5);
}

TEST (SStringView, FromSpan) {
  using namespace pstore;
  std::array<char, 5> const src{{'a', 'r', 'r', 'a', 'y'}};
  auto sv = make_sstring_view (gsl::make_span (src));
  EXPECT_THAT (sv, testing::ElementsAreArray (src));
}

TEST (SStringView, OperatorIndex) {
  auto const src = "ABCDE"sv;
  pstore::sstring_view<char const *> sv = pstore::make_sstring_view (src);
  ASSERT_EQ (sv.length (), src.length ());
  EXPECT_FALSE (sv.empty ());
  EXPECT_EQ (sv[0], 'A');
  EXPECT_EQ (sv[1], 'B');
  EXPECT_EQ (sv[4], 'E');
}

TEST (SStringView, At) {
  auto const src = "ABCDE"sv;
  pstore::sstring_view<char const *> sv = pstore::make_sstring_view (src);
  ASSERT_EQ (sv.length (), src.length ());
  EXPECT_FALSE (sv.empty ());
  EXPECT_EQ (sv.at (0), 'A');
  EXPECT_EQ (sv.at (1), 'B');
  EXPECT_EQ (sv.at (4), 'E');
#ifdef PSTORE_EXCEPTIONS
  EXPECT_THROW (sv.at (5), std::out_of_range);
#endif // PSTORE_EXCEPTIONS
}

TEST (SStringView, Back) {
  auto const src = "ABCDE"sv;
  auto const length = src.length ();
  std::shared_ptr<char> ptr = new_shared (src);
  pstore::sstring_view<std::shared_ptr<char>> sv = pstore::make_shared_sstring_view (ptr, length);

  ASSERT_EQ (sv.length (), length);
  EXPECT_EQ (sv.back (), src[length - 1]);
  EXPECT_EQ (&sv.back (), sv.data () + length - 1U);
}

TEST (SStringView, Data) {
  std::string const src{"ABCDE"};
  auto const length = src.length ();
  std::shared_ptr<char> ptr = new_shared (src);
  pstore::sstring_view<std::shared_ptr<char>> sv = pstore::make_shared_sstring_view (ptr, length);

  ASSERT_EQ (sv.length (), length);
  EXPECT_EQ (sv.data (), ptr.get ());
}

TEST (SStringView, Front) {
  std::string const src{"ABCDE"};
  auto const length = src.length ();
  std::shared_ptr<char> ptr = new_shared (src);
  pstore::sstring_view<std::shared_ptr<char>> sv = pstore::make_shared_sstring_view (ptr, length);

  ASSERT_EQ (sv.length (), length);
  EXPECT_EQ (sv.front (), src[0]);
  EXPECT_EQ (&sv.front (), sv.data ());
}

TEST (SStringView, Index) {
  std::string const src{"ABCDE"};
  auto const length = src.length ();
  std::shared_ptr<char> ptr = new_shared (src);
  pstore::sstring_view<std::shared_ptr<char>> sv = pstore::make_shared_sstring_view (ptr, length);

  EXPECT_EQ (sv[0], src[0]);
  EXPECT_EQ (&sv[0], ptr.get () + 0);
  EXPECT_EQ (sv[1], src[1]);
  EXPECT_EQ (&sv[1], ptr.get () + 1);
  EXPECT_EQ (sv[4], src[4]);
  EXPECT_EQ (&sv[4], ptr.get () + 4);
}

TEST (SStringView, RBeginEmpty) {
  std::string const src;
  using sv_type = pstore::sstring_view<char const *>;
  sv_type sv = pstore::make_sstring_view (src);
  sv_type const & csv = sv;

  sv_type::reverse_iterator rbegin = sv.rbegin ();
  sv_type::const_reverse_iterator const_rbegin1 = csv.rbegin ();
  sv_type::const_reverse_iterator const_rbegin2 = sv.crbegin ();
  EXPECT_EQ (rbegin, const_rbegin1);
  EXPECT_EQ (rbegin, const_rbegin2);
  EXPECT_EQ (const_rbegin1, const_rbegin2);
}

TEST (SStringView, RBegin) {
  std::string const src{"abc"};
  using sv_type = pstore::sstring_view<char const *>;
  sv_type sv = pstore::make_sstring_view (src);
  sv_type const & csv = sv;

  sv_type::reverse_iterator rbegin = sv.rbegin ();
  sv_type::const_reverse_iterator const_begin1 = csv.rbegin ();
  sv_type::const_reverse_iterator const_begin2 = sv.crbegin ();

  std::size_t const last = sv.size () - 1;
  EXPECT_EQ (*rbegin, sv[last]);
  EXPECT_EQ (&*rbegin, &sv[last]);
  EXPECT_EQ (*const_begin1, sv[last]);
  EXPECT_EQ (&*const_begin1, &sv[last]);
  EXPECT_EQ (*const_begin2, sv[last]);
  EXPECT_EQ (&*const_begin2, &sv[last]);

  EXPECT_EQ (rbegin, const_begin1);
  EXPECT_EQ (rbegin, const_begin2);
  EXPECT_EQ (const_begin1, const_begin2);
}

TEST (SStringView, REndEmpty) {
  auto const src = ""sv;
  using sv_type = pstore::sstring_view<char const *>;
  sv_type sv = pstore::make_sstring_view (src);
  sv_type const & csv = sv;

  sv_type::reverse_iterator rend = sv.rend ();
  sv_type::const_reverse_iterator const_rend1 = csv.rend ();
  sv_type::const_reverse_iterator const_rend2 = sv.crend ();

  EXPECT_EQ (rend, sv.rbegin ());
  EXPECT_EQ (const_rend1, csv.rbegin ());
  EXPECT_EQ (const_rend2, sv.rbegin ());

  EXPECT_EQ (rend - sv.rbegin (), 0);
  EXPECT_EQ (const_rend1 - csv.rbegin (), 0);
  EXPECT_EQ (const_rend2 - sv.crbegin (), 0);

  EXPECT_EQ (rend, const_rend1);
  EXPECT_EQ (rend, const_rend2);
  EXPECT_EQ (const_rend1, const_rend2);
}

TEST (SStringView, REnd) {
  auto const src = "abc"sv;
  using sv_type = pstore::sstring_view<char const *>;
  sv_type sv = pstore::make_sstring_view (src);
  sv_type const & csv = sv;

  sv_type::reverse_iterator rend = sv.rend ();
  sv_type::const_reverse_iterator const_rend1 = csv.rend ();
  sv_type::const_reverse_iterator const_rend2 = sv.crend ();

  EXPECT_NE (rend, sv.rbegin ());
  EXPECT_NE (const_rend1, csv.rbegin ());
  EXPECT_NE (const_rend2, sv.rbegin ());

  EXPECT_EQ (rend - sv.rbegin (), 3);
  EXPECT_EQ (const_rend1 - csv.rbegin (), 3);
  EXPECT_EQ (const_rend2 - sv.crbegin (), 3);

  EXPECT_EQ (rend, const_rend1);
  EXPECT_EQ (rend, const_rend2);
  EXPECT_EQ (const_rend1, const_rend2);
}

TEST (SStringView, Clear) {
  std::string const empty_str;
  pstore::sstring_view<char const *> empty = pstore::make_sstring_view (empty_str);
  {
    pstore::sstring_view<char const *> sv1 = pstore::make_sstring_view ("abc"sv);
    sv1.clear ();
    EXPECT_EQ (sv1.size (), 0U);
    EXPECT_EQ (sv1, empty);
  }
  {
    pstore::sstring_view<char const *> sv2 = pstore::make_sstring_view (empty_str);
    sv2.clear ();
    EXPECT_EQ (sv2.size (), 0U);
    EXPECT_EQ (sv2, empty);
  }
}

TEST (SStringView, FindChar) {
  using sv_type = pstore::sstring_view<char const *>;
  sv_type sv = pstore::make_sstring_view ("abc"sv);

  EXPECT_EQ (sv.find ('a'), 0U);
  EXPECT_EQ (sv.find ('c'), 2U);
  EXPECT_EQ (sv.find ('d'), sv_type::npos);
  EXPECT_EQ (sv.find ('c', 1U), 2U);
  EXPECT_EQ (sv.find ('c', 3U), sv_type::npos);
}

TEST (SStringView, Substr) {
  pstore::sstring_view<char const *> sv = pstore::make_sstring_view ("abc"sv);
  EXPECT_EQ (sv.substr (0U, 1U), "a"sv);
  EXPECT_EQ (sv.substr (0U, 4U), "abc"sv);
  EXPECT_EQ (sv.substr (1U, 1U), "b"sv);
  EXPECT_EQ (sv.substr (3U, 1U), ""sv);
}

TEST (SStringView, OperatorWrite) {
  auto check = [] (std::string const & str) {
    std::ostringstream stream;
    stream << pstore::make_sstring_view (str);
    EXPECT_EQ (stream.str (), str);
  };
  check ("");
  check ("abcdef");
  check ("hello world");
}

namespace {

  template <typename StringType>
  class SStringViewRelational : public testing::Test {};

  class sstringview_maker {
  public:
    explicit sstringview_maker (std::string_view s)
            : view_{pstore::make_sstring_view (s)} {}

    operator pstore::sstring_view<char const *> () const noexcept { return view_; }

  private:
    pstore::sstring_view<char const *> view_;
  };

  using StringTypes = testing::Types<sstringview_maker, sstringview_maker const>;

} // end anonymous namespace

namespace pstore {

  template <>
  struct string_traits<sstringview_maker> {
    static std::size_t length (sstring_view<char const *> const & s) noexcept {
      return string_traits<sstring_view<char const *>>::length (s);
    }
    static char const * data (sstring_view<char const *> const & s) noexcept {
      return string_traits<sstring_view<char const *>>::data (s);
    }
  };

} // end namespace pstore

namespace {

  template <typename T>
  void eq (std::string_view lhs, T rhs, bool expected) {
    auto const lhs_view = pstore::make_sstring_view (lhs);
    EXPECT_EQ (lhs_view == rhs, expected);
    EXPECT_EQ (rhs == lhs_view, expected);
  }

  template <typename T>
  void ne (std::string_view lhs, T rhs, bool expected) {
    auto const lhs_view = pstore::make_sstring_view (lhs);
    EXPECT_EQ (lhs_view != rhs, expected);
    EXPECT_EQ (rhs != lhs_view, expected);
  }

  template <typename T>
  void ge (std::string_view lhs, T rhs, bool x, bool y) {
    auto const lhs_view = pstore::make_sstring_view (lhs);
    EXPECT_EQ (lhs_view >= rhs, x);
    EXPECT_EQ (rhs >= lhs_view, y);
  }

  template <typename T>
  void gt (std::string_view lhs, T rhs, bool x, bool y) {
    auto const lhs_view = pstore::make_sstring_view (lhs);
    EXPECT_EQ (lhs_view > rhs, x);
    EXPECT_EQ (rhs > lhs_view, y);
  }

  template <typename T>
  void le (std::string_view lhs, T rhs, bool x, bool y) {
    auto const lhs_view = pstore::make_sstring_view (lhs);
    EXPECT_EQ (lhs_view <= rhs, x);
    EXPECT_EQ (rhs <= lhs_view, y);
  }

  template <typename T>
  void lt (std::string_view lhs, T rhs, bool x, bool y) {
    auto const lhs_view = pstore::make_sstring_view (lhs);
    EXPECT_EQ (lhs_view < rhs, x);
    EXPECT_EQ (rhs < lhs_view, y);
  }

} // end anonymous namespace

#ifdef PSTORE_IS_INSIDE_LLVM
TYPED_TEST_CASE (SStringViewRelational, StringTypes);
#else
TYPED_TEST_SUITE (SStringViewRelational, StringTypes, );
#endif

TYPED_TEST (SStringViewRelational, Eq) {
  eq (""sv, TypeParam (""), true);
  eq ("", TypeParam ("abcde"), false);
  eq ("", TypeParam ("abcdefghij"), false);
  eq ("", TypeParam ("abcdefghijklmnopqrst"), false);
  eq ("abcde", TypeParam (""), false);
  eq ("abcde", TypeParam ("abcde"), true);
  eq ("abcde", TypeParam ("abcdefghij"), false);
  eq ("abcde", TypeParam ("abcdefghijklmnopqrst"), false);
  eq ("abcdefghij", TypeParam (""), false);
  eq ("abcdefghij", TypeParam ("abcde"), false);
  eq ("abcdefghij", TypeParam ("abcdefghij"), true);
  eq ("abcdefghij", TypeParam ("abcdefghijklmnopqrst"), false);
  eq ("abcdefghijklmnopqrst", TypeParam (""), false);
  eq ("abcdefghijklmnopqrst", TypeParam ("abcde"), false);
  eq ("abcdefghijklmnopqrst", TypeParam ("abcdefghij"), false);
  eq ("abcdefghijklmnopqrst", TypeParam ("abcdefghijklmnopqrst"), true);
}

TYPED_TEST (SStringViewRelational, Ne) {
  ne ("", TypeParam (""), false);
  ne ("", TypeParam ("abcde"), true);
  ne ("", TypeParam ("abcdefghij"), true);
  ne ("", TypeParam ("abcdefghijklmnopqrst"), true);
  ne ("abcde", TypeParam (""), true);
  ne ("abcde", TypeParam ("abcde"), false);
  ne ("abcde", TypeParam ("abcdefghij"), true);
  ne ("abcde", TypeParam ("abcdefghijklmnopqrst"), true);
  ne ("abcdefghij", TypeParam (""), true);
  ne ("abcdefghij", TypeParam ("abcde"), true);
  ne ("abcdefghij", TypeParam ("abcdefghij"), false);
  ne ("abcdefghij", TypeParam ("abcdefghijklmnopqrst"), true);
  ne ("abcdefghijklmnopqrst", TypeParam (""), true);
  ne ("abcdefghijklmnopqrst", TypeParam ("abcde"), true);
  ne ("abcdefghijklmnopqrst", TypeParam ("abcdefghij"), true);
  ne ("abcdefghijklmnopqrst", TypeParam ("abcdefghijklmnopqrst"), false);
}

TYPED_TEST (SStringViewRelational, Ge) {
  ge ("", TypeParam (""), true, true);
  ge ("", TypeParam ("abcde"), false, true);
  ge ("", TypeParam ("abcdefghij"), false, true);
  ge ("", TypeParam ("abcdefghijklmnopqrst"), false, true);
  ge ("abcde", TypeParam (""), true, false);
  ge ("abcde", TypeParam ("abcde"), true, true);
  ge ("abcde", TypeParam ("abcdefghij"), false, true);
  ge ("abcde", TypeParam ("abcdefghijklmnopqrst"), false, true);
  ge ("abcdefghij", TypeParam (""), true, false);
  ge ("abcdefghij", TypeParam ("abcde"), true, false);
  ge ("abcdefghij", TypeParam ("abcdefghij"), true, true);
  ge ("abcdefghij", TypeParam ("abcdefghijklmnopqrst"), false, true);
  ge ("abcdefghijklmnopqrst", TypeParam (""), true, false);
  ge ("abcdefghijklmnopqrst", TypeParam ("abcde"), true, false);
  ge ("abcdefghijklmnopqrst", TypeParam ("abcdefghij"), true, false);
  ge ("abcdefghijklmnopqrst", TypeParam ("abcdefghijklmnopqrst"), true, true);
}

TYPED_TEST (SStringViewRelational, Gt) {
  gt ("", TypeParam (""), false, false);
  gt ("", TypeParam ("abcde"), false, true);
  gt ("", TypeParam ("abcdefghij"), false, true);
  gt ("", TypeParam ("abcdefghijklmnopqrst"), false, true);
  gt ("abcde", TypeParam (""), true, false);
  gt ("abcde", TypeParam ("abcde"), false, false);
  gt ("abcde", TypeParam ("abcdefghij"), false, true);
  gt ("abcde", TypeParam ("abcdefghijklmnopqrst"), false, true);
  gt ("abcdefghij", TypeParam (""), true, false);
  gt ("abcdefghij", TypeParam ("abcde"), true, false);
  gt ("abcdefghij", TypeParam ("abcdefghij"), false, false);
  gt ("abcdefghij", TypeParam ("abcdefghijklmnopqrst"), false, true);
  gt ("abcdefghijklmnopqrst", TypeParam (""), true, false);
  gt ("abcdefghijklmnopqrst", TypeParam ("abcde"), true, false);
  gt ("abcdefghijklmnopqrst", TypeParam ("abcdefghij"), true, false);
  gt ("abcdefghijklmnopqrst", TypeParam ("abcdefghijklmnopqrst"), false, false);
}

TYPED_TEST (SStringViewRelational, Le) {
  le ("", TypeParam (""), true, true);
  le ("", TypeParam ("abcde"), true, false);
  le ("", TypeParam ("abcdefghij"), true, false);
  le ("", TypeParam ("abcdefghijklmnopqrst"), true, false);
  le ("abcde", TypeParam (""), false, true);
  le ("abcde", TypeParam ("abcde"), true, true);
  le ("abcde", TypeParam ("abcdefghij"), true, false);
  le ("abcde", TypeParam ("abcdefghijklmnopqrst"), true, false);
  le ("abcdefghij", TypeParam (""), false, true);
  le ("abcdefghij", TypeParam ("abcde"), false, true);
  le ("abcdefghij", TypeParam ("abcdefghij"), true, true);
  le ("abcdefghij", TypeParam ("abcdefghijklmnopqrst"), true, false);
  le ("abcdefghijklmnopqrst", TypeParam (""), false, true);
  le ("abcdefghijklmnopqrst", TypeParam ("abcde"), false, true);
  le ("abcdefghijklmnopqrst", TypeParam ("abcdefghij"), false, true);
  le ("abcdefghijklmnopqrst", TypeParam ("abcdefghijklmnopqrst"), true, true);
}

TYPED_TEST (SStringViewRelational, Lt) {
  lt ("", TypeParam (""), false, false);
  lt ("", TypeParam ("abcde"), true, false);
  lt ("", TypeParam ("abcdefghij"), true, false);
  lt ("", TypeParam ("abcdefghijklmnopqrst"), true, false);
  lt ("abcde", TypeParam (""), false, true);
  lt ("abcde", TypeParam ("abcde"), false, false);
  lt ("abcde", TypeParam ("abcdefghij"), true, false);
  lt ("abcde", TypeParam ("abcdefghijklmnopqrst"), true, false);
  lt ("abcdefghij", TypeParam (""), false, true);
  lt ("abcdefghij", TypeParam ("abcde"), false, true);
  lt ("abcdefghij", TypeParam ("abcdefghij"), false, false);
  lt ("abcdefghij", TypeParam ("abcdefghijklmnopqrst"), true, false);
  lt ("abcdefghijklmnopqrst", TypeParam (""), false, true);
  lt ("abcdefghijklmnopqrst", TypeParam ("abcde"), false, true);
  lt ("abcdefghijklmnopqrst", TypeParam ("abcdefghij"), false, true);
  lt ("abcdefghijklmnopqrst", TypeParam ("abcdefghijklmnopqrst"), false, false);
}
