//===- include/pstore/command_line/option.hpp -------------*- mode: C++ -*-===//
//*              _   _              *
//*   ___  _ __ | |_(_) ___  _ __   *
//*  / _ \| '_ \| __| |/ _ \| '_ \  *
//* | (_) | |_) | |_| | (_) | | | | *
//*  \___/| .__/ \__|_|\___/|_| |_| *
//*       |_|                       *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_COMMAND_LINE_OPTION_HPP
#define PSTORE_COMMAND_LINE_OPTION_HPP

#include "pstore/command_line/csv.hpp"
#include "pstore/command_line/parser.hpp"

namespace pstore::command_line {

  template <typename T, typename = void>
  struct type_description {};
  template <>
  struct type_description<std::string> {
    static gsl::czstring value;
  };
  template <>
  struct type_description<int> {
    static gsl::czstring value;
  };
  template <>
  struct type_description<long> {
    static gsl::czstring value;
  };
  template <>
  struct type_description<long long> {
    static gsl::czstring value;
  };
  template <>
  struct type_description<unsigned short> {
    static gsl::czstring value;
  };
  template <>
  struct type_description<unsigned int> {
    static gsl::czstring value;
  };
  template <>
  struct type_description<unsigned long> {
    static gsl::czstring value;
  };
  template <>
  struct type_description<unsigned long long> {
    static gsl::czstring value;
  };
  template <typename T>
  struct type_description<T, typename std::enable_if_t<std::is_enum_v<T>>> {
    static gsl::czstring value;
  };
  template <typename T>
  gsl::czstring type_description<T, typename std::enable_if_t<std::is_enum_v<T>>>::value = "enum";

  enum class num_occurrences_flag {
    optional,     // Zero or One occurrence
    zero_or_more, // Zero or more occurrences allowed
    required,     // One occurrence required
    one_or_more,  // One or more occurrences required
  };
  class option_category;
  class alias;

  class option;
  class options_container {
  public:
    using value_type = std::unique_ptr<option>;

    options_container () noexcept = default;
    options_container (options_container const &) = delete;
    options_container (options_container &&) noexcept = delete;
    ~options_container () noexcept = default;

    options_container & operator= (options_container const &) = delete;
    options_container & operator= (options_container &&) noexcept = delete;


    template <typename OptionType, typename... Args>
    auto & add (Args &&... args) {
      auto p = std::make_unique<OptionType> (std::forward<Args> (args)...);
      auto & result = *p;
      opts_.emplace_back (std::move (p));
      return result;
    }

    auto begin () const { return std::begin (opts_); }
    auto end () const { return std::end (opts_); }

    std::list<value_type> opts_;
  };

  //*           _   _           *
  //*  ___ _ __| |_(_)___ _ _   *
  //* / _ \ '_ \  _| / _ \ ' \  *
  //* \___/ .__/\__|_\___/_||_| *
  //*     |_|                   *
  class option {
  public:
    option (option const &) = delete;
    option (option &&) noexcept = delete;
    option & operator= (option const &) = delete;
    option & operator= (option &&) noexcept = delete;
    virtual ~option ();

    virtual void set_num_occurrences_flag (num_occurrences_flag n);
    virtual num_occurrences_flag get_num_occurrences_flag () const;

    virtual unsigned get_num_occurrences () const;
    bool is_satisfied () const;
    bool can_accept_another_occurrence () const;


    virtual void set_description (std::string const & d);
    std::string const & description () const noexcept;

    virtual void set_usage (std::string const & d);
    std::string const & usage () const noexcept;

    void set_comma_separated () noexcept { comma_separated_ = true; }
    bool allow_comma_separated () const noexcept { return comma_separated_; }

    void set_category (option_category const * const cat) { category_ = cat; }
    virtual option_category const * category () const noexcept { return category_; }

    virtual void set_positional ();
    virtual bool is_positional () const;

    virtual bool is_alias () const;
    virtual alias * as_alias ();
    virtual alias const * as_alias () const;

