//===- include/pstore/command_line/command_line.hpp -------*- mode: C++ -*-===//
//*                                                _   _ _             *
//*   ___ ___  _ __ ___  _ __ ___   __ _ _ __   __| | | (_)_ __   ___  *
//*  / __/ _ \| '_ ` _ \| '_ ` _ \ / _` | '_ \ / _` | | | | '_ \ / _ \ *
//* | (_| (_) | | | | | | | | | | | (_| | | | | (_| | | | | | | |  __/ *
//*  \___\___/|_| |_| |_|_| |_| |_|\__,_|_| |_|\__,_| |_|_|_| |_|\___| *
//*                                                                    *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_COMMAND_LINE_COMMAND_LINE_HPP
#define PSTORE_COMMAND_LINE_COMMAND_LINE_HPP

#include <functional>
#include <list>
#include <iostream>
#include <string_view>

#include "pstore/command_line/help.hpp"
#include "pstore/command_line/csv.hpp"
#include "pstore/command_line/modifiers.hpp"
#include "pstore/command_line/option.hpp"
#include "pstore/command_line/parser.hpp"
#include "pstore/command_line/tchar.hpp"
#include "pstore/command_line/stream_traits.hpp"
#include "pstore/command_line/word_wrapper.hpp"
#include "pstore/os/path.hpp"
#include "pstore/support/unsigned_cast.hpp"

namespace pstore::command_line {

  namespace details {

    template <typename T, typename = std::enable_if_t<std::is_unsigned_v<T>>>
    constexpr int int_cast (T value) noexcept {
      using common = std::common_type_t<T, unsigned>;
      return static_cast<int> (
        std::min (static_cast<common> (value),
                  static_cast<common> (unsigned_cast (std::numeric_limits<int>::max ()))));
    }

  } // end namespace details



  class argument_parser;

  template <typename OutputStream>
  class help final : public option {
  public:
    template <typename... Mods>
    explicit help (argument_parser const & owner, std::string program_name,
                   std::string program_overview, OutputStream & outs, Mods &&... mods)
            : owner_{owner}
            , program_name_{std::move (program_name)}
            , overview_{std::move (program_overview)}
            , outs_{outs} {
      apply_to_option (*this, std::forward<Mods> (mods)...);
    }

    help (help const &) = delete;
    help (help &&) noexcept = delete;

    help & operator= (help const &) = delete;
    help & operator= (help &&) noexcept = delete;

    bool takes_argument () const override { return false; }
    bool add_occurrence () override;

    bool value (std::string_view v) override {
      (void) v;
      return false;
    }

    void show ();

  private:
    using ostream_traits = stream_trait<typename OutputStream::char_type>;

    /// Returns true if the program has any non-positional arguments.
    bool has_switches () const;
    /// Writes the program's usage string to the output stream given to the ctor.
    void usage () const;

    argument_parser const & owner_;
    std::string const program_name_;
    std::string const overview_;
    OutputStream & outs_;
  };


  namespace details {

    bool starts_with (std::string const & s, gsl::czstring prefix);
    bool argument_is_positional (std::string const & arg_name);
    bool handler_takes_argument (std::optional<option *> handler);
    bool handler_set_value (std::optional<option *> handler, std::string const & value);

    /// Splits the name and possible argument values from an argument string.
    ///
    /// A string prefixed with a double-dash may include an optional value preceeded
    /// by an equals sign. This function splits out the leading dash or double dash and
    /// the optional value to yield the option name and value.
    ///
    /// \param arg A command-line argument string.
    /// \returns A tuple containing the argument name (shorn of leading dashes) and a
    ///   value string if one was present.
    std::tuple<std::string, std::optional<std::string>> get_option_and_value (std::string arg);

    /// A simple wrapper for a bool where as soon as StickTo is assigned, subsequent
    /// assignments are ignored.
    template <bool StickTo = false>
    class sticky_bool {
    public:
      static constexpr auto stick_to = StickTo;

      explicit constexpr sticky_bool (bool const v) noexcept
              : v_{v} {}
      sticky_bool (sticky_bool const &) noexcept = default;
      sticky_bool (sticky_bool &&) noexcept = default;
      ~sticky_bool () noexcept = default;

