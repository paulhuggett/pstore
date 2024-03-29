//===- include/pstore/adt/sstring_view.hpp ----------------*- mode: C++ -*-===//
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
/// \file sstring_view.hpp
/// \brief Implements sstring_view, a class which is based on std::string_view but holds a
/// pointer which may be a std::shared_ptr<char> or std::unique_ptr<char> as well has a raw pointer.
///
/// This class is intended to improve the performance of the string set -- where it avoids the
/// construction of std::string instances -- and to enable string values from the database and
/// in-memory string values to be used interchangably.

#ifndef PSTORE_ADT_SSTRING_VIEW_HPP
#define PSTORE_ADT_SSTRING_VIEW_HPP

#include <cstring>
#include <string_view>

#include "pstore/support/fnv.hpp"
#include "pstore/support/unsigned_cast.hpp"
#include "pstore/support/varint.hpp"

namespace pstore {

  //*     _       _             _            _ _       *
  //*  __| |_ _ _(_)_ _  __ _  | |_ _ _ __ _(_) |_ ___ *
  //* (_-<  _| '_| | ' \/ _` | |  _| '_/ _` | |  _(_-< *
  //* /__/\__|_| |_|_||_\__, |  \__|_| \__,_|_|\__/__/ *
  //*                   |___/                          *
  template <typename StringType>
  struct string_traits {
    static std::size_t length (StringType const & s) noexcept { return s.length (); }
    static char const * data (StringType const & s) noexcept { return s.data (); }
  };

  //*            _     _             _            _ _       *
  //*  _ __  ___(_)_ _| |_ ___ _ _  | |_ _ _ __ _(_) |_ ___ *
  //* | '_ \/ _ \ | ' \  _/ -_) '_| |  _| '_/ _` | |  _(_-< *
  //* | .__/\___/_|_||_\__\___|_|    \__|_| \__,_|_|\__/__/ *
  //* |_|                                                   *
  template <typename PointerType>
  struct pointer_traits {
    static constexpr bool is_pointer = false;
  };
  template <>
  struct pointer_traits<char const *> {
    static constexpr bool is_pointer = true;
    using value_type = char const;
    static constexpr char const * as_raw (char const * const p) noexcept { return p; }
  };

  namespace details {

    template <typename T>
    struct pointer_traits_helper {
      static constexpr bool is_pointer = true;
      using value_type = typename T::element_type;
      static_assert (std::is_same_v<std::remove_const_t<value_type>, char>,
                     "pointer element type must be char or char const");
      static constexpr char const * as_raw (T const & p) noexcept { return p.get (); }
    };

  } // end namespace details

  template <>
  struct pointer_traits<std::shared_ptr<char const>>
          : details::pointer_traits_helper<std::shared_ptr<char const>> {};
  template <>
  struct pointer_traits<std::shared_ptr<char>>
          : details::pointer_traits_helper<std::shared_ptr<char>> {};
  template <>
  struct pointer_traits<std::unique_ptr<char const[]>>
          : details::pointer_traits_helper<std::unique_ptr<char const[]>>{};
  template <>
  struct pointer_traits<std::unique_ptr<char[]>>
          : details::pointer_traits_helper<std::unique_ptr<char[]>> {};


  template <typename Deleter>
  struct pointer_traits<std::unique_ptr<char const, Deleter>>
          : details::pointer_traits_helper<std::unique_ptr<char const, Deleter>> {};
  template <typename Deleter>
  struct pointer_traits<std::unique_ptr<char, Deleter>>
          : details::pointer_traits_helper<std::unique_ptr<char, Deleter>> {};

  //*        _       _                 _             *
  //*  _____| |_ _ _(_)_ _  __ _  __ _(_)_____ __ __ *
  //* (_-<_-<  _| '_| | ' \/ _` | \ V / / -_) V  V / *
  //* /__/__/\__|_| |_|_||_\__, |  \_/|_\___|\_/\_/  *
  //*                      |___/                     *
  /// \note This class implements a subset of the std::string_view methods. Complete the set of
  /// members as we need them...
  template <typename PointerType>
  class sstring_view {
  public:
    static_assert (pointer_traits<PointerType>::is_pointer,
                   "PointerType is not a known pointer type!");

