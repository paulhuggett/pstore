//===- include/pstore/serialize/types.hpp -----------------*- mode: C++ -*-===//
//*  _                          *
//* | |_ _   _ _ __   ___  ___  *
//* | __| | | | '_ \ / _ \/ __| *
//* | |_| |_| | |_) |  __/\__ \ *
//*  \__|\__, | .__/ \___||___/ *
//*      |___/|_|               *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file serialize/types.hpp
/// \brief Provides serialization capabilities for trivial and user-defined types.
///
/// There are two basic serialization operations: writing and reading. For example:
///
/// - Serializing an object to a byte sequence or counting the number of bytes that would be
///   produced by serialization.
/// - Consuming a byte sequence and using it to produce an object.
///
/// These operations are implemented by instances of an Archiver.
/// The individual pieces of an object are written to, or read from, an Archiver using the
/// serialize() function. This module provides a number of overrides of this function to serialize
/// trivial types as well as some of the common types from the Standard Library.
///
/// Writing the built-in types is easy:


/// \example ex1.cpp
/// This example demonstrates one way in which a sequence of two integers -- 10 and 20 -- can be
/// written to a vector of bytes managed by the serialize::archive::vector_writer class. It finishes
/// by  writing the resulting byte sequence (as a series of two-character hexadecimal digits) to
/// `std::cout`.
///
/// On a little-endian system with a four-byte int type, it will produce the following output:
///
///     Wrote these bytes:
///     0a 00 00 00 14 00 00 00


/// \example ex2.cpp
/// This example shows that writing simple compound standard-layout types is straightforward.
///
/// On a little-endian system with a four-byte int type, it will produce the following output:
///
///     Wrote these bytes:
///     1e 00 00 00 28 00 00 00
///
/// \note This approach is both fast and simple, but does not consider endianness, internal padding,
///       size of fundamental types, or alignment.


/// \example nonpod1.cpp
/// Writing non-standard-layout types requires a custom implementation of the serialize() template
/// function. For many types, this is also a simple operation.
///
/// On a little-endian system with a four-byte int type, it will produce the following output:
///
///     Wrote these bytes: 0a 00 00 00
///     Read: foo(10)


/// \example nonpod2.cpp
/// This example shows how it is possible to stream a simple non-standard-layout type without having
/// to modify the class itself. This involves writing an explicit specialization of the
/// `serialize::serializer<>` template which implements read() and write() methods.
///
/// On a little-endian system with a four-byte int type, it will produce the following output:
///
///     Writing: foo(42)
///     Wrote these bytes: 2a 00 00 00
///     Read: foo(42)


/// \example vector_int_writer.cpp
/// This example shows how to read data from a container of bytes (a std::vector<> in this
/// particular case).
///
/// On a little-endian system with a four-byte int type, it will produce the following output:
///
/// Reading two ints from the following input data:
///
///     1e 00 00 00 28 00 00 00
///     Reading one int at a time produced 30, 40
///     Reading an array of ints produced 30, 40
///     Reading a series of ints produced 30, 40

#ifndef PSTORE_SERIALIZE_TYPES_HPP
#define PSTORE_SERIALIZE_TYPES_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <type_traits>

#include "pstore/serialize/common.hpp"
#include "pstore/support/assert.hpp"
#include "pstore/support/gsl.hpp"

namespace pstore::serialize {

  template <typename Archive>
  using archive_result_type = typename std::remove_reference_t<Archive>::result_type;

  /// \brief The primary template for serialization of non standard layout types.
  ///
  /// Define read() and write() methods which understand how to transfer an instance of an
  /// object of type Ty to and from an archive.
  ///
  /// The default implementations of these functions rely on the type implementing a
  /// de-archiving constructor and a write() method respectively; see read() and write() for
  /// details.
  template <typename Ty, typename Enable = void>
  struct serializer {
    /// \brief Writes an single value of type Ty to an archive.
    ///
    /// Requires that the type Ty implements a method with the following signature:
    ///
    ///     template <typename Archive>
    ///     void write (Archive & archive) const;
    ///
    /// This function should write the contents of the object to the supplied archive
    /// instance.
    ///
    /// \param archive  The archive to which the value 'ty' should be written.
    /// \param v        An object which is the source for the write operation. Its contents
    ///                 are written to the archive.
    template <typename Archive>
    static auto write (Archive & archive, Ty const & v) -> archive_result_type<Archive> {
      return v.write (archive);
    }

