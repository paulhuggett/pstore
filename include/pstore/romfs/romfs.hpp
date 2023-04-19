//===- include/pstore/romfs/romfs.hpp ---------------------*- mode: C++ -*-===//
//*                       __      *
//*  _ __ ___  _ __ ___  / _|___  *
//* | '__/ _ \| '_ ` _ \| |_/ __| *
//* | | | (_) | | | | | |  _\__ \ *
//* |_|  \___/|_| |_| |_|_| |___/ *
//*                               *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_ROMFS_ROMFS_HPP
#define PSTORE_ROMFS_ROMFS_HPP

#include <cerrno>
#include <string_view>

#include "pstore/romfs/directory.hpp"
#include "pstore/romfs/dirent.hpp"

namespace pstore::romfs {

  enum class error_code : int {
    einval = static_cast<int> (std::errc::invalid_argument),
    enoent = static_cast<int> (std::errc::no_such_file_or_directory),
    enotdir = static_cast<int> (std::errc::not_a_directory),
  };

  class error_category final : public std::error_category {
  public:
    // The need for this constructor was removed by CWG defect 253 but Clang (prior
    // to 3.9.0) and GCC (before 4.6.4) require its presence.
    error_category () noexcept {} // NOLINT
    char const * name () const noexcept override;
    std::string message (int error) const override;
  };

  inline error_category const category;

  std::error_code make_error_code (pstore::romfs::error_code e);

} // end namespace pstore::romfs

namespace std {

  template <>
  struct is_error_code_enum<pstore::romfs::error_code> : std::true_type {};

} // namespace std

namespace pstore::romfs {

  class open_file;
  class open_directory;

  /// Use to determine the interpretation of the offset parameter to descriptor::seek().
  enum class seek_mode {
    /// The seek offset is relative to the start of the file (equivalent to SEEK_SET).
    set,
    /// The seek offset is relative to the current position indicator (equivalent to
    /// SEEK_CUR).
    cur,
    /// The seek offset is relative to the end of the file (equivalent to SEEK_END).
    end
  };

  //*     _                _      _            *
  //*  __| |___ ___ __ _ _(_)_ __| |_ ___ _ _  *
  //* / _` / -_|_-</ _| '_| | '_ \  _/ _ \ '_| *
  //* \__,_\___/__/\__|_| |_| .__/\__\___/_|   *
  //*                       |_|                *
  class descriptor {
    friend class romfs;

  public:
    std::size_t read (gsl::not_null<void *> buffer, std::size_t size, std::size_t count) const;
    error_or<std::size_t> seek (off_t offset, seek_mode whence) const;
    struct stat stat () const;

  private:
    explicit descriptor (std::shared_ptr<open_file> f)
            : f_{std::move (f)} {}
    // Using a shared_ptr<> here so that descriptor instances can be passed around in
    // the same way as they would if 'descriptor' was the int type that's traditionally
    // used to represent file descriptors.
    std::shared_ptr<open_file> f_;
  };

  //*     _ _             _        _                _      _            *
  //*  __| (_)_ _ ___ _ _| |_   __| |___ ___ __ _ _(_)_ __| |_ ___ _ _  *
  //* / _` | | '_/ -_) ' \  _| / _` / -_|_-</ _| '_| | '_ \  _/ _ \ '_| *
  //* \__,_|_|_| \___|_||_\__| \__,_\___/__/\__|_| |_| .__/\__\___/_|   *
  //*                                                |_|                *
  class dirent_descriptor {
    friend class romfs;

  public:
    dirent const * read () const;
    void rewind () const;

  private:
    explicit dirent_descriptor (std::shared_ptr<open_directory> f)
            : f_{std::move (f)} {}
    std::shared_ptr<open_directory> f_;
  };


  //*                 __     *
  //*  _ _ ___ _ __  / _|___ *
  //* | '_/ _ \ '  \|  _(_-< *
  //* |_| \___/_|_|_|_| /__/ *
  //*                        *
  class romfs {
  public:
    explicit romfs (gsl::not_null<directory const *> root);

    error_or<descriptor> open (std::string_view const & path) const;
    error_or<dirent_descriptor> opendir (std::string_view const & path);
    error_or<struct stat> stat (std::string_view const & path) const;

    error_or<std::string> getcwd () const;
    /// Change the current working directory
    std::error_code chdir (std::string_view const & path);

    /// \brief Check that the file system's structures are intact.
    ///
    /// Since the data is read-only there should be no need to call this function except as
    /// a belf-and-braces debug check.
    bool fsck () const;

  private:
    using dirent_ptr = gsl::not_null<dirent const *>;

    error_or<std::string> dir_to_string (gsl::not_null<directory const *> dir) const;

    static gsl::not_null<dirent const *> directory_to_dirent (gsl::not_null<directory const *> d);

    error_or<dirent_ptr> parse_path (std::string_view const & path) const {
      return parse_path (path, cwd_);
    }

    /// Parse a path string returning the directory-entry to which it refers or an error.
    /// Paths follow the POSIX convention of using a slash ('/') to separate components. A
    /// leading slash indicates that the search should start at the file system's root
    /// directory rather than the default directory given by the \p start_dir argument.
    ///
    /// \param path The path string to be parsed.
    /// \param start_dir  The directory to which the path is relative. Ignored if the
    ///   initial character of the \p path argument is a slash.
    /// \returns  The directory entry described by the \p path argument or an error if the
    ///   string was not valid.
    error_or<dirent_ptr> parse_path (std::string_view const & path,
                                     gsl::not_null<directory const *> start_dir) const;

    gsl::not_null<directory const *> root_;
    gsl::not_null<directory const *> cwd_;
  };

} // end namespace pstore::romfs

#endif // PSTORE_ROMFS_ROMFS_HPP
