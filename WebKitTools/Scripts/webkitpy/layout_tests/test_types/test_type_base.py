#!/usr/bin/env python
# Copyright (C) 2010 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Defines the interface TestTypeBase which other test types inherit from.

Also defines the TestArguments "struct" to pass them additional arguments.
"""

import cgi
import errno
import logging
import os.path

_log = logging.getLogger("webkitpy.layout_tests.test_types.test_type_base")


class TestArguments(object):
    """Struct-like wrapper for additional arguments needed by
    specific tests."""
    # Whether to save new baseline results.
    new_baseline = False

    # Path to the actual PNG file generated by pixel tests
    png_path = None

    # Value of checksum generated by pixel tests.
    hash = None

    # Whether to use wdiff to generate by-word diffs.
    wdiff = False

    # Whether to report the locations of the expected result files used.
    show_sources = False

# Python bug workaround.  See the wdiff code in WriteOutputFiles for an
# explanation.
_wdiff_available = True


class TestTypeBase(object):

    # Filename pieces when writing failures to the test results directory.
    FILENAME_SUFFIX_ACTUAL = "-actual"
    FILENAME_SUFFIX_EXPECTED = "-expected"
    FILENAME_SUFFIX_DIFF = "-diff"
    FILENAME_SUFFIX_WDIFF = "-wdiff.html"
    FILENAME_SUFFIX_COMPARE = "-diff.png"

    def __init__(self, port, platform, root_output_dir):
        """Initialize a TestTypeBase object.

        Args:
          platform: the platform (e.g., 'chromium-mac-leopard')
            identifying the platform-specific results to be used.
          root_output_dir: The unix style path to the output dir.
        """
        self._root_output_dir = root_output_dir
        self._port = port
        self._platform = platform

    def _make_output_directory(self, filename):
        """Creates the output directory (if needed) for a given test
        filename."""
        output_filename = os.path.join(self._root_output_dir,
            self._port.relative_test_filename(filename))
        self._port.maybe_make_directory(os.path.split(output_filename)[0])

    def _save_baseline_data(self, filename, data, modifier):
        """Saves a new baseline file into the platform directory.

        The file will be named simply "<test>-expected<modifier>", suitable for
        use as the expected results in a later run.

        Args:
          filename: path to the test file
          data: result to be saved as the new baseline
          modifier: type of the result file, e.g. ".txt" or ".png"
        """
        relative_dir = os.path.dirname(
            self._port.relative_test_filename(filename))
        output_dir = os.path.join(
            self._port.chromium_baseline_path(self._platform), relative_dir)
        output_file = os.path.basename(os.path.splitext(filename)[0] +
            self.FILENAME_SUFFIX_EXPECTED + modifier)

        self._port.maybe_make_directory(output_dir)
        output_path = os.path.join(output_dir, output_file)
        _log.debug('writing new baseline to "%s"' % (output_path))
        open(output_path, "wb").write(data)

    def output_filename(self, filename, modifier):
        """Returns a filename inside the output dir that contains modifier.

        For example, if filename is c:/.../fast/dom/foo.html and modifier is
        "-expected.txt", the return value is
        c:/cygwin/tmp/layout-test-results/fast/dom/foo-expected.txt

        Args:
          filename: absolute filename to test file
          modifier: a string to replace the extension of filename with

        Return:
          The absolute windows path to the output filename
        """
        output_filename = os.path.join(self._root_output_dir,
            self._port.relative_test_filename(filename))
        return os.path.splitext(output_filename)[0] + modifier

    def compare_output(self, port, filename, output, test_args, target):
        """Method that compares the output from the test with the
        expected value.

        This is an abstract method to be implemented by all sub classes.

        Args:
          filename: absolute filename to test file
          output: a string containing the output of the test
          test_args: a TestArguments object holding optional additional
              arguments
          target: Debug or Release

        Return:
          a list of TestFailure objects, empty if the test passes
        """
        raise NotImplemented

    def write_output_files(self, port, filename, test_type, file_type,
                           output, expected, diff=True, wdiff=False):
        """Writes the test output, the expected output and optionally the diff
        between the two to files in the results directory.

        The full output filename of the actual, for example, will be
          <filename><test_type>-actual<file_type>
        For instance,
          my_test-simp-actual.txt

        Args:
          filename: The test filename
          test_type: A string describing the test type, e.g. "simp"
          file_type: A string describing the test output file type, e.g. ".txt"
          output: A string containing the test output
          expected: A string containing the expected test output
          diff: if True, write a file containing the diffs too. This should be
              False for results that are not text
          wdiff: if True, write an HTML file containing word-by-word diffs
        """
        self._make_output_directory(filename)
        actual_filename = self.output_filename(filename,
            test_type + self.FILENAME_SUFFIX_ACTUAL + file_type)
        expected_filename = self.output_filename(filename,
            test_type + self.FILENAME_SUFFIX_EXPECTED + file_type)
        if output:
            open(actual_filename, "wb").write(output)
        if expected:
            open(expected_filename, "wb").write(expected)

        if not output or not expected:
            return

        if diff:
            diff = port.diff_text(expected, output, expected_filename,
                                  actual_filename)
            diff_filename = self.output_filename(filename,
                test_type + self.FILENAME_SUFFIX_DIFF + file_type)
            open(diff_filename, "wb").write(diff)

        if wdiff:
            # Shell out to wdiff to get colored inline diffs.
            wdiff = port.wdiff_text(expected_filename, actual_filename)
            filename = self.output_filename(filename, test_type +
                                            self.FILENAME_SUFFIX_WDIFF)
            out = open(filename, 'wb').write(wdiff)