    virtual parser_base * get_parser () = 0;
    virtual parser_base const * get_parser () const = 0;

    std::string const & name () const;
    void set_name (std::string const & name);

    virtual bool takes_argument () const = 0;
    virtual bool value (std::string const & v) = 0;
    virtual bool add_occurrence ();

    virtual gsl::czstring arg_description () const noexcept;

  protected:
    option () = default;
    explicit option (num_occurrences_flag occurrences);

  private:
    std::string name_;
    std::string usage_;
    std::string description_;
    num_occurrences_flag occurrences_ = num_occurrences_flag::optional;
    bool positional_ = false;
    bool comma_separated_ = false;
    unsigned num_occurrences_ = 0U;
    option_category const * category_ = nullptr;
  };

  template <typename Modifier>
  Modifier && make_modifier (Modifier && m) {
    return std::forward<Modifier> (m);
  }

  class name;
  name make_modifier (gsl::czstring n);
  name make_modifier (std::string const & n);

  template <typename Option>
  void apply_to_option (Option &&) {}

  template <typename Option, typename M0, typename... Mods>
  void apply_to_option (Option && opt, M0 && m0, Mods &&... mods) {
    make_modifier (std::forward<M0> (m0)).apply (opt);
    apply_to_option (std::forward<Option> (opt), std::forward<Mods> (mods)...);
  }


  //*           _    *
  //*  ___ _ __| |_  *
  //* / _ \ '_ \  _| *
  //* \___/ .__/\__| *
  //*     |_|        *
  /// \tparam T The type produced by this option.
  /// \tparam Parser The parser which will convert from the user's string to type T.
  template <typename T, typename Parser = parser<T>>
  class opt final : public option {
  public:
    template <class... Mods>
    explicit opt (Mods const &... mods) {
      apply_to_option (*this, mods...);
    }
    opt (opt const &) = delete;
    opt (opt &&) = delete;
    ~opt () noexcept override = default;

    opt & operator= (opt const &) = delete;
    opt & operator= (opt &&) = delete;

    template <typename U>
    void set_initial_value (U const & u) {
      value_ = u;
    }

    explicit operator T () const { return get (); }
    T const & get () const noexcept { return value_; }

    bool empty () const { return std::empty (value_); }
    bool value (std::string const & v) override;
    bool takes_argument () const override;
    parser_base * get_parser () override;
    parser_base const * get_parser () const override;

    gsl::czstring arg_description () const noexcept override { return type_description<T>::value; }

  private:
    T value_{};
    Parser parser_;
  };

  template <typename T, typename Parser>
  bool opt<T, Parser>::takes_argument () const {
    return true;
  }
  template <typename T, typename Parser>
  bool opt<T, Parser>::value (std::string const & v) {
    if (auto m = parser_ (v)) {
      value_ = m.value ();
      return true;
    }
    return false;
  }
  template <typename T, typename Parser>
  parser_base * opt<T, Parser>::get_parser () {
    return &parser_;
  }
  template <typename T, typename Parser>
  parser_base const * opt<T, Parser>::get_parser () const {
    return &parser_;
  }


  //*           _     _              _  *
  //*  ___ _ __| |_  | |__  ___  ___| | *
  //* / _ \ '_ \  _| | '_ \/ _ \/ _ \ | *
  //* \___/ .__/\__| |_.__/\___/\___/_| *
  //*     |_|                           *
  template <>
  class opt<bool> final : public option {
  public:
    template <class... Mods>
    explicit opt (Mods const &... mods) {
      apply_to_option (*this, mods...);
    }
    opt (opt const &) = delete;
    opt (opt &&) = delete;
    ~opt () noexcept override = default;

    opt & operator= (opt const &) = delete;
    opt & operator= (opt &&) = delete;

    explicit operator bool () const noexcept { return get (); }
    bool get () const noexcept { return value_; }

    bool takes_argument () const override { return false; }
    bool value (std::string const & v) override;
    bool add_occurrence () override;
    parser_base * get_parser () override;
    parser_base const * get_parser () const override;

