//===- unittests/os/test_path.cpp -----------------------------------------===//
//*              _   _      *
//*  _ __   __ _| |_| |__   *
//* | '_ \ / _` | __| '_ \  *
//* | |_) | (_| | |_| | | | *
//* | .__/ \__,_|\__|_| |_| *
//* |_|                     *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file test_path.cpp
/// \brief Unit tests for the path.hpp (path management) interfaces.

#include "pstore/os/path.hpp"

// 3rd party includes
#include <gtest/gtest.h>

TEST (Path, PosixDirName) {
  EXPECT_EQ ("", pstore::path::posix::dir_name (""));
  EXPECT_EQ ("/", pstore::path::posix::dir_name ("/"));
  EXPECT_EQ ("", pstore::path::posix::dir_name ("foo"));
  EXPECT_EQ ("foo/", pstore::path::posix::dir_name ("foo/"));
  EXPECT_EQ ("foo/", pstore::path::posix::dir_name ("foo/bar"));
  EXPECT_EQ ("/foo/", pstore::path::posix::dir_name ("/foo/bar"));
}

TEST (Path, Win32DirName) {
  EXPECT_EQ ("", pstore::path::win32::dir_name (""));

  EXPECT_EQ ("/", pstore::path::win32::dir_name ("/"));
  EXPECT_EQ ("", pstore::path::win32::dir_name ("foo"));
  EXPECT_EQ ("foo/", pstore::path::win32::dir_name ("foo/"));
  EXPECT_EQ ("foo/", pstore::path::win32::dir_name ("foo/bar"));
  EXPECT_EQ ("/foo/", pstore::path::win32::dir_name ("/foo/bar"));

  EXPECT_EQ ("\\", pstore::path::win32::dir_name ("\\"));
  EXPECT_EQ ("", pstore::path::win32::dir_name ("foo"));
  EXPECT_EQ ("foo\\", pstore::path::win32::dir_name ("foo\\"));
  EXPECT_EQ ("foo\\", pstore::path::win32::dir_name ("foo\\bar"));
  EXPECT_EQ ("\\foo\\", pstore::path::win32::dir_name ("\\foo\\bar"));
  EXPECT_EQ ("\\share\\mount\\path\\",
             pstore::path::win32::dir_name ("\\share\\mount\\path\\file"));

  EXPECT_EQ ("c:", pstore::path::win32::dir_name ("c:"));
  EXPECT_EQ ("c:", pstore::path::win32::dir_name ("c:foo"));
  EXPECT_EQ ("c:\\", pstore::path::win32::dir_name ("c:\\foo"));
  EXPECT_EQ ("c:\\foo\\", pstore::path::win32::dir_name ("c:\\foo\\bar"));
  EXPECT_EQ ("c:/", pstore::path::win32::dir_name ("c:/foo"));
  EXPECT_EQ ("c:/foo\\", pstore::path::win32::dir_name ("c:/foo\\bar"));
}

TEST (Path, PlatformDirName) {
  char const * p1 = "c:foo";
#ifdef _WIN32
  using pstore::path::win32::dir_name;
#else
  using pstore::path::posix::dir_name;
#endif
  auto const expected = dir_name (p1);
  EXPECT_EQ (expected, pstore::path::dir_name (p1));
}


TEST (Path, PosixBaseName) {
  EXPECT_EQ ("bar", pstore::path::posix::base_name ("/foo/bar"));
  EXPECT_EQ ("foo", pstore::path::posix::base_name ("foo"));
  EXPECT_EQ ("", pstore::path::posix::base_name ("/foo/bar/"));
}

TEST (Path, Win32BaseName) {
  EXPECT_EQ ("bar", pstore::path::win32::base_name ("/foo/bar"));
  EXPECT_EQ ("bar", pstore::path::win32::base_name ("\\foo\\bar"));
  EXPECT_EQ ("bar", pstore::path::win32::base_name ("\\foo/bar"));
  EXPECT_EQ ("", pstore::path::win32::base_name ("\\foo\\bar\\"));
  EXPECT_EQ ("", pstore::path::win32::base_name ("/foo/bar/"));
  EXPECT_EQ ("foo", pstore::path::win32::base_name ("foo"));
  EXPECT_EQ ("foo", pstore::path::win32::base_name ("d:foo"));
  EXPECT_EQ ("", pstore::path::win32::base_name ("d:"));
  EXPECT_EQ ("", pstore::path::win32::base_name ("d:\\"));
  EXPECT_EQ ("", pstore::path::win32::base_name ("d:/"));
  EXPECT_EQ ("foo", pstore::path::win32::base_name ("d:\\foo"));
  EXPECT_EQ ("file", pstore::path::win32::base_name ("\\share\\mount\\path\\file"));
}

TEST (Path, PlatformBaseName) {
  char const * p = "d:foo";
#ifdef _WIN32
  using pstore::path::win32::base_name;
#else
  using pstore::path::posix::base_name;
#endif
  auto const expected = base_name (p);
  EXPECT_EQ (expected, pstore::path::base_name (p));
}


