//===- include/pstore/support/maybe.hpp -------------------*- mode: C++ -*-===//
//*                        _           *
//*  _ __ ___   __ _ _   _| |__   ___  *
//* | '_ ` _ \ / _` | | | | '_ \ / _ \ *
//* | | | | | | (_| | |_| | |_) |  __/ *
//* |_| |_| |_|\__,_|\__, |_.__/ \___| *
//*                  |___/             *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file maybe.hpp
/// \brief An implementation of the Haskell Maybe type.
/// In Haskell, this simply looks like:
///    data Maybe a = Just a | Nothing
/// This is pretty much std::optional<> and this definition deliberately implements some of the
/// methods of that type, so we should switch to the standard type once we're able to migrate to
/// C++17.

#ifndef PSTORE_SUPPORT_MAYBE_HPP
#define PSTORE_SUPPORT_MAYBE_HPP

#include <algorithm>
#include <new>
#include <optional>
#include <stdexcept>

#include "pstore/support/assert.hpp"
#include "pstore/support/inherit_const.hpp"


namespace pstore {

  template <typename T, typename = std::enable_if_t<!std::is_reference_v<T>>>
  class maybe {
  public:
    using value_type = T;

    static_assert (std::is_object_v<value_type>,
                   "Instantiation of maybe<> with a non-object type is undefined behavior.");
    static_assert (std::is_nothrow_destructible_v<value_type>,
                   "Instantiation of maybe<> with an object type that is not noexcept "
                   "destructible is undefined behavior.");

    /// Constructs an object that does not contain a value.
    constexpr maybe () noexcept = default;

    /// Constructs an optional object that contains a value, initialized with the expression
    /// std::forward<U>(value).
    template <typename U, typename = std::enable_if_t<std::is_constructible_v<T, U &&> &&
                                                      !std::is_same_v<remove_cvref_t<U>, maybe<T>>>>
    explicit maybe (U && value) noexcept (std::is_nothrow_move_constructible_v<T> &&
                                          std::is_nothrow_copy_constructible_v<T> &&
                                          !std::is_convertible_v<U &&, T>)
            : valid_{true} {

      new (&storage_) T (std::forward<U> (value));
    }

    template <typename... Args>
    explicit maybe (std::in_place_t const inp, Args &&... args)
            : valid_{true} {
      (void) inp;
      new (&storage_) T (std::forward<Args> (args)...);
    }

    /// Copy constructor: If other contains a value, initializes the contained value with the
    /// expression *other. If other does not contain a value, constructs an object that does not
    /// contain a value.
    maybe (maybe const & other) noexcept (std::is_nothrow_copy_constructible_v<T>) {
      if (other.valid_) {
        new (&storage_) T (*other);
        valid_ = true;
      }
    }

    /// Move constructor: If other contains a value, initializes the contained value with the
    /// expression std::move(*other) and does not make other empty: a moved-from optional still
    /// contains a value, but the value itself is moved from. If other does not contain a value,
    /// constructs an object that does not contain a value.
    maybe (maybe && other) noexcept (std::is_nothrow_move_constructible_v<T>) {
      if (other.valid_) {
        new (&storage_) T (std::move (*other));
        valid_ = true;
      }
    }

    ~maybe () noexcept { this->reset (); }

    /// If *this contains a value, destroy it. *this does not contain a value after this call.
    void reset () noexcept {
      if (valid_) {
        // Set valid_ to false before calling the dtor.
        valid_ = false;
        reinterpret_cast<T const *> (&storage_)->~T ();
      }
    }

    maybe & operator= (maybe const & other) noexcept (
      std::is_nothrow_copy_assignable_v<T> && std::is_nothrow_copy_constructible_v<T>) {
      if (&other != this) {
        if (!other.has_value ()) {
          this->reset ();
        } else {
          this->operator= (other.value ());
        }
      }
      return *this;
    }

    maybe & operator= (maybe && other) noexcept (
      std::is_nothrow_move_assignable_v<T> && std::is_nothrow_move_constructible_v<T>) {

      if (&other != this) {
        if (!other.has_value ()) {
          this->reset ();
        } else {
          this->operator= (std::forward<T> (other.value ()));
        }
      }
      return *this;
    }


