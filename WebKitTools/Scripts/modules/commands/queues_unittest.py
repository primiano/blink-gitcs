# Copyright (C) 2009 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#    * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#    * Neither the name of Google Inc. nor the names of its
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

import os
import unittest

from modules.commands.commandtest import CommandsTest
from modules.commands.queues import *
from modules.commands.queuestest import QueuesTest
from modules.mock_bugzillatool import MockBugzillaTool
from modules.outputcapture import OutputCapture


class TestQueue(AbstractQueue):
    name = "test-queue"


class AbstractQueueTest(CommandsTest):
    def _assert_log_progress_output(self, patch_ids, progress_output):
        OutputCapture().assert_outputs(self, TestQueue().log_progress, [patch_ids], expected_stderr=progress_output)

    def test_log_progress(self):
        self._assert_log_progress_output([1,2,3], "3 patches in test-queue [1, 2, 3]\n")
        self._assert_log_progress_output(["1","2","3"], "3 patches in test-queue [1, 2, 3]\n")
        self._assert_log_progress_output([1], "1 patch in test-queue [1]\n")

    def _assert_run_bugzilla_tool(self, run_args):
        queue = TestQueue()
        tool = MockBugzillaTool()
        queue.bind_to_tool(tool)

        queue.run_bugzilla_tool(run_args)
        expected_run_args = ["echo", "--status-host=example.com"] + map(str, run_args)
        tool.executive.run_and_throw_if_fail.assert_called_with(expected_run_args)

    def test_run_bugzilla_tool(self):
        self._assert_run_bugzilla_tool([1])
        self._assert_run_bugzilla_tool(["one", 2])


class CommitQueueTest(QueuesTest):
    def test_style_queue(self):
        expected_stderr = {
            "begin_work_queue" : "CAUTION: commit-queue will discard all local changes in \"%s\"\nRunning WebKit commit-queue.\n" % os.getcwd(),
            "next_work_item" : "2 patches in commit-queue [197, 128]\n",
        }
        self.assert_queue_outputs(CommitQueue(), expected_stderr=expected_stderr)


class StyleQueueTest(QueuesTest):
    def test_style_queue(self):
        expected_stderr = {
            "begin_work_queue" : "CAUTION: style-queue will discard all local changes in \"%s\"\nRunning WebKit style-queue.\n" % os.getcwd(),
            "handle_unexpected_error" : "Mock error message\n",
        }
        self.assert_queue_outputs(StyleQueue(), expected_stderr=expected_stderr)
