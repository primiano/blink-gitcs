/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

WebInspector.TextViewer = function(textModel, platform, url)
{
    this._textModel = textModel;
    this._textModel.changeListener = this._buildChunks.bind(this);
    this._highlighter = new WebInspector.TextEditorHighlighter(this._textModel, this._highlightDataReady.bind(this));

    this.element = document.createElement("div");
    this.element.className = "text-editor monospace";
    this.element.tabIndex = 0;

    this.element.addEventListener("scroll", this._scroll.bind(this), false);

    this._url = url;

    this._linesContainerElement = document.createElement("table");
    this._linesContainerElement.className = "text-editor-lines";
    this._linesContainerElement.setAttribute("cellspacing", 0);
    this._linesContainerElement.setAttribute("cellpadding", 0);
    this.element.appendChild(this._linesContainerElement);

    this._defaultChunkSize = 50;
    this._paintCoalescingLevel = 0;
}

WebInspector.TextViewer.prototype = {
    set mimeType(mimeType)
    {
        this._highlighter.mimeType = mimeType;
    },

    get textModel()
    {
        return this._textModel;
    },

    revealLine: function(lineNumber)
    {
        if (lineNumber >= this._textModel.linesCount)
            return;

        var chunk = this._makeLineAChunk(lineNumber);
        chunk.element.scrollIntoViewIfNeeded();
    },

    addDecoration: function(lineNumber, decoration)
    {
        var chunk = this._makeLineAChunk(lineNumber);
        chunk.addDecoration(decoration);
    },

    removeDecoration: function(lineNumber, decoration)
    {
        var chunk = this._makeLineAChunk(lineNumber);
        chunk.removeDecoration(decoration);
    },

    markAndRevealRange: function(range)
    {
        if (this._rangeToMark) {
            var markedLine = this._rangeToMark.startLine;
            this._rangeToMark = null;
            this._paintLines(markedLine, markedLine + 1);
        }

        if (range) {
            this._rangeToMark = range;
            this.revealLine(range.startLine);
            this._paintLines(range.startLine, range.startLine + 1);
        }
    },

    highlightLine: function(lineNumber)
    {
        if (typeof this._highlightedLine === "number") {
            var chunk = this._makeLineAChunk(this._highlightedLine);
            chunk.removeDecoration("webkit-highlighted-line");
        }
        this._highlightedLine = lineNumber;
        this.revealLine(lineNumber);
        var chunk = this._makeLineAChunk(lineNumber);
        chunk.addDecoration("webkit-highlighted-line");
    },

    _buildChunks: function()
    {
        this._linesContainerElement.removeChildren();

        var paintLinesCallback = this._paintLines.bind(this);
        this._textChunks = [];
        for (var i = 0; i < this._textModel.linesCount; i += this._defaultChunkSize) {
            var chunk = new WebInspector.TextChunk(this._textModel, i, i + this._defaultChunkSize, paintLinesCallback);
            this._textChunks.push(chunk);
            this._linesContainerElement.appendChild(chunk.element);
        }
        this._indexChunks();
        this._repaintAll();
    },

    _makeLineAChunk: function(lineNumber)
    {
        if (!this._textChunks)
            this._buildChunks();

        var chunkNumber = this._chunkNumberForLine(lineNumber);
        var oldChunk = this._textChunks[chunkNumber];
        if (oldChunk.linesCount === 1)
            return oldChunk;

        var wasExpanded = oldChunk.expanded;
        oldChunk.expanded = false;

        var insertIndex = oldChunk.chunkNumber + 1;
        var paintLinesCallback = this._paintLines.bind(this);

        // Prefix chunk.
        if (lineNumber > oldChunk.startLine) {
            var prefixChunk = new WebInspector.TextChunk(this._textModel, oldChunk.startLine, lineNumber, paintLinesCallback);
            this._textChunks.splice(insertIndex++, 0, prefixChunk);
            this._linesContainerElement.insertBefore(prefixChunk.element, oldChunk.element);
        }

        // Line chunk.
        var lineChunk = new WebInspector.TextChunk(this._textModel, lineNumber, lineNumber + 1, paintLinesCallback);
        this._textChunks.splice(insertIndex++, 0, lineChunk);
        this._linesContainerElement.insertBefore(lineChunk.element, oldChunk.element);

        // Suffix chunk.
        if (oldChunk.startLine + oldChunk.linesCount > lineNumber + 1) {
            var suffixChunk = new WebInspector.TextChunk(this._textModel, lineNumber + 1, oldChunk.startLine + oldChunk.linesCount, paintLinesCallback);
            this._textChunks.splice(insertIndex, 0, suffixChunk);
            this._linesContainerElement.insertBefore(suffixChunk.element, oldChunk.element);
        }

        // Remove enclosing chunk.
        this._textChunks.splice(oldChunk.chunkNumber, 1);
        this._linesContainerElement.removeChild(oldChunk.element);
        this._indexChunks();

        if (wasExpanded) {
            if (prefixChunk)
                prefixChunk.expanded = true;
            lineChunk.expanded = true;
            if (suffixChunk)
                suffixChunk.expanded = true;
        }

        return lineChunk;
    },

    _indexChunks: function()
    {
        for (var i = 0; i < this._textChunks.length; ++i)
            this._textChunks[i].chunkNumber = i;
    },

    _scroll: function()
    {
        this._repaintAll();
    },

    beginUpdates: function(enabled)
    {
        this._paintCoalescingLevel++;
    },

    endUpdates: function(enabled)
    {
        this._paintCoalescingLevel--;
        if (!this._paintCoalescingLevel)
            this._repaintAll();
    },

    _chunkForOffset: function(offset)
    {
        var currentOffset = 0;
        var row = this._linesContainerElement.firstChild;
        while (row) {
            var rowHeight = row.offsetHeight;
            if (offset >= currentOffset && offset < currentOffset + rowHeight)
                return row.chunkNumber;
            row = row.nextSibling;
            currentOffset += rowHeight;
        }
        return this._textChunks.length - 1;
    },

    _chunkNumberForLine: function(lineNumber)
    {
        for (var i = 0; i < this._textChunks.length; ++i) {
            var line = this._textChunks[i].startLine;
            if (lineNumber >= this._textChunks[i].startLine && lineNumber < this._textChunks[i].startLine + this._textChunks[i].linesCount)
                return i;
        }
        return this._textChunks.length - 1;
    },

    _chunkForLine: function(lineNumber)
    {
        return this._textChunks[this._chunkNumberForLine(lineNumber)];
    },

    _chunkStartLine: function(chunkNumber)
    {
        var lineNumber = 0;
        for (var i = 0; i < chunkNumber && i < this._textChunks.length; ++i)
            lineNumber += this._textChunks[i].linesCount;
        return lineNumber;
    },

    _repaintAll: function()
    {
        if (this._paintCoalescingLevel)
            return;

        if (!this._textChunks)
            this._buildChunks();

        var visibleFrom = this.element.scrollTop;
        var visibleTo = this.element.scrollTop + this.element.clientHeight;

        var offset = 0;
        var firstVisibleLine = -1;
        var lastVisibleLine = 0;
        var toExpand = [];
        var toCollapse = [];
        for (var i = 0; i < this._textChunks.length; ++i) {
            var chunk = this._textChunks[i];
            var chunkHeight = chunk.height;
            if (offset + chunkHeight > visibleFrom && offset < visibleTo) {
                toExpand.push(chunk);
                if (firstVisibleLine === -1)
                    firstVisibleLine = chunk.startLine;
                lastVisibleLine = chunk.startLine + chunk.linesCount;
            } else {
                toCollapse.push(chunk);
                if (offset >= visibleTo)
                    break;
            }
            offset += chunkHeight;
        }

        for (var j = i; j < this._textChunks.length; ++j)
            toCollapse.push(this._textChunks[i]);

        var selection = this._getSelection();

        this._muteHighlightListener = true;
        this._highlighter.highlight(lastVisibleLine);
        delete this._muteHighlightListener;

        for (var i = 0; i < toCollapse.length; ++i)
            toCollapse[i].expanded = false;
        for (var i = 0; i < toExpand.length; ++i)
            toExpand[i].expanded = true;

        this._restoreSelection(selection);
    },

    _highlightDataReady: function(fromLine, toLine)
    {
        if (this._muteHighlightListener)
            return;

        var selection;
        for (var i = fromLine; i < toLine; ++i) {
            var lineRow = this._textModel.getAttribute(i, "line-row");
            if (!lineRow || lineRow.highlighted)
                continue;
            if (!selection)
                selection = this._getSelection();
            this._paintLine(lineRow, i);
        }
        this._restoreSelection(selection);
    },

    _paintLines: function(fromLine, toLine)
    {
        for (var i = fromLine; i < toLine; ++i) {
            var lineRow = this._textModel.getAttribute(i, "line-row");
            if (lineRow)
                this._paintLine(lineRow, i);
        }
    },

    _paintLine: function(lineRow, lineNumber)
    {
        var element = lineRow.lastChild;
        var highlighterState = this._textModel.getAttribute(lineNumber, "highlighter-state");
        var line = this._textModel.line(lineNumber);

        if (!highlighterState) {
            if (this._rangeToMark && this._rangeToMark.startLine === lineNumber)
                this._markRange(element, line, this._rangeToMark.startColumn, this._rangeToMark.endColumn);
            return;
        }

        element.removeChildren();

        var plainTextStart = -1;
        for (var j = 0; j < line.length;) {
            if (j > 1000) {
                // This line is too long - do not waste cycles on minified js highlighting.
                break;
            }
            var attribute = highlighterState && highlighterState.attributes[j];
            if (!attribute || !attribute.style) {
                if (plainTextStart === -1)
                    plainTextStart = j;
                j++;
            } else {
                if (plainTextStart !== -1) {
                    element.appendChild(document.createTextNode(line.substring(plainTextStart, j)));
                    plainTextStart = -1;
                }
                element.appendChild(this._createSpan(line.substring(j, j + attribute.length), attribute.tokenType));
                j += attribute.length;
            }
        }
        if (plainTextStart !== -1)
            element.appendChild(document.createTextNode(line.substring(plainTextStart, line.length)));
        if (this._rangeToMark && this._rangeToMark.startLine === lineNumber)
            this._markRange(element, line, this._rangeToMark.startColumn, this._rangeToMark.endColumn);
        if (lineRow.decorationsElement)
            element.appendChild(lineRow.decorationsElement);
    },

    _getSelection: function()
    {
        var selection = window.getSelection();
        if (selection.isCollapsed)
            return null;
        var selectionRange = selection.getRangeAt(0);
        var start = this._selectionToPosition(selectionRange.startContainer, selectionRange.startOffset);
        var end = this._selectionToPosition(selectionRange.endContainer, selectionRange.endOffset);
        return new WebInspector.TextRange(start.line, start.column, end.line, end.column);
    },

    _restoreSelection: function(range)
    {
        if (!range)
            return;
        var startRow = this._textModel.getAttribute(range.startLine, "line-row");
        if (startRow)
            var start = startRow.lastChild.rangeBoundaryForOffset(range.startColumn);
        else {
            var offset = range.startColumn;
            var chunkNumber = this._chunkNumberForLine(range.startLine);
            for (var i = this._chunkStartLine(chunkNumber); i < range.startLine; ++i)
                offset += this._textModel.line(i).length + 1; // \n
            var lineCell = this._textChunks[chunkNumber].element.lastChild;
            if (lineCell.firstChild)
                var start = { container: lineCell.firstChild, offset: offset };
            else
                var start = { container: lineCell, offset: 0 };
        }

        var endRow = this._textModel.getAttribute(range.endLine, "line-row");
        if (endRow)
            var end = endRow.lastChild.rangeBoundaryForOffset(range.endColumn);
        else {
            var offset = range.endColumn;
            var chunkNumber = this._chunkNumberForLine(range.endLine);
            for (var i = this._chunkStartLine(chunkNumber); i < range.endLine; ++i)
                offset += this._textModel.line(i).length + 1; // \n
            var lineCell = this._textChunks[chunkNumber].element.lastChild;
            if (lineCell.firstChild)
                var end = { container: lineCell.firstChild, offset: offset };
            else
                var end = { container: lineCell, offset: 0 };
        }

        var selectionRange = document.createRange();
        selectionRange.setStart(start.container, start.offset);
        selectionRange.setEnd(end.container, end.offset);

        var selection = window.getSelection();
        selection.removeAllRanges();
        selection.addRange(selectionRange);
    },

    _selectionToPosition: function(container, offset)
    {
        if (container === this.element && offset === 0)
            return { line: 0, column: 0 };
        if (container === this.element && offset === 1)
            return { line: this._textModel.linesCount - 1, column: this._textModel.lineLength(this._textModel.linesCount - 1) };

        var lineRow = container.enclosingNodeOrSelfWithNodeName("tr");
        var lineNumber = lineRow.lineNumber;
        if (container.nodeName === "TD" && offset === 0)
            return { line: lineNumber, column: 0 };
        if (container.nodeName === "TD" && offset === 1)
            return { line: lineNumber, column: this._textModel.lineLength(lineNumber) };

        var column = 0;
        if (lineRow.chunk) {
            // This is chunk.
            var text = lineRow.lastChild.textContent;
            for (var i = 0; i < offset; ++i) {
                if (text.charAt(i) === "\n") {
                    lineNumber++;
                    column = 0;
                } else
                    column++; 
            }
            return { line: lineNumber, column: column };
        }

        // This is individul line.
        var column = 0;
        var node = lineRow.lastChild.traverseNextTextNode(lineRow.lastChild);
        while (node && node !== container) {
            column += node.textContent.length;
            node = node.traverseNextTextNode(lineRow.lastChild);
        }
        column += offset;
        return { line: lineRow.lineNumber, column: column };
    },

    _createSpan: function(content, className)
    {
        if (className === "html-resource-link" || className === "html-external-link")
            return this._createLink(content, className === "html-external-link");

        var span = document.createElement("span");
        span.className = "webkit-" + className;
        span.appendChild(document.createTextNode(content));
        return span;
    },

    _createLink: function(content, isExternal)
    {
        var quote = content.charAt(0);
        if (content.length > 1 && (quote === "\"" ||   quote === "'"))
            content = content.substring(1, content.length - 1);
        else
            quote = null;

        var a = WebInspector.linkifyURLAsNode(this._rewriteHref(content), content, null, isExternal);
        var span = document.createElement("span");
        span.className = "webkit-html-attribute-value";
        if (quote)
            span.appendChild(document.createTextNode(quote));
        span.appendChild(a);
        if (quote)
            span.appendChild(document.createTextNode(quote));
        return span;
    },

    _rewriteHref: function(hrefValue, isExternal)
    {
        if (!this._url || !hrefValue || hrefValue.indexOf("://") > 0)
            return hrefValue;
        return WebInspector.completeURL(this._url, hrefValue);
    },

    _markRange: function(element, lineText, startOffset, endOffset)
    {
        var markNode = document.createElement("span");
        markNode.className = "webkit-markup";
        markNode.textContent = lineText.substring(startOffset, endOffset);

        var markLength = endOffset - startOffset;
        var boundary = element.rangeBoundaryForOffset(startOffset);
        var textNode = boundary.container;
        var text = textNode.textContent;

        if (boundary.offset + markLength < text.length) {
            // Selection belong to a single split mode.
            textNode.textContent = text.substring(boundary.offset + markLength);
            textNode.parentElement.insertBefore(markNode, textNode);
            var prefixNode = document.createTextNode(text.substring(0, boundary.offset));
            textNode.parentElement.insertBefore(prefixNode, markNode);
            return;
        }

        var parentElement = textNode.parentElement;
        var anchorElement = textNode.nextSibling;

        markLength -= text.length - boundary.offset;
        textNode.textContent = text.substring(0, boundary.offset);
        textNode = textNode.traverseNextTextNode(element);

        while (textNode) {
            var text = textNode.textContent;
            if (markLength < text.length) {
                textNode.textContent = text.substring(markLength);
                break;
            }

            markLength -= text.length;
            textNode.textContent = "";
            textNode = textNode.traverseNextTextNode(element);
        }

        parentElement.insertBefore(markNode, anchorElement);
    },

    resize: function()
    {
        this._repaintAll();
    }
}

