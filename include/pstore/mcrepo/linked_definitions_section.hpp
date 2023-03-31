//===- include/pstore/mcrepo/linked_definitions_section.hpp *- mode: C++ -*-===//
//*  _ _       _            _       _       __ _       _ _   _                  *
//* | (_)_ __ | | _____  __| |   __| | ___ / _(_)_ __ (_) |_(_) ___  _ __  ___  *
//* | | | '_ \| |/ / _ \/ _` |  / _` |/ _ \ |_| | '_ \| | __| |/ _ \| '_ \/ __| *
//* | | | | | |   <  __/ (_| | | (_| |  __/  _| | | | | | |_| | (_) | | | \__ \ *
//* |_|_|_| |_|_|\_\___|\__,_|  \__,_|\___|_| |_|_| |_|_|\__|_|\___/|_| |_|___/ *
//*                                                                             *
//*                _   _              *
//*  ___  ___  ___| |_(_) ___  _ __   *
//* / __|/ _ \/ __| __| |/ _ \| '_ \  *
//* \__ \  __/ (__| |_| | (_) | | | | *
//* |___/\___|\___|\__|_|\___/|_| |_| *
//*                                   *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file linked_definitions_section.hpp
/// \brief Contains declarations for the "linked-definition" section type.

#ifndef PSTORE_MCREPO_LINKED_DEFINITIONS_SECTION_HPP
#define PSTORE_MCREPO_LINKED_DEFINITIONS_SECTION_HPP

#include "pstore/mcrepo/compilation.hpp"
#include "pstore/mcrepo/section.hpp"
#include "pstore/support/inherit_const.hpp"

namespace pstore {
  namespace repo {

    namespace details {

      template <typename InputIt>
      auto udistance (InputIt first, InputIt last) {
        auto d = std::distance (first, last);
        PSTORE_ASSERT (d >= 0);
        return static_cast<std::make_unsigned_t<decltype (d)>> (d);
      }

    } // end namespace details

    //*  _ _      _          _      _      __ _      _ _   _              *
    //* | (_)_ _ | |_____ __| |  __| |___ / _(_)_ _ (_) |_(_)___ _ _  ___ *
    //* | | | ' \| / / -_) _` | / _` / -_)  _| | ' \| |  _| / _ \ ' \(_-< *
    //* |_|_|_||_|_\_\___\__,_| \__,_\___|_| |_|_||_|_|\__|_\___/_||_/__/ *
    //*                                                                   *
    /// \brief Represents the definitions linked to a fragment.
    ///
    /// When a new global object is generated by an LLVM optimisation pass after the repository
    /// hash-generation pass has run, the digest for those objects will be generated by the
    /// repository back-end (object writer) code. If a fragment has an external fixup
    /// referencing this global object, we link the two with a record in the linked-definitions
    /// section.
    ///
    /// When a fragment is pruned, its linked definitions need to be recorded in the
    /// repo.tickets metadata to guarantee that they are present in the final compilation
    /// record.
    class linked_definitions : public section_base {
    public:
      struct value_type {
        value_type () noexcept = default;
        constexpr value_type (index::digest const compilation_, std::uint32_t const index_,
                              typed_address<definition> const pointer_) noexcept
                : compilation{compilation_}
                , index{index_}
                , pointer{pointer_} {}

        bool operator== (value_type const & rhs) const noexcept;
        bool operator!= (value_type const & rhs) const noexcept { return !operator== (rhs); }

        /// The digest of the compilation containing the definition to which the owning
        /// fragment is linked.
        index::digest compilation;
        /// The index of the definition within the compilation whose digest is given by \ref
        /// value_type.compilation. This value must be less than the number of definitions
        /// in this compilation.
        std::uint32_t index = 0;
        std::uint32_t unused = 0;
        /// The address of the definition given by \ref value_type.compilation and \ref
        /// value_type.index. This is used as a shortcut by consumers to eliminate the need
        /// to search the compilation index. Its value must be identical to the result of
        /// searching for the digest \ref value_type.compilation and taking the address of
        /// \ref value_type.index definition.
        typed_address<definition> pointer;
      };

      using iterator = value_type *;
      using const_iterator = value_type const *;

      template <typename Iterator,
                typename = typename std::enable_if_t<
                  std::is_same_v<typename std::iterator_traits<Iterator>::value_type, value_type>>>
      linked_definitions (Iterator begin, Iterator end);

      linked_definitions (linked_definitions const &) = delete;
      linked_definitions & operator= (linked_definitions const &) = delete;

      /// \name Element access
      ///@{

      value_type const & operator[] (std::size_t const i) const { return index_impl (*this, i); }
      value_type & operator[] (std::size_t const i) { return index_impl (*this, i); }
      ///@}

      /// \name Iterators
      ///@{

      iterator begin () noexcept { return definitions_; }
      const_iterator begin () const noexcept { return definitions_; }
      const_iterator cbegin () const noexcept { return this->begin (); }

      iterator end () noexcept { return definitions_ + size_; }
      const_iterator end () const noexcept { return definitions_ + size_; }
      const_iterator cend () const noexcept { return this->end (); }
      ///@}

      /// \name Capacity
      ///@{

      /// Checks whether the container is empty.
      bool empty () const noexcept { return size_ == 0; }
      /// Returns the number of elements.
      std::size_t size () const noexcept { return size_; }
      ///@}

      /// \name Storage
      ///@{

      /// Returns the number of bytes of storage required for an instance of this class with
      /// 'size' children.
      static std::size_t size_bytes (std::uint64_t size) noexcept;

