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
#include "pstore/command_line/literal.hpp"
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
  namespace details {

    class values {
    public:
      explicit values (std::initializer_list<literal> options)
              : values_{std::move (options)} {}

      template <typename Opt>
      Opt & apply (Opt & opt) const {
        if (parser_base * const p = opt.get_parser ()) {
          for (auto const & v : values_) {
            p->add_literal_option (v.name, v.value, v.description);
          }
        }
        return opt;
      }

    private:
      small_vector<literal, 3> values_;
    };

  } // end namespace details

  /// Helper to build a values collection by forwarding a variable number of arguments
  /// as an initializer list to the details::values constructor.
  template <typename... OptsTy>
  details::values values (OptsTy &&... options) {
    return details::values{std::forward<OptsTy> (options)...};
  }
  inline details::values values (std::initializer_list<literal> options) {
    return details::values (options);
  }

  class name {
  public:
    explicit name (std::string_view name)
            : name_{name} {}
    template <typename Opt>
    Opt & apply (Opt & opt) const {
      opt.set_name (name_);
      return opt;
    }

  private:
    std::string name_;
  };

  /// A modifier to set the usage information shown in the -help output.
  /// Only applicable to positional arguments.
  class usage {
  public:
    explicit usage (std::string str)
            : usage_{std::move (str)} {}
    template <typename Opt>
    Opt & apply (Opt & opt) const {
      opt.set_usage (usage_);
      return opt;
    }

  private:
    std::string usage_;
  };

  //*     _             *
  //*  __| |___ ___ __  *
  //* / _` / -_|_-</ _| *
  //* \__,_\___/__/\__| *
  //*                   *
  /// A modifier to set the description shown in the -help output...
  class desc {
  public:
    explicit desc (std::string str)
            : desc_{std::move (str)} {}
    template <typename Opt>
    Opt & apply (Opt & opt) const {
      opt.set_description (desc_);
      return opt;
    }

  private:
    std::string const desc_;
  };

  //*  _      _ _    *
  //* (_)_ _ (_) |_  *
  //* | | ' \| |  _| *
  //* |_|_||_|_|\__| *
  //*                *
  template <typename T>
  class init {
  public:
    constexpr explicit init (T const & t)
            : init_{t} {}
    template <typename Opt>
    Opt & apply (Opt & opt) const {
      opt.set_initial_value (init_);
      return opt;
    }

  private:
    T init_;
  };

  template <typename T>
  init (T const & t) -> init<T>;


  namespace details {

    struct comma_separated {
      template <typename Opt>
      Opt & apply (Opt & opt) const {
        opt.set_comma_separated ();
        return opt;
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


  namespace details {

    class category {
    public:
      explicit constexpr category (option_category const & cat) noexcept
              : cat_{cat} {}
      template <typename Opt>
      Opt & apply (Opt & opt) const {
        opt.set_category (&cat_);
        return opt;
      }

    private:
      option_category const & cat_;
    };

  } // end namespace details

  inline details::category cat (option_category const & c) {
    return details::category{c};
  }

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
  void apply_to_option (Option &&) {}

  template <typename Option, typename M0, typename... Mods>
  void apply_to_option (Option && opt, M0 && m0, Mods &&... mods) {
    make_modifier (std::forward<M0> (m0)).apply (opt);
    apply_to_option (std::forward<Option> (opt), std::forward<Mods> (mods)...);
  }

} // namespace pstore::command_line

#endif // PSTORE_COMMAND_LINE_MODIFIERS_HPP