    using value_type = char const;
    using traits = std::char_traits<std::remove_const_t<value_type>>;
    using pointer = value_type *;
    using const_pointer = value_type const *;
    using reference = value_type &;
    using const_reference = value_type const &;
    using const_iterator = value_type const *;
    /// iterator and const_iterator are the same type because sstring_view refers to a constant
    /// sequence.
    using iterator = const_iterator;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using reverse_iterator = const_reverse_iterator;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    static constexpr auto const npos = static_cast<size_type> (-1);

    // 7.3, sstring_view constructors and assignment operators
    constexpr sstring_view () noexcept
            : ptr_{} {}
    sstring_view (PointerType ptr, size_type const size) noexcept
            : ptr_{std::move (ptr)}
            , size_{size} {}
    sstring_view (sstring_view const &) = default;
    sstring_view (sstring_view &&) noexcept = default;
    ~sstring_view () = default;
    sstring_view & operator= (sstring_view const &) = default;
    sstring_view & operator= (sstring_view &&) noexcept = default;

    // 7.4, sstring_view iterator support
    const_iterator begin () const noexcept { return data (); }
    const_iterator end () const noexcept { return begin () + size_; }
    const_iterator cbegin () const noexcept { return begin (); }
    const_iterator cend () const noexcept { return end (); }
    const_reverse_iterator rbegin () const noexcept { return const_reverse_iterator (end ()); }
    const_reverse_iterator rend () const noexcept { return const_reverse_iterator (begin ()); }
    const_reverse_iterator crbegin () const noexcept { return const_reverse_iterator (end ()); }
    const_reverse_iterator crend () const noexcept { return const_reverse_iterator (begin ()); }

    // 7.5, sstring_view capacity
    size_type size () const noexcept { return size_; }
    size_type length () const noexcept { return size_; }
    size_type max_size () const noexcept { return std::numeric_limits<size_t>::max (); }
    bool empty () const noexcept { return size_ == 0; }

    // 7.6, sstring_view element access
    const_reference operator[] (size_type const pos) const {
      PSTORE_ASSERT (pos < size_);
      return (this->data ())[pos];
    }
    const_reference at (size_type const pos) const {
#ifdef PSTORE_EXCEPTIONS
      if (pos >= size_) {
        throw std::out_of_range ("sstring_view access out of range");
      }
#endif
      return (*this)[pos];
    }
    const_reference front () const {
      PSTORE_ASSERT (size_ > 0);
      return (*this)[0];
    }
    const_reference back () const {
      PSTORE_ASSERT (size_ > 0);
      return (*this)[size_ - 1];
    }
    const_pointer data () const noexcept { return pointer_traits<PointerType>::as_raw (ptr_); }

    // 7.7, sstring_view modifiers
    void clear () noexcept { size_ = 0; }

    void swap (sstring_view & s) noexcept {
      std::swap (ptr_, s.ptr_);
      std::swap (size_, s.size_);
    }

    // 7.8, sstring_view string operations
    explicit operator std::string () const { return this->to_string (); }

    template <class Allocator = std::allocator<char>>
    std::basic_string<char, std::char_traits<char>, Allocator>
    to_string (Allocator const & a = Allocator ()) const {
      return {data (), size_, a};
    }

    /// Returns a view of the substring [\p pos, \p pos + rcount), where rcount is the smaller
    /// of \p n and size() - \p pos.
    /// \param pos position of the first character
    /// \param n requested length
    sstring_view<gsl::czstring> substr (size_type pos = 0, size_type const n = npos) const {
      pos = std::min (pos, size_);
      return {data () + pos, std::min (n, size_ - pos)};
    }

    template <typename StringType>
    int compare (StringType const & s) const;

    /// Finds the first occurrence of \p ch in this view, starting at position \p pos.
    size_type find (value_type ch, size_type pos = 0) const noexcept {
      for (; pos < size_; ++pos) {
        if (ptr_[pos] == ch) {
          return pos;
        }
      }
      return npos;
    }

