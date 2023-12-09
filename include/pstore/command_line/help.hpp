//===- include/pstore/command_line/help.hpp ---------------*- mode: C++ -*-===//
//*  _          _        *
//* | |__   ___| |_ __   *
//* | '_ \ / _ \ | '_ \  *
//* | | | |  __/ | |_) | *
//* |_| |_|\___|_| .__/  *
//*              |_|     *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_COMMAND_LINE_HELP_HPP
#define PSTORE_COMMAND_LINE_HELP_HPP

#include <iomanip>
#include <map>
#include <set>
#include <tuple>

#include "pstore/adt/small_vector.hpp"
#include "pstore/support/gsl.hpp"

namespace pstore::command_line {

  class option;
  class argument_parser;
  class option_category;

  struct less_name {
    bool operator() (gsl::not_null<option const *> x, gsl::not_null<option const *> y) const;
  };

  using options_set = std::set<option const *, less_name>;
  using categories_collection = std::map<option_category const *, options_set>;
  using switch_strings = std::map<gsl::not_null<option const *>,
                                  small_vector<std::tuple<std::string, std::size_t>, 1>, less_name>;

  /// Builds a container which maps from an option to one or more fully decorated
  /// strings. Each string has leading dashes and trailing meta-description added.
  /// Alias options are not found in the collection: instead their description is
  /// included in that of the original option. Multiple short descriptions are folded
  /// into a single string for presentation to the user.
  switch_strings get_switch_strings (options_set const & ops);

  /// Builds a container which maps from each option-category to the set of its member
  /// options (including their aliases).
  ///
  /// \param self  Should be the help option so that it is excluded from the test.
  /// \param all  The collection of all switches.
  /// \returns  A container which maps from each option-category to the set of its
  ///   member options (including their aliases).
  categories_collection build_categories (option const * self, argument_parser const & all);

  /// Scans the collection of option names and returns the longest that will be
  /// presented the user. The maximum return value is overlong_opt_max.
  ///
  /// \returns  The width to be allowed in the left column for option names.
  std::size_t widest_option (categories_collection const & categories);

  /// Returns an estimation of the terminal width. This can be used to determine the
  /// point at which output text should be word-wrapped.
  std::size_t get_max_width ();

  /// The maximum allowed length of the name of an option in the help output.
  constexpr auto help_overlong_opt_max = std::size_t{26};

  /// This string is used as a prefix for all option names in the help output.
  constexpr auto help_prefix_indent = std::string_view{"  "};

} // end namespace pstore::command_line

#endif // PSTORE_COMMAND_LINE_HELP_HPP
