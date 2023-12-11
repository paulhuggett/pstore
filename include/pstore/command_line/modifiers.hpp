//===- include/pstore/command_line/modifiers.hpp ----------*- mode: C++ -*-===//
//*                      _ _  __ _                *
//*  _ __ ___   ___   __| (_)/ _(_) ___ _ __ ___  *
//* | '_ ` _ \ / _ \ / _` | | |_| |/ _ \ '__/ __| *
//* | | | | | | (_) | (_| | |  _| |  __/ |  \__ \ *
//* |_| |_| |_|\___/ \__,_|_|_| |_|\___|_|  |___/ *
//*                                               *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_COMMAND_LINE_MODIFIERS_HPP
#define PSTORE_COMMAND_LINE_MODIFIERS_HPP

#include "pstore/adt/small_vector.hpp"
#include "pstore/command_line/category.hpp"
#include "pstore/command_line/option.hpp"
#include "pstore/command_line/parser.hpp"

namespace pstore::command_line {

  //*           _              *
  //* __ ____ _| |_  _ ___ ___ *
  //* \ V / _` | | || / -_|_-< *
  //*  \_/\__,_|_|\_,_\___/__/ *
  //*                          *
  //===----------------------------------------------------------------------===//
  // Enum valued command line option
  //

  // values - For custom data types, allow specifying a group of values together
  // as the values that go into the mapping that the option handler uses.
  // This represents a single enum value.

  /// Represents the connection between a user-visible name and an internal value.
  template <typename T>
  struct literal {
    /// \param name  The user-visible name of the option value.
    /// \param value  The internal value associated with the name \p name.
    /// \param description  A description for the help output.
    literal (std::string_view const name, T const value, std::string_view const description)
            : name_{name}
            , value_{value}
            , description_{description} {}
    /// \param name  The user-visible name of the option value.
    /// \param value  The internal value associated with the name \p name.
    literal (std::string_view const name, T const value)
            : literal (name, value, name) {}
    std::string const & name () const noexcept { return name_; }
    T const & value () const noexcept { return value_; }
    std::string const & description () const noexcept { return description_; }

  private:
    std::string name_;
    T value_{};
    std::string description_;
  };

  template <typename T>
  literal (std::string_view n, T v, std::string_view d) -> literal<T>;
  template <typename T>
  literal (std::string_view n, T v) -> literal<T>;

  namespace details {

    /// The type of an option's parser() member function. Intended to be used in a SFINAE context.
    template <typename Option>
    using parser_t = decltype (std::declval<Option> ().parser ());
    template <typename Option, typename = std::void_t<>>
    struct has_parser : std::false_type {};
    template <typename Option>
    struct has_parser<Option, std::void_t<parser_t<Option>>> : std::true_type {};
    /// A boolean which indicates whether the option has a parser() member function.
    template <typename Option>
    inline constexpr bool has_parser_v = has_parser<Option>::value;

    /// A modifier type that accepts a sequence of literals and passes them to the parser associated
    /// with an option instance.
    template <typename T>
    class values {
    public:
      explicit values (std::initializer_list<literal<T>> literals)
              : literals_{literals} {}

      /// \tparam Option A subclass of option which implements the parser()
      ///   member function.
      /// \param option  An option instance.
      /// \returns  A reference to \p opt.
      template <typename Option, typename = std::enable_if_t<std::is_base_of_v<option, Option> &&
                                                             has_parser_v<Option>>>
      Option & apply (Option & option) const {
        auto & p = option.parser ();
        for (auto const & v : literals_) {
          p.add_literal (v.name (), v.value (), v.description ());
        }
        return option;
      }

    private:
      small_vector<literal<T>, 3> literals_;
    };

    template <typename T>
    values (std::initializer_list<literal<T>>) -> values<T>;

  } // end namespace details

  /// Helper to build a values collection by forwarding a variable number of arguments
  /// as an initializer list to the details::values constructor.
  template <typename... Literals>
  auto values (Literals &&... literals) {
    return details::values{std::forward<Literals> (literals)...};
  }

  template <typename T>
  inline auto values (std::initializer_list<literal<T>> literals) {
    return details::values (literals);
  }

  //*                       *
  //*  _ _  __ _ _ __  ___  *
  //* | ' \/ _` | '  \/ -_) *
  //* |_||_\__,_|_|_|_\___| *
  //*                       *
  /// A modifier which sets the name of an option.
  class name {
  public:
    explicit name (std::string_view name)
            : name_{name} {}
    /// \tparam Option A subclass of option.
    /// \param option  An option instance.
    /// \returns  A reference to \p option.
    template <typename Option, typename = std::enable_if_t<std::is_base_of_v<option, Option>>>
    Option & apply (Option & option) const {
      option.set_name (name_);
      return option;
    }

