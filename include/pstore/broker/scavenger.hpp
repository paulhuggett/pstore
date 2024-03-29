//===- include/pstore/broker/scavenger.hpp ----------------*- mode: C++ -*-===//
//*                                                 *
//*  ___  ___ __ ___   _____ _ __   __ _  ___ _ __  *
//* / __|/ __/ _` \ \ / / _ \ '_ \ / _` |/ _ \ '__| *
//* \__ \ (_| (_| |\ V /  __/ | | | (_| |  __/ |    *
//* |___/\___\__,_| \_/ \___|_| |_|\__, |\___|_|    *
//*                                |___/            *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file scavenger.hpp

#ifndef PSTORE_BROKER_SCAVENGER_HPP
#define PSTORE_BROKER_SCAVENGER_HPP

#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

namespace pstore::broker {

  class command_processor;

  class scavenger {
  public:
    explicit scavenger (std::weak_ptr<command_processor> const & cp)
            : cp_{cp} {}

    // no copying or deletion.
    scavenger (scavenger const &) = delete;
    scavenger & operator= (scavenger const &) = delete;

    void thread_entry ();

    void shutdown ();

  private:
    std::mutex mut_;
    std::condition_variable cv_;
    std::weak_ptr<command_processor> cp_;
  };

} // namespace pstore::broker

#endif // PSTORE_BROKER_SCAVENGER_HPP