  private:
    PointerType ptr_;
    size_type size_ = size_type{0};
  };

  template <typename PointerType>
  struct string_traits<sstring_view<PointerType>> {
    static std::size_t length (sstring_view<PointerType> const & s) noexcept { return s.length (); }
    static gsl::czstring data (sstring_view<PointerType> const & s) noexcept { return s.data (); }
  };

  // compare
  // ~~~~~~~
  template <typename PointerType>
  template <typename StringType>
  int sstring_view<PointerType>::compare (StringType const & s) const {
    auto const slen = string_traits<StringType>::length (s);
    auto const size = this->size ();
    if (int const result =
          traits::compare (data (), string_traits<StringType>::data (s), std::min (size, slen));
        result != 0) {
      return result;
    }
    if (size == slen) {
      return 0;
    }
    return size < slen ? -1 : 1;
  }

  // operator==
  // ~~~~~~~~~~
  template <typename PointerType1, typename PointerType2>
  inline bool operator== (sstring_view<PointerType1> const & lhs,
                          sstring_view<PointerType2> const & rhs) noexcept {
    if (lhs.size () != rhs.size ()) {
      return false;
    }
    return lhs.compare (rhs) == 0;
  }
  template <typename PointerType, typename StringType>
  inline bool operator== (sstring_view<PointerType> const & lhs, StringType const & rhs) noexcept {
    if (string_traits<sstring_view<PointerType>>::length (lhs) !=
        string_traits<StringType>::length (rhs)) {
      return false;
    }
    return lhs.compare (rhs) == 0;
  }
  template <typename StringType, typename PointerType>
  inline bool operator== (StringType const & lhs, sstring_view<PointerType> const & rhs) noexcept {
    if (string_traits<StringType>::length (lhs) !=
        string_traits<sstring_view<PointerType>>::length (rhs)) {
      return false;
    }
    return rhs.compare (lhs) == 0;
  }

  // operator!=
  // ~~~~~~~~~~
  template <typename PointerType1, typename PointerType2>
  inline bool operator!= (sstring_view<PointerType1> const & lhs,
                          sstring_view<PointerType2> const & rhs) noexcept {
    return !operator== (lhs, rhs);
  }
  template <typename PointerType, typename StringType>
  inline bool operator!= (sstring_view<PointerType> const & lhs, StringType const & rhs) noexcept {
    return !operator== (lhs, rhs);
  }
  template <typename StringType, typename PointerType>
  inline bool operator!= (StringType const & lhs, sstring_view<PointerType> const & rhs) noexcept {
    return !operator== (lhs, rhs);
  }

  // operator>=
  // ~~~~~~~~~~
  template <typename PointerType1, typename PointerType2>
  inline bool operator>= (sstring_view<PointerType1> const & lhs,
                          sstring_view<PointerType2> const & rhs) noexcept {
    return lhs.compare (rhs) >= 0;
  }
  template <typename PointerType, typename StringType>
  inline bool operator>= (sstring_view<PointerType> const & lhs, StringType const & rhs) noexcept {
    return lhs.compare (rhs) >= 0;
  }
  template <typename StringType, typename PointerType>
  inline bool operator>= (StringType const & lhs, sstring_view<PointerType> const & rhs) noexcept {
    return rhs.compare (lhs) <= 0;
  }

  // operator>
  // ~~~~~~~~~
  template <typename PointerType1, typename PointerType2>
  inline bool operator> (sstring_view<PointerType1> const & lhs,
                         sstring_view<PointerType2> const & rhs) noexcept {
    return lhs.compare (rhs) > 0;
  }
  template <typename PointerType, typename StringType>
  inline bool operator> (sstring_view<PointerType> const & lhs, StringType const & rhs) noexcept {
    return lhs.compare (rhs) > 0;
  }
  template <typename StringType, typename PointerType>
  inline bool operator> (StringType const & lhs, sstring_view<PointerType> const & rhs) noexcept {
    return rhs.compare (lhs) < 0;
  }

