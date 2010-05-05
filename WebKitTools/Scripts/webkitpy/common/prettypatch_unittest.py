# Copyright (C) 2010 Google Inc. All rights reserved.
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

import os.path
import unittest

from webkitpy.common.system.executive import Executive
from webkitpy.common.prettypatch import PrettyPatch


class PrettyPatchTest(unittest.TestCase):

    _diff_with_multiple_encodings = """
Index: utf8_test
===================================================================
--- utf8_test\t(revision 0)
+++ utf8_test\t(revision 0)
@@ -0,0 +1 @@
+utf-8 test: \xc2\xa0
Index: latin1_test
===================================================================
--- latin1_test\t(revision 0)
+++ latin1_test\t(revision 0)
@@ -0,0 +1 @@
+latin1 test: \xa0
"""

    def _webkit_root(self):
        webkitpy_common = os.path.dirname(__file__)
        webkitpy = os.path.dirname(webkitpy_common)
        scripts = os.path.dirname(webkitpy)
        webkit_tools = os.path.dirname(scripts)
        webkit_root = os.path.dirname(webkit_tools)
        return webkit_root

    def test_pretty_diff_encodings(self):
        pretty_patch = PrettyPatch(Executive(), self._webkit_root())
        pretty = pretty_patch.pretty_diff(self._diff_with_multiple_encodings)
        self.assertTrue(pretty)  # We got some output
        self.assertTrue(isinstance(pretty, str))  # It's a byte array, not unicode

    def test_pretty_print_empty_string(self):
        # Make sure that an empty diff does not hang the process.
        pretty_patch = PrettyPatch(Executive(), self._webkit_root())
        self.assertEqual(pretty_patch.pretty_diff(""), "")
