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

import unittest
from modules.credentials import Credentials
from modules.executive import Executive
from modules.outputcapture import OutputCapture
from modules.mock import Mock

class CredentialsTest(unittest.TestCase):
    example_security_output = """keychain: "/Users/test/Library/Keychains/login.keychain"
class: "inet"
attributes:
    0x00000007 <blob>="bugs.webkit.org (test@webkit.org)"
    0x00000008 <blob>=<NULL>
    "acct"<blob>="test@webkit.org"
    "atyp"<blob>="form"
    "cdat"<timedate>=0x32303039303832353233353231365A00  "20090825235216Z\000"
    "crtr"<uint32>=<NULL>
    "cusi"<sint32>=<NULL>
    "desc"<blob>="Web form password"
    "icmt"<blob>="default"
    "invi"<sint32>=<NULL>
    "mdat"<timedate>=0x32303039303930393137323635315A00  "20090909172651Z\000"
    "nega"<sint32>=<NULL>
    "path"<blob>=<NULL>
    "port"<uint32>=0x00000000 
    "prot"<blob>=<NULL>
    "ptcl"<uint32>="htps"
    "scrp"<sint32>=<NULL>
    "sdmn"<blob>=<NULL>
    "srvr"<blob>="bugs.webkit.org"
    "type"<uint32>=<NULL>
password: "SECRETSAUCE"
"""

    def test_keychain_lookup_on_non_mac(self):
        class FakeCredentials(Credentials):
            def _is_mac_os_x(self):
                return False
        credentials = FakeCredentials("bugs.webkit.org")
        self.assertEqual(credentials._is_mac_os_x(), False)
        self.assertEqual(credentials._credentials_from_keychain("foo"), ["foo", None])

    def test_security_output_parse(self):
        credentials = Credentials("bugs.webkit.org")
        self.assertEqual(credentials._parse_security_tool_output(self.example_security_output), ["test@webkit.org", "SECRETSAUCE"])

    def _assert_security_call(self, username=None):
        executive_mock = Mock()
        credentials = Credentials("example.com", executive=executive_mock)

        output = OutputCapture()
        output.capture_output()
        credentials._run_security_tool(username)
        (stdout_output, stderr_output) = output.restore_output()
        self.assertEqual(stdout_output, "")
        self.assertEqual(stderr_output, "Reading Keychain for example.com account and password.  Click \"Allow\" to continue...\n")

        security_args = ["/usr/bin/security", "find-internet-password", "-g", "-s", "example.com"]
        if username:
            security_args += ["-a", username]
        executive_mock.run_command.assert_called_with(security_args)

    def test_security_calls(self):
        self._assert_security_call()
        self._assert_security_call(username="foo")

    def test_git_config_calls(self):
        executive_mock = Mock()
        credentials = Credentials("example.com", executive=executive_mock)
        credentials._read_git_config("foo")
        executive_mock.run_command.assert_called_with(["git", "config", "--get", "foo"], error_handler=Executive.ignore_error)

        credentials = Credentials("example.com", git_prefix="test_prefix", executive=executive_mock)
        credentials._read_git_config("foo")
        executive_mock.run_command.assert_called_with(["git", "config", "--get", "test_prefix.foo"], error_handler=Executive.ignore_error)


if __name__ == '__main__':
    unittest.main()