WebInspector.TextChunk = function(textModel, startLine, endLine, paintLinesCallback)
{
    this.element = document.createElement("tr");
    this._textModel = textModel;
    this.element.chunk = this;
    this.element.lineNumber = startLine;

    this.startLine = startLine;
    endLine = Math.min(this._textModel.linesCount, endLine);
    this.linesCount = endLine - startLine;

    this._lineNumberElement = document.createElement("td");
    this._lineNumberElement.className = "webkit-line-number";
    this._lineNumberElement.textContent = this._lineNumberText(this.startLine);
    this.element.appendChild(this._lineNumberElement);

    this._lineContentElement = document.createElement("td");
    this._lineContentElement.className = "webkit-line-content";
    this.element.appendChild(this._lineContentElement);

    this._expanded = false;

    var lines = [];
    for (var i = this.startLine; i < this.startLine + this.linesCount; ++i)
        lines.push(this._textModel.line(i));
    this._lineContentElement.textContent = lines.join("\n");
    this._paintLines = paintLinesCallback;
}

WebInspector.TextChunk.prototype = {
    addDecoration: function(decoration)
    {
        if (typeof decoration === "string") {
            this.element.addStyleClass(decoration);
            return;
        }
        if (!this.element.decorationsElement) {
            this.element.decorationsElement = document.createElement("div");
            this._lineContentElement.appendChild(this.element.decorationsElement);
        }
        this.element.decorationsElement.appendChild(decoration);
    },

    removeDecoration: function(decoration)
    {
        if (typeof decoration === "string") {
            this.element.removeStyleClass(decoration);
            return;
        }
        if (!this.element.decorationsElement)
            return;
        this.element.decorationsElement.removeChild(decoration);
    },

    get expanded()
    {
        return this._expanded;
    },

    set expanded(expanded)
    {
        if (this._expanded === expanded)
            return;

        this._expanded = expanded;

        if (this.linesCount === 1) {
            this._textModel.setAttribute(this.startLine, "line-row", this.element);
            if (expanded)
                this._paintLines(this.startLine, this.startLine + 1);
            return;
        }

        if (expanded) {
            var parentElement = this.element.parentElement;
            for (var i = this.startLine; i < this.startLine + this.linesCount; ++i) {
                var lineRow = document.createElement("tr");
                lineRow.lineNumber = i;

                var lineNumberElement = document.createElement("td");
                lineNumberElement.className = "webkit-line-number";
                lineNumberElement.textContent = this._lineNumberText(i);
                lineRow.appendChild(lineNumberElement);

                var lineContentElement = document.createElement("td");
                lineContentElement.className = "webkit-line-content";
                lineContentElement.textContent = this._textModel.line(i);
                lineRow.appendChild(lineContentElement);

                this._textModel.setAttribute(i, "line-row", lineRow);
                parentElement.insertBefore(lineRow, this.element);
            }
            parentElement.removeChild(this.element);

            this._paintLines(this.startLine, this.startLine + this.linesCount);
        } else {
            var firstLine = this._textModel.getAttribute(this.startLine, "line-row");
            var parentElement = firstLine.parentElement;

            parentElement.insertBefore(this.element, firstLine);
            for (var i = this.startLine; i < this.startLine + this.linesCount; ++i) {
                var lineRow = this._textModel.getAttribute(i, "line-row");
                this._textModel.removeAttribute(i, "line-row");
                parentElement.removeChild(lineRow);
            }
        }
    },

    get height()
    {
        if (!this._expanded)
            return this.element.offsetHeight;
        var result = 0;
        for (var i = this.startLine; i < this.startLine + this.linesCount; ++i) {
            var lineRow = this._textModel.getAttribute(i, "line-row");
            result += lineRow.offsetHeight;
        }
        return result;
    },

    _lineNumberText: function(lineNumber)
    {
        var totalDigits = Math.ceil(Math.log(this._textModel.linesCount + 1) / Math.log(10));
        var digits = Math.ceil(Math.log(lineNumber + 2) / Math.log(10));

        var text = "";
        for (var i = digits; i < totalDigits; ++i)
            text += " ";
        text += lineNumber + 1;
        return text;
    }
}
