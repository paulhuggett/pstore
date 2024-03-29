//===- include/pstore/os/memory_mapper.hpp ----------------*- mode: C++ -*-===//
//*                                             *
//*  _ __ ___   ___ _ __ ___   ___  _ __ _   _  *
//* | '_ ` _ \ / _ \ '_ ` _ \ / _ \| '__| | | | *
//* | | | | | |  __/ | | | | | (_) | |  | |_| | *
//* |_| |_| |_|\___|_| |_| |_|\___/|_|   \__, | *
//*                                      |___/  *
//*                                         *
//*  _ __ ___   __ _ _ __  _ __   ___ _ __  *
//* | '_ ` _ \ / _` | '_ \| '_ \ / _ \ '__| *
//* | | | | | | (_| | |_) | |_) |  __/ |    *
//* |_| |_| |_|\__,_| .__/| .__/ \___|_|    *
//*                 |_|   |_|               *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file memory_mapper.hpp
/// \brief Provides a portable interface for memory mapping portions of a file or for
/// treating regions of memory as though they were memory-mapped files, which is useful for
/// unit testing.

#ifndef PSTORE_OS_MEMORY_MAPPER_HPP
#define PSTORE_OS_MEMORY_MAPPER_HPP

#include "pstore/os/file.hpp"

namespace pstore {

  /// Allocates memory whose start address is a multiple of 'align'. This function uses the O/S
  /// memory allocation API directly and bypasses operator new/malloc(). This is to ensure that
  /// the library can safely change the memory permission (with mprotect() or equivalent).
  std::shared_ptr<std::uint8_t> aligned_valloc (std::size_t size, unsigned align);

  /// An interface for accessing the fundamental virtual memory page size on the host.
  class system_page_size_interface {
  public:
    system_page_size_interface () = default;
    system_page_size_interface (system_page_size_interface &&) noexcept = default;
    system_page_size_interface (system_page_size_interface const &) = default;

    virtual ~system_page_size_interface () noexcept;

    system_page_size_interface & operator= (system_page_size_interface &&) noexcept = default;
    system_page_size_interface & operator= (system_page_size_interface const &) = default;

    virtual unsigned get () const = 0;
  };


  class system_page_size final : public system_page_size_interface {
  public:
    system_page_size () = default;
    ~system_page_size () noexcept override;

    unsigned get () const override { return size_; }

  private:
    unsigned size_ = sysconf ();
    static unsigned sysconf ();
  };


  class memory_mapper_base {
  public:
    virtual ~memory_mapper_base () = 0;

    memory_mapper_base (memory_mapper_base &&) noexcept = default;
    memory_mapper_base (memory_mapper_base const &) = default;
    memory_mapper_base & operator= (memory_mapper_base &&) noexcept = default;
    memory_mapper_base & operator= (memory_mapper_base const &) = default;

    //@{
    /// Returns the base address of this memory-mapped region. Returns nullptr if unmap() has
    /// been called.
    std::shared_ptr<void> const & data () { return ptr_; }
    std::shared_ptr<void const> data () const { return ptr_; }
    //@}

    /// \brief Returns true if the memory is to be writable.
    /// \note The operating system may separately protect memory pages, so it's perfectly likely
    /// that a memory page may be read-only even if this method returns true.
    bool is_writable () const { return is_writable_; }

    /// Returns the file offset of the start of the memory represented by this object.
    std::uint64_t offset () const { return offset_; }

    /// Returns the size of the memory region owned by this object.
    std::uint64_t size () const { return size_; }

    /// A convenience method which returns the file offset of the end of the memory represented
    /// by this object.
    std::uint64_t end () const { return offset () + size (); }

    static unsigned long page_size (system_page_size_interface const & intf);

    /// \brief Marks the range of addresses given by addr and len as read-only.
    ///
    /// This function validates the input parameter before calling read_only_impl() which
    /// is responsible for calling the real OS API.
    ///
    /// \param addr  A pointer that describes the starting page of the region of pages whose
    ///              protection attributes are to be changed.
    /// \param len   The size of the region whose protection attributes are to be changed
    /// \note The function is virtual for mocking.
    virtual void read_only (void * addr, std::size_t len);

  protected:
    /// \param ptr          A pointer to the mapped memory.
    /// \param is_writable  If the mapped memory  writeable? If true, then the underlying file,
    /// as given by the 'file' parameters, must be writable.
    /// \param offset       The starting offset within the container for the mapped region.
    /// \param size         The number of mapped bytes.
    memory_mapper_base (std::shared_ptr<void> ptr, bool const is_writable,
                        std::uint64_t const offset, std::uint64_t const size)
            : ptr_{std::move (ptr)}
            , is_writable_{is_writable}
            , offset_{offset}
            , size_{size} {}

  private:
    /// \brief Marks the range of addresses given by addr and len as read-only.
    ///
    /// \param addr  A pointer that describes the starting page of the region of pages whose
    ///              protection attributes are to be changed.
    /// \param len   The size of the region whose protection attributes are to be changed
    ///
    /// \note This method is implemented directly in the base class in order that each subclass
    ///       automatically gains the behavior.
    void read_only_impl (void * addr, std::size_t len);

    /// A pointer to the mapped memory.
    std::shared_ptr<void> ptr_;
    /// True if the underlying memory is writable.
    bool is_writable_;
    /// The starting offset within the file for the mapped region. This value must be correctly
    /// aligned for the host OS.
    std::uint64_t offset_;
    /// The number of mapped bytes.
    std::uint64_t size_;
  };

  std::ostream & operator<< (std::ostream & os, memory_mapper_base const & mm);

  /// memory_mapper provides an operating system independent interface for memory mapping of
  /// files. The underlying constaints imposed by the OS are not affected. They are:
  ///
  /// Linux: the 'offset' parameter must be a multiple of the value returned by
  /// sysconf(_SC_PAGESIZE)
  /// Windows: the 'offset' parameter must be a multiple of the allocation granularity given by
  /// SYSTEM_INFO structure filled in by a call to GetSystemInfo().

  class memory_mapper final : public memory_mapper_base {
  public:
    /// \param file           The file whose contents are to be mapped into memory.
    /// \param write_enabled  Should the mapped memory be writeable? If true, then the
    ///                       underlying file, as given by the 'file' parameter, must be
    ///                       writable.
    /// \param offset         The starting offset within the file for the mapped region. This
    ///                       value must be correctly aligned for the host OS.
    /// \param length         The number of bytes to be mapped.

    memory_mapper (file::file_handle & file, bool write_enabled, std::uint64_t offset,
                   std::uint64_t length);
    ~memory_mapper () noexcept override;

  private:
    static std::shared_ptr<void> mmap (file::file_handle & file, bool write_enabled,
                                       std::uint64_t offset, std::uint64_t length);
  };


  class in_memory_mapper : public memory_mapper_base {
  public:
    in_memory_mapper (file::in_memory & file, bool const write_enabled, std::uint64_t const offset,
                      std::uint64_t const length)
            : memory_mapper_base (pointer (file, offset), write_enabled, offset, length) {}
    ~in_memory_mapper () noexcept override;

    static std::shared_ptr<std::uint8_t> pointer (pstore::file::in_memory & file,
                                                  std::uint64_t const offset) {
      auto const p = std::static_pointer_cast<std::uint8_t> (file.data ());
      return std::shared_ptr<std::uint8_t> (p, p.get () + offset);
    }
  };

} // namespace pstore
#endif // PSTORE_OS_MEMORY_MAPPER_HPP