    /// \brief Reads a value of type Ty from an archive.
    ///
    /// Requires that the type Ty provides a constructor with the following signature:
    ///
    ///     template <typename Archive>
    ///     explicit Ty (Archive & archive);
    ///
    /// (You are encouraged to make such a constructor `explicit`, although this is not
    /// required.) This method should build the value of the new object from the contents
    /// of the supplied archive.
    ///
    /// \param archive  The archive from which the value will be read.
    /// \param v        The memory into which a newly constructed instance of Ty should be
    ///   placed. This is uninitialized memory suitable for construction of an instance of
    ///   type Ty.
    template <typename Archive>
    static void read (Archive & archive, Ty & v) {
      PSTORE_ASSERT (reinterpret_cast<std::uintptr_t> (&v) % alignof (Ty) == 0);
      new (&v) Ty (archive);
    }
  };


  namespace details {

    struct getn_helper {
    public:
      template <typename Archive, typename ElementType, std::ptrdiff_t Extent>
      static void getn (Archive & archive, gsl::span<ElementType, Extent> span) {
        getn_helper::invoke (archive, span, nullptr);
      }

    private:
      /// This overload is always in the set of overloads but a function with
      /// ellipsis parameter has the lowest ranking for overload resolution.
      template <typename Archive, typename ElementType, std::ptrdiff_t Extent>
      static void invoke (Archive & archive, gsl::span<ElementType, Extent> span, ...) {
        for (auto & v : span) {
          archive.get (v);
        }
      }

      /// This overload is called if Archive has a getn() method.
      template <typename Archive, typename ElementType, std::ptrdiff_t Extent>
      static void
      invoke (Archive & archive, gsl::span<ElementType, Extent> span,
              decltype (&std::remove_reference_t<Archive>::template getn<ElementType, Extent>)) {
        archive.getn (span);
      }
    };

    struct readn_helper {
    public:
      template <typename Archive, typename ElementType, std::ptrdiff_t Extent>
      static void readn (Archive & archive, gsl::span<ElementType, Extent> span) {
        readn_helper::invoke (archive, span, nullptr);
      }

    private:
      /// This overload is always in the set of overloads but a function with
      /// ellipsis parameter has the lowest ranking for overload resolution.
      template <typename Archive, typename ElementType, std::ptrdiff_t Extent>
      static void invoke (Archive & archive, gsl::span<ElementType, Extent> span, ...) {
        for (auto & v : span) {
          serializer<ElementType>::read (archive, v);
        }
      }

      /// This overload is called if the serializer for SpanType::element_type has a
      /// readn() method. SFINAE means that we fall back to the ellipsis overload if it
      /// does not.
      template <typename Archive, typename ElementType, std::ptrdiff_t Extent>
      static void
      invoke (Archive & archive, gsl::span<ElementType, Extent> span,
              decltype (&serializer<ElementType>::template readn<std::remove_reference_t<Archive>,
                                                                 ElementType, Extent>)) {
        serializer<ElementType>::readn (archive, span);
      }
    };


    //*             _ _              _        _                *
    //* __ __ ___ _(_) |_ ___ _ _   | |_  ___| |_ __  ___ _ _  *
    //* \ V  V / '_| |  _/ -_) ' \  | ' \/ -_) | '_ \/ -_) '_| *
    //*  \_/\_/|_| |_|\__\___|_||_| |_||_\___|_| .__/\___|_|   *
    //*                                        |_|             *
    /// This template class enables us to provide the "writen()" interface on all
    /// serialization classes without forcing every one to reimplement identical boilerplate
    /// code.
    ///
    /// If the target class for the span's element type -- serializer<Span::element_type> --
    /// implements a writen() method then it will be called otherwise we call back to a loop
    /// implemented here.

    struct writen_helper {
    public:
      template <typename Archive, typename ElementType, std::ptrdiff_t Extent>
      static auto writen (Archive & archive, gsl::span<ElementType, Extent> span)
        -> archive_result_type<std::decay_t<Archive>> {
        return writen_helper::invoke (archive, span, nullptr);
      }

