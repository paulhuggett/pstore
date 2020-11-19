//*  _                            _                     _            _    *
//* (_)_ __ ___  _ __   ___  _ __| |_    ___ ___  _ __ | |_ _____  _| |_  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __|  / __/ _ \| '_ \| __/ _ \ \/ / __| *
//* | | | | | | | |_) | (_) | |  | |_  | (_| (_) | | | | ||  __/>  <| |_  *
//* |_|_| |_| |_| .__/ \___/|_|   \__|  \___\___/|_| |_|\__\___/_/\_\\__| *
//*             |_|                                                       *
//===- include/pstore/exchange/import_context.hpp -------------------------===//
// Copyright (c) 2017-2020 by Sony Interactive Entertainment, Inc.
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
#ifndef PSTORE_EXCHANGE_IMPORT_CONTEXT_HPP
#define PSTORE_EXCHANGE_IMPORT_CONTEXT_HPP

#include <list>
#include <stack>

#include "pstore/support/gsl.hpp"

namespace pstore {
    class database;
    class transaction_base;

    namespace exchange {
        namespace import {

            class patcher {
            public:
                virtual ~patcher () noexcept = default;
                virtual std::error_code operator() (transaction_base * t) = 0;
            };

            class rule;

            struct context {
                explicit context (gsl::not_null<database *> const db_) noexcept
                        : db{db_} {}
                context (context const &) = delete;
                context (context &&) = delete;

                ~context () noexcept = default;

                context & operator= (context const &) = delete;
                context & operator= (context &&) = delete;

                std::error_code apply_patches (transaction_base * const t) {
                    for (auto const & patch : patches) {
                        if (std::error_code const erc = (*patch) (t)) {
                            return erc;
                        }
                    }
                    // Ensure that we can't apply the patches more than once.
                    patches.clear ();
                    return {};
                }

                gsl::not_null<database *> const db;
                std::stack<std::unique_ptr<rule>> stack;
                std::list<std::unique_ptr<patcher>> patches;
            };

        } // end namespace import
    }     // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_IMPORT_CONTEXT_HPP
