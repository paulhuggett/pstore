//===- examples/serialize/nonpod1/nonpod1.cpp -----------------------------===//
//*                                    _ _  *
//*  _ __   ___  _ __  _ __   ___   __| / | *
//* | '_ \ / _ \| '_ \| '_ \ / _ \ / _` | | *
//* | | | | (_) | | | | |_) | (_) | (_| | | *
//* |_| |_|\___/|_| |_| .__/ \___/ \__,_|_| *
//*                   |_|                   *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// A example showing one approach to implementing archiving for a non-standard layout type.

#include <iostream>

#include "pstore/serialize/archive.hpp"
#include "pstore/serialize/types.hpp"

namespace {

  class foo {
  public:
    explicit constexpr foo (int const a) noexcept
            : a_{a} {}
    foo (foo const &) noexcept = default;
    foo (foo &&) noexcept = default;

    // The presence of virtual methods in this class means that it is not "standard layout".
    virtual ~foo () noexcept = default;

    // Archival methods. The following two methods can be implemented on "non-standard layout"
    // types to enable reading from and writing to an archive. An alternative approach (which
    // also applies to standard layout types) is to write an explicit specialization of
    // pstore::serialize::serializer<Ty>.

    template <typename Archive, typename = std::enable_if_t<!std::is_same_v<Archive, foo>>>
    explicit foo (Archive && archive)
            : a_ (pstore::serialize::read<int> (std::forward<Archive> (archive))) {}

    template <typename Archive>
    auto write (Archive && archive) const -> typename std::decay_t<Archive>::result_type {
      return pstore::serialize::write (std::forward<Archive> (archive), a_);
    }

    foo & operator= (foo const &) noexcept = default;

    // This method doesn't really need to be virtual except that we're deliberately creating
    // a class with non-standard layout.
    virtual int get () const { return a_; }

  private:
    int a_;
  };

  std::ostream & operator<< (std::ostream & os, foo const & f) {
    return os << "foo(" << f.get () << ')';
  }

  // Serialize an instance of "foo" to the "bytes" vector.
  auto write_foo () {
    std::vector<std::uint8_t> bytes;
    pstore::serialize::archive::vector_writer writer{bytes};
    foo f (42);
    std::cout << "Writing: " << f << '\n';
    pstore::serialize::write (writer, f);
    std::cout << "Wrote these bytes: " << writer << '\n';
    return bytes;
  }

  // Materialize an instance of "foo" from the "bytes" container.
  void read (std::vector<std::uint8_t> const & bytes) {
    auto reader = pstore::serialize::archive::make_reader (std::begin (bytes));
    auto const f = pstore::serialize::read<foo> (reader);
    std::cout << "Read: " << f << '\n';
  }

} // end anonymous namespace

int main () {
  // Write a foo.
  auto bytes = write_foo ();
  // Read it back again.
  read (bytes);
}
