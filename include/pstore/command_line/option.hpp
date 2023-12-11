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
#include "pstore/command_line/type_description.hpp"

namespace pstore::command_line {

  enum class occurrences_flag {
    optional,     ///< Zero or One occurrence
    zero_or_more, ///< Zero or more occurrences allowed
    required,     ///< One occurrence required
    one_or_more,  ///< One or more occurrences required
  };

  class alias;
  class option_category;

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
    virtual ~option () noexcept = default;

    virtual void set_occurrences_flag (occurrences_flag n) { occurrences_ = n; }
    virtual occurrences_flag get_occurrences_flag () const { return occurrences_; }

    virtual unsigned get_num_occurrences () const { return num_occurrences_; }
    bool is_satisfied () const;
    bool can_accept_another_occurrence () const;

    virtual void set_description (std::string_view d) { description_ = d; }
    std::string const & description () const noexcept { return description_; }

    virtual void set_usage (std::string_view d) { usage_ = d; }
    std::string const & usage () const noexcept { return usage_; }

    void set_comma_separated () noexcept { comma_separated_ = true; }
    bool allow_comma_separated () const noexcept { return comma_separated_; }

    void set_category (option_category const * const cat) { category_ = cat; }
    virtual option_category const * category () const noexcept { return category_; }

    virtual void set_positional () { positional_ = true; }
    virtual bool is_positional () const { return positional_; }

    virtual std::optional<std::reference_wrapper<alias const>> as_alias () const { return {}; }

    void set_name (std::string const & name) {
      PSTORE_ASSERT ((name.empty () || name[0] != '-') && "Option can't start with '-");
      name_ = name;
    }

    std::string const & name () const { return name_; }

    virtual bool takes_argument () const = 0;
    virtual bool value (std::string_view v) = 0;
    virtual bool add_occurrence () {
      ++num_occurrences_;
      return true;
    }

    virtual std::optional<std::string_view> arg_description () const noexcept {
      return std::nullopt;
    }

  protected:
    option () = default;
    explicit option (occurrences_flag occurrences) noexcept
            : occurrences_{occurrences} {}

