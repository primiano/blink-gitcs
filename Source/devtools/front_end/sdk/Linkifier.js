/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @interface
 */
WebInspector.LinkifierFormatter = function()
{
}

WebInspector.LinkifierFormatter.prototype = {
    /**
     * @param {!Element} anchor
     * @param {!WebInspector.UILocation} uiLocation
     */
    formatLiveAnchor: function(anchor, uiLocation) { }
}

/**
 * @constructor
 * @implements {WebInspector.TargetManager.Observer}
 * @param {!WebInspector.LinkifierFormatter=} formatter
 */
WebInspector.Linkifier = function(formatter)
{
    this._formatter = formatter || new WebInspector.Linkifier.DefaultFormatter(WebInspector.Linkifier.MaxLengthForDisplayedURLs);
    this._liveLocationsByTarget = new Map();
    WebInspector.targetManager.observeTargets(this);
}

/**
 * @param {!WebInspector.Linkifier.LinkHandler} handler
 */
WebInspector.Linkifier.setLinkHandler = function(handler)
{
    WebInspector.Linkifier._linkHandler = handler;
}

/**
 * @param {string} url
 * @param {number=} lineNumber
 * @return {boolean}
 */
WebInspector.Linkifier.handleLink = function(url, lineNumber)
{
    if (!WebInspector.Linkifier._linkHandler)
        return false;
    return WebInspector.Linkifier._linkHandler.handleLink(url, lineNumber)
}

/**
 * @param {!Object} revealable
 * @param {string} text
 * @param {string=} fallbackHref
 * @param {number=} fallbackLineNumber
 * @param {string=} title
 * @param {string=} classes
 * @return {!Element}
 */
WebInspector.Linkifier.linkifyUsingRevealer = function(revealable, text, fallbackHref, fallbackLineNumber, title, classes)
{
    var a = document.createElement("a");
    a.className = (classes || "") + " webkit-html-resource-link";
    a.textContent = text.trimMiddle(WebInspector.Linkifier.MaxLengthForDisplayedURLs);
    a.title = title || text;
    if (fallbackHref) {
        a.href = fallbackHref;
        a.lineNumber = fallbackLineNumber;
    }
    /**
     * @param {!Event} event
     * @this {Object}
     */
    function clickHandler(event)
    {
        event.stopImmediatePropagation();
        event.preventDefault();
        if (fallbackHref && WebInspector.Linkifier.handleLink(fallbackHref, fallbackLineNumber))
            return;

        WebInspector.Revealer.reveal(this);
    }
    a.addEventListener("click", clickHandler.bind(revealable), false);
    return a;
}

WebInspector.Linkifier.prototype = {
    /**
     * @param {!WebInspector.Target} target
     */
    targetAdded: function(target)
    {
        this._liveLocationsByTarget.put(target, []);
    },

    /**
     * @param {!WebInspector.Target} target
     */
    targetRemoved: function(target)
    {
        var liveLocations = this._liveLocationsByTarget.remove(target);
        for (var i = 0; i < liveLocations.length; ++i) {
            delete liveLocations[i].anchor.__uiLocation;
            var anchor = liveLocations[i].anchor;
            if (anchor.__fallbackAnchor) {
                anchor.href = anchor.__fallbackAnchor.href;
                anchor.lineNumber = anchor.__fallbackAnchor.lineNumber;
                anchor.title = anchor.__fallbackAnchor.title;
                anchor.className = anchor.__fallbackAnchor.className;
                anchor.textContent = anchor.__fallbackAnchor.textContent;
            }
            liveLocations[i].location.dispose();
        }
    },

    /**
     * @param {?WebInspector.Target} target
     * @param {string} scriptId
     * @param {string} sourceURL
     * @param {number} lineNumber
     * @param {number=} columnNumber
     * @param {string=} classes
     * @return {?Element}
     */
    linkifyLocationByScriptId: function(target, scriptId, sourceURL, lineNumber, columnNumber, classes)
    {
        var rawLocation = target ? target.debuggerModel.createRawLocationByScriptId(scriptId, sourceURL, lineNumber, columnNumber || 0) : null;
        var fallbackAnchor = WebInspector.linkifyResourceAsNode(sourceURL, lineNumber, classes);
        if (!rawLocation)
            return fallbackAnchor;

        var anchor = this.linkifyRawLocation(rawLocation, classes);
        anchor.__fallbackAnchor = fallbackAnchor;
        return anchor;

    },

    /**
     * @param {!WebInspector.Target} target
     * @param {string} sourceURL
     * @param {number} lineNumber
     * @param {number=} columnNumber
     * @param {string=} classes
     * @return {?Element}
     */
    linkifyLocation: function(target, sourceURL, lineNumber, columnNumber, classes)
    {
        var rawLocation = target.debuggerModel.createRawLocationByURL(sourceURL, lineNumber, columnNumber || 0);
        if (!rawLocation)
            return WebInspector.linkifyResourceAsNode(sourceURL, lineNumber, classes);
        return this.linkifyRawLocation(rawLocation, classes);
    },

    /**
     * @param {!WebInspector.DebuggerModel.Location} rawLocation
     * @param {string=} classes
     * @return {?Element}
     */
    linkifyRawLocation: function(rawLocation, classes)
    {
        // FIXME: this check should not be here.
        var script = rawLocation.target().debuggerModel.scriptForId(rawLocation.scriptId);
        if (!script)
            return null;
        var anchor = this._createAnchor(classes);
        var liveLocation = rawLocation.createLiveLocation(this._updateAnchor.bind(this, anchor));
        this._liveLocationsByTarget.get(rawLocation.target()).push({anchor: anchor, location: liveLocation});
        return anchor;
    },

    /**
     * @param {!WebInspector.CSSLocation} rawLocation
     * @param {string=} classes
     * @return {?Element}
     */
    linkifyCSSLocation: function(rawLocation, classes)
    {
        var anchor = this._createAnchor(classes);
        var liveLocation = rawLocation.createLiveLocation(this._updateAnchor.bind(this, anchor));
        if (!liveLocation)
            return null;
        this._liveLocationsByTarget.get(rawLocation.target()).push({anchor: anchor, location: liveLocation});
        return anchor;
    },

    /**
     * @param {!WebInspector.CSSMedia} media
     * @return {?Element}
     */
    linkifyMedia: function(media)
    {
        var location = media.rawLocation();
        if (location)
            return this.linkifyCSSLocation(location);

        // The "linkedStylesheet" case.
        return WebInspector.linkifyResourceAsNode(media.sourceURL, undefined, "subtitle", media.sourceURL);
    },

    /**
     * @param {string=} classes
     * @return {!Element}
     */
    _createAnchor: function(classes)
    {
        var anchor = document.createElement("a");
        anchor.className = (classes || "") + " webkit-html-resource-link";

        /**
         * @param {!Event} event
         */
        function clickHandler(event)
        {
            if (!anchor.__uiLocation)
                return;
            event.stopImmediatePropagation();
            event.preventDefault();
            if (WebInspector.Linkifier.handleLink(anchor.__uiLocation.uiSourceCode.url, anchor.__uiLocation.lineNumber))
                return;
            WebInspector.Revealer.reveal(anchor.__uiLocation);
        }
        anchor.addEventListener("click", clickHandler, false);
        return anchor;
    },

    reset: function()
    {
        var keys = this._liveLocationsByTarget.keys();
        for (var i = 0; i < keys.length; ++i) {
            var target = keys[i];
            this.targetRemoved(target);
            this.targetAdded(target);
        }
    },

    /**
     * @param {!Element} anchor
     * @param {!WebInspector.UILocation} uiLocation
     */
    _updateAnchor: function(anchor, uiLocation)
    {
        anchor.__uiLocation = uiLocation;
        this._formatter.formatLiveAnchor(anchor, uiLocation);
    }
}

