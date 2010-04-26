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

import os

from webkitpy.common.system.executive import ScriptError
from webkitpy.tool.steps.abstractstep import AbstractStep
from webkitpy.tool.steps.options import Options
from webkitpy.common.system.deprecated_logging import error

class CheckStyle(AbstractStep):
    @classmethod
    def options(cls):
        return [
            Options.non_interactive,
            Options.check_style,
            Options.git_commit,
            Options.no_squash,
            Options.squash,
        ]

    def run(self, state):
        if not self._options.check_style:
            return
        os.chdir(self._tool.scm().checkout_root)
        try:
            args = []
            if self._options.git_commit:
                args.append("--git-commit")
                args.append(self._options.git_commit)
            if self._tool.scm().should_squash(self._options.squash):
                args.append("--squash")
            else:
                args.append("--no-squash")

            self._run_script("check-webkit-style", args)
        except ScriptError, e:
            if self._options.non_interactive:
                # We need to re-raise the exception here to have the
                # style-queue do the right thing.
                raise e
            if not self._tool.user.confirm("Are you sure you want to continue?"):
                exit(1)
