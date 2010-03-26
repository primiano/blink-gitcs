# Copyright (c) 2010 Google Inc. All rights reserved.
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

import threading

from webkitpy.common.net.irc.ircbot import IRCBot
from webkitpy.common.thread.threadedmessagequeue import ThreadedMessageQueue
from webkitpy.common.system.deprecated_logging import log


class _IRCThread(threading.Thread):
    def __init__(self, message_queue, irc_delegate):
        threading.Thread.__init__(self)
        self.setDaemon(True)
        self._message_queue = message_queue
        self._irc_delegate = irc_delegate

    def run(self):
        bot = IRCBot(self._message_queue, self.irc_delegate)
        bot.start()


class IRCProxy(object):
    def __init__(self, irc_delegate):
        log("Connecting to IRC")
        self._message_queue = ThreadedMessageQueue()
        self._child_thread = _IRCThread(self._message_queue, irc_delegate)
        self._child_thread.start()

    def post(self, message):
        self._message_queue.post(message)

    def disconnect(self):
        log("Disconnecting from IRC...")
        self._message_queue.stop()
        self._child_thread.join()
