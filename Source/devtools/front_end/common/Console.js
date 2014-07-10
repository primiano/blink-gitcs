// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @constructor
 * @extends {WebInspector.Object}
 */
WebInspector.Console = function()
{
    /** @type {!Array.<!WebInspector.Console.Message>} */
    this._messages = [];
}

/**
 * @enum {string}
 */
WebInspector.Console.Events = {
    MessageAdded: "messageAdded"
}

/**
 * @enum {string}
 */
WebInspector.Console.MessageLevel = {
    Log: "log",
    Warning: "warning",
    Error: "error"
}

/**
 * @constructor
 * @param {string} text
 * @param {!WebInspector.Console.MessageLevel} level
 * @param {number} timestamp
 * @param {boolean} show
 */
WebInspector.Console.Message = function(text, level, timestamp, show)
{
    this.text = text;
    this.level = level;
    this.timestamp = (typeof timestamp === "number") ? timestamp : Date.now();
    this.show = show;
}

WebInspector.Console.prototype = {
    /**
     * @param {string} text
     * @param {!WebInspector.Console.MessageLevel} level
     * @param {boolean=} show
     */
    addMessage: function(text, level, show)
    {
        var message = new WebInspector.Console.Message(text, level || WebInspector.Console.MessageLevel.Log, Date.now(), show || false);
        this._messages.push(message);
        this.dispatchEventToListeners(WebInspector.Console.Events.MessageAdded, message);
    },

    /**
     * @param {string} text
     * @param {boolean=} show
     */
    addErrorMessage: function(text, show)
    {
        this.addMessage(text, WebInspector.Console.MessageLevel.Error, show);
    },

    /**
     * @return {!Array.<!WebInspector.Console.Message>}
     */
    messages: function()
    {
        return this._messages;
    },

    __proto__: WebInspector.Object.prototype
}

WebInspector.console = new WebInspector.Console();