TEST (Path, JoinPosix) {
  EXPECT_EQ ("", pstore::path::posix::join ("", ""));
  EXPECT_EQ ("", pstore::path::posix::join ("", {}));
  EXPECT_EQ ("", pstore::path::posix::join ("", {""}));
  EXPECT_EQ ("", pstore::path::posix::join ("", {"", ""}));
  EXPECT_EQ ("", pstore::path::posix::join ("", {"", "", ""}));

  EXPECT_EQ ("a", pstore::path::posix::join ("a", {}));
  EXPECT_EQ ("/a", pstore::path::posix::join ("/a", {}));

  EXPECT_EQ ("a/b", pstore::path::posix::join ("a", "b"));
  EXPECT_EQ ("/a/b", pstore::path::posix::join ("/a", "b"));
  EXPECT_EQ ("/b", pstore::path::posix::join ("a", "/b"));
  EXPECT_EQ ("/b", pstore::path::posix::join ("/a", "/b"));
  EXPECT_EQ ("a/b", pstore::path::posix::join ("a/", "b"));
  EXPECT_EQ ("a/b/", pstore::path::posix::join ("a", "b/"));
  EXPECT_EQ ("a/b/", pstore::path::posix::join ("a/", "b/"));
}

TEST (Path, JoinWindows) {
  EXPECT_EQ ("", pstore::path::win32::join ("", ""));
  EXPECT_EQ ("", pstore::path::win32::join ("", {}));
  EXPECT_EQ ("", pstore::path::win32::join ("", {""}));
  EXPECT_EQ ("", pstore::path::win32::join ("", {"", ""}));
  EXPECT_EQ ("", pstore::path::win32::join ("", {"", "", ""}));

  EXPECT_EQ ("a", pstore::path::win32::join ("a", {}));
  EXPECT_EQ ("/a", pstore::path::win32::join ("/a", {}));
  EXPECT_EQ ("\\a", pstore::path::win32::join ("\\a", {}));

  EXPECT_EQ ("a:", pstore::path::win32::join ("a:", {}));
  EXPECT_EQ ("a:\\b", pstore::path::win32::join ("a:", "\\b"));
  EXPECT_EQ ("\\b", pstore::path::win32::join ("a", "\\b"));
  EXPECT_EQ ("a\\b\\c", pstore::path::win32::join ("a", {"b", "c"}));
  EXPECT_EQ ("a\\b\\c", pstore::path::win32::join ("a\\", {"b", "c"}));
  EXPECT_EQ ("a\\b\\c", pstore::path::win32::join ("a", {"b\\", "c"}));
  EXPECT_EQ ("\\c", pstore::path::win32::join ("a", {"b", "\\c"}));
  EXPECT_EQ ("d:\\pleep", pstore::path::win32::join ("d:\\", "\\pleep"));
  EXPECT_EQ ("d:\\a\\b", pstore::path::win32::join ("d:\\", {"a", "b"}));

  EXPECT_EQ ("a", pstore::path::win32::join ("", "a"));
  EXPECT_EQ ("a", pstore::path::win32::join ("", {"", "", "", "a"}));
  EXPECT_EQ ("a\\", pstore::path::win32::join ("a", ""));
  EXPECT_EQ ("a\\", pstore::path::win32::join ("a", {"", "", "", ""}));
  EXPECT_EQ ("a\\", pstore::path::win32::join ("a\\", ""));
  EXPECT_EQ ("a\\", pstore::path::win32::join ("a\\", {"", "", "", ""}));
  EXPECT_EQ ("a/", pstore::path::win32::join ("a/", ""));

  EXPECT_EQ ("a/b\\x/y", pstore::path::win32::join ("a/b", "x/y"));
  EXPECT_EQ ("/a/b\\x/y", pstore::path::win32::join ("/a/b", "x/y"));
  EXPECT_EQ ("/a/b/x/y", pstore::path::win32::join ("/a/b/", "x/y"));
  EXPECT_EQ ("c:x/y", pstore::path::win32::join ("c:", "x/y"));
  EXPECT_EQ ("c:a/b\\x/y", pstore::path::win32::join ("c:a/b", "x/y"));
  EXPECT_EQ ("c:a/b/x/y", pstore::path::win32::join ("c:a/b/", "x/y"));
  EXPECT_EQ ("c:/x/y", pstore::path::win32::join ("c:/", "x/y"));
  EXPECT_EQ ("c:/a/b\\x/y", pstore::path::win32::join ("c:/a/b", "x/y"));
  EXPECT_EQ ("c:/a/b/x/y", pstore::path::win32::join ("c:/a/b/", "x/y"));
  EXPECT_EQ ("//computer/share\\x/y", pstore::path::win32::join ("//computer/share", "x/y"));
  EXPECT_EQ ("//computer/share/x/y", pstore::path::win32::join ("//computer/share/", "x/y"));
  EXPECT_EQ ("//computer/share/a/b\\x/y",
             pstore::path::win32::join ("//computer/share/a/b", "x/y"));

  EXPECT_EQ ("/x/y", pstore::path::win32::join ("a/b", "/x/y"));
  EXPECT_EQ ("/x/y", pstore::path::win32::join ("/a/b", "/x/y"));
  EXPECT_EQ ("c:/x/y", pstore::path::win32::join ("c:", "/x/y"));
  EXPECT_EQ ("c:/x/y", pstore::path::win32::join ("c:a/b", "/x/y"));
  EXPECT_EQ ("c:/x/y", pstore::path::win32::join ("c:/", "/x/y"));
  EXPECT_EQ ("c:/x/y", pstore::path::win32::join ("c:/a/b", "/x/y"));
  EXPECT_EQ ("//computer/share/x/y", pstore::path::win32::join ("//computer/share", "/x/y"));
  EXPECT_EQ ("//computer/share/x/y", pstore::path::win32::join ("//computer/share/", "/x/y"));
  EXPECT_EQ ("//computer/share/x/y", pstore::path::win32::join ("//computer/share/a", "/x/y"));

  EXPECT_EQ ("C:x/y", pstore::path::win32::join ("c:", "C:x/y"));
  EXPECT_EQ ("C:a/b\\x/y", pstore::path::win32::join ("c:a/b", "C:x/y"));
  EXPECT_EQ ("C:/x/y", pstore::path::win32::join ("c:/", "C:x/y"));
  EXPECT_EQ ("C:/a/b\\x/y", pstore::path::win32::join ("c:/a/b", "C:x/y"));

  for (auto x : {"", "a/b", "/a/b", "c:", "c:a/b", "c:/", "c:/a/b"}) {
    for (auto y : {"d:", "d:x/y", "d:/", "d:/x/y"}) {
      EXPECT_EQ (y, pstore::path::win32::join (x, y));
    }
  }
}