      /// Returns the number of bytes of storage required for the linked_definitions instance.
      std::size_t size_bytes () const noexcept;
      ///@}

      /// \brief Returns a pointer to the linked_definitions instance.
      ///
      /// \param db The database from which the linked_definitions should be loaded.
      /// \param ld Address of the linked definitions record in the store.
      /// \result A pointer to the linked_definitions in-store memory.
      static auto load (database const & db, typed_address<linked_definitions> const ld)
        -> std::shared_ptr<linked_definitions const>;

    private:
      template <typename LinkedDefinitions,
                typename ResultType = typename inherit_const<LinkedDefinitions, value_type>::type>
      static ResultType & index_impl (LinkedDefinitions & v, std::size_t pos) {
        PSTORE_ASSERT (pos < v.size ());
        return v.definitions_[pos];
      }

      std::uint64_t size_;
      std::uint64_t unused_ = 0;
      value_type definitions_[1];
    };

    PSTORE_STATIC_ASSERT (std::is_standard_layout<linked_definitions::value_type>::value);
    PSTORE_STATIC_ASSERT (sizeof (linked_definitions::value_type) == 32);
    PSTORE_STATIC_ASSERT (alignof (linked_definitions::value_type) == 16);
    PSTORE_STATIC_ASSERT (offsetof (linked_definitions::value_type, compilation) == 0);
    PSTORE_STATIC_ASSERT (offsetof (linked_definitions::value_type, index) == 16);
    PSTORE_STATIC_ASSERT (offsetof (linked_definitions::value_type, unused) == 20);
    PSTORE_STATIC_ASSERT (offsetof (linked_definitions::value_type, pointer) == 24);

    template <typename Iterator, typename>
    linked_definitions::linked_definitions (Iterator begin, Iterator end)
            : size_{details::udistance (begin, end)} {
      std::copy (begin, end, &definitions_[0]);

      unused_ = 0; // to suppress a warning about the field being unused.
      PSTORE_STATIC_ASSERT (std::is_standard_layout<linked_definitions>::value);
      PSTORE_STATIC_ASSERT (alignof (linked_definitions) == 16);
      PSTORE_STATIC_ASSERT (offsetof (linked_definitions, size_) == 0);
      PSTORE_STATIC_ASSERT (offsetof (linked_definitions, unused_) == 8);
      PSTORE_STATIC_ASSERT (offsetof (linked_definitions, definitions_) == 16);
    }


    template <>
    inline unsigned section_alignment<pstore::repo::linked_definitions> (
      pstore::repo::linked_definitions const &) noexcept {
      return 1U;
    }
    template <>
    inline std::uint64_t section_size<pstore::repo::linked_definitions> (
      pstore::repo::linked_definitions const &) noexcept {
      return 0U;
    }

    std::ostream & operator<< (std::ostream & os, linked_definitions::value_type const & ld);

    //*                  _   _               _ _               _      _             *
    //*  __ _ _ ___ __ _| |_(_)___ _ _    __| (_)____ __  __ _| |_ __| |_  ___ _ _  *
    //* / _| '_/ -_) _` |  _| / _ \ ' \  / _` | (_-< '_ \/ _` |  _/ _| ' \/ -_) '_| *
    //* \__|_| \___\__,_|\__|_\___/_||_| \__,_|_/__/ .__/\__,_|\__\__|_||_\___|_|   *
    //*                                            |_|                              *
    class linked_definitions_creation_dispatcher final : public section_creation_dispatcher {
    public:
      using const_iterator = linked_definitions::value_type const *;

      linked_definitions_creation_dispatcher (const_iterator const begin, const_iterator const end)
              : section_creation_dispatcher (section_kind::linked_definitions)
              , begin_ (begin)
              , end_ (end) {
        PSTORE_ASSERT (std::distance (begin, end) > 0 && "a linked_definitions section must hold "
                                                         "at least one reference to a definition");
      }
      linked_definitions_creation_dispatcher (linked_definitions_creation_dispatcher const &) =
        delete;
      linked_definitions_creation_dispatcher &
      operator= (linked_definitions_creation_dispatcher const &) = delete;

      std::size_t size_bytes () const final;

      std::uint8_t * write (std::uint8_t * out) const final;

      /// Returns a const_iterator for the beginning of the definition address range.
      const_iterator begin () const { return begin_; }
      /// Returns a const_iterator for the end of definition address range.
      const_iterator end () const { return end_; }

    private:
      std::uintptr_t aligned_impl (std::uintptr_t in) const final;

      const_iterator begin_;
      const_iterator end_;
    };

    template <>
    struct section_to_creation_dispatcher<linked_definitions> {
      using type = linked_definitions_creation_dispatcher;
    };

    class linked_definitions_dispatcher final : public dispatcher {
    public:
      explicit linked_definitions_dispatcher (linked_definitions const & d) noexcept
              : d_{d} {}
      ~linked_definitions_dispatcher () noexcept override;

      std::size_t size_bytes () const final { return d_.size_bytes (); }
      unsigned align () const final { error (); }
      std::size_t size () const final { error (); }
      container<internal_fixup> ifixups () const final { error (); }
      container<external_fixup> xfixups () const final { error (); }
      container<std::uint8_t> payload () const final { error (); }

    private:
      PSTORE_NO_RETURN void error () const;
      linked_definitions const & d_;
    };


    template <>
    struct section_to_dispatcher<linked_definitions> {
      using type = linked_definitions_dispatcher;
    };

  } // end namespace repo
} // end namespace pstore

#endif // PSTORE_MCREPO_LINKED_DEFINITIONS_SECTION_HPP
