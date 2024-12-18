//===- include/pstore/adt/chunked_sequence.hpp ------------*- mode: C++ -*-===//
//*       _                 _            _  *
//*   ___| |__  _   _ _ __ | | _____  __| | *
//*  / __| '_ \| | | | '_ \| |/ / _ \/ _` | *
//* | (__| | | | |_| | | | |   <  __/ (_| | *
//*  \___|_| |_|\__,_|_| |_|_|\_\___|\__,_| *
//*                                         *
//*                                            *
//*  ___  ___  __ _ _   _  ___ _ __   ___ ___  *
//* / __|/ _ \/ _` | | | |/ _ \ '_ \ / __/ _ \ *
//* \__ \  __/ (_| | |_| |  __/ | | | (_|  __/ *
//* |___/\___|\__, |\__,_|\___|_| |_|\___\___| *
//*              |_|                           *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file chunked_sequence.hpp
/// \brief Defines the chunked_sequence<> container.
///
/// chunked_sequence<> ensures very fast append times at the expense of only permitting
/// bi-directional iterators. Insertion preserves iterators.
#ifndef PSTORE_ADT_CHUNKED_SEQUENCE_HPP
#define PSTORE_ADT_CHUNKED_SEQUENCE_HPP

#include <algorithm>
#include <array>
#include <list>

#include "pstore/adt/pointer_based_iterator.hpp"
#include "pstore/support/assert.hpp"
#include "pstore/support/inherit_const.hpp"

namespace pstore {

  /// Chunked-sequence is a sequence-container which uses a list of large blocks ("chunks") to
  /// ensure very fast append times at the expense of only permitting bi-directional iterators:
  /// random access is not supported, unlike std::deque<> or std::vector<>.
  ///
  /// Each chunk has storage for a number of objects. This number is a compile-time constant and
  /// is usually reasonably large and chosen so that the memory required is a multiple of the VM
  /// page size. Appending is performed in amortized constant time where we either bump a pointer
  /// in an existing chunk or allocate a new one. Unlike std::vector<>, no moving or copying
  /// occurs after append, and only the past-the-end iterator is invalidated.
  ///
  /// \tparam T The type of the elements.
  /// \tparam ElementsPerChunk The number of elements in an individual chunk.
  /// \tparam ActualSize The storage allocated to an individual element. Normally equal to
  ///   sizeof(T), this can be increased to allow for dynamically-sized types.
  /// \tparam ActualAlign The alignment of an individual element. Normally equal to alignof(T).
  //-MARK: chunked_sequence
  template <typename T, std::size_t ElementsPerChunk = std::max (4096 / sizeof (T), std::size_t{1}),
            std::size_t ActualSize = sizeof (T), std::size_t ActualAlign = alignof (T)>
  class chunked_sequence {
    static_assert (ElementsPerChunk > 0, "Must be at least 1 element per chunk");
    static_assert (ActualSize >= sizeof (T), "ActualSize must be at least sizeof(T)");
    static_assert (ActualAlign >= alignof (T), "ActualAlign must be at least alignof(T)");

    template <bool Const = true>
    class iterator_base;

  public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type &;
    using const_reference = value_type const &;
    using pointer = T *;
    using const_pointer = T const *;

    using iterator = iterator_base<false>;
    using const_iterator = iterator_base<true>;
    // using reverse_iterator =
    // using const_reverse_iterator =

    class chunk;
    using chunk_list = std::list<chunk>;

    /// The number of elements in an individual chunk.
    static constexpr std::size_t elements_per_chunk = ElementsPerChunk;

    chunked_sequence ();
    chunked_sequence (chunked_sequence const &) = delete;
    chunked_sequence (chunked_sequence && other) noexcept = default;

    ~chunked_sequence () noexcept = default;

    chunked_sequence & operator= (chunked_sequence const & other) noexcept = delete;
    chunked_sequence & operator= (chunked_sequence && other) noexcept = default;

