/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

var InjectedFakeWorker = function()
{

Worker = function(url)
{
    var impl = new FakeWorker(this, url);
    if (impl === null)
        return null;

    this.isFake = true;
    this.postMessage = bind(impl.postMessage, impl);
    this.terminate = bind(impl.terminate, impl);
    this.onmessage = noop;
}

function FakeWorker(worker, url)
{
    var scriptURL = new URL(document.baseURI).completeWith(url);

    if (!scriptURL.sameOrigin(location.href))
        throw new DOMCoreException("SECURITY_ERR",18);

    this._worker = worker;
    this._buildWorker(scriptURL);
}

FakeWorker.prototype = {
    postMessage: function(msg)
    {
        if (this._frame != null)
            this._dispatchMessage(this._frame, bind(this._onmessageWrapper, this), msg);
    },

    terminate: function()
    {
        if (this._frame != null) {
            this._frame.onmessage = this._worker.onmessage = noop;
            this._frame.frameElement.parentNode.removeChild(this._frame.frameElement);
        }
        this._frame = null;
        this._worker = null; // Break reference loop.
    },

    _onmessageWrapper: function(msg)
    {
        // Shortcut -- if no exception handlers installed, avoid try/catch so as not to obscure line number.
        if (!this._frame.onerror && !this._worker.onerror) {
            this._frame.onmessage(msg);
            return;
        }

        try {
            this._frame.onmessage(msg);
        } catch (e) {
            this._handleException(e, this._frame.onerror, this._worker.onerror);
        }
    },

    _dispatchMessage: function(targetWindow, handler, msg)
    {
        var event = this._document.createEvent("MessageEvent");
        event.initMessageEvent("MessageEvent", false, false, msg);
        targetWindow.setTimeout(handler, 0, event);
    },

    _handleException: function(e)
    {
        // NB: it should be an ErrorEvent, but creating it from script is not
        // currently supported, to emulate it on top of plain vanilla Event.
        var errorEvent = this._document.createEvent("Event");
        errorEvent.initEvent("Event", false, false);
        errorEvent.message = "Uncaught exception";

        for (var i = 1; i < arguments.length; ++i) {
            if (arguments[i] && arguments[i](errorEvent))
                return;
        }

        throw e;
    },

    _buildWorker: function(url)
    {
        var code = this._loadScript(url.url);
        var iframeElement = document.createElement("iframe");
        iframeElement.style.display = "none";
        document.body.appendChild(iframeElement);

        var frame = window.frames[window.frames.length - 1];

        this._document = document;
        this._frame = frame;

        this._setupWorkerContext(frame, url);

        var frameContents = '(function(location, window) { ' + code + '})(__devtools.location, undefined);\n' + '//@ sourceURL=' + url.url;

        frame.eval(frameContents);
    },

    _setupWorkerContext: function(workerFrame, url)
    {
        workerFrame.__devtools = {
            handleException: bind(this._handleException, this),
            location: url.mockLocation()
        };
        var worker = this._worker;

        function handler(event) // Late binding to onmessage desired, so no bind() here.
        {
            worker.onmessage(event);
        }

        workerFrame.onmessage = noop;
        workerFrame.postMessage = bind(this._dispatchMessage, this, window, handler);
        workerFrame.importScripts = bind(this._importScripts, this, workerFrame);
        workerFrame.close = bind(this.terminate, this);
    },

    _importScripts: function(evalTarget)
    {
        for (var i = 1; i < arguments.length; ++i)
            evalTarget.eval(this._loadScript(arguments[i]));
    },

    _loadScript: function(url)
    {
        var xhr = new XMLHttpRequest();
        xhr.open("GET", url, false);
        xhr.send(null);

        var text = xhr.responseText;
        if (xhr.status != 0 && xhr.status/100 !== 2) { // We're getting status === 0 when using file://.
            console.error("Failed to load worker: " + url + "[" + xhr.status + "]");
            text = ""; // We've got error message, not worker code.
        }
        return text;
    }
};

function URL(url)
{
    this.url = url;
    this.split();
}

URL.prototype = {
    urlRegEx: (/^(http[s]?|file):\/\/([^\/:]*)(:[\d]+)?(?:(\/[^#?]*)(\?[^#]*)?(?:#(.*))?)?$/i),

    split: function()
    {
        function emptyIfNull(str)
        {
            return str == null ? "" : str;
        }
        var parts = this.urlRegEx.exec(this.url);

        this.schema = parts[1];
        this.host = parts[2];
        this.port = emptyIfNull(parts[3]);
        this.path = emptyIfNull(parts[4]);
        this.query = emptyIfNull(parts[5]);
        this.fragment = emptyIfNull(parts[6]);
    },

    mockLocation: function()
    {
        var host = this.host.replace(/^[^@]*@/, "");

        return {
            href:     this.url,
            protocol: this.schema + ":",
            host:     host,
            hostname: host,
            port:     this.port,
            pathname: this.path,
            search:   this.query,
            hash:     this.fragment
        };
    },

    completeWith: function(url)
    {
        if (url === "" || /^[^/]*:/.exec(url)) // If given absolute url, return as is now.
            return new URL(url);

        var relParts = /^([^#?]*)(.*)$/.exec(url); // => [ url, path, query-andor-fragment ]

        var path = (relParts[1].slice(0, 1) === "/" ? "" : this.path.replace(/[^/]*$/, "")) + relParts[1];
        path = path.replace(/(\/\.)+(\/|$)/g, "/").replace(/[^/]*\/\.\.(\/|$)/g, "");

        return new URL(this.schema + "://" + this.host + this.port + path + relParts[2]);
    },

    sameOrigin: function(url)
    {
        function normalizePort(schema, port)
        {
            var portNo = port.slice(1);
            return (schema === "https" && portNo == 443 || schema === "http" && portNo == 80) ? "" : port;
        }

        var other = new URL(url);

        return this.schema === other.schema &&
            this.host === other.host &&
            normalizePort(this.schema, this.port) === normalizePort(other.schema, other.port);
    }
};

function DOMCoreException(name, code)
{
    function formatError()
    {
        return "Error: " + this.message;
    }

    this.name = name;
    this.message = name + ": DOM Exception " + code;
    this.code = code;
    this.toString = bind(formatError, this);
}

function bind(func, thisObject)
{
    var args = Array.prototype.slice.call(arguments, 2);
    return function() { return func.apply(thisObject, args.concat(Array.prototype.slice.call(arguments, 0))); };
}

function noop()
{
}

}
