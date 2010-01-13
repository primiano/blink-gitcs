/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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

WebInspector.TextEditorHighlighter = function(textModel, damageCallback)
{
    this._textModel = textModel;
    this._tokenizer = new WebInspector.JavaScriptTokenizer();

    this._styles = [];
    this._styles["comment"] = "rgb(0, 116, 0)";
    this._styles["string"] = "rgb(196, 26, 22)";
    this._styles["regex"] = "rgb(196, 26, 22)";
    this._styles["keyword"] = "rgb(170, 13, 145)";
    this._styles["number"] = "rgb(28, 0, 207)";

    this._damageCallback = damageCallback;    
}

WebInspector.TextEditorHighlighter.prototype = {
    highlight: function(endLine)
    {
        // First check if we have work to do.
        var state = this._textModel.getAttribute(endLine - 1, "highlighter-state")
        if (state && !state.outOfDate) {
            // Last line is highlighted, just exit.
            return;
        }

        this._requestedEndLine = endLine;

        if (this._highlightTimer) {
            // There is a timer scheduled, it will catch the new job based on the new endLine set.
            return;
        }

        // We will be highlighting. First rewind to the last highlighted line to gain proper highlighter context.
        var startLine = endLine;
        while (startLine > 0) {
            var state = this._textModel.getAttribute(startLine - 1, "highlighter-state");
            if (state && !state.outOfDate)
                break;
            startLine--;
        }

        // Do small highlight synchronously. This will provide instant highlight on PageUp / PageDown, gentle scrolling.
        var toLine = Math.min(startLine + 200, endLine);
        this._highlightLines(startLine, toLine);

        // Schedule tail highlight if necessary.
        if (endLine > toLine)
            this._highlightTimer = setTimeout(this._highlightInChunks.bind(this, toLine, endLine), 100);
    },

    _highlightInChunks: function(startLine, endLine)
    {
        delete this._highlightTimer;

        // First we always check if we have work to do. Could be that user scrolled back and we can quit.
        var state = this._textModel.getAttribute(this._requestedEndLine - 1, "highlighter-state");
        if (state && !state.outOfDate)
            return;

        if (this._requestedEndLine !== endLine) {
            // User keeps updating the job in between of our timer ticks. Just reschedule self, don't eat CPU (they must be scrolling).
            this._highlightTimer = setTimeout(this._highlightInChunks.bind(this, startLine, this._requestedEndLine), 200);
            return;
        }

        // Highlight 500 lines chunk.
        var toLine = Math.min(startLine + 500, this._requestedEndLine);
        this._highlightLines(startLine, toLine);

        // Schedule tail highlight if necessary.
        if (toLine < this._requestedEndLine)
            this._highlightTimer = setTimeout(this._highlightInChunks.bind(this, toLine, this._requestedEndLine), 10);
    },

    updateHighlight: function(startLine, endLine)
    {
        // Start line was edited, we should highlight everything until endLine synchronously.
        var state = this._textModel.getAttribute(startLine, "highlighter-state");
        if (!state || state.outOfDate) {
            // Highlighter did not reach this point yet, nothing to update. It will reach it on subsequent timer tick and do the job.
            return;
        }

        var restored = this._highlightLines(startLine, endLine);

        // Set invalidated flag to the subsequent lines.
        for (var i = endLine; i < this._textModel.linesCount; ++i) {
            var highlighterState = this._textModel.getAttribute(i, "highlighter-state");
            if (highlighterState)
                highlighterState.outOfDate = !restored;
            else
                return;
        }
    },

    _highlightLines: function(startLine, endLine)
    {
        // Restore highlighter context taken from previous line.
        var state = this._textModel.getAttribute(startLine - 1, "highlighter-state");
        if (state)
            this._tokenizer.condition = state.postCondition;
        else
            this._tokenizer.condition = this._tokenizer.initialCondition;

        var damagedFrom = startLine;
        var damagedTo = startLine;
        for (var i = startLine; i < endLine; ++i) {
            damagedTo = i;

            state = {};
            state.preCondition = this._tokenizer.condition;
            state.attributes = {};

            this._lex(this._textModel.line(i), i, state.attributes);

            state.postCondition = this._tokenizer.condition;
            this._textModel.setAttribute(i, "highlighter-state", state);

            var nextLineState = this._textModel.getAttribute(i + 1, "highlighter-state");
            if (nextLineState && nextLineState.preCondition === state.postCondition) {
                // Following lines are up to date, no need re-highlight.
                this._damageCallback(damagedFrom, damagedTo);
                return true;
            }
        }
        this._damageCallback(damagedFrom, damagedTo);
        return false;
    },

    _lex: function(line, lineNumber, attributes) {
         this._tokenizer.line = line;
         var column = 0;
         do {
             var newColumn = this._tokenizer.nextToken(column);
             var tokenType = this._tokenizer.tokenType;
             if (tokenType)
                 attributes[column] = { length: newColumn - column, style: this._styles[tokenType] };
             column = newColumn;
         } while (column < line.length)
    }
}