  // operator<=
  // ~~~~~~~~~~
  template <typename PointerType1, typename PointerType2>
  inline bool operator<= (sstring_view<PointerType1> const & lhs,
                          sstring_view<PointerType2> const & rhs) noexcept {
    return lhs.compare (rhs) <= 0;
  }
  template <typename PointerType, typename StringType>
  inline bool operator<= (sstring_view<PointerType> const & lhs, StringType const & rhs) noexcept {
    return lhs.compare (rhs) <= 0;
  }
  template <typename StringType, typename PointerType>
  inline bool operator<= (StringType const & lhs, sstring_view<PointerType> const & rhs) noexcept {
    return rhs.compare (lhs) >= 0;
  }

  // operator<
  // ~~~~~~~~~
  template <typename PointerType1, typename PointerType2>
  inline bool operator<(sstring_view<PointerType1> const & lhs,
                        sstring_view<PointerType2> const & rhs) noexcept {
    return lhs.compare (rhs) < 0;
  }
  template <typename PointerType, typename StringType>
  inline bool operator<(sstring_view<PointerType> const & lhs, StringType const & rhs) noexcept {
    return lhs.compare (rhs) < 0;
  }
  template <typename StringType, typename PointerType>
  inline bool operator<(StringType const & lhs, sstring_view<PointerType> const & rhs) noexcept {
    return rhs.compare (lhs) > 0;
  }

  // operator <<
  // ~~~~~~~~~~~
  template <typename OStream, typename PointerType>
  inline OStream & operator<< (OStream & os, sstring_view<PointerType> const & str) {
    auto const size = std::min (str.length (), static_cast<std::make_unsigned_t<std::streamsize>> (
                                                 std::numeric_limits<std::streamsize>::max ()));
    os.write (str.data (), static_cast<std::streamsize> (size));
    return os;
  }


  using shared_sstring_view = sstring_view<std::shared_ptr<char const>>;
  using unique_sstring_view = sstring_view<std::unique_ptr<char const>>;
  using raw_sstring_view = sstring_view<gsl::czstring>;

  //*             _               _       _                 _             *
  //*  _ __  __ _| |_____   _____| |_ _ _(_)_ _  __ _  __ _(_)_____ __ __ *
  //* | '  \/ _` | / / -_) (_-<_-<  _| '_| | ' \/ _` | \ V / / -_) V  V / *
  //* |_|_|_\__,_|_\_\___| /__/__/\__|_| |_|_||_\__, |  \_/|_\___|\_/\_/  *
  //*                                           |___/                     *
  template <typename ValueType>
  inline sstring_view<std::shared_ptr<ValueType>>
  make_shared_sstring_view (std::shared_ptr<ValueType> const & ptr, std::size_t length) {
    return {ptr, length};
  }

  shared_sstring_view make_shared_sstring_view (std::string_view str);

  template <typename ValueType>
  inline sstring_view<std::unique_ptr<ValueType>>
  make_unique_sstring_view (std::unique_ptr<ValueType> && ptr, std::size_t length) {
    return {std::move (ptr), length};
  }

  template <std::ptrdiff_t Extent>
  inline raw_sstring_view make_sstring_view (gsl::span<char const, Extent> const & span) {
    PSTORE_ASSERT (span.size () >= 0);
    return {span.data (), unsigned_cast (span.size ())};
  }

  inline raw_sstring_view make_sstring_view (std::string_view str) {
    return {str.data (), str.length ()};
  }

} // namespace pstore

namespace std {

  template <typename StringType>
  struct equal_to<pstore::sstring_view<StringType>> {
    template <typename S1, typename S2>
    bool operator() (S1 const & x, S2 const & y) const {
      return x == y;
    }
  };

  template <typename StringType>
  struct hash<pstore::sstring_view<StringType>> {
    size_t operator() (pstore::sstring_view<StringType> const & str) const {
      return static_cast<size_t> (
        pstore::fnv_64a_buf (pstore::gsl::make_span (str.begin (), str.end ())));
    }
  };

} // namespace std

#endif // PSTORE_ADT_SSTRING_VIEW_HPP
