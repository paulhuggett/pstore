//===- include/pstore/exchange/import_compilation.hpp -----*- mode: C++ -*-===//
//*  _                            _    *
//* (_)_ __ ___  _ __   ___  _ __| |_  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| *
//* | | | | | | | |_) | (_) | |  | |_  *
//* |_|_| |_| |_| .__/ \___/|_|   \__| *
//*             |_|                    *
//*                            _ _       _   _              *
//*   ___ ___  _ __ ___  _ __ (_) | __ _| |_(_) ___  _ __   *
//*  / __/ _ \| '_ ` _ \| '_ \| | |/ _` | __| |/ _ \| '_ \  *
//* | (_| (_) | | | | | | |_) | | | (_| | |_| | (_) | | | | *
//*  \___\___/|_| |_| |_| .__/|_|_|\__,_|\__|_|\___/|_| |_| *
//*                     |_|                                 *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_EXCHANGE_IMPORT_COMPILATION_HPP
#define PSTORE_EXCHANGE_IMPORT_COMPILATION_HPP

#include <bitset>

#include "pstore/exchange/import_names.hpp"
#include "pstore/exchange/import_non_terminals.hpp"
#include "pstore/exchange/import_terminals.hpp"
#include "pstore/mcrepo/compilation.hpp"

namespace pstore {
    namespace exchange {
        namespace import {

            using fragment_index_pointer = std::shared_ptr<index::fragment_index>;

            //*     _      __ _      _ _   _           *
            //*  __| |___ / _(_)_ _ (_) |_(_)___ _ _   *
            //* / _` / -_)  _| | ' \| |  _| / _ \ ' \  *
            //* \__,_\___|_| |_|_||_|_|\__|_\___/_||_| *
            //*                                        *
            //-MARK: definition
            class definition final : public rule {
            public:
                using container = std::vector<repo::definition>;

                definition (not_null<context *> ctxt, not_null<container *> definitions,
                            not_null<name_mapping const *> names,
                            fragment_index_pointer const & fragments);
                definition (definition const &) = delete;
                definition (definition &&) noexcept = delete;

                ~definition () noexcept override = default;

                definition & operator= (definition const &) = delete;
                definition & operator= (definition &&) noexcept = delete;

                gsl::czstring name () const noexcept override { return "definition"; }

                std::error_code key (std::string const & k) override;
                std::error_code end_object () override;

                static maybe<repo::linkage> decode_linkage (std::string const & linkage);
                static maybe<repo::visibility> decode_visibility (std::string const & visibility);

            private:
                not_null<container *> const definitions_;
                not_null<name_mapping const *> const names_;
                fragment_index_pointer const fragments_;

                enum { digest_index, name_index, linkage_index, visibility_index };
                std::bitset<visibility_index + 1> seen_;

                std::string digest_;
                std::uint64_t name_ = 0;
                std::string linkage_;
                std::string visibility_;
            };


            //*     _      __ _      _ _   _                _     _        _    *
            //*  __| |___ / _(_)_ _ (_) |_(_)___ _ _    ___| |__ (_)___ __| |_  *
            //* / _` / -_)  _| | ' \| |  _| / _ \ ' \  / _ \ '_ \| / -_) _|  _| *
            //* \__,_\___|_| |_|_||_|_|\__|_\___/_||_| \___/_.__// \___\__|\__| *
            //*                                                |__/             *
            //-MARK: definition object
            class definition_object final : public rule {
            public:
                definition_object (not_null<context *> ctxt,
                                   not_null<definition::container *> definitions,
                                   not_null<name_mapping const *> names,
                                   fragment_index_pointer const & fragments);
                definition_object (definition_object const &) = delete;
                definition_object (definition_object &&) noexcept = delete;

                ~definition_object () noexcept override = default;

                definition_object & operator= (definition_object const &) = delete;
                definition_object & operator= (definition_object &&) noexcept = delete;

                gsl::czstring name () const noexcept override;

                std::error_code begin_object () override;
                std::error_code end_array () override;

            private:
                not_null<definition::container *> const definitions_;
                not_null<name_mapping const *> const names_;
                fragment_index_pointer const fragments_;
            };

            //*                    _ _      _   _           *
            //*  __ ___ _ __  _ __(_) |__ _| |_(_)___ _ _   *
            //* / _/ _ \ '  \| '_ \ | / _` |  _| / _ \ ' \  *
            //* \__\___/_|_|_| .__/_|_\__,_|\__|_\___/_||_| *
            //*              |_|                            *
            //-MARK: compilation
            class compilation final : public rule {
            public:
                compilation (not_null<context *> ctxt, not_null<transaction_base *> transaction,
                             not_null<name_mapping const *> names,
                             fragment_index_pointer const & fragments, index::digest digest);
                compilation (compilation const &) = delete;
                compilation (compilation &&) noexcept = delete;
                ~compilation () noexcept override = default;

                compilation & operator= (compilation const &) = delete;
                compilation & operator= (compilation &&) noexcept = delete;

                gsl::czstring name () const noexcept override { return "compilation"; }

                std::error_code key (std::string const & k) override;
                std::error_code end_object () override;

            private:
                not_null<transaction_base *> const transaction_;
                not_null<name_mapping const *> const names_;
                fragment_index_pointer const fragments_;
                index::digest const digest_;

                enum { path_index, triple_index };
                std::bitset<triple_index + 1> seen_;

                std::uint64_t path_ = 0;
                std::uint64_t triple_ = 0;
                definition::container definitions_;
            };

            //*                    _ _      _   _               _         _          *
            //*  __ ___ _ __  _ __(_) |__ _| |_(_)___ _ _  ___ (_)_ _  __| |_____ __ *
            //* / _/ _ \ '  \| '_ \ | / _` |  _| / _ \ ' \(_-< | | ' \/ _` / -_) \ / *
            //* \__\___/_|_|_| .__/_|_\__,_|\__|_\___/_||_/__/ |_|_||_\__,_\___/_\_\ *
            //*              |_|                                                     *
            //-MARK: compilation index
            class compilations_index final : public rule {
            public:
                compilations_index (not_null<context *> ctxt,
                                    not_null<transaction_base *> transaction,
                                    not_null<name_mapping const *> names);
                compilations_index (compilations_index const &) = delete;
                compilations_index (compilations_index &&) noexcept = delete;
                ~compilations_index () noexcept override = default;

                compilations_index & operator= (compilations_index const &) = delete;
                compilations_index & operator= (compilations_index &&) = delete;

                gsl::czstring name () const noexcept override;
                std::error_code key (std::string const & s) override;
                std::error_code end_object () override;

            private:
                not_null<transaction_base *> const transaction_;
                not_null<name_mapping const *> const names_;
                fragment_index_pointer const fragments_;
            };

        } // end namespace import
    }     // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_IMPORT_COMPILATION_HPP