    private:
      template <typename Archive, typename ElementType, std::ptrdiff_t Extent>
      static auto invoke (Archive & archive, gsl::span<ElementType, Extent> span, ...)
        -> archive_result_type<std::decay_t<Archive>> {
        // A simmple implementation of writen() which loops over the span calling
        // write() for each element.
        sticky_assign<archive_result_type<Archive>> r;
        for (auto & v : span) {
          r = serializer<std::decay_t<ElementType>>::write (archive, v);
        }
        return r.get ();
      }

      // This overload is called if Archive has a writen() method. SFINAE means that we
      // fall back to the ellipsis overload if it does not.
      template <typename Archive, typename ElementType, std::ptrdiff_t Extent>
      static auto invoke (Archive & archive, gsl::span<ElementType, Extent> span,
                          decltype (&serializer<std::decay_t<ElementType>>::template writen<
                                    std::remove_reference_t<Archive>, ElementType, Extent>))
        -> archive_result_type<std::decay_t<Archive>> {
        return serializer<std::decay_t<ElementType>>::writen (archive, span);
      }
    };

  } // namespace details

  /// \brief A serializer for trivial types
  template <typename Ty>
  struct serializer<Ty, typename std::enable_if_t<std::is_trivial_v<Ty>>> {
    /// \brief Writes an individual object to an archive.
    ///
    /// \param archive  The archive to which the span will be written.
    /// \param v        The object value which is to be written.
    template <typename Archive>
    static auto write (Archive & archive, Ty const & v) -> archive_result_type<Archive> {
      return archive.put (v);
    }

    /// \brief Writes a span of objects to an archive.
    ///
    /// \param archive  The archive to which the span will be written.
    /// \param span     The span which is to be written.
    template <typename Archive, typename ElementType, std::ptrdiff_t Extent,
              typename = std::enable_if_t<std::is_same_v<ElementType, Ty>>>
    static auto writen (Archive & archive, gsl::span<ElementType, Extent> span)
      -> archive_result_type<Archive> {
      return archive.putn (span);
    }

    /// \brief Reads a standard-layout value of type Ty from an archive.
    ///
    /// \param archive  The archive from which the value will be read.
    /// \param out      A reference to uninitialized memory into which an instance of Ty
    ///                 will be read.
    template <typename Archive>
    static void read (Archive & archive, Ty & out) {
      PSTORE_ASSERT (reinterpret_cast<std::uintptr_t> (&out) % alignof (Ty) == 0);
      archive.get (out);
    }

    /// \brief Reads a standard-layout value of type Ty from an archive.
    ///
    /// \param archive  The archive from which the value will be read.
    /// \param span     A span pointing to uninitialized memory
    template <typename Archive, typename ElementType, std::ptrdiff_t Extent,
              typename = std::enable_if_t<std::is_same_v<ElementType, Ty>>>
    static void readn (Archive & archive, gsl::span<ElementType, Extent> span) {
      details::getn_helper::getn (archive, span);
    }
  };


  /// \brief If the two types T1 and T2 have a compatible representation when serialized,
  /// provides the member constant value equal to `true`, otherwise `false`.
  template <typename T1, typename T2>
  struct is_compatible : std::false_type {};
  template <typename T>
  struct is_compatible<T, T> : std::true_type {};

  template <typename T1, typename T2>
  struct is_compatible<T1 const, T2> : is_compatible<T1, T2> {};
  template <typename T1, typename T2>
  struct is_compatible<T1, T2 const> : is_compatible<T1, T2> {};
  template <typename T1, typename T2>
  struct is_compatible<T1 const, T2 const> : is_compatible<T1, T2> {};

  template <typename T1, typename T2>
  constexpr bool is_compatible_v = is_compatible<T1, T2>::value;

  template <typename Archive, typename ElementType>
  void read_uninit (Archive & archive, ElementType & uninit) {
    serializer<ElementType>::read (archive, uninit);
  }
  /// A read of a span of 1 is optimized into a read of an individual value.
  template <typename Archive, typename ElementType>
  void read_uninit (Archive & archive, gsl::span<ElementType, 1> uninit_span) {
    static_assert (uninit_span.size () == 1, "Expected span to be 1 in specialization");
    serializer<ElementType>::read (archive, uninit_span[0]);
  }

