#===- unittests/brokerface/CMakeLists.txt ---------------------------------===//
#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===----------------------------------------------------------------------===//
#
# Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
# See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
# information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===//
add_pstore_unit_test (
  pstore-brokerface-unit-tests test_message_type.cpp test_pubsub.cpp
  test_send_message.cpp
)

# The pstore-brokerface library is already linked by pstore-unit-test-common.
target_link_libraries (
  pstore-brokerface-unit-tests PUBLIC pstore-unit-test-common
)