      sticky_bool & operator= (sticky_bool const & other) = default;
      sticky_bool & operator= (sticky_bool && other) noexcept = default;

      sticky_bool & operator= (bool const b) noexcept {
        if (v_ != stick_to) {
          v_ = b;
        }
        return *this;
      }

      constexpr bool get () const noexcept { return v_; }
      explicit constexpr operator bool () const noexcept { return get (); }

    private:
      bool v_;
    };

    template <typename ErrorStream>
    auto record_value_if_available (std::optional<option *> handler,
                                    std::optional<std::string> const & value,
                                    std::string const & program_name, ErrorStream & errs)
      -> std::tuple<std::optional<option *>, bool> {
      using str = stream_trait<typename ErrorStream::char_type>;
      bool ok = true;
      if ((*handler)->takes_argument ()) {
        if (value) {
          if (!handler_set_value (handler, *value)) {
            errs << str::out_string (program_name) << str::out_text (": Unknown value '")
                 << str::out_string (*value) << str::out_text ("'");
            ok = false;
          }
          handler.reset ();
        } else {
          // The option takes an argument but we haven't yet seen the value
          // string.
        }
      } else {
        if (value) {
          // We got a value but don't want one.
          errs << str::out_string (program_name) << str::out_text (": Argument '")
               << str::out_string ((*handler)->name ())
               << str::out_text ("' does not take a value\n");
          ok = false;
        } else {
          ok = (*handler)->add_occurrence ();
          handler.reset ();
        }
      }
      return std::make_tuple (std::move (handler), ok);
    }

  } // end namespace details

  class argument_parser {
  public:
    using value_type = std::unique_ptr<option>;

    argument_parser () noexcept = default;
    argument_parser (argument_parser const &) = delete;
    argument_parser (argument_parser &&) noexcept = delete;
    ~argument_parser () noexcept = default;

    argument_parser & operator= (argument_parser const &) = delete;
    argument_parser & operator= (argument_parser &&) noexcept = delete;

    template <typename OptionType, typename... Args>
    auto & add (Args &&... args) {
      auto p = std::make_unique<OptionType> (std::forward<Args> (args)...);
      auto & result = *p;
      opts_.emplace_back (std::move (p));
      return result;
    }

    auto begin () const { return std::begin (opts_); }
    auto end () const { return std::end (opts_); }

    template <typename InputIterator, typename OutputStream, typename ErrorStream>
    bool parse_args (InputIterator first_arg, InputIterator last_arg, std::string const & overview,
                     OutputStream & outs, ErrorStream & errs);

#ifdef _WIN32
    /// For Windows, a variation on parse_args() which takes the
    /// arguments as either UTF-16 or MBCS strings and converts them to UTF-8 as expected
    /// by the rest of the code.
    template <typename CharType>
    void parse_args (int const argc, CharType * const argv[], std::string const & overview) {
      std::vector<std::string> args;
      args.reserve (argc);
      std::transform (argv, argv + argc, std::back_inserter (args),
                      [] (CharType const * str) { return pstore::utf::from_native_string (str); });
      if (!this->parse_args (std::begin (args), std::end (args), overview, out_stream,
                             error_stream)) {
        std::exit (EXIT_FAILURE);
      }
    }
#else
    inline void parse_args (int const argc, gsl::zstring const argv[],
                            std::string const & overview) {
      if (!this->parse_args (argv, argv + argc, overview, out_stream, error_stream)) {
        std::exit (EXIT_FAILURE);
      }
    }
#endif // _WIN32

    /// Returns true if the program has any non-positional arguments.
    ///
    /// \param exclude  A option to be excluded from the test (usually the help switch).
    /// \return True if the program has any non-positional arguments, false otherwise.
    bool has_switches (option const * const exclude) const;

  private:
    static bool is_positional (argument_parser::value_type const & opt) {
      return opt->is_positional ();
    }

    template <typename ErrorStream>
    std::tuple<std::optional<option *>, bool> process_single_dash (std::string arg_name,
                                                                   std::string const & program_name,
                                                                   ErrorStream & errs) const;

    template <typename InputIterator, typename ErrorStream>
    std::tuple<InputIterator, bool>
    parse_option_arguments (InputIterator first_arg, InputIterator last_arg,
                            std::string const & program_name, ErrorStream & errs) const;

