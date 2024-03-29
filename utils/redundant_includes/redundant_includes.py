#!/usr/bin/env python3
# ===- utils/redundant_includes/redundant_includes.py ---------------------===//
# *               _                 _             _    *
# *  _ __ ___  __| |_   _ _ __   __| | __ _ _ __ | |_  *
# * | '__/ _ \/ _` | | | | '_ \ / _` |/ _` | '_ \| __| *
# * | | |  __/ (_| | |_| | | | | (_| | (_| | | | | |_  *
# * |_|  \___|\__,_|\__,_|_| |_|\__,_|\__,_|_| |_|\__| *
# *                                                    *
# *  _            _           _            *
# * (_)_ __   ___| |_   _  __| | ___  ___  *
# * | | '_ \ / __| | | | |/ _` |/ _ \/ __| *
# * | | | | | (__| | |_| | (_| |  __/\__ \ *
# * |_|_| |_|\___|_|\__,_|\__,_|\___||___/ *
# *                                        *
# ===----------------------------------------------------------------------===//
#
#  Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
#  See https://github.com/paulhuggett/pstore/blob/master/LICENSE.txt for license
#  information.
#  SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# ===----------------------------------------------------------------------===//
from __future__ import print_function
import argparse
import logging
import os
import platform
import re
import sys


def include_files(path):
    """
    A generator which produces the #includes from the file at the given path.

    :param path:
    """

    with open(path, 'r') as f:
        for line in f:
            # Don't match up to the end of the line to allow for comments.
            r = re.match(r'\s*#\s*include\s*["<](.*)[">]', line)
            if r:
                yield r.group(1)


def filter_by_extensions(name, extensions):
    return any(name.endswith(x) for x in extensions)


def sources_in(directory, extensions):
    """
    A generator which yields the relative paths of source files in the given directory. Only files
    whose extension is in the given by the 'extensions' parameter are returned.

    :param directory:
    :param extensions: A list of the file extensions to be included in the results.
    """

    for root, dirs, files in os.walk(directory):
        for name in files:
            if filter_by_extensions(name, extensions):
                yield os.path.relpath(os.path.join(root, name))


def dependencies_from_source(source_directory, extensions):
    """
    Scans the source files contained within the directory hierarchy rooted at 'source_directory' and
    returns a dictionary whose keys are the source file paths and where the corresponding value is a
    set of files referenced by that source file.

    :param source_directory:  The path of a directory containing source files (files whose extension
                              is .hpp or .cpp)
    :param extensions  A list of the file extensions to be included in the results.
    :return: A dictionary with the source file to component name mapping.
    """

    result = dict()
    is_windows = platform.system() == 'Windows'
    for path in sources_in(source_directory, extensions):
        p = os.path.relpath(path, source_directory)
        if is_windows:
            p = p.replace ('\\', '/')
        result[p] = list(include_files(path))
    return result


def search(graph, callback):
    """
    :param graph The include graph. A dictionary whose key are the names of files. Each value is a
                 list of the files included by the key.
    :param callback  A callable with the signature 'def callback(predecessor,successor)'.
                     'predecessor' is the path of a file with a #include statement; 'successor' is
                     the path of a redundant include. The return value is ignored.
    """

    def notify(*args):
        if args not in notified:
            callback(*args)
            notified.add(args)

    def dfs(vertex, visited):
        # If we've previously visited this vertex then stop immediately. If not, remember this
        # visit.
        if vertex in visited:
            return set()
        visited.add(vertex)

        # If we don't have a record of this file then that's equivalent to there being no referenced
        # files. Unrecorded files  are typically external includes such as the standard library.
        successors = graph.get(vertex, set())

        # Now recursively visit every include that's reachable from here and add them to the
        # result set.
        result = set()
        for successor in successors:
            result |= dfs(successor, visited)

        for successor in successors:
            # Is this file reachable from one of the files that we include?
            if successor in result:
                notify(vertex, successor)

        # Finally add the collection of files that are directly included here.
        return result | {successor for successor in successors}

    notified = set()
    for k in graph.keys():
        dfs(k, set())


def find_redundant_includes(search_dirs, extensions, notify):
    graph = {}
    for directory in search_dirs:
        graph.update(dependencies_from_source(directory, extensions))
    search(graph, notify)


def main(args=sys.argv[1:]):
    parser = argparse.ArgumentParser(description='Check for redundant #include statements',
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('search_dir', default=['./include', './lib'], nargs='*',
                        help='''A collection of directories whose contents are searched for source
                        files containing #include statements.''')
    parser.add_argument('-x', '--extension', default=['.h', '.hpp', '.c', '.cpp'], nargs='*',
                        help='''A list of file extensions. Files to be searched must have a name
                        with one of the extensions listed here.''')
    parser.add_argument('-v', '--verbose', default=0, action='count',
                        help='Produce more verbose output')
    options = parser.parse_args(args)
    find_redundant_includes(options.search_dir, options.extension,
                            lambda pred, succ: print(pred, ':', succ))
    return 0


if __name__ == '__main__':
    logging.getLogger().setLevel(logging.NOTSET)
    sys.exit(main())