  private:
    std::string name_;
    std::string usage_;
    std::string description_;
    occurrences_flag occurrences_ = occurrences_flag::optional;
    bool positional_ = false;
    bool comma_separated_ = false;
    unsigned num_occurrences_ = 0U;
    option_category const * category_ = nullptr;
  };

  inline bool option::is_satisfied () const {
    bool result = true;
    switch (this->get_occurrences_flag ()) {
    case occurrences_flag::required: result = num_occurrences_ >= 1U; break;
    case occurrences_flag::one_or_more: result = num_occurrences_ > 1U; break;
    case occurrences_flag::optional:
    case occurrences_flag::zero_or_more: break;
    }
    return result;
  }

  inline bool option::can_accept_another_occurrence () const {
    bool result = true;
    switch (this->get_occurrences_flag ()) {
    case occurrences_flag::optional:
    case occurrences_flag::required: result = num_occurrences_ == 0U; break;
    case occurrences_flag::zero_or_more:
    case occurrences_flag::one_or_more: break;
    }
    return result;
  }

  /// Checks whether Option is a type derived from pstore::command_line::option.
  template <typename Option>
  inline constexpr bool is_option =
    std::is_base_of_v<option, std::remove_reference_t<std::remove_cv_t<Option>>>;

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
    template <typename... Mods>
    explicit opt (Mods &&... mods) {
      apply_modifiers_to_option (*this, std::forward<Mods> (mods)...);
    }
    opt (opt const &) = delete;
    opt (opt &&) noexcept = delete;
    ~opt () noexcept override = default;

    opt & operator= (opt const &) = delete;
    opt & operator= (opt &&) noexcept = delete;

    template <typename U>
    void set_initial_value (U const & u) {
      value_ = u;
    }

    explicit operator T () const { return get (); }
    T const & get () const noexcept { return value_; }

    bool empty () const { return std::empty (value_); }
    bool value (std::string_view v) override;
    bool takes_argument () const override { return true; }

    Parser & parser () noexcept { return parser_; }
    Parser const & parser () const noexcept { return parser_; }

    std::optional<std::string_view> arg_description () const noexcept override { return meta_; }

    void set_meta (std::string_view meta) { meta_ = meta; }

  private:
    T value_{};
    Parser parser_{};
    std::string meta_ = type_description<T>::value;
  };

  template <typename T, typename Parser>
  bool opt<T, Parser>::value (std::string_view v) {
    if (auto m = parser_ (v)) {
      value_ = m.value ();
      return true;
    }
    return false;
  }

  class meta {
  public:
    explicit meta (std::string const & s) noexcept
            : s_{s} {}
    template <typename T>
    opt<T> & apply (opt<T> & opt) const {
      opt.set_meta (s_);
      return opt;
    }

  private:
    std::string s_;
  };


  //*           _     _              _  *
  //*  ___ _ __| |_  | |__  ___  ___| | *
  //* / _ \ '_ \  _| | '_ \/ _ \/ _ \ | *
  //* \___/ .__/\__| |_.__/\___/\___/_| *
  //*     |_|                           *
  template <>
  class opt<bool> final : public option {
  public:
    template <typename... Mods>
    explicit opt (Mods &&... mods) {
      apply_modifiers_to_option (*this, std::forward<Mods> (mods)...);
    }
    opt (opt const &) = delete;
    opt (opt &&) noexcept = delete;
    ~opt () noexcept override;

    opt & operator= (opt const &) = delete;
    opt & operator= (opt &&) noexcept = delete;

    explicit operator bool () const noexcept { return get (); }
    bool get () const noexcept { return value_; }

    bool takes_argument () const override { return false; }
    bool value (std::string_view v) override {
      (void) v;
      return false;
    }
    bool add_occurrence () override;

    template <typename U>
    void set_initial_value (U const & u) {
      value_ = u;
    }

  private:
    bool value_ = false;
  };


  using string_opt = opt<std::string>;
  using bool_opt = opt<bool>;
  using unsigned_opt = opt<unsigned>;

  //*  _ _    _    *
  //* | (_)__| |_  *
  //* | | (_-<  _| *
  //* |_|_/__/\__| *
  //*              *
  template <typename T, typename Parser = parser<T>>
  class list final : public option {
  public:
    using value_type = T;

    template <typename... Mods>
    explicit list (Mods &&... mods)
            : option (occurrences_flag::zero_or_more) {
      apply_modifiers_to_option (*this, std::forward<Mods> (mods)...);
    }

    list (list const &) = delete;
    list (list &&) noexcept = delete;
    ~list () noexcept override = default;

    list & operator= (list const &) = delete;
    list & operator= (list &&) noexcept = delete;

    bool takes_argument () const override { return true; }
    bool value (std::string_view v) override;

    Parser & parser () noexcept { return parser_; }
    Parser const & parser () const noexcept { return parser_; }

    std::optional<std::string_view> arg_description () const noexcept override {
      return type_description<T>::value;
    }

    /// Results access
    auto begin () const { return std::begin (values_); }
    auto end () const { return std::end (values_); }
    std::size_t size () const noexcept { return values_.size (); }
    bool empty () const noexcept { return values_.empty (); }

  private:
    bool comma_separated (std::string_view v);
    bool simple_value (std::string_view v);

    Parser parser_{};
    small_vector<T, 4> values_;
  };

  // value
  // ~~~~~
  template <typename T, typename Parser>
  bool list<T, Parser>::value (std::string_view v) {
    if (this->allow_comma_separated ()) {
      return this->comma_separated (v);
    }
    return this->simple_value (v);
  }

  // comma separated
  // ~~~~~~~~~~~~~~~
  template <typename T, typename Parser>
  bool list<T, Parser>::comma_separated (std::string_view v) {
    std::list<std::string> vl = csv (v);
    return std::all_of (std::begin (vl), std::end (vl), [this] (std::string const & subvalue) {
      return this->simple_value (subvalue);
    });
  }

  // simple value
  // ~~~~~~~~~~~~
  template <typename T, typename Parser>
  bool list<T, Parser>::simple_value (std::string_view v) {
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
    explicit alias (Mods &&... mods) {
      apply_modifiers_to_option (*this, std::forward<Mods> (mods)...);
    }

    alias (alias const &) = delete;
    alias (alias &&) noexcept = delete;
    ~alias () noexcept override = default;

    alias & operator= (alias const &) = delete;
    alias & operator= (alias &&) noexcept = delete;

    std::optional<std::reference_wrapper<alias const>> as_alias () const override { return *this; }

    option_category const * category () const noexcept override { return original_->category (); }

    bool add_occurrence () override;
    void set_occurrences_flag (occurrences_flag n) override;
    occurrences_flag get_occurrences_flag () const override;
    unsigned get_num_occurrences () const override;

    void set_positional () override;
    bool is_positional () const override;

    bool takes_argument () const override;
    bool value (std::string_view v) override;

    std::optional<std::string_view> arg_description () const noexcept override {
      return original_->arg_description ();
    }

    void set_original (option * o);
    option const * original () const noexcept { return original_; }

  private:
    option * original_ = nullptr;
  };

  //*       _ _                   _    *
  //*  __ _| (_)__ _ ___ ___ _ __| |_  *
  //* / _` | | / _` (_-</ _ \ '_ \  _| *
  //* \__,_|_|_\__,_/__/\___/ .__/\__| *
  //*                       |_|        *
  class aliasopt {
  public:
    explicit constexpr aliasopt (option & original) noexcept
            : original_{original} {}
    alias & apply (alias & opt) const {
      opt.set_original (&original_);
      return opt;
    }

  private:
    option & original_;
  };


} // end namespace pstore::command_line

#endif // PSTORE_COMMAND_LINE_OPTION_HPP
