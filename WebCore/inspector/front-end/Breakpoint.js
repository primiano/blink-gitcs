/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

WebInspector.Breakpoint = function(url, line, sourceID, condition)
{
    this.url = url;
    this.line = line;
    this.sourceID = sourceID;
    this._enabled = true;
    this._sourceText = "";
    this._condition = condition || "";
}

WebInspector.Breakpoint.prototype = {
    get enabled()
    {
        return this._enabled;
    },

    set enabled(x)
    {
        if (this._enabled === x)
            return;

        this._enabled = x;

        if (this._enabled)
            this.dispatchEventToListeners("enabled");
        else
            this.dispatchEventToListeners("disabled");
    },

    get sourceText()
    {
        return this._sourceText;
    },

    set sourceText(text)
    {
        this._sourceText = text;
        this.dispatchEventToListeners("text-changed");
    },

    get label()
    {
        var displayName = (this.url ? WebInspector.displayNameForURL(this.url) : WebInspector.UIString("(program)"));
        return displayName + ":" + this.line;
    },

    get id()
    {
        return this.sourceID + ":" + this.line;
    },

    get condition()
    {
        return this._condition;
    },

    set condition(c)
    {
        c = c || "";
        if (this._condition === c)
            return;

        this._condition = c;
        this.dispatchEventToListeners("condition-changed");

        if (this.enabled)
            InspectorBackend.updateBreakpoint(this.sourceID, this.line, c);
    }
}

WebInspector.Breakpoint.prototype.__proto__ = WebInspector.Object.prototype;