  template <typename Archive, typename ElementType, std::ptrdiff_t Extent>
  void read_uninit (Archive & archive, gsl::span<ElementType, Extent> uninit_span) {
    details::readn_helper::readn (archive, uninit_span);
  }


#ifndef NDEBUG
  template <std::ptrdiff_t SpanExtent>
  void flood (gsl::span<std::uint8_t, SpanExtent> sp) noexcept {
    static std::array<std::uint8_t, 4> const deadbeef{{0xDE, 0xAD, 0xBE, 0xEF}};

    auto it = std::begin (sp);
    auto end = std::end (sp);

    for (auto uint32_count = sp.size () / 4U; uint32_count > 0U; --uint32_count) {
      *(it++) = deadbeef[0];
      *(it++) = deadbeef[1];
      *(it++) = deadbeef[2];
      *(it++) = deadbeef[3];
    }

    auto index = std::size_t{0};
    for (; it != end; ++it) {
      *it = deadbeef.at (index);
      index = (index + 1) % deadbeef.size ();
    }
  }
#else
  template <std::ptrdiff_t SpanExtent>
  constexpr void flood (gsl::span<std::uint8_t, SpanExtent>) noexcept {}
#endif
  template <typename T>
  inline void flood (T * const t) noexcept {
    flood (gsl::make_span (reinterpret_cast<std::uint8_t *> (t), sizeof (T)));
  }

  //*                  _  *
  //*  _ _ ___ __ _ __| | *
  //* | '_/ -_) _` / _` | *
  //* |_| \___\__,_\__,_| *
  //*                     *
  ///@{
  /// \brief Read a single value from an archive
  template <typename Ty, typename Archive>
  Ty read (Archive & archive) {
    using T2 = typename std::remove_const_t<Ty>;
    std::aligned_storage_t<sizeof (T2), alignof (T2)> uninit_buffer;
    flood (&uninit_buffer);

    // Deserialize into the uninitialized buffer.
    auto & t2 = reinterpret_cast<T2 &> (uninit_buffer);
    read_uninit (archive, t2);

    // This object will destroy the remains of the T2 instance in uninit_buffer.
    auto const dtor = [] (T2 * const p) { p->~T2 (); };
    std::unique_ptr<T2, decltype (dtor)> d (&t2, dtor);
    return std::move (t2);
  }

  /// \brief Read a span containing a single value from an archive.
  /// This is optimized as a read of a single value.
  template <typename Ty, typename Archive>
  void read (Archive & archive, gsl::span<Ty, 1> span) {
    PSTORE_ASSERT (span.size () == 1U);
    span[0] = read<Ty> (archive);
  }

  template <typename ElementType, std::ptrdiff_t Extent, typename Archive>
  void read (Archive & archive, gsl::span<ElementType, Extent> span) {
    for (auto & element : span) {
      element.~ElementType ();
    }
    read_uninit (archive, span);
  }
  ///@}

  //*             _ _        *
  //* __ __ ___ _(_) |_ ___  *
  //* \ V  V / '_| |  _/ -_) *
  //*  \_/\_/|_| |_|\__\___| *
  //*                        *
  /// \brief Write a series of one of more values to an archive.
  template <typename Archive, typename Ty>
  auto write (Archive & archive, Ty const & ty) -> archive_result_type<Archive> {
    return serializer<Ty>::write (archive, ty);
  }

  template <typename Archive, typename Ty, typename... Args>
  auto write (Archive & archive, Ty const & ty, Args const &... args)
    -> archive_result_type<Archive> {
    auto const result = write (archive, ty);
    write (archive, args...);
    return result;
  }
  ///@{
  /// \brief Write a span of elements to an archive.
  template <typename Archive, typename ElementType, std::ptrdiff_t Extent>
  auto write (Archive & archive, gsl::span<ElementType, Extent> sp)
    -> archive_result_type<Archive> {
    return details::writen_helper::writen (archive, sp);
  }

  /// \brief Write a single-element span to an archive.
  template <typename Archive, typename ElementType>
  auto write (Archive & archive, gsl::span<ElementType, 1> sp) -> archive_result_type<Archive> {
    static_assert (sp.size () == 1, "Expected size to be 1 in specialization");
    return serializer<ElementType>::write (archive, sp[0]);
  }

  ///@}

} // namespace pstore::serialize

#endif // PSTORE_SERIALIZE_TYPES_HPP