    template <typename InputIterator>
    bool parse_positional_arguments (InputIterator first_arg, InputIterator last_arg) const;

    std::optional<option *> find_handler (std::string const & name) const;
    std::optional<option *> lookup_nearest_option (std::string const & arg) const;

    template <typename ErrorStream>
    void report_unknown_option (std::string const & program_name, std::string const & arg_name,
                                std::string const & value, ErrorStream & errs) const;
    template <typename ErrorStream>
    void report_unknown_option (std::string const & program_name, std::string const & arg_name,
                                std::optional<std::string> const & value,
                                ErrorStream & errs) const {
      this->report_unknown_option (program_name, arg_name, value.value_or (""), errs);
    }

    /// Makes sure that all of the required arguments have been specified.
    template <typename ErrorStream>
    bool check_for_missing (std::string const & program_name, ErrorStream & errs);

    std::list<value_type> opts_;
  };

  // parse command line options
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~
  template <typename InputIterator, typename OutputStream, typename ErrorStream>
  bool argument_parser::parse_args (InputIterator first_arg, InputIterator last_arg,
                                    std::string const & overview, OutputStream & outs,
                                    ErrorStream & errs) {
    std::string const program_name = pstore::path::base_name (*(first_arg++));
    this->add<help<OutputStream>> (*this, program_name, overview, outs).set_name ("help");

    bool ok = true;
    std::tie (first_arg, ok) =
      this->parse_option_arguments (first_arg, last_arg, program_name, errs);
    if (!ok) {
      return false;
    }
    if (!this->parse_positional_arguments (first_arg, last_arg)) {
      return false;
    }
    return this->check_for_missing (program_name, errs);
  }

  // parse positional arguments
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~
  template <typename InputIterator>
  bool argument_parser::parse_positional_arguments (InputIterator first_arg,
                                                    InputIterator last_arg) const {
    bool ok = true;

    auto end = std::end (opts_);
    auto it = std::find_if (std::begin (opts_), end, is_positional);
    for (; first_arg != last_arg && it != end; ++first_arg) {
      argument_parser::value_type const & handler = *it;
      PSTORE_ASSERT (handler->is_positional ());
      ok = handler->add_occurrence ();
      if (!handler->value (*first_arg)) {
        ok = false;
      }
      if (!handler->can_accept_another_occurrence ()) {
        it = std::find_if (++it, end, is_positional);
      }
    }
    return ok;
  }

  template <typename ErrorStream>
  std::tuple<std::optional<option *>, bool>
  argument_parser::process_single_dash (std::string arg_name, std::string const & program_name,
                                        ErrorStream & errs) const {
    PSTORE_ASSERT (details::starts_with (arg_name, "-"));
    arg_name.erase (0, 1U); // Remove the leading dash.

    std::optional<option *> handler;
    details::sticky_bool<false> ok{true};
    while (ok && !arg_name.empty ()) {
      char const name[2]{arg_name[0], '\0'};
      handler = this->find_handler (name);
      if (!handler || (*handler)->is_positional ()) {
        this->report_unknown_option (program_name, name, std::optional<std::string> (), errs);
        ok = false;
        break;
      }

      if ((*handler)->takes_argument ()) {
        arg_name.erase (0, 1U);
        if (arg_name.length () == 0U) {
          // No value was supplied immediately after the argument name. It
          // could be the next argument.
          break;
        }
        ok = details::handler_set_value (handler, arg_name);
        arg_name.clear ();
      } else {
        arg_name.erase (0, 1U);
        ok = (*handler)->add_occurrence ();
      }
      handler.reset ();
    }
    return std::make_tuple (std::move (handler), ok.get ());
  }


