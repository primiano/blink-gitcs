# Copyright (c) 2009, Google Inc. All rights reserved.
# Copyright (c) 2009 Apple Inc. All rights reserved.
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
#
# WebKit's Python module for interacting with Bugzilla

import getpass
import platform
import re
import subprocess
import urllib2

from datetime import datetime # used in timestamp()

# Import WebKit-specific modules.
from modules.logging import error, log
from modules.committers import CommitterList

# WebKit includes a built copy of BeautifulSoup in Scripts/modules
# so this import should always succeed.
from .BeautifulSoup import BeautifulSoup

try:
    from mechanize import Browser
except ImportError, e:
    print """
mechanize is required.

To install:
sudo easy_install mechanize

Or from the web:
http://wwwsearch.sourceforge.net/mechanize/
"""
    exit(1)

def credentials_from_git():
    return [read_config("username"), read_config("password")]

def credentials_from_keychain(username=None):
    if not is_mac_os_x():
        return [username, None]

    command = "/usr/bin/security %s -g -s %s" % ("find-internet-password", Bugzilla.bug_server_host)
    if username:
        command += " -a %s" % username

    log('Reading Keychain for %s account and password.  Click "Allow" to continue...' % Bugzilla.bug_server_host)
    keychain_process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True)
    value = keychain_process.communicate()[0]
    exit_code = keychain_process.wait()

    if exit_code:
        return [username, None]

    match = re.search('^\s*"acct"<blob>="(?P<username>.+)"', value, re.MULTILINE)
    if match:
        username = match.group('username')

    password = None
    match = re.search('^password: "(?P<password>.+)"', value, re.MULTILINE)
    if match:
        password = match.group('password')

    return [username, password]

def is_mac_os_x():
    return platform.mac_ver()[0]

# FIXME: This should not depend on git for config storage
def read_config(key):
    # Need a way to read from svn too
    config_process = subprocess.Popen("git config --get bugzilla." + key, stdout=subprocess.PIPE, shell=True)
    value = config_process.communicate()[0]
    return_code = config_process.wait()

    if return_code:
        return None
    return value.rstrip('\n')

def read_credentials():
    (username, password) = credentials_from_git()

    if not username or not password:
        (username, password) = credentials_from_keychain(username)

    if not username:
        username = raw_input("Bugzilla login: ")
    if not password:
        password = getpass.getpass("Bugzilla password for %s: " % username)

    return [username, password]

def timestamp():
    return datetime.now().strftime("%Y%m%d%H%M%S")

