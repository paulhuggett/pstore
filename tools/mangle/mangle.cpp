//===- tools/mangle/mangle.cpp --------------------------------------------===//
//*                              _       *
//*  _ __ ___   __ _ _ __   __ _| | ___  *
//* | '_ ` _ \ / _` | '_ \ / _` | |/ _ \ *
//* | | | | | | (_| | | | | (_| | |  __/ *
//* |_| |_| |_|\__,_|_| |_|\__, |_|\___| *
//*                        |___/         *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file mangle.cpp
/// \brief A simple file header fuzzer.
/// The basic technique here is based on Ilja van Sprundel's "mangle.c", although this is a
/// new implementation in C++11 and using our libraries for portability.
///
/// Here is the comment from the top of the original source file
/// (http://www.digitaldwarf.be/products/mangle.c):
/// ---
///  trivial binary file fuzzer by Ilja van Sprundel.
///  It's usage is very simple, it takes a filename and headersize
///  as input. it will then change approximatly between 0 and 10% of
///  the header with random bytes (biased towards the highest bit set)
///
///  obviously you need a bash script or something as a wrapper !
///
///  so far this broke:
///  - libmagic (used file)
///  - preview (osX pdf viewer)
///  - xpdf (hang, not a crash ...)
///  - mach-o loading (osX 10.3.7, seems to be fixed later)
///  - qnx elf loader (panics almost instantly, yikes !)
///  - FreeBSD elf loading
///  - openoffice
///  - amp
///  - osX image loading (.dmg)
///  - libbfd (used objdump)
///  - libtiff (used tiff2pdf)
///  - xine (division by 0, took 20 minutes of fuzzing)
///  - OpenBSD elf loading (3.7 on a sparc)
///  - unixware 713 elf loading
///  - DragonFlyBSD elf loading
///  - solaris 10 elf loading
///  - cistron-radiusd
///  - linux ext2fs (2.4.29) image loading (division by 0)
///  - linux reiserfs (2.4.29) image loading (instant panic !!!)
///  - linux jfs (2.4.29) image loading (long (uninteruptable) loop, 2 oopses)
///  - linux xfs (2.4.29) image loading (instant panic)
///  - windows macromedia flash .swf loading (obviously the windows version of mangle needs a few
///  tweaks to work ...)
///  - Quicktime player 7.0.1 for MacOS X
///  - totem
///  - gnumeric
///  - vlc
///  - mplayer
///  - python bytecode interpreter
///  - realplayer 10.0.6.776 (GOLD)
///  - dvips
/// ---

#include <cstdint>
#include <limits>
#include <random>
#include <exception>
#include <iostream>

#include "pstore/core/file_header.hpp"
#include "pstore/os/file.hpp"
#include "pstore/os/memory_mapper.hpp"
#include "pstore/support/portab.hpp"

namespace {

  template <typename Ty>
  class random_generator {
  public:
    random_generator ()
            : device_ ()
            , generator_ (device_ ())
            , distribution_ () {}

    Ty get (Ty max) { return distribution_ (generator_) % max; }
    Ty get () {
      constexpr auto max = std::numeric_limits<Ty>::max ();
      static_assert (max > Ty (0), "max must be > 0");
      return distribution_ (generator_) % max;
    }

  private:
    std::random_device device_;
    std::mt19937_64 generator_;
    std::uniform_int_distribution<Ty> distribution_;
  };

} // end anonymous namespace

int main (int argc, char ** argv) {
  int exit_code = EXIT_SUCCESS;

  if (argc != 2) {
    std::cout << "Usage: " << argv[0] << " path-name\n"
              << " \"Fuzzes\" the header and r0 footer of the given file.\n"
              << " Warning: The file is modified in-place.\n";
    return EXIT_FAILURE;
  }

  PSTORE_TRY {
    random_generator<std::size_t> rand;

    auto const header_size = pstore::leader_size + sizeof (pstore::trailer);

    pstore::file::file_handle file{argv[1]};
    file.open (pstore::file::file_handle::create_mode::open_existing,
               pstore::file::file_handle::writable_mode::read_write);
    pstore::memory_mapper mapper (file,
                                  true,         // writable
                                  0,            // offset
                                  header_size); // length
    auto data = std::static_pointer_cast<std::uint8_t> (mapper.data ());
    auto ptr = data.get ();

    std::size_t const num_to_hit = rand.get (header_size / 10);
    for (std::size_t ctr = 0; ctr < num_to_hit; ctr++) {
      std::size_t const offset = rand.get (header_size);
      std::uint8_t new_value = rand.get () % std::numeric_limits<std::uint8_t>::max ();

      // We want the highest bit set more often, in case of signedness issues.
      if (rand.get () % 2) {
        new_value |= 0x80;
      }

      ptr[offset] = new_value;
    }
  }
  // clang-format off
  PSTORE_CATCH (std::exception const & ex, {
    std::cerr << "Error: " << ex.what () << std::endl;
    exit_code = EXIT_FAILURE;
  })
  PSTORE_CATCH (..., {
    std::cerr << "Unknown error" << std::endl;
    exit_code = EXIT_FAILURE;
  })
  // clang-format on
  std::cerr << "Mangle returning " << exit_code << '\n';
  return exit_code;
}
