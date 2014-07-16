/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

(function () {

module("ui");

var kExampleResultsByTest = {
    "scrollbars/custom-scrollbar-with-incomplete-style.html": {
        "Mock Builder": {
            "expected": "IMAGE",
            "actual": "CRASH"
        },
        "Mock Linux": {
            "expected": "TEXT",
            "actual": "CRASH"
        }
    },
    "userscripts/another-test.html": {
        "Mock Builder": {
            "expected": "PASS",
            "actual": "TEXT"
        }
    }
}

test("ui.onebar", 2, function() {
    if (window.location.hash) {
        window.location.hash = '';
    }

    onebar = new ui.onebar();
    onebar.attach();
    equal(onebar.innerHTML,
        '<ul class="ui-tabs-nav ui-helper-reset ui-helper-clearfix ui-widget-header ui-corner-all">' +
            '<li class="ui-state-default ui-corner-top ui-tabs-selected ui-state-active"><a href="#unexpected">Unexpected Failures</a></li>' +
            '<li class="ui-state-default ui-corner-top ui-state-disabled"><a href="#results">Results</a></li>' +
        '</ul>' +
        '<div id="link-handling"><input type="checkbox" id="new-window-for-links"><label for="new-window-for-links">Open links in new window</label></div>' +
        '<div id="unexpected" class="ui-tabs-panel ui-widget-content ui-corner-bottom"></div>' +
        '<div id="results" class="ui-tabs-panel ui-widget-content ui-corner-bottom ui-tabs-hide"></div>');

    onebar.select('unexpected');
    equal(window.location.hash, '#unexpected');

    onebar.remove();
});

// FIXME: These three results.* tests should be moved ot ui/results_unittests.js.
test("results.ResultsGrid", 8, function() {
    var grid = new ui.results.ResultsGrid()
    grid.addResults([
        'http://example.com/foo-bar-diff.txt',
        'http://example.com/foo-bar-expected.png',
        'http://example.com/foo-bar-actual.png',
        'http://example.com/foo-bar-diff.png',
    ]);

    var comparisons = grid.querySelectorAll("ct-results-comparison");

    equal(comparisons[0].type, "text");
    equal(comparisons[0].actualUrl, "");
    equal(comparisons[0].expectedUrl, "");
    equal(comparisons[0].diffUrl, "http://example.com/foo-bar-diff.txt");

    equal(comparisons[1].type, "image");
    equal(comparisons[1].actualUrl, "http://example.com/foo-bar-actual.png");
    equal(comparisons[1].expectedUrl, "http://example.com/foo-bar-expected.png");
    equal(comparisons[1].diffUrl, "http://example.com/foo-bar-diff.png");
});

test("results.ResultsGrid (crashlog)", 1, function() {
    var grid = new ui.results.ResultsGrid()
    grid.addResults(['http://example.com/layout-test-results/foo-bar-crash-log.txt']);
    equal(grid.innerHTML, '<ct-test-output></ct-test-output>');
});

test("results.ResultsGrid (empty)", 1, function() {
    var grid = new ui.results.ResultsGrid()
    grid.addResults([]);
    equal(grid.innerHTML, 'No results to display.');
});

var openTreeJson = {
    "username": "erg@chromium.org",
    "date": "2013-10-14 20:22:00.887390",
    "message": "Tree is open",
    "can_commit_freely": true,
    "general_state": "open"
};

asyncTest("TreeStatus", 2, function() {
    var simulator = new NetworkSimulator();

    simulator.json = function(url)
    {
        return Promise.resolve(openTreeJson);
    };

    var treestatus;
    simulator.runTest(function() {
        treeStatus = new ui.TreeStatus();
    }).then(function() {
        equal(treeStatus.innerHTML, '<div> blink status: <span>OPEN</span></div><div> chromium status: <span>OPEN</span></div>');
        start();
    });
});

function generateRoll(fromRevision, toRevision)
{
    return {
        "results": [
            {"messages":[], "base_url":"svn://svn.chromium.org/chrome/trunk/src", "subject":"Blink roll " + fromRevision + ":" + toRevision, "closed":false, "issue":1000}
        ]
    };
}

asyncTest("RevisionDetailsSmallRoll", 2, function() {
    var rollFromRevision = 540;
    var rollToRevision = 550;
    var simulator = new NetworkSimulator();
    simulator.json = function(url)
    {
        return Promise.resolve(generateRoll(rollFromRevision, rollToRevision));
    }

    simulator.get = function (url)
    {
        return Promise.resolve(rollFromRevision);
    }

    model.state.resultsByBuilder = {
        "Linux": {
            "blink_revision": "554",
        }
    };
    model.state.recentCommits = [
    {
        "revision": "555",
    }];

    var revisionDetails;
    simulator.runTest(function() {
        revisionDetails = ui.revisionDetails();
    }).then(function() {
        equal(revisionDetails.innerHTML,
            'Latest revision processed by every bot: ' +
                '<details>' +
                    '<summary>' +
                        '<a href="http://src.chromium.org/viewvc/blink?view=rev&amp;revision=554">554' +
                            '<span id="revisionPopUp">' +
                                '<table>' +
                                    '<tr>' +
                                        '<td><a href="http://build.chromium.org/p/chromium.webkit/waterfall?builder=Linux">Linux</a></td>' +
                                        '<td>554</td>' +
                                    '</tr>' +
                                '</table>' +
                            '</span>' +
                        '</a>' +
                    '</summary>' +
                    '<table>' +
                        '<tr>' +
                            '<td><a href="http://build.chromium.org/p/chromium.webkit/waterfall?builder=Linux">Linux</a></td>' +
                            '<td>554</td>' +
                        '</tr>' +
                    '</table>' +
                '</details>' +
                ', trunk is at <a href="http://src.chromium.org/viewvc/blink?view=rev&amp;revision=555">555</a>');
        start();
    });
});

asyncTest("RevisionDetailsMediumRoll", 2, function() {
    var rollFromRevision = 500;
    var rollToRevision = 550;
    var simulator = new NetworkSimulator();
    simulator.json = function(url)
    {
        return Promise.resolve(generateRoll(rollFromRevision, rollToRevision));
    }

    simulator.get = function (url)
    {
        return Promise.resolve(rollFromRevision);
    }

    model.state.resultsByBuilder = {
        "Linux": {
            "blink_revision": "554",
        }
    };
    model.state.recentCommits = [
    {
        "revision": "555",
    }];

    var revisionDetails;
    simulator.runTest(function() {
        revisionDetails = ui.revisionDetails();
    }).then(function() {
        equal(revisionDetails.innerHTML,
            'Latest revision processed by every bot: ' +
                '<details>' +
                    '<summary>' +
                        '<a href="http://src.chromium.org/viewvc/blink?view=rev&amp;revision=554">554' +
                            '<span id="revisionPopUp">' +
                                '<table>' +
                                    '<tr>' +
                                        '<td><a href="http://build.chromium.org/p/chromium.webkit/waterfall?builder=Linux">Linux</a></td>' +
                                        '<td>554</td>' +
                                    '</tr>' +
                                '</table>' +
                            '</span>' +
                        '</a>' +
                    '</summary>' +
                    '<table>' +
                        '<tr>' +
                            '<td><a href="http://build.chromium.org/p/chromium.webkit/waterfall?builder=Linux">Linux</a></td>' +
                            '<td>554</td>' +
                        '</tr>' +
                    '</table>' +
                '</details>' +
                ', trunk is at <a href="http://src.chromium.org/viewvc/blink?view=rev&amp;revision=555">555</a>');
        start();
    });
});

asyncTest("RevisionDetailsBigRoll", 2, function() {
    var rollFromRevision = 440;
    var rollToRevision = 550;
    var simulator = new NetworkSimulator();
    simulator.json = function(url)
    {
        return Promise.resolve(generateRoll(rollFromRevision, rollToRevision));
    }

    simulator.get = function (url)
    {
        return Promise.resolve(rollFromRevision);
    }

    model.state.resultsByBuilder = {
        "Linux": {
            "blink_revision": "554",
        }
    };
    model.state.recentCommits = [
    {
        "revision": "555",
    }];

    var revisionDetails;
    simulator.runTest(function() {
        revisionDetails = ui.revisionDetails();
    }).then(function() {
        equal(revisionDetails.innerHTML,
            'Latest revision processed by every bot: ' +
                '<details>' +
                    '<summary>' +
                        '<a href="http://src.chromium.org/viewvc/blink?view=rev&amp;revision=554">554' +
                            '<span id="revisionPopUp">' +
                                '<table>' +
                                    '<tr>' +
                                        '<td><a href="http://build.chromium.org/p/chromium.webkit/waterfall?builder=Linux">Linux</a></td>' +
                                        '<td>554</td>' +
                                    '</tr>' +
                                '</table>' +
                            '</span>' +
                        '</a>' +
                    '</summary>' +
                    '<table>' +
                        '<tr>' +
                            '<td><a href="http://build.chromium.org/p/chromium.webkit/waterfall?builder=Linux">Linux</a></td>' +
                            '<td>554</td>' +
                        '</tr>' +
                    '</table>' +
                '</details>' +
                ', trunk is at <a href="http://src.chromium.org/viewvc/blink?view=rev&amp;revision=555">555</a>');
        start();
    });
});

})();