class Bugzilla:
    def __init__(self, dryrun=False, committers=CommitterList()):
        self.dryrun = dryrun
        self.authenticated = False

        self.browser = Browser()
        # Ignore bugs.webkit.org/robots.txt until we fix it to allow this script
        self.browser.set_handle_robots(False)
        self.committers = committers

    # Defaults (until we support better option parsing):
    bug_server_host = "bugs.webkit.org"
    bug_server_regex = "https?://%s/" % re.sub('\.', '\\.', bug_server_host)
    bug_server_url = "https://%s/" % bug_server_host

    def bug_url_for_bug_id(self, bug_id, xml=False):
        content_type = "&ctype=xml" if xml else ""
        return "%sshow_bug.cgi?id=%s%s" % (self.bug_server_url, bug_id, content_type)
    
    def attachment_url_for_id(self, attachment_id, action="view"):
        action_param = ""
        if action and action != "view":
            action_param = "&action=" + action
        return "%sattachment.cgi?id=%s%s" % (self.bug_server_url, attachment_id, action_param)

    def _parse_attachment_element(self, element, bug_id):
        attachment = {}
        attachment['bug_id'] = bug_id
        attachment['is_obsolete'] = (element.has_key('isobsolete') and element['isobsolete'] == "1")
        attachment['is_patch'] = (element.has_key('ispatch') and element['ispatch'] == "1")
        attachment['id'] = str(element.find('attachid').string)
        attachment['url'] = self.attachment_url_for_id(attachment['id'])
        attachment['name'] = unicode(element.find('desc').string)
        attachment['type'] = str(element.find('type').string)

        review_flag = element.find('flag', attrs={"name" : "review"})
        if review_flag and review_flag['status'] == '+':
            reviewer_email = review_flag['setter']
            reviewer = self.committers.reviewer_by_bugzilla_email(reviewer_email)
            attachment['reviewer'] = reviewer.full_name

        commit_queue_flag = element.find('flag', attrs={"name" : "commit-queue"})
        if commit_queue_flag and commit_queue_flag['status'] == '+':
            committer_email = commit_queue_flag['setter']
            committer = self.committers.committer_by_bugzilla_email(committer_email)
            attachment['commit-queue'] = committer.full_name

        return attachment

    def fetch_attachments_from_bug(self, bug_id):
        bug_url = self.bug_url_for_bug_id(bug_id, xml=True)
        log("Fetching: " + bug_url)

        page = urllib2.urlopen(bug_url)
        soup = BeautifulSoup(page)

        attachments = []
        for element in soup.findAll('attachment'):
            attachment = self._parse_attachment_element(element, bug_id)
            attachments.append(attachment)
        return attachments

    def fetch_patches_from_bug(self, bug_id):
        patches = []
        for attachment in self.fetch_attachments_from_bug(bug_id):
            if attachment['is_patch'] and not attachment['is_obsolete']:
                patches.append(attachment)
        return patches

    def fetch_reviewed_patches_from_bug(self, bug_id):
        reviewed_patches = []
        for attachment in self.fetch_attachments_from_bug(bug_id):
            if 'reviewer' in attachment and not attachment['is_obsolete']:
                reviewed_patches.append(attachment)
        return reviewed_patches

    def fetch_commit_queue_patches_from_bug(self, bug_id):
        commit_queue_patches = []
        for attachment in self.fetch_reviewed_patches_from_bug(bug_id):
            if 'commit-queue' in attachment and not attachment['is_obsolete']:
                commit_queue_patches.append(attachment)
        return commit_queue_patches

    def fetch_bug_ids_from_commit_queue(self):
        commit_queue_url = self.bug_server_url + "buglist.cgi?query_format=advanced&bug_status=UNCONFIRMED&bug_status=NEW&bug_status=ASSIGNED&bug_status=REOPENED&field0-0-0=flagtypes.name&type0-0-0=equals&value0-0-0=commit-queue%2B"

        page = urllib2.urlopen(commit_queue_url)
        soup = BeautifulSoup(page)

        bug_ids = []
        # Grab the cells in the first column (which happens to be the bug ids)
        for bug_link_cell in soup('td', "first-child"): # tds with the class "first-child"
            bug_link = bug_link_cell.find("a")
            bug_ids.append(bug_link.string) # the contents happen to be the bug id

        return bug_ids

    def fetch_patches_from_commit_queue(self):
        patches_to_land = []
        for bug_id in self.fetch_bug_ids_from_commit_queue():
            patches = self.fetch_commit_queue_patches_from_bug(bug_id)
            patches_to_land += patches
        return patches_to_land

    def authenticate(self):
        if self.authenticated:
            return

        if self.dryrun:
            log("Skipping log in for dry run...")
            self.authenticated = True
            return

        (username, password) = read_credentials()

        log("Logging in as %s..." % username)
        self.browser.open(self.bug_server_url + "index.cgi?GoAheadAndLogIn=1")
        self.browser.select_form(name="login")
        self.browser['Bugzilla_login'] = username
        self.browser['Bugzilla_password'] = password
        response = self.browser.submit()

        match = re.search("<title>(.+?)</title>", response.read())
        # If the resulting page has a title, and it contains the word "invalid" assume it's the login failure page.
        if match and re.search("Invalid", match.group(1), re.IGNORECASE):
            # FIXME: We could add the ability to try again on failure.
            raise ScriptError("Bugzilla login failed: %s" % match.group(1))

        self.authenticated = True

    def add_patch_to_bug(self, bug_id, patch_file_object, description, comment_text=None, mark_for_review=False):
        self.authenticate()
        
        log('Adding patch "%s" to bug %s' % (description, bug_id))
        if self.dryrun:
            log(comment_text)
            return
        
        self.browser.open(self.bug_server_url + "attachment.cgi?action=enter&bugid=" + bug_id)
        self.browser.select_form(name="entryform")
        self.browser['description'] = description
        self.browser['ispatch'] = ("1",)
        if comment_text:
            log(comment_text)
            self.browser['comment'] = comment_text
        self.browser['flag_type-1'] = ('?',) if mark_for_review else ('X',)
        self.browser.add_file(patch_file_object, "text/plain", "bug-%s-%s.patch" % (bug_id, timestamp()))
        self.browser.submit()

    def prompt_for_component(self, components):
        log("Please pick a component:")
        i = 0
        for name in components:
            i += 1
            log("%2d. %s" % (i, name))
        result = int(raw_input("Enter a number: ")) - 1
        return components[result]

    def _check_create_bug_response(self, response_html):
        match = re.search("<title>Bug (?P<bug_id>\d+) Submitted</title>", response_html)
        if match:
            return match.group('bug_id')

        match = re.search('<div id="bugzilla-body">(?P<error_message>.+)<div id="footer">', response_html, re.DOTALL)
        error_message = "FAIL"
        if match:
            text_lines = BeautifulSoup(match.group('error_message')).findAll(text=True)
            error_message = "\n" + '\n'.join(["  " + line.strip() for line in text_lines if line.strip()])
        raise ScriptError("Bug not created: %s" % error_message)

    def create_bug_with_patch(self, bug_title, bug_description, component, patch_file_object, patch_description, cc, mark_for_review=False):
        self.authenticate()

        log('Creating bug with patch description "%s"' % patch_description)
        if self.dryrun:
            log(bug_description)
            return

        self.browser.open(self.bug_server_url + "enter_bug.cgi?product=WebKit")
        self.browser.select_form(name="Create")
        component_items = self.browser.find_control('component').items
        component_names = map(lambda item: item.name, component_items)
        if not component or component not in component_names:
            component = self.prompt_for_component(component_names)
        self.browser['component'] = [component]
        self.browser['cc'] = cc
        self.browser['short_desc'] = bug_title
        if bug_description:
            log(bug_description)
            self.browser['comment'] = bug_description
        self.browser['description'] = patch_description
        self.browser['ispatch'] = ("1",)
        self.browser['flag_type-1'] = ('?',) if mark_for_review else ('X',)
        self.browser.add_file(patch_file_object, "text/plain", "%s.patch" % timestamp(), 'data')
        response = self.browser.submit()

        bug_id = self._check_create_bug_response(response.read())
        log("Bug %s created." % bug_id)
        log(self.bug_server_url + "show_bug.cgi?id=" + bug_id)
        return bug_id

    def clear_attachment_review_flag(self, attachment_id, additional_comment_text=None):
        self.authenticate()

        comment_text = "Clearing review flag on attachment: %s" % attachment_id
        if additional_comment_text:
            comment_text += "\n\n" + additional_comment_text
        log(comment_text)

        if self.dryrun:
            return

        self.browser.open(self.attachment_url_for_id(attachment_id, 'edit'))
        self.browser.select_form(nr=1)
        self.browser.set_value(comment_text, name='comment', nr=0)
        self.browser.find_control(type='select', nr=0).value = ("X",)
        self.browser.submit()

    def obsolete_attachment(self, attachment_id, comment_text = None):
        self.authenticate()

        log("Obsoleting attachment: %s" % attachment_id)
        if self.dryrun:
            log(comment_text)
            return

        self.browser.open(self.attachment_url_for_id(attachment_id, 'edit'))
        self.browser.select_form(nr=1)
        self.browser.find_control('isobsolete').items[0].selected = True
        # Also clear any review flag (to remove it from review/commit queues)
        self.browser.find_control(type='select', nr=0).value = ("X",)
        if comment_text:
            log(comment_text)
            # Bugzilla has two textareas named 'comment', one is somehow hidden.  We want the first.
            self.browser.set_value(comment_text, name='comment', nr=0)
        self.browser.submit()
    
    def post_comment_to_bug(self, bug_id, comment_text):
        self.authenticate()

        log("Adding comment to bug %s" % bug_id)
        if self.dryrun:
            log(comment_text)
            return

        self.browser.open(self.bug_url_for_bug_id(bug_id))
        self.browser.select_form(name="changeform")
        self.browser['comment'] = comment_text
        self.browser.submit()

    def close_bug_as_fixed(self, bug_id, comment_text=None):
        self.authenticate()

        log("Closing bug %s as fixed" % bug_id)
        if self.dryrun:
            log(comment_text)
            return

        self.browser.open(self.bug_url_for_bug_id(bug_id))
        self.browser.select_form(name="changeform")
        if comment_text:
            log(comment_text)
            self.browser['comment'] = comment_text
        self.browser['bug_status'] = ['RESOLVED']
        self.browser['resolution'] = ['FIXED']
        self.browser.submit()