  template <typename InputIterator, typename ErrorStream>
  std::tuple<InputIterator, bool>
  argument_parser::parse_option_arguments (InputIterator first_arg, InputIterator last_arg,
                                           std::string const & program_name,
                                           ErrorStream & errs) const {
    using str = stream_trait<typename ErrorStream::char_type>;

    std::optional<std::string> value;
    std::optional<option *> handler;
    details::sticky_bool<false> ok{true};

    for (; first_arg != last_arg; ++first_arg) {
      std::string arg_name = *first_arg;
      // Is this the argument for the preceeding switch?
      if (details::handler_takes_argument (handler)) {
        ok = details::handler_set_value (handler, arg_name);
        handler.reset ();
        continue;
      } else {
        // A double-dash argument on its own indicates that the following are
        // positional arguments.
        if (arg_name == "--") {
          ++first_arg; // swallow this argument.
          break;
        }
        // If this argument has no leading dash, this and the following are
        // positional arguments.
        if (details::argument_is_positional (arg_name)) {
          break;
        }

        if (details::starts_with (arg_name, "--")) {
          std::tie (arg_name, value) = details::get_option_and_value (arg_name);

          handler = this->find_handler (arg_name);
          if (!handler || (*handler)->is_positional ()) {
            this->report_unknown_option (program_name, arg_name, value, errs);
            ok = false;
            continue;
          }

          std::tie (handler, ok) =
            details::record_value_if_available (handler, value, program_name, errs);
        } else {
          std::tie (handler, ok) = this->process_single_dash (arg_name, program_name, errs);
        }
      }
    }

    if (handler && (*handler)->takes_argument ()) {
      errs << str::out_string (program_name) << str::out_text (": Argument '")
           << str::out_string ((*handler)->name ()) << str::out_text ("' requires a value\n");
      ok = false;
    }
    return std::make_tuple (first_arg, static_cast<bool> (ok));
  }

  // report unknown option
  // ~~~~~~~~~~~~~~~~~~~~~
  template <typename ErrorStream>
  void argument_parser::report_unknown_option (std::string const & program_name,
                                               std::string const & arg_name,
                                               std::string const & value,
                                               ErrorStream & errs) const {
    using str = stream_trait<typename ErrorStream::char_type>;
    errs << str::out_string (program_name) << str::out_text (": Unknown command line argument '")
         << str::out_string (arg_name) << str::out_text ("'\n");

    if (std::optional<option *> const best_option = this->lookup_nearest_option (arg_name)) {
      std::string nearest_string = (*best_option)->name ();
      gsl::czstring const dashes = utf::length (nearest_string) < 2U ? "-" : "--";
      if (!value.empty ()) {
        nearest_string += '=';
        nearest_string += value;
      }
      errs << str::out_text ("Did you mean '") << str::out_string (dashes)
           << str::out_string (nearest_string) << str::out_text ("'?\n");
    }
  }

  /// Makes sure that all of the required arguments have been specified.
  template <typename ErrorStream>
  bool argument_parser::check_for_missing (std::string const & program_name, ErrorStream & errs) {
    using str = stream_trait<typename ErrorStream::char_type>;
    using pstore::command_line::occurrences_flag;
    using pstore::command_line::option;

    bool ok = true;
    auto positional_missing = 0U;

    for (argument_parser::value_type const & opt : opts_) {
      switch (opt->get_occurrences_flag ()) {
      case occurrences_flag::required:
      case occurrences_flag::one_or_more:
        if (opt->get_num_occurrences () == 0U) {
          if (opt->is_positional ()) {
            ++positional_missing;
          } else {
            errs << str::out_string (program_name) << str::out_text (": option '")
                 << str::out_string (opt->name ())
                 << str::out_text ("' must be specified at least once\n");
          }
          ok = false;
        }
        break;
      case occurrences_flag::optional:
      case occurrences_flag::zero_or_more: break;
      }
    }

    if (positional_missing == 1U) {
      errs << str::out_string (program_name)
           << str::out_text (": a positional argument was missing\n");
    } else if (positional_missing > 1U) {
      errs << str::out_string (program_name) << positional_missing
           << str::out_text (": positional arguments are missing\n");
    }

    return ok;
  }


  //*  _        _       *
  //* | |_  ___| |_ __  *
  //* | ' \/ -_) | '_ \ *
  //* |_||_\___|_| .__/ *
  //*            |_|    *
  // add occurrence
  // ~~~~~~~~~~~~~~
  template <typename OutputStream>
  bool help<OutputStream>::add_occurrence () {
    this->show ();
    return false;
  }

  // has switches
  // ~~~~~~~~~~~~
  template <typename OutputStream>
  bool help<OutputStream>::has_switches () const {
    return owner_.has_switches (this);
  }

