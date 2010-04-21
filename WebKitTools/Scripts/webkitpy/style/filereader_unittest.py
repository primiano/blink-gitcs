# Copyright (C) 2010 Chris Jerdonek (cjerdonek@webkit.org)
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Contains unit tests for filereader.py."""

import os
import shutil
import tempfile
import unittest

from webkitpy.common.system.logtesting import LoggingTestCase
from webkitpy.style.checker import ProcessorBase
from webkitpy.style.filereader import TextFileReader


class TextFileReaderTest(LoggingTestCase):

    class MockProcessor(ProcessorBase):

        """A processor for test purposes.

        This processor simply records the parameters passed to its process()
        method for later checking by the unittest test methods.

        """

        def __init__(self):
            self.processed = []
            """The parameters passed for all calls to the process() method."""

        def should_process(self, file_path):
            return not file_path.endswith('should_not_process.txt')

        def process(self, lines, file_path, test_kwarg=None):
            self.processed.append((lines, file_path, test_kwarg))

    def setUp(self):
        LoggingTestCase.setUp(self)
        processor = TextFileReaderTest.MockProcessor()

        temp_dir = tempfile.mkdtemp()

        self._file_reader = TextFileReader(processor)
        self._processor = processor
        self._temp_dir = temp_dir

    def tearDown(self):
        LoggingTestCase.tearDown(self)
        shutil.rmtree(self._temp_dir)

    def _create_file(self, rel_path, text):
        """Create a file with given text and return the path to the file."""
        file_path = os.path.join(self._temp_dir, rel_path)

        file = open(file_path, 'w')
        file.write(text)
        file.close()

        return file_path

    def _passed_to_processor(self):
        """Return the parameters passed to MockProcessor.process()."""
        return self._processor.processed

    def _assert_file_reader(self, passed_to_processor, file_count):
        """Assert the state of the file reader."""
        self.assertEquals(passed_to_processor, self._passed_to_processor())
        self.assertEquals(file_count, self._file_reader.file_count)

    def test_process_file__should_not_process(self):
        self._file_reader.process_file('should_not_process.txt')
        self._assert_file_reader([], 1)

    def test_process_file__does_not_exist(self):
        try:
            self._file_reader.process_file('does_not_exist.txt')
        except SystemExit, err:
            self.assertEquals(str(err), '1')
        else:
            self.fail('No Exception raised.')
        self._assert_file_reader([], 1)
        self.assertLog(["ERROR: File does not exist: 'does_not_exist.txt'\n"])

    def test_process_file__is_dir(self):
        temp_dir = os.path.join(self._temp_dir, 'test_dir')
        os.mkdir(temp_dir)

        self._file_reader.process_file(temp_dir)

        # Because the log message below contains exception text, it is
        # possible that the text varies across platforms.  For this reason,
        # we check only the portion of the log message that we control,
        # namely the text at the beginning.
        log_messages = self.logMessages()
        # We remove the message we are looking at to prevent the tearDown()
        # from raising an exception when it asserts that no log messages
        # remain.
        message = log_messages.pop()

        self.assertTrue(message.startswith('WARNING: Could not read file. '
                                           "Skipping: '%s'\n  " % temp_dir))

        self._assert_file_reader([], 1)

    def test_process_file__multiple_lines(self):
        file_path = self._create_file('foo.txt', 'line one\r\nline two\n')

        self._file_reader.process_file(file_path)
        processed = [(['line one\r', 'line two', ''], file_path, None)]
        self._assert_file_reader(processed, 1)

    def test_process_file__with_kwarg(self):
        file_path = self._create_file('foo.txt', 'file contents')

        self._file_reader.process_file(file_path=file_path, test_kwarg='foo')
        processed = [(['file contents'], file_path, 'foo')]
        self._assert_file_reader(processed, 1)

    def test_process_paths(self):
        # We test a list of paths that contains both a file and a directory.
        dir = os.path.join(self._temp_dir, 'foo_dir')
        os.mkdir(dir)

        file_path1 = self._create_file('file1.txt', 'foo')

        rel_path = os.path.join('foo_dir', 'file2.txt')
        file_path2 = self._create_file(rel_path, 'bar')

        self._file_reader.process_paths([dir, file_path1])
        processed = [(['bar'], file_path2, None),
                     (['foo'], file_path1, None)]
        self._assert_file_reader(processed, 2)