  private:
    std::string name_;
  };

  //*                          *
  //*  _  _ ___ __ _ __ _ ___  *
  //* | || (_-</ _` / _` / -_) *
  //*  \_,_/__/\__,_\__, \___| *
  //*               |___/      *
  /// A modifier to set the usage information shown in the -help output.
  /// Only applicable to positional arguments.
  class usage {
  public:
    explicit usage (std::string str)
            : usage_{std::move (str)} {}
    /// \tparam Option A subclass of option.
    /// \param option  An option instance.
    /// \returns  A reference to \p option.
    template <typename Option, typename = std::enable_if_t<std::is_base_of_v<option, Option>>>
    Option & apply (Option & option) const {
      option.set_usage (usage_);
      return option;
    }

  private:
    std::string usage_;
  };

  //*     _             *
  //*  __| |___ ___ __  *
  //* / _` / -_|_-</ _| *
  //* \__,_\___/__/\__| *
  //*                   *
  /// A modifier to set the description shown in the -help output.
  class desc {
  public:
    explicit desc (std::string str)
            : desc_{std::move (str)} {}
    /// \tparam Option A subclass of option.
    /// \param option  An option instance.
    /// \returns  A reference to \p option.
    template <typename Option, typename = std::enable_if_t<std::is_base_of_v<option, Option>>>
    Option & apply (Option & option) const {
      option.set_description (desc_);
      return option;
    }

  private:
    std::string const desc_;
  };

  //*  _      _ _    *
  //* (_)_ _ (_) |_  *
  //* | | ' \| |  _| *
  //* |_|_||_|_|\__| *
  //*                *
  /// A modifier to set the initial (default) value of an option.
  template <typename T>
  class init {
  public:
    constexpr explicit init (T const & t)
            : init_{t} {}
    /// \tparam Option A subclass of option.
    /// \param option  An option instance.
    /// \returns  A reference to \p option.
    template <typename Option, typename = std::enable_if_t<std::is_base_of_v<option, Option>>>
    Option & apply (Option & option) const {
      option.set_initial_value (init_);
      return option;
    }

  private:
    T init_;
  };

  template <typename T>
  init (T const & t) -> init<T>;


  namespace details {

    struct comma_separated {
      /// \tparam Option A subclass of option.
      /// \param option  An option instance.
      /// \returns  A reference to \p option.
      template <typename Option, typename = std::enable_if_t<std::is_base_of_v<option, Option>>>
      Option & apply (Option & option) const {
        option.set_comma_separated ();
        return option;
      }
    };

  } // end namespace details

  /// When this modifier is added to a list option, it will consider each of the argument
  /// strings to be a sequence of one or more comma-separated values. These are broken
  /// apart before being passed to the argument parser. The modifier has no effect on
  /// other option types.
  ///
  /// For example, a list option named "opt" with comma-separated
  /// enabled will consider command-lines such as "--opt a,b,c", "--opt a,b --opt c", and
  /// "--opt a --opt b --opt c" to be equivalent. Without the option "--opt a,b" is has a
  /// single value "a,b".
  constexpr details::comma_separated const comma_separated;

  /// A modifier which sets the help category of an option. The help output groups options
  /// from the same category together to help the user understand how different options are
  /// related and affect one another.
  class category {
  public:
    explicit constexpr category (option_category const & cat) noexcept
            : cat_{cat} {}
    /// \tparam Option A subclass of option.
    /// \param option  An option instance.
    /// \returns  A reference to \p option.
    template <typename Option, typename = std::enable_if_t<std::is_base_of_v<option, Option>>>
    Option & apply (Option & option) const {
      option.set_category (&cat_);
      return option;
    }

  private:
    option_category const & cat_;
  };


  template <typename Modifier>
  decltype (auto) make_modifier (Modifier && m) {
    return std::forward<Modifier> (m);
  }
  inline name make_modifier (std::string_view n) {
    return name{n};
  }
  inline name make_modifier (std::string const & n) {
    return name{n};
  }
  inline name make_modifier (char const * n) {
    return name{n};
  }

  template <typename Option>
  std::enable_if_t<is_option<Option>, void> apply_modifiers_to_option (Option &&) {}

  template <typename Option, typename Modifier, typename... Mods>
  std::enable_if_t<is_option<Option> && !is_option<Modifier>, void>
  apply_modifiers_to_option (Option && opt, Modifier && m0, Mods &&... mods) {
    make_modifier (std::forward<Modifier> (m0)).apply (opt);
    apply_modifiers_to_option (std::forward<Option> (opt), std::forward<Mods> (mods)...);
  }

} // namespace pstore::command_line

#endif // PSTORE_COMMAND_LINE_MODIFIERS_HPP