/**
 * @constructor
 * @implements {WebInspector.LinkifierFormatter}
 * @param {number=} maxLength
 */
WebInspector.Linkifier.DefaultFormatter = function(maxLength)
{
    this._maxLength = maxLength;
}

WebInspector.Linkifier.DefaultFormatter.prototype = {
    /**
     * @param {!Element} anchor
     * @param {!WebInspector.UILocation} uiLocation
     */
    formatLiveAnchor: function(anchor, uiLocation)
    {
        var text = uiLocation.linkText();
        if (this._maxLength)
            text = text.trimMiddle(this._maxLength);
        anchor.textContent = text;

        var titleText = uiLocation.uiSourceCode.originURL();
        if (typeof uiLocation.lineNumber === "number")
            titleText += ":" + (uiLocation.lineNumber + 1);
        anchor.title = titleText;
    }
}

/**
 * @constructor
 * @extends {WebInspector.Linkifier.DefaultFormatter}
 */
WebInspector.Linkifier.DefaultCSSFormatter = function()
{
    WebInspector.Linkifier.DefaultFormatter.call(this, WebInspector.Linkifier.DefaultCSSFormatter.MaxLengthForDisplayedURLs);
}

WebInspector.Linkifier.DefaultCSSFormatter.MaxLengthForDisplayedURLs = 30;

WebInspector.Linkifier.DefaultCSSFormatter.prototype = {
    /**
     * @param {!Element} anchor
     * @param {!WebInspector.UILocation} uiLocation
     */
    formatLiveAnchor: function(anchor, uiLocation)
    {
        WebInspector.Linkifier.DefaultFormatter.prototype.formatLiveAnchor.call(this, anchor, uiLocation);
        anchor.classList.add("webkit-html-resource-link");
        anchor.setAttribute("data-uncopyable", anchor.textContent);
        anchor.textContent = "";
    },
    __proto__: WebInspector.Linkifier.DefaultFormatter.prototype
}

/**
 * The maximum number of characters to display in a URL.
 * @const
 * @type {number}
 */
WebInspector.Linkifier.MaxLengthForDisplayedURLs = 150;

/**
 * @interface
 */
WebInspector.Linkifier.LinkHandler = function()
{
}

WebInspector.Linkifier.LinkHandler.prototype = {
    /**
     * @param {string} url
     * @param {number=} lineNumber
     * @return {boolean}
     */
    handleLink: function(url, lineNumber) {}
}

/**
 * @param {!WebInspector.Target} target
 * @param {string} scriptId
 * @param {number} lineNumber
 * @param {number=} columnNumber
 * @return {string}
 */
WebInspector.Linkifier.liveLocationText = function(target, scriptId, lineNumber, columnNumber)
{
    var script = target.debuggerModel.scriptForId(scriptId);
    if (!script)
        return "";
    var uiLocation = script.rawLocationToUILocation(lineNumber, columnNumber);
    return uiLocation.linkText();
}