    /// Checks whether the container is empty.
    constexpr bool empty () const noexcept { return size_ == 0; }
    /// Returns the number of elements in the container.
    constexpr size_type size () const noexcept { return size_; }

    iterator begin () noexcept { return {chunks_.begin (), 0U}; }
    const_iterator begin () const noexcept {
      PSTORE_ASSERT (empty () || chunks_.front ().size () > 0);
      return {chunks_.begin (), 0U};
    }
    const_iterator cbegin () const noexcept { return begin (); }

    iterator end () noexcept { return end_impl (*this); }
    const_iterator end () const noexcept { return end_impl (*this); }
    const_iterator cend () const noexcept { return end (); }

    /// \name Chunk Access
    ///@{

    /// Returns an iterator to the beginning of the collection of chunks that are used to store
    /// the container's members.
    constexpr typename chunk_list::iterator chunks_begin () noexcept { return chunks_.begin (); }
    constexpr typename chunk_list::const_iterator chunks_begin () const noexcept {
      return chunks_.begin ();
    }
    constexpr typename chunk_list::const_iterator chunks_cbegin () noexcept {
      return chunks_.cbegin ();
    }
    /// Returns an iterator to the end of the collection of chunks that are used to store the
    /// container's members.
    constexpr typename chunk_list::iterator chunks_end () noexcept { return chunks_.end (); }
    constexpr typename chunk_list::const_iterator chunks_end () const noexcept {
      return chunks_.end ();
    }
    constexpr typename chunk_list::const_iterator chunks_cend () noexcept {
      return chunks_.cend ();
    }

    /// Returns the number of chunks of storage being used by the container.
    constexpr std::size_t chunks_size () const noexcept { return chunks_.size (); }

    ///@}

    void clear () {
      chunks_.clear ();
      // Ensure that there's always at least one chunk.
      chunks_.emplace_back ();
      size_ = 0;
    }
    void reserve (std::size_t const /*size*/) {
      // TODO: Not currently implemented.
    }

    /// Returns the number of elements that the container has currently allocated space for.
    ///
    /// \returns Capacity of the currently allocated storage.
    std::size_t capacity () const noexcept { return chunks_.size () * ElementsPerChunk; }

    /// \brief Resizes the container to contain count elements.
    ///
    /// If the current size is greater than count, the container is reduced to its first \p
    /// count elements. If the current size is less than \p count, additional default-inserted
    /// elements are appended.
    ///
    /// \param count  The new size of the container.
    void resize (size_type count) {
      if (count > size_) {
        return this->resize_grow (count);
      }
      if (count < size_) {
        this->resize_shrink (count);
      }
    }

    template <typename... Args>
    reference emplace_back (Args &&... args);

    reference push_back (T const & value) { return emplace_back (value); }
    reference push_back (T && value) { return emplace_back (std::move (value)); }

    /// Returns a reference to the first element in the container. Calling front
    /// on an empty container is undefined.
    reference front () {
      PSTORE_ASSERT (size_ > 0);
      return chunks_.front ().front ();
    }
    /// Returns a reference to the first element in the container. Calling front
    /// on an empty container is undefined.
    const_reference front () const {
      PSTORE_ASSERT (size_ > 0);
      return chunks_.front ().front ();
    }

    /// Returns a reference to the last element in the container. Calling back
    /// on an empty container is undefined.
    reference back () {
      PSTORE_ASSERT (size_ > 0);
      return chunks_.back ().back ();
    }
    /// Returns a reference to the last element in the container. Calling back
    /// on an empty container is undefined.
    const_reference back () const {
      PSTORE_ASSERT (size_ > 0);
      return chunks_.back ().back ();
    }

    void swap (chunked_sequence & other) noexcept {
      std::swap (chunks_, other.chunks_);
      std::swap (size_, other.size_);
    }