  // usage
  // ~~~~~
  template <typename OutputStream>
  void help<OutputStream>::usage () const {
    outs_ << ostream_traits::out_text ("USAGE: ") << ostream_traits::out_string (program_name_);
    if (this->has_switches ()) {
      outs_ << ostream_traits::out_text (" [options]");
    }
    for (auto const & op : owner_) {
      if (op.get () != this && op->is_positional ()) {
        outs_ << ostream_traits::out_text (" ") << ostream_traits::out_string (op->usage ());
      }
    }
    outs_ << '\n';
  }

  // show
  // ~~~~
  template <typename OutputStream>
  void help<OutputStream>::show () {
    static auto const separator = ostream_traits::out_text (std::string_view{" - "});

    std::size_t const max_width = pstore::command_line::get_max_width ();
    auto const new_line = ostream_traits::out_text ("\n");

    outs_ << ostream_traits::out_text ("OVERVIEW: ") << ostream_traits::out_string (overview_)
          << new_line;
    usage ();

    auto const categories = build_categories (this, owner_);
    std::size_t const max_name_len = widest_option (categories);

    auto const indent = max_name_len + separator.length ();
    auto const description_width =
      max_width - max_name_len - separator.length () - help_prefix_indent.length ();

    for (auto const & cat : categories) {
      outs_ << new_line
            << ostream_traits::out_string (cat.first == nullptr ? "OPTIONS" : cat.first->title ())
            << ostream_traits::out_text (":\n\n");

      for (auto const & sw : get_switch_strings (cat.second)) {
        option const * const op = sw.first;
        auto is_first = true;
        auto is_overlong = false;
        for (std::tuple<std::string, std::size_t> const & name : sw.second) {
          if (!is_first) {
            outs_ << new_line;
          }
          outs_ << ostream_traits::out_string (help_prefix_indent) << std::left
                << std::setw (details::int_cast (max_name_len))
                << ostream_traits::out_string (std::get<std::string> (name));

          is_first = false;
          PSTORE_ASSERT (pstore::utf::length (std::get<std::string> (name)) ==
                         std::get<std::size_t> (name));
          is_overlong = std::get<std::size_t> (name) > help_overlong_opt_max;
        }
        outs_ << separator;

        std::string const & description = op->description ();
        is_first = true;
        std::for_each (
          word_wrapper (description, description_width),
          word_wrapper::end (description, description_width), [&] (std::string const & str) {
            if (!is_first || is_overlong) {
              outs_ << new_line
                    << std::setw (details::int_cast (indent + help_prefix_indent.length ())) << ' ';
            }
            outs_ << ostream_traits::out_string (str);
            is_first = false;
            is_overlong = false;
          });
        outs_ << new_line;
      }
    }
  }

  //*                                              *
  //*  ___  __ __ _  _ _ _ _ _ ___ _ _  __ ___ ___ *
  //* / _ \/ _/ _| || | '_| '_/ -_) ' \/ _/ -_|_-< *
  //* \___/\__\__|\_,_|_| |_| \___|_||_\__\___/__/ *
  //*                                              *
  namespace details {

    struct positional {
      template <typename Opt>
      void apply (Opt & o) const {
        o.set_positional ();
      }
    };

    struct required {
      template <typename Opt>
      void apply (Opt & o) const {
        o.set_occurrences_flag (occurrences_flag::required);
      }
    };

    struct optional {
      template <typename Opt>
      void apply (Opt & o) const {
        o.set_occurrences_flag (occurrences_flag::optional);
      }
    };

    struct one_or_more {
      template <typename Opt>
      void apply (Opt & o) const {
        bool const is_optional = o.get_occurrences_flag () == occurrences_flag::optional;
        o.set_occurrences_flag (is_optional ? occurrences_flag::zero_or_more
                                            : occurrences_flag::one_or_more);
      }
    };

  } // end namespace details

  constexpr details::one_or_more one_or_more;
  constexpr details::optional const optional;
  constexpr details::positional const positional;
  constexpr details::required const required;

} // end namespace pstore::command_line

#endif // PSTORE_COMMAND_LINE_COMMAND_LINE_HPP
