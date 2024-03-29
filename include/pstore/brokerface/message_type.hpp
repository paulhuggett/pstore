//===- include/pstore/brokerface/message_type.hpp ---------*- mode: C++ -*-===//
//*                                             _                     *
//*  _ __ ___   ___  ___ ___  __ _  __ _  ___  | |_ _   _ _ __   ___  *
//* | '_ ` _ \ / _ \/ __/ __|/ _` |/ _` |/ _ \ | __| | | | '_ \ / _ \ *
//* | | | | | |  __/\__ \__ \ (_| | (_| |  __/ | |_| |_| | |_) |  __/ *
//* |_| |_| |_|\___||___/___/\__,_|\__, |\___|  \__|\__, | .__/ \___| *
//*                                |___/            |___/|_|          *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file message_type.hpp
/// \brief Declares the pstore::brokerface::message_type class. Instances of this type are pushed
/// into the named pipe by clients; the broker then reassembles complete messages from these pieces.

#ifndef PSTORE_BROKERFACE_MESSAGE_TYPE_HPP
#define PSTORE_BROKERFACE_MESSAGE_TYPE_HPP

#include <cstdint>

#ifndef _WIN32
#  include <limits.h> // POSIX definition of PIPE_BUF
#  ifndef PIPE_BUF
#    define PIPE_BUF (512) // POSIX minimum value for PIPE_BUF
#  endif
#endif

#include "pstore/support/error.hpp"

namespace pstore::brokerface {

  constexpr std::size_t message_size = 256;

#ifdef PIPE_BUF
  static_assert (message_size < PIPE_BUF, "message_size must be smaller than PIPE_BUF to "
                                          "ensure messages are transferred atomically");
#endif

  /// Instances of this structure are written to the broker's communication pipe.
  class message_type {
  public:
    constexpr message_type () noexcept = default;

    /// \param mid The message ID.
    /// \param pno The message part number.
    /// \param nump The number of parts making up this message.
    /// \param content A string which will be the payload of this message packet. If shorter
    /// than #payload_chars, the remainder of the message is padded with null characters. If
    /// longer, the content will be truncated.
    message_type (std::uint32_t mid, std::uint16_t pno, std::uint16_t nump,
                  std::string const & content);

    template <typename InputIterator>
    message_type (std::uint32_t const mid, std::uint16_t const pno, std::uint16_t const nump,
                  InputIterator first, InputIterator last)
            : sender_id{process_id}
            , message_id{mid}
            , part_no{pno}
            , num_parts{nump}
            , payload (get_payload (first, last)) {

      if (part_no >= num_parts) {
        raise (::pstore::error_code::bad_message_part_number);
      }
    }

    bool operator== (message_type const & rhs) const;
    bool operator!= (message_type const & rhs) const { return !operator== (rhs); }

    /// The maximum number of characters that can be send in a single message packet.
    static constexpr std::size_t payload_chars =
      message_size - 2 * sizeof (std::uint32_t) - 2 * sizeof (std::uint16_t);

    static std::uint32_t const process_id;

    using payload_type = std::array<char, payload_chars>;

    /// The message sender's ID. By convention this is the sender's process ID. The intent
    /// is that the sender_id and message_id fields together form a pair which uniquely
    /// identifies this message.
    std::uint32_t const sender_id = 0;

    /// \brief An identifier for this message.
    /// The message ID combines with the #sender_id field to form a pair which uniquely
    /// identifies this message.
    std::uint32_t const message_id = 0;

    /// \brief A single large message can be split into several parts by the sender.
    /// This value indicates which part of the overall data this specific packet represents.
    /// Must be less than #num_parts.
    std::uint16_t const part_no = 0;

    /// \brief The total number of parts that make up this message.
    std::uint16_t const num_parts = 1;

    /// \brief A character array containing the actual message content.
    payload_type const payload{{'\0'}};

  private:
    template <typename InputIterator>
    static payload_type get_payload (InputIterator first, InputIterator last) {
      payload_type result;

      auto dist = std::distance (first, last);
      using distance_type = decltype (dist);
      using udistance_type = typename std::make_unsigned_t<distance_type>;
      auto count = static_cast<udistance_type> (std::max (dist, distance_type{0}));

      static_assert (payload_chars < std::numeric_limits<udistance_type>::max (),
                     "payload chars is too large for static conversion to difference_type");
      count = std::min (count, static_cast<udistance_type> (payload_chars));
      auto out_first = std::begin (result);
      if (count > 0) {
        out_first = std::copy_n (first, count, out_first);
      }
      std::fill (out_first, std::end (result), '\0');
      return result;
    }
  };

  static_assert (std::is_standard_layout_v<message_type>, "message_type must be standard layout");
  static_assert (sizeof (message_type) == message_size, "sizeof (message_type) != message_size");
  static_assert (offsetof (message_type, sender_id) == 0, "offset of sender_id must be 0");
  static_assert (offsetof (message_type, message_id) == 4, "offset of message_id must be 4");
  static_assert (offsetof (message_type, part_no) == 8, "offset of part_no must be 8");
  static_assert (offsetof (message_type, num_parts) == 10, "offset of num_parts must be 1");
  static_assert (offsetof (message_type, payload) == 12, "offset of payload must be 16");

  using message_ptr = std::unique_ptr<message_type>;

} // end namespace pstore::brokerface

#endif // PSTORE_BROKERFACE_MESSAGE_TYPE_HPP