    void splice (chunked_sequence && other) {
      // If the other CV is empty then do nothing.
      if (other.empty ()) {
        return;
      }
      // If this CV is empty then we need to replace the pre-allocated chunk rather than
      // splice onto the end of our chunk list.
      if (empty ()) {
        size_ = other.size ();
        chunks_ = std::move (other.chunks_);
        return;
      }
      size_ += other.size ();
      chunks_.splice (chunks_.end (), std::move (other.chunks_));
    }

  private:
    template <typename ChunkedSequence,
              typename Result = inherit_const_t<ChunkedSequence, iterator, const_iterator>>
    static Result end_impl (ChunkedSequence & cv) noexcept;

    std::size_t grow_chunk (chunk * const c, std::size_t limit);
    // Fill the tail chunk with new default-constructed elements.
    std::size_t fill_tail_chunk (size_type count);

    /// Add default-initialized members to increase the number of elements held in the container
    /// to \p count.
    ///
    /// \param count  The number of elements in the container after the resize operation.
    void resize_grow (size_type count);

    /// Remove elements from the end of the container so that it contains \p count members.
    ///
    /// \param count  The number of elements in the container after the resize operation.
    void resize_shrink (size_type count);

    chunk_list chunks_;
    size_type size_ = 0; ///< The number of elements.
  };

  namespace details {

    /// Yields either 'T' or 'T const' depending on the value is IsConst.
    template <typename T, bool IsConst>
    struct value_type {
      using type = std::conditional_t<IsConst, T const, T>;
    };

  } // end namespace details

  // (ctor)
  // ~~~~~~
  template <typename T, std::size_t ElementsPerChunk, std::size_t ActualSize,
            std::size_t ActualAlign>
  chunked_sequence<T, ElementsPerChunk, ActualSize, ActualAlign>::chunked_sequence () {
    // Create an initial, empty chunk. This avoids checking whether the chunk list is empty
    // in the (performance sensitive) append function.
    chunks_.emplace_back ();
  }

  // emplace back
  // ~~~~~~~~~~~~
  template <typename T, std::size_t ElementsPerChunk, std::size_t ActualSize,
            std::size_t ActualAlign>
  template <typename... Args>
  auto
  chunked_sequence<T, ElementsPerChunk, ActualSize, ActualAlign>::emplace_back (Args &&... args)
    -> reference {
    PSTORE_ASSERT (chunks_.size () > 0U);
    auto * tail = &chunks_.back ();
    if (PSTORE_UNLIKELY (tail->size () >= ElementsPerChunk)) {
      // Append a new chunk.
      chunks_.emplace_back ();
      tail = &chunks_.back ();
    }
    // Append a new instance to the tail chunk.
    reference result = tail->emplace_back (std::forward<Args> (args)...);
    ++size_;
    return result;
  }

  // end impl
  // ~~~~~~~~
  template <typename T, std::size_t ElementsPerChunk, std::size_t ActualSize,
            std::size_t ActualAlign>
  template <typename ChunkedSequence, typename Result>
  Result chunked_sequence<T, ElementsPerChunk, ActualSize, ActualAlign>::end_impl (
    ChunkedSequence & cv) noexcept {
    // We always have at least 1 chunk. If the container is empty then return the begin iterator
    // otherwise an iterator referencing end and of the last member of the last chunk.
    PSTORE_ASSERT (cv.chunks_.size () > 0U);
    if (cv.size () > 0U) {
      return {cv.chunks_.end (), 0U};
    }
    return cv.begin ();
  }

  // grow chunk
  // ~~~~~~~~~~
  template <typename T, std::size_t ElementsPerChunk, std::size_t ActualSize,
            std::size_t ActualAlign>
  std::size_t chunked_sequence<T, ElementsPerChunk, ActualSize, ActualAlign>::grow_chunk (
    chunk * const c, std::size_t const limit) {
    PSTORE_ASSERT (limit <= c->capacity ());
    for (auto ctr = limit; ctr > 0U; --ctr) {
      c->emplace_back ();
      ++size_; // Increment size each time in case emplace_back() throws.
    }
    return limit;
  }

