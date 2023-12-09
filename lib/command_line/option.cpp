//===- lib/command_line/option.cpp ----------------------------------------===//
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
#include "pstore/command_line/option.hpp"

namespace pstore::command_line {

  //*           _     _              _  *
  //*  ___ _ __| |_  | |__  ___  ___| | *
  //* / _ \ '_ \  _| | '_ \/ _ \/ _ \ | *
  //* \___/ .__/\__| |_.__/\___/\___/_| *
  //*     |_|                           *
  opt<bool>::~opt () noexcept = default;

  bool opt<bool>::add_occurrence () {
    option::add_occurrence ();
    if (this->get_num_occurrences () == 1U) {
      value_ = !value_;
    }
    return true;
  }

  //*       _ _          *
  //*  __ _| (_)__ _ ___ *
  //* / _` | | / _` (_-< *
  //* \__,_|_|_\__,_/__/ *
  //*                    *
  void alias::set_original (option * const o) {
    PSTORE_ASSERT (o != nullptr && o != this);
    original_ = o;
  }
  bool alias::add_occurrence () {
    return original_->add_occurrence ();
  }
  void alias::set_occurrences_flag (occurrences_flag const n) {
    original_->set_occurrences_flag (n);
  }
  occurrences_flag alias::get_occurrences_flag () const {
    return original_->get_occurrences_flag ();
  }
  void alias::set_positional () {
    original_->set_positional ();
  }
  bool alias::is_positional () const {
    return original_->is_positional ();
  }
  unsigned alias::get_num_occurrences () const {
    return original_->get_num_occurrences ();
  }
  bool alias::takes_argument () const {
    return original_->takes_argument ();
  }
  bool alias::value (std::string_view v) {
    return original_->value (v);
  }

} // end namespace pstore::command_line