    template <typename U>
    void set_initial_value (U const & u) {
      value_ = u;
    }

  private:
    bool value_ = false;
  };


  using string_opt = opt<std::string>;
  using bool_opt = opt<bool>;

  //*  _ _    _    *
  //* | (_)__| |_  *
  //* | | (_-<  _| *
  //* |_|_/__/\__| *
  //*              *
  template <typename T, typename Parser = parser<T>>
  class list final : public option {
    using container = std::list<T>;

  public:
    using value_type = T;

    template <class... Mods>
    explicit list (Mods const &... mods)
            : option (num_occurrences_flag::zero_or_more) {
      apply_to_option (*this, mods...);
    }

    list (list const &) = delete;
    list (list &&) = delete;
    ~list () noexcept override = default;

    list & operator= (list const &) = delete;
    list & operator= (list &&) = delete;

    bool takes_argument () const override { return true; }
    bool value (std::string const & v) override;
    parser_base * get_parser () override { return &parser_; }
    parser_base const * get_parser () const override { return &parser_; }

    using iterator = typename container::const_iterator;
    using const_iterator = iterator;

    std::list<T> const & get () const noexcept { return values_; }

    const_iterator begin () const { return std::begin (values_); }
    const_iterator end () const { return std::end (values_); }
    std::size_t size () const { return values_.size (); }
    bool empty () const { return values_.empty (); }

    gsl::czstring arg_description () const noexcept override { return type_description<T>::value; }

  private:
    bool comma_separated (std::string const & v);
    bool simple_value (std::string const & v);

    Parser parser_;
    std::list<T> values_;
  };

  // value
  // ~~~~~
  template <typename T, typename Parser>
  bool list<T, Parser>::value (std::string const & v) {
    if (this->allow_comma_separated ()) {
      return this->comma_separated (v);
    }

    return this->simple_value (v);
  }

  // comma separated
  // ~~~~~~~~~~~~~~~
  template <typename T, typename Parser>
  bool list<T, Parser>::comma_separated (std::string const & v) {
    std::list<std::string> vl = csv (v);
    return std::all_of (std::begin (vl), std::end (vl), [this] (std::string const & subvalue) {
      return this->simple_value (subvalue);
    });
  }

  // simple value
  // ~~~~~~~~~~~~
  template <typename T, typename Parser>
  bool list<T, Parser>::simple_value (std::string const & v) {
    if (std::optional<T> const m = parser_ (v)) {
      values_.push_back (m.value ());
      return true;
    }
    return false;
  }

  //*       _ _          *
  //*  __ _| (_)__ _ ___ *
  //* / _` | | / _` (_-< *
  //* \__,_|_|_\__,_/__/ *
  //*                    *
  class alias final : public option {
  public:
    template <typename... Mods>
    explicit alias (Mods const &... mods) {
      apply_to_option (*this, mods...);
    }

    alias (alias const &) = delete;
    alias (alias &&) = delete;
    ~alias () noexcept override = default;

    alias & operator= (alias const &) = delete;
    alias & operator= (alias &&) = delete;

    alias * as_alias () override { return this; }
    alias const * as_alias () const override { return this; }

    option_category const * category () const noexcept override { return original_->category (); }

    bool add_occurrence () override;
    void set_num_occurrences_flag (num_occurrences_flag n) override;
    num_occurrences_flag get_num_occurrences_flag () const override;
    void set_positional () override;
    bool is_positional () const override;
    bool is_alias () const override;
    unsigned get_num_occurrences () const override;
    parser_base * get_parser () override;
    parser_base const * get_parser () const override;
    bool takes_argument () const override;
    bool value (std::string const & v) override;

    gsl::czstring arg_description () const noexcept override {
      return original_->arg_description ();
    }

    void set_original (option * o);
    option const * original () const { return original_; }

  private:
    option * original_ = nullptr;
  };

} // end namespace pstore::command_line

#endif // PSTORE_COMMAND_LINE_OPTION_HPP