  // fill tail chunk
  // ~~~~~~~~~~~~~~~
  template <typename T, std::size_t ElementsPerChunk, std::size_t ActualSize,
            std::size_t ActualAlign>
  std::size_t chunked_sequence<T, ElementsPerChunk, ActualSize, ActualAlign>::fill_tail_chunk (
    size_type count) {
    // First we must fill the current tail chunk with new default-constructed elements.
    auto & tail = chunks_.back ();
    std::size_t const tc = tail.capacity ();
    // If the last chunk partially occupied? If so, fill the rest of it.
    std::size_t const limit = std::min (count, tc);
    count -= this->grow_chunk (&tail, limit);
    PSTORE_ASSERT (tail.capacity () == tc - limit);
    return count;
  }

  // resize grow
  // ~~~~~~~~~~~
  template <typename T, std::size_t ElementsPerChunk, std::size_t ActualSize,
            std::size_t ActualAlign>
  void
  chunked_sequence<T, ElementsPerChunk, ActualSize, ActualAlign>::resize_grow (size_type count) {
    if (count <= size_) {
      return; // Nothing to do.
    }

    count = this->fill_tail_chunk (count - size_);
    // Allocate as many chunks as necessary filling them with the correct number of
    // default-initialized members.
    while (count > 0U) {
      // Append a new chunk.
      chunks_.emplace_back ();
      // Fill the chunk with default-constructed elements.
      count -= this->grow_chunk (&chunks_.back (), std::min (ElementsPerChunk, count));
    }
  }

  // resize shrink
  // ~~~~~~~~~~~~~
  template <typename T, std::size_t ElementsPerChunk, std::size_t ActualSize,
            std::size_t ActualAlign>
  void
  chunked_sequence<T, ElementsPerChunk, ActualSize, ActualAlign>::resize_shrink (size_type count) {
    if (count >= size_) {
      return; // Nothing to do.
    }

    // 'count' becomes the number of elements to remove.
    count = size_ - count;
    auto & head = chunks_.front ();
    while (count > 0U) {
      auto & tail = chunks_.back ();
      auto in_chunk = tail.size ();
      // Note that we don't delete head: there is always at least one chunk.
      if (count >= in_chunk && &tail != &head) {
        chunks_.pop_back ();
      } else {
        PSTORE_ASSERT (in_chunk >= count);
        tail.shrink (in_chunk - count);
        in_chunk = count;
      }
      count -= in_chunk;
      size_ -= in_chunk;
    }
  }

  //*     _             _    *
  //*  __| |_ _  _ _ _ | |__ *
  //* / _| ' \ || | ' \| / / *
  //* \__|_||_\_,_|_||_|_\_\ *
  //*                        *
  //-MARK: chunk
  template <typename T, std::size_t ElementsPerChunk, std::size_t ActualSize,
            std::size_t ActualAlign>
  class chunked_sequence<T, ElementsPerChunk, ActualSize, ActualAlign>::chunk {
  public:
    using iterator = pointer_based_iterator<T>;
    using const_iterator = pointer_based_iterator<T const>;

    chunk () noexcept { // NOLINT
      // Don't use "=default" here. We don't want the zero initializer for membs_ to run.
    }
    chunk (chunk const &) = delete;
    chunk (chunk &&) noexcept = delete;

    ~chunk () noexcept;

    chunk & operator= (chunk const &) = delete;
    chunk & operator= (chunk &&) noexcept = delete;

    T * data () noexcept { return membs_.data (); }
    T const * data () const noexcept { return membs_.data (); }

