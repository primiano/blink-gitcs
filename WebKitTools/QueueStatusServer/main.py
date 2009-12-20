# Copyright (C) 2009 Google Inc. All rights reserved.
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

# Request a modern Django
from google.appengine.dist import use_library
use_library('django', '1.1')

from google.appengine.ext import webapp
from google.appengine.ext.webapp.util import run_wsgi_app

from handlers.dashboard import Dashboard
from handlers.patch import Patch
from handlers.patchstatus import PatchStatus
from handlers.recentstatus import RecentStatus
from handlers.showresults import ShowResults
from handlers.statusbubble import StatusBubble
from handlers.updatestatus import UpdateStatus

webapp.template.register_template_library('filters.webkit_extras')

routes = [
    ('/', RecentStatus),
    ('/queue-status/(.*)', RecentStatus),
    ('/update-status', UpdateStatus),
    ('/dashboard', Dashboard),
    (r'/patch-status/(.*)/(.*)', PatchStatus),
    (r'/patch/(.*)', Patch),
    (r'/status-bubble/(.*)', StatusBubble),
    (r'/results/(.*)', ShowResults)
]

application = webapp.WSGIApplication(routes, debug=True)

def main():
    run_wsgi_app(application)

if __name__ == "__main__":
    main()
