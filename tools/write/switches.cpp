//*               _ _       _                *
//*  _____      _(_) |_ ___| |__   ___  ___  *
//* / __\ \ /\ / / | __/ __| '_ \ / _ \/ __| *
//* \__ \\ V  V /| | || (__| | | |  __/\__ \ *
//* |___/ \_/\_/ |_|\__\___|_| |_|\___||___/ *
//*                                          *
//===- tools/write/switches.cpp -------------------------------------------===//
// Copyright (c) 2017 by Sony Interactive Entertainment, Inc.
// All rights reserved.
//
// Developed by:
//   Toolchain Team
//   SN Systems, Ltd.
//   www.snsystems.com
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal with the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimers.
//
// - Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimers in the
//   documentation and/or other materials provided with the distribution.
//
// - Neither the names of SN Systems Ltd., Sony Interactive Entertainment,
//   Inc. nor the names of its contributors may be used to endorse or
//   promote products derived from this Software without specific prior
//   written permission.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
//===----------------------------------------------------------------------===//
#include "switches.hpp"

#if PSTORE_IS_INSIDE_LLVM
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Error.h"
#else
#include "pstore_cmd_util/cl/command_line.hpp"
#endif

#include "pstore_support/error.hpp"
#include "pstore_support/gsl.hpp"
#include "pstore_support/utf.hpp"

#include "error.hpp"
#include "to_value_pair.hpp"

#if PSTORE_IS_INSIDE_LLVM
using namespace llvm;
#else
using namespace pstore::cmd_util;
#endif

namespace {

    cl::list<std::string>
        Add ("add", cl::desc ("Add key with corresponding string value. Specified as 'key,value'."
                              " May be repeated to add several keys."));
    cl::alias Add2 ("a", cl::desc ("Alias for --add"), cl::aliasopt (Add));

    cl::list<std::string>
        AddString ("add-string",
                   cl::desc ("Add key to string set. May be repeated to add several strings."));
    cl::alias AddString2 ("s", cl::desc ("Alias for --add-string"), cl::aliasopt (AddString));

    cl::list<std::string>
        AddFile ("add-file",
                 cl::desc ("Add key with the named file's contents as the corresponding value. May "
                           "be repeated to add several files."));
    cl::alias AddFile2 ("f", cl::desc ("Alias for --add-file"), cl::aliasopt (AddString));


    cl::opt<std::string> DbPath (cl::Positional,
                                 cl::desc ("Path of the pstore repository to be written."),
                                 cl::Required);
    cl::list<std::string> Files (cl::Positional, cl::desc ("<filename>..."));

    cl::opt<std::string> VacuumMode ("compact", cl::Optional,
                                     cl::desc ("Set the compaction mode. Argument must one of: "
                                               "'disabled', 'immediate', 'background'."));
    cl::alias VacuumMode2 ("c", cl::desc ("Alias for --compact"), cl::aliasopt (VacuumMode));

    pstore::database::vacuum_mode to_vacuum_mode (std::string const & opt) {
        if (opt == "disabled") {
            return pstore::database::vacuum_mode::disabled;
        } else if (opt == "immediate") {
            return pstore::database::vacuum_mode::immediate;
        } else if (opt == "background") {
            return pstore::database::vacuum_mode::background;
        }

        pstore::raise_error_code (
            std::make_error_code (write_error_code::unrecognized_compaction_mode));
    }

} // end anonymous namespace

std::pair<switches, int> get_switches (int argc, char * argv[]) {
    cl::ParseCommandLineOptions (argc, argv, "pstore write utility\n");

    auto make_value_pair = [](std::string const & arg) {
        return to_value_pair (pstore::utf::from_native_string (arg));
    };

    switches result;

    result.db_path = pstore::utf::from_native_string (DbPath);
    if (!VacuumMode.empty ()) {
        result.vmode = to_vacuum_mode (pstore::utf::from_native_string (VacuumMode));
    }

    std::transform (std::begin (Add), std::end (Add), std::back_inserter (result.add),
                    make_value_pair);

    std::transform (std::begin (AddString), std::end (AddString),
                    std::back_inserter (result.strings),
                    [](std::string const & str) { return pstore::utf::from_native_string (str); });

    std::transform (std::begin (AddFile), std::end (AddFile), std::back_inserter (result.files),
                    make_value_pair);

    std::transform (std::begin (Files), std::end (Files), std::back_inserter (result.files),
                    [](std::string const & path) {
                        return std::make_pair (pstore::utf::from_native_string (path),
                                               pstore::utf::from_native_string (path));
                    });

    return {result, EXIT_SUCCESS};
}
// eof: tools/write/switches.cpp