    T & operator[] (std::size_t const index) noexcept {
      PSTORE_ASSERT (index < ElementsPerChunk);
      return reinterpret_cast<T &> (membs_[index]);
    }
    T const & operator[] (std::size_t const index) const noexcept {
      PSTORE_ASSERT (index < ElementsPerChunk);
      return reinterpret_cast<T const &> (membs_[index]);
    }
    constexpr std::size_t size () const noexcept { return size_; }
    constexpr bool empty () const noexcept { return size_ == 0U; }

    constexpr std::size_t capacity () const noexcept { return ElementsPerChunk - this->size (); }

    reference front () { return (*this)[0]; }
    const_reference front () const { return (*this)[0]; }
    reference back () { return (*this)[size_ - 1U]; }
    const_reference back () const { return (*this)[size_ - 1U]; }


    constexpr auto begin () noexcept { return iterator{&(*this)[0]}; }
    constexpr auto begin () const noexcept { return const_iterator{&(*this)[0]}; }
    constexpr auto end () noexcept { return iterator{begin () + size_}; }
    constexpr auto end () const noexcept { return const_iterator{begin () + size_}; }

    template <typename... Args>
    reference emplace_back (Args &&... args);

    void shrink (std::size_t new_size) noexcept;

  private:
    std::size_t size_ = 0U;
    std::array<typename std::aligned_storage_t<ActualSize, ActualAlign>, ElementsPerChunk> membs_;
  };

  // (dtor)
  // ~~~~~~
  template <typename T, std::size_t ElementsPerChunk, std::size_t ActualSize,
            std::size_t ActualAlign>
  chunked_sequence<T, ElementsPerChunk, ActualSize, ActualAlign>::chunk::~chunk () noexcept {
    this->shrink (0U);
  }

  // emplace back
  // ~~~~~~~~~~~~
  template <typename T, std::size_t ElementsPerChunk, std::size_t ActualSize,
            std::size_t ActualAlign>
  template <typename... Args>
  auto chunked_sequence<T, ElementsPerChunk, ActualSize, ActualAlign>::chunk::emplace_back (
    Args &&... args) -> reference {
    PSTORE_ASSERT (size_ < membs_.size ());
    T & place = (*this)[size_];
    new (&place) T (std::forward<Args> (args)...);
    ++size_;
    return place;
  }

  // shrink
  // ~~~~~~
  template <typename T, std::size_t ElementsPerChunk, std::size_t ActualSize,
            std::size_t ActualAlign>
  void chunked_sequence<T, ElementsPerChunk, ActualSize, ActualAlign>::chunk::shrink (
    std::size_t const new_size) noexcept {
    PSTORE_ASSERT (new_size <= size_);
    std::destroy_n (&(*this)[new_size], size_ - new_size);
    size_ = new_size;
  }

  //*  _ _                _             _                   *
  //* (_) |_ ___ _ _ __ _| |_ ___ _ _  | |__  __ _ ___ ___  *
  //* | |  _/ -_) '_/ _` |  _/ _ \ '_| | '_ \/ _` (_-</ -_) *
  //* |_|\__\___|_| \__,_|\__\___/_|   |_.__/\__,_/__/\___| *
  //*                                                       *
  // -MARK: iterator base
  template <typename T, std::size_t ElementsPerChunk, std::size_t ActualSize,
            std::size_t ActualAlign>
  template <bool IsConst>
  class chunked_sequence<T, ElementsPerChunk, ActualSize, ActualAlign>::iterator_base {
    using list_iterator = typename std::conditional_t<IsConst, typename chunk_list::const_iterator,
                                                      typename chunk_list::iterator>;

    friend class iterator_base<true>;

  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = typename details::value_type<T, IsConst>::type;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type *;
    using reference = value_type &;

    iterator_base (list_iterator const it, std::size_t const index)
            : chunk_it_{it}
            , index_{index} {}
    // Note that we want to allow implicit conversions from non-const to const iterators.
    iterator_base (iterator_base<false> const & rhs) // NOLINT(hicpp-explicit-conversions)
            : chunk_it_{rhs.chunk_it_}
            , index_{rhs.index_} {}
    iterator_base (iterator_base<false> && rhs) // NOLINT(hicpp-explicit-conversions)
            : chunk_it_{std::move (rhs.chunk_it_)}
            , index_{std::move (rhs.index_)} {}

