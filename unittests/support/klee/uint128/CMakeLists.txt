#===- unittests/support/klee/uint128/CMakeLists.txt -----------------------===//
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
pstore_add_klee_test (CATEGORY uint128 NAME add DEPENDS pstore-support)
pstore_add_klee_test (CATEGORY uint128 NAME postinc DEPENDS pstore-support)
pstore_add_klee_test (CATEGORY uint128 NAME postdec DEPENDS pstore-support)
pstore_add_klee_test (CATEGORY uint128 NAME shiftleft DEPENDS pstore-support)
