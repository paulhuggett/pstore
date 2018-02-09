//*              _ _    *
//*   __ _ _   _(_) |_  *
//*  / _` | | | | | __| *
//* | (_| | |_| | | |_  *
//*  \__, |\__,_|_|\__| *
//*     |_|             *
//===- include/broker/quit.hpp --------------------------------------------===//
// Copyright (c) 2017-2018 by Sony Interactive Entertainment, Inc.
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
/// \file quit.hpp

#ifndef PSTORE_BROKER_QUIT_HPP
#define PSTORE_BROKER_QUIT_HPP

#include <cstdlib>
#include <memory>
#include <thread>

namespace pstore {
    namespace broker {

        class fifo_path;
        class command_processor;
        class scavenger;

        void shutdown (command_processor * const cp, scavenger * const scav, int signum,
                       unsigned num_read_thread);

        /// Wakes up the quit thread to start the process of shutting down the server.
        void notify_quit_thread ();

        std::thread create_quit_thread (std::weak_ptr<command_processor> cp,
                                        std::weak_ptr<scavenger> scav, unsigned num_read_threads);

    } // namespace broker
} // namespace pstore

#endif // PSTORE_BROKER_QUIT_HPP
// eof: include/broker/quit.hpp