TEST (Path, PlatformJoin) {
  char const * p1 = "c:/foo";
  char const * p2 = "d:/bar";
#ifdef _WIN32
  using pstore::path::win32::join;
#else
  using pstore::path::posix::join;
#endif
  auto const expected = join (p1, p2);
  EXPECT_EQ (expected, pstore::path::join (p1, p2));
  EXPECT_EQ (expected, pstore::path::join (p1, {p2}));
}


TEST (Path, PosixSplitDrive) {
  using pair = std::pair<std::string, std::string>;

  EXPECT_EQ (pair ("", "bar"), pstore::path::posix::split_drive ("bar"));
  EXPECT_EQ (pair ("", "/foo/bar"), pstore::path::posix::split_drive ("/foo/bar"));
  EXPECT_EQ (pair ("", "foo/bar"), pstore::path::posix::split_drive ("foo/bar"));
  EXPECT_EQ (pair ("", "c:/foo/bar"), pstore::path::posix::split_drive ("c:/foo/bar"));
}

TEST (Path, Win32SplitDrive) {
  using pair = std::pair<std::string, std::string>;

  EXPECT_EQ (pair ("", ""), pstore::path::win32::split_drive (""));
  EXPECT_EQ (pair ("", "a"), pstore::path::win32::split_drive ("a"));
  EXPECT_EQ (pair ("", "bar"), pstore::path::win32::split_drive ("bar"));
  EXPECT_EQ (pair ("", "/foo/bar"), pstore::path::win32::split_drive ("/foo/bar"));
  EXPECT_EQ (pair ("", "foo/bar"), pstore::path::win32::split_drive ("foo/bar"));

  EXPECT_EQ (pair ("c:", "\\foo\\bar"), pstore::path::win32::split_drive ("c:\\foo\\bar"));
  EXPECT_EQ (pair ("c:", "/foo/bar"), pstore::path::win32::split_drive ("c:/foo/bar"));
  EXPECT_EQ (pair ("\\\\server\\share", "\\foo\\bar"),
             pstore::path::win32::split_drive ("\\\\server\\share\\foo\\bar"));
  EXPECT_EQ (pair ("", "\\\\\\server\\share\\foo\\bar"),
             pstore::path::win32::split_drive ("\\\\\\server\\share\\foo\\bar"));
  EXPECT_EQ (pair ("", "///server/share/foo/bar"),
             pstore::path::win32::split_drive ("///server/share/foo/bar"));
  EXPECT_EQ (pair ("", "\\\\server\\\\share\\foo\\bar"),
             pstore::path::win32::split_drive ("\\\\server\\\\share\\foo\\bar"));
}

TEST (Path, PlatformSplitDrive) {
  char const * input = "c:/foo/bar";
#ifdef _WIN32
  using pstore::path::win32::split_drive;
#else
  using pstore::path::posix::split_drive;
#endif
  auto const expected = split_drive (input);
  EXPECT_EQ (expected, pstore::path::split_drive (input));
}
