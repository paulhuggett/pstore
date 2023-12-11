//===- examples/serialize/nonpod_multiple/nonpod_multiple.cpp -------------===//
//*                                    _                   _ _   _       _       *
//*  _ __   ___  _ __  _ __   ___   __| |  _ __ ___  _   _| | |_(_)_ __ | | ___  *
//* | '_ \ / _ \| '_ \| '_ \ / _ \ / _` | | '_ ` _ \| | | | | __| | '_ \| |/ _ \ *
//* | | | | (_) | | | | |_) | (_) | (_| | | | | | | | |_| | | |_| | |_) | |  __/ *
//* |_| |_|\___/|_| |_| .__/ \___/ \__,_| |_| |_| |_|\__,_|_|\__|_| .__/|_|\___| *
//*                   |_|                                         |_|            *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// Demonstrates a technique for writing an array of custom objects to an archive and then reading
/// them back into memory.

#include <iostream>
#include <new>

#include "pstore/serialize/archive.hpp"
#include "pstore/serialize/types.hpp"
#include "pstore/support/gsl.hpp"

namespace {

  class foo {
    friend struct pstore::serialize::serializer<foo>;
    friend std::ostream & operator<< (std::ostream & os, foo const & f);

  public:
    foo () = default;
    constexpr explicit foo (int const a) noexcept
            : a_ (a) {}

  private:
    int a_ = 0;
  };

  std::ostream & operator<< (std::ostream & os, foo const & f) {
    return os << f.a_;
  }

} // end anonymous namespace

namespace pstore::serialize {

  // A serializer for struct foo
  template <>
  struct serializer<foo> {
    // Writes an instance of foo to an archive. The data stream contains
    // a single int value.
    template <typename Archive>
    static auto write (Archive && archive, foo const & value) ->
      typename std::decay_t<Archive>::result_type {
      return serialize::write (std::forward<Archive> (archive), value.a_);
    }

    // Reads an instance of foo from an archive. To do this, we read an integer from
    // the supplied archive and use it to construct a new foo instance into the
    // uninitialized memory supplied by the caller.
    template <typename Archive>
    static void read (Archive && archive, foo & sp) {
      new (&sp) foo (serialize::read<decltype (sp.a_)> (std::forward<Archive> (archive)));
    }
  };

} // end namespace pstore::serialize

namespace {

  // Write an array of "foo" instance to the "bytes" container.
  std::vector<std::byte> write_two_foos () {
    using pstore::gsl::make_span;
    using pstore::serialize::write;
    using pstore::serialize::archive::vector_writer;

    // We're going to write data to this container.
    std::vector<std::byte> bytes;
    // 'vector_writer' is an archiver which writes to a std::vector.
    vector_writer writer (bytes);
    // This is the array of foo instances that we will be writing.
    std::array<foo, 2> const src{{foo{37}, foo{42}}};

    // Some output to show what's about to happen.
    std::cout << "Writing: ";
    std::copy (std::begin (src), std::end (src), std::ostream_iterator<foo> (std::cout, " "));
    std::cout << std::endl;

    // Write the 'src' array to the archive.
    write (writer, make_span (src));
    std::cout << "Wrote these bytes: " << writer << '\n';
    return bytes;
  }

  // Now use the contents of the "bytes" vector to materialize two foo instances.
  void read_two_foos (std::vector<std::byte> const & bytes) {
    using pstore::gsl::make_span;
    using pstore::serialize::read;
    using pstore::serialize::archive::make_reader;

    auto reader = make_reader (std::begin (bytes));
    std::array<foo, 2> dest;
    read (reader, make_span (dest));

    std::cout << "Read: ";
    std::copy (std::begin (dest), std::end (dest), std::ostream_iterator<foo> (std::cout, " "));
    std::cout << std::endl;
  }

} // end anonymous namespace

int main () {
  // First write an array of "foo" to a container.
  auto const bytes = write_two_foos ();
  // Now use the contents of the "bytes" vector to materialize two foo instances.
  read_two_foos (bytes);
}