    template <typename U, typename = std::enable_if_t<std::is_constructible_v<T, U &&> &&
                                                      !std::is_same_v<remove_cvref_t<U>, maybe<T>>>>
    maybe & operator= (U && other) noexcept (std::is_nothrow_copy_assignable_v<T> &&
                                             std::is_nothrow_copy_constructible_v<T> &&
                                             std::is_nothrow_move_assignable_v<T> &&
                                             std::is_nothrow_move_constructible_v<T>) {

      if (valid_) {
        T temp = std::forward<U> (other);
        std::swap (this->value (), temp);
      } else {
        new (&storage_) T (std::forward<U> (other));
        valid_ = true;
      }
      return *this;
    }

    /// Constructs the contained value in-place. If *this already contains a value before the
    /// call, the contained value is destroyed by calling its destructor.
    ///
    /// \p args... The arguments to pass to the constructor
    /// \returns A reference to the new contained value.
    template <typename... Args>
    T & emplace (Args &&... args) {
      if (valid_) {
        T temp (std::forward<Args> (args)...);
        std::swap (this->value (), temp);
      } else {
        new (&storage_) T (std::forward<Args> (args)...);
        valid_ = true;
      }
      return this->value ();
    }

    bool operator== (maybe const & other) const {
      return this->has_value () == other.has_value () &&
             (!this->has_value () || this->value () == other.value ());
    }
    bool operator!= (maybe const & other) const { return !operator== (other); }

    /// accesses the contained value
    T const & operator* () const noexcept { return *(operator->()); }
    /// accesses the contained value
    T & operator* () noexcept { return *(operator->()); }
    /// accesses the contained value
    T const * operator->() const noexcept {
      PSTORE_ASSERT (valid_);
      return reinterpret_cast<T const *> (&storage_);
    }
    /// accesses the contained value
    T * operator->() noexcept {
      PSTORE_ASSERT (valid_);
      return reinterpret_cast<T *> (&storage_);
    }

    /// checks whether the object contains a value
    constexpr explicit operator bool () const noexcept { return valid_; }
    /// checks whether the object contains a value
    constexpr bool has_value () const noexcept { return valid_; }

    T const & value () const noexcept { return value_impl (*this); }
    T & value () noexcept { return value_impl (*this); }

    template <typename U>
    constexpr T value_or (U && default_value) const {
      return this->has_value () ? this->value () : default_value;
    }

  private:
    template <typename Maybe, typename ResultType = inherit_const_t<Maybe, T>>
    static ResultType & value_impl (Maybe && m) noexcept {
      PSTORE_ASSERT (m.has_value ());
      return *m;
    }

    bool valid_ = false;
    typename std::aligned_storage_t<sizeof (T), alignof (T)> storage_{};
  };


  // just
  // ~~~~
  template <typename T>
  constexpr decltype (auto) just (T && value) {
    return maybe<remove_cvref_t<T>>{std::forward<T> (value)};
  }

  template <typename T, typename... Args>
  constexpr decltype (auto) just (std::in_place_t const inp, Args &&... args) {
    (void) inp;
    return maybe<remove_cvref_t<T>>{std::in_place, std::forward<Args> (args)...};
  }

  // nothing
  // ~~~~~~~
  template <typename T>
  constexpr decltype (auto) nothing () noexcept {
    return maybe<remove_cvref_t<T>>{};
  }

  /// The monadic "bind" operator for maybe<T>. If \p t is "nothing", then returns nothing where
  /// the type of the return is derived from the return type of \p f.  If \p t has a value then
  /// returns the result of calling \p f.
  ///
  /// \tparam T  The input type wrapped by a maybe<>.
  /// \tparam Function  A callable object whose signature is of the form `maybe<U> f(T t)`.
  template <typename T, typename Function>
  auto operator>>= (maybe<T> && t, Function f) -> decltype (f (*t)) {
    if (t) {
      return f (*t);
    }
    return maybe<typename decltype (f (*t))::value_type>{};
  }

  /// The monadic "bind" operator for std::optional<T>. If \p t is "nothing", then returns nothing
  /// where the type of the return is derived from the return type of \p f.  If \p t has a value
  /// then returns the result of calling \p f.
  ///
  /// \tparam T  The input type wrapped by a maybe<>.
  /// \tparam Function  A callable object whose signature is of the form `std::optional<U> f(T t)`.
  template <typename T, typename Function>
  auto operator>>= (std::optional<T> && t, Function f) -> decltype (f (*t)) {
    if (t) {
      return f (*t);
    }
    return std::optional<typename decltype (f (*t))::value_type>{};
  }

} // namespace pstore

#endif // PSTORE_SUPPORT_MAYBE_HPP
