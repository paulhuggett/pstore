//===- include/pstore/exchange/export_section.hpp ---------*- mode: C++ -*-===//
//*                             _                   _   _              *
//*   _____  ___ __   ___  _ __| |_   ___  ___  ___| |_(_) ___  _ __   *
//*  / _ \ \/ / '_ \ / _ \| '__| __| / __|/ _ \/ __| __| |/ _ \| '_ \  *
//* |  __/>  <| |_) | (_) | |  | |_  \__ \  __/ (__| |_| | (_) | | | | *
//*  \___/_/\_\ .__/ \___/|_|   \__| |___/\___|\___|\__|_|\___/|_| |_| *
//*           |_|                                                      *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_EXCHANGE_EXPORT_SECTION_HPP
#define PSTORE_EXCHANGE_EXPORT_SECTION_HPP

#include "pstore/exchange/export_fixups.hpp"
#include "pstore/exchange/export_ostream.hpp"
#include "pstore/mcrepo/fragment.hpp"
#include "pstore/support/base64.hpp"

namespace pstore::exchange::export_ns {

  namespace details {

    /// Maps from a stream type to the corresponding OutputIterator that writes
    /// successive objects of type T into stream object for which it was constructed,
    /// using operator<<.
    template <typename OStream, typename T>
    struct output_iterator {};
    template <typename T>
    struct output_iterator<std::ostream, T> {
      using type = std::ostream_iterator<T>;
    };
    template <typename T>
    struct output_iterator<std::ostringstream, T> {
      using type = std::ostream_iterator<T>;
    };
    template <typename T>
    struct output_iterator<ostream_base, T> {
      using type = ostream_inserter;
    };
    template <typename T>
    struct output_iterator<ostream, T> {
      using type = ostream_inserter;
    };
    template <typename T>
    struct output_iterator<ostringstream, T> {
      using type = ostream_inserter;
    };

    template <typename Content>
    class section_content_exporter;
    template <>
    class section_content_exporter<repo::generic_section> {
    public:
      template <typename OStream>
      OStream & operator() (OStream & os, indent const ind, class database const & db,
                            string_mapping const & strings, repo::generic_section const & content,
                            bool const comments) {
        return emit_object (os, ind, content,
                            [&db, &strings, comments] (OStream & os1, indent const ind1,
                                                       repo::generic_section const & content1) {
                              write_member (os1, ind1, content1, db, strings, comments);
                            });
      }

    private:
      template <typename OStream>
      static void write_member (OStream & os, indent const ind,
                                repo::generic_section const & content, class database const & db,
                                string_mapping const & strings, bool comments) {
        auto const * separator = "";
        if (auto const align = content.align (); align != 1U) {
          os << ind << R"("align":)" << align;
          separator = ",\n";
        }
        {
          os << separator << ind << R"("data":")";
          repo::container<std::uint8_t> const payload = content.payload ();
          using output_iterator = typename details::output_iterator<OStream, char>::type;
          to_base64 (std::begin (payload), std::end (payload), output_iterator{os});
          os << '"';
        }
        if (repo::container<repo::internal_fixup> const ifx = content.ifixups (); !ifx.empty ()) {
          os << ",\n" << ind << R"("ifixups":)";
          emit_internal_fixups (os, ind, std::begin (ifx), std::end (ifx));
        }
        if (repo::container<repo::external_fixup> const xfx = content.xfixups (); !xfx.empty ()) {
          os << ",\n" << ind << R"("xfixups":)";
          emit_external_fixups (os, ind, db, strings, std::begin (xfx), std::end (xfx), comments);
        }
        os << '\n';
      }
    };

    template <>
    class section_content_exporter<repo::bss_section> {
    public:
      using bsss = repo::bss_section;

      template <typename OStream>
      OStream & operator() (OStream & os, indent const ind, class database const & /*db*/,
                            string_mapping const & /*strings*/, bsss const & content,
                            bool const /*comments*/) {
        return emit_object (os, ind, content,
                            [] (OStream & os1, indent const ind1, bsss const & content1) {
                              auto const * separator = "";
                              if (auto const align = content1.align (); align != 1U) {
                                os1 << ind1 << R"("align":)" << align;
                                separator = ",\n";
                              }
                              os1 << separator << ind1 << R"("size":)" << content1.size () << '\n';
                              PSTORE_ASSERT (content1.ifixups ().empty ());
                              PSTORE_ASSERT (content1.xfixups ().empty ());
                            });
      }
    };

    template <>
    class section_content_exporter<repo::debug_line_section> {
    public:
      using dls = repo::debug_line_section;

      template <typename OStream>
      OStream & operator() (OStream & os, indent const ind, class database const & /*db*/,
                            string_mapping const & /*strings*/, dls const & content,
                            bool const /*comments*/) {
        return emit_object (os, ind, content, write_member<OStream>);
      }

    private:
      template <typename OStream>
      static void write_member (OStream & os, indent const ind, dls const & content) {
        PSTORE_ASSERT (content.align () == 1U);
        PSTORE_ASSERT (content.xfixups ().size () == 0U);
        os << ind << R"("header":)";
        emit_digest (os, content.header_digest ());
        os << ",\n";
        write_data (os, ind, content);
        write_ifixups (os, ind, content);
      }

      template <typename OStream>
      static void write_data (OStream & os, indent const ind, dls const & content) {
        os << ind << R"("data":")";
        repo::container<std::uint8_t> const payload = content.payload ();
        using output_iterator = typename details::output_iterator<OStream, char>::type;
        to_base64 (std::begin (payload), std::end (payload), output_iterator{os});
        os << "\",\n";
      }

      template <typename OStream>
      static void write_ifixups (OStream & os, indent const ind, dls const & content) {
        os << ind << R"("ifixups":)";
        repo::container<repo::internal_fixup> const ifixups = content.ifixups ();
        emit_internal_fixups (os, ind, std::begin (ifixups), std::end (ifixups));
        os << '\n';
      }
    };

    template <>
    class section_content_exporter<repo::linked_definitions> {
    public:
      template <typename OStream>
      OStream & operator() (OStream & os, indent const ind, class database const & /*db*/,
                            string_mapping const & /*strings*/,
                            repo::linked_definitions const & content, bool const /*comments*/) {
        return emit_array (
          os, ind, std::begin (content), std::end (content),
          [] (OStream & os1, indent const ind1, repo::linked_definitions::value_type const & d) {
            os1 << ind1 << '{' << R"("compilation":)";
            emit_digest (os1, d.compilation);
            os1 << R"(,"index":)" << d.index << '}';
          });
      }
    };

    template <repo::section_kind Kind,
              typename Content = typename repo::enum_to_section<Kind>::type>
    struct section_exporter {
      template <typename OStream>
      OStream & operator() (OStream & os, indent const ind, class database const & db,
                            string_mapping const & strings, Content const & content,
                            bool comments) {
        return section_content_exporter<Content>{}(os, ind, db, strings, content, comments);
      }
    };

  } // end namespace details

  template <repo::section_kind Kind, typename OStream,
            typename Content = typename repo::enum_to_section<Kind>::type>
  OStream & emit_section (OStream & os, indent const ind, class database const & db,
                          string_mapping const & strings, Content const & content,
                          bool const comments) {
    return details::section_exporter<Kind>{}(os, ind, db, strings, content, comments);
  }

} // end namespace pstore::exchange::export_ns

#endif // PSTORE_EXCHANGE_EXPORT_SECTION_HPP