    iterator_base & operator= (iterator_base<false> const & rhs) {
      chunk_it_ = rhs.chunk_it_;
      index_ = rhs.index_;
      return *this;
    }

    iterator_base & operator= (iterator_base<false> && rhs) {
      if (&rhs != this) {
        chunk_it_ = std::move (rhs.it_);
        index_ = std::move (rhs.index_);
      }
      return *this;
    }

    bool operator== (iterator_base const & other) const {
      return chunk_it_ == other.chunk_it_ && index_ == other.index_;
    }
    bool operator!= (iterator_base const & other) const { return !operator== (other); }

    /// Dereference operator
    /// \return the value of the element to which this iterator is currently
    /// pointing.
    reference operator* () const noexcept { return (*chunk_it_)[index_]; }
    pointer operator->() const noexcept { return &(*chunk_it_)[index_]; }

    iterator_base & operator++ ();
    iterator_base operator++ (int);
    iterator_base & operator-- ();
    iterator_base operator-- (int);

  private:
    /// The iterator's position within the container's list of chunks.
    list_iterator chunk_it_;
    /// The iterator's position within the current chunk.
    std::size_t index_;
  };

  // Prefix increment
  template <typename T, std::size_t ElementsPerChunk, std::size_t ActualSize,
            std::size_t ActualAlign>
  template <bool IsConst>
  auto chunked_sequence<T, ElementsPerChunk, ActualSize,
                        ActualAlign>::iterator_base<IsConst>::operator++ ()
    -> iterator_base<IsConst> & {
    if (++index_ >= chunk_it_->size ()) {
      ++chunk_it_;
      index_ = 0;
    }
    return *this;
  }

  // Postfix increment operator (e.g., it++)
  template <typename T, std::size_t ElementsPerChunk, std::size_t ActualSize,
            std::size_t ActualAlign>
  template <bool IsConst>
  auto chunked_sequence<T, ElementsPerChunk, ActualSize,
                        ActualAlign>::iterator_base<IsConst>::operator++ (int)
    -> iterator_base<IsConst> {
    auto const old = *this;
    ++(*this);
    return old;
  }

  // Prefix decrement
  template <typename T, std::size_t ElementsPerChunk, std::size_t ActualSize,
            std::size_t ActualAlign>
  template <bool IsConst>
  auto chunked_sequence<T, ElementsPerChunk, ActualSize,
                        ActualAlign>::iterator_base<IsConst>::operator-- ()
    -> iterator_base<IsConst> & {
    if (index_ > 0U) {
      --index_;
    } else {
      --chunk_it_;
      PSTORE_ASSERT (chunk_it_->size () > 0);
      index_ = chunk_it_->size () - 1U;
    }
    return *this;
  }

  // Postfix decrement
  template <typename T, std::size_t ElementsPerChunk, std::size_t ActualSize,
            std::size_t ActualAlign>
  template <bool IsConst>
  auto chunked_sequence<T, ElementsPerChunk, ActualSize,
                        ActualAlign>::iterator_base<IsConst>::operator-- (int)
    -> iterator_base<IsConst> {
    auto old = *this;
    --(*this);
    return old;
  }

  template <typename T, std::size_t ElementsPerChunk, std::size_t ActualSize,
            std::size_t ActualAlign>
  void swap (chunked_sequence<T, ElementsPerChunk, ActualSize, ActualAlign> & lhs,
             chunked_sequence<T, ElementsPerChunk, ActualSize, ActualAlign> & rhs) noexcept {
    lhs.swap (rhs);
  }

} // end namespace pstore

#endif // PSTORE_ADT_CHUNKED_SEQUENCE_HPP
