//===- tools/genromfs/genromfs.cpp ----------------------------------------===//
//*                                        __      *
//*   __ _  ___ _ __  _ __ ___  _ __ ___  / _|___  *
//*  / _` |/ _ \ '_ \| '__/ _ \| '_ ` _ \| |_/ __| *
//* | (_| |  __/ | | | | | (_) | | | | | |  _\__ \ *
//*  \__, |\___|_| |_|_|  \___/|_| |_| |_|_| |___/ *
//*  |___/                                         *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifdef _WIN32
#  define NOMINMAX
#  define WIN32_LEAN_AND_MEAN
#endif

// Standard Library includes
#include <iostream>
#include <string>
#include <tuple>
#include <unordered_set>
#include <vector>

// Platform includes
#ifdef _WIN32
#  include <tchar.h>
#endif

// pstore includes
#include "pstore/command_line/command_line.hpp"
#include "pstore/command_line/tchar.hpp"
#include "pstore/support/array_elements.hpp"
#include "pstore/support/error.hpp"
#include "pstore/support/portab.hpp"

// local includes
#include "copy.hpp"
#include "dump_tree.hpp"
#include "scan.hpp"
#include "vars.hpp"

namespace {

  enum class genromfs_erc : int {
    empty_name_component = 1,
  };

  class error_category final : public std::error_category {
  public:
    error_category () {}
    char const * name () const noexcept override { return "pstore genromfs category"; }
    std::string message (int error) const override {
      static_assert (std::is_same_v<std::underlying_type_t<genromfs_erc>, decltype (error)>,
                     "base type of genromfs_erc must be int to permit safe static cast");
      auto * result = "unknown value error";
      switch (static_cast<genromfs_erc> (error)) {
      case genromfs_erc::empty_name_component: result = "Name component is empty"; break;
      }
      return result;
    }
  };

  std::error_code make_error_code (genromfs_erc e) {
    static_assert (std::is_same_v<std::underlying_type_t<decltype (e)>, int>,
                   "base type of error_code must be int to permit safe static cast");
    static error_category const cat;
    return {static_cast<int> (e), cat};
  }

} // end anonymous namespace

namespace std {

  template <>
  struct is_error_code_enum<genromfs_erc> : std::true_type {};

} // end namespace std

namespace {

  template <typename Function>
  std::string::size_type for_each_namespace (std::string const & s, Function function) {
    auto start = std::string::size_type{0};
    auto end = std::string::npos;
    static char const two_colons[] = "::";
    auto part = 0U;

    while ((end = s.find (two_colons, start)) != std::string::npos) {
      ++part;
      PSTORE_ASSERT (end >= start);
      auto const length = end - start;
      if (length == 0 && part > 1U) {
        pstore::raise (genromfs_erc::empty_name_component);
      }
      if (length > 0) {
        function (s.substr (start, length));
      }
      start = end + pstore::array_elements (two_colons) - 1U;
    }
    return start;
  }

  std::ostream & write_definition (std::ostream & os, std::string const & var_name,
                                   std::string const & root) {
    auto const start = for_each_namespace (
      var_name, [&os] (std::string const & ns) { os << "namespace " << ns << " {\n"; });

    auto const name = var_name.substr (start);
    if (name.length () == 0) {
      pstore::raise (genromfs_erc::empty_name_component);
    }
    os << "extern ::pstore::romfs::romfs " << name << ";\n"
       << "::pstore::romfs::romfs " << name << " (&" << root << ");\n";
    for_each_namespace (
      var_name, [&os] (std::string const & ns) { os << "} // end namespace " << ns << '\n'; });
    return os;
  }

} // end anonymous namespace

#ifdef _WIN32
int _tmain (int argc, TCHAR * argv[]) {
#else
int main (int argc, char * argv[]) {
#endif
  using namespace pstore::command_line;
  options_container all;
  auto & src_path = all.add<string_opt> (positional, init ("."), desc ("source-path"));
#define DEFAULT_VAR "fs"
  auto & root_var = all.add<string_opt> (
    name{"var"},
    desc ("Variable name for the file system root "
          "(may contain '::' to place in a specifc namespace). (Default: '" DEFAULT_VAR "')"),
    init (DEFAULT_VAR));
#undef DEFAULT_VAR

  parse_command_line_options (all, argc, argv, "pstore romfs generation utility\n");
  int exit_code = EXIT_SUCCESS;

  PSTORE_TRY {
    std::ostream & os = std::cout;

    os << "// This file was generated by genromfs. DO NOT EDIT!\n"
          "#include <array>\n"
          "#include <cstdint>\n"
          "#include \"pstore/romfs/romfs.hpp\"\n"
          "\n"
          "namespace {\n"
          "\n";
    auto root = std::make_unique<directory_container> ();
    auto root_id = scan (*root, src_path.get (), 0);
    std::unordered_set<unsigned> forwards;
    dump_tree (os, forwards, *root, root_id, root_id);

    os << "\n"
          "} // end anonymous namespace\n"
          "\n";

    write_definition (os, root_var.get (), directory_var (root_id).as_string ());
  }
  PSTORE_CATCH (std::exception const & ex, {
    pstore::command_line::error_stream << PSTORE_NATIVE_TEXT ("Error: ")
                                       << pstore::utf::to_native_string (ex.what ())
                                       << PSTORE_NATIVE_TEXT ('\n');
    exit_code = EXIT_FAILURE;
  })
  PSTORE_CATCH (..., {
    pstore::command_line::error_stream << PSTORE_NATIVE_TEXT ("An unknown error occurred\n");
    exit_code = EXIT_FAILURE;
  })
  return exit_code;
}
