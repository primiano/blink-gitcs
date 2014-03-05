/*
 * Copyright (C) 2014 Google Inc. All rights reserved.
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
 * @constructor
 * @implements {WebInspector.FlameChartDataProvider}
 * @param {!WebInspector.TimelineModel} model
 * @param {!WebInspector.TimelineFrameModel} frameModel
 * @param {boolean} mainThread
 */
WebInspector.TimelineFlameChartDataProvider = function(model, frameModel, mainThread)
{
    WebInspector.FlameChartDataProvider.call(this);
    this._model = model;
    this._frameModel = frameModel;
    this._mainThread = mainThread;
    this._font = "bold 12px " + WebInspector.fontFamily();
}

WebInspector.TimelineFlameChartDataProvider.prototype = {
    /**
     * @return {number}
     */
    barHeight: function()
    {
        return 20;
    },

    /**
     * @return {number}
     */
    textBaseline: function()
    {
        return 6;
    },

    /**
     * @return {number}
     */
    textPadding: function()
    {
        return 5;
    },

    /**
     * @param {number} entryIndex
     * @return {string}
     */
    entryFont: function(entryIndex)
    {
        return this._font;
    },

    /**
     * @param {number} entryIndex
     * @return {?string}
     */
    entryTitle: function(entryIndex)
    {
        return this._isSelfSegment[entryIndex] ? this._records[entryIndex].title() : null;
    },

    /**
     * @param {number} startTime
     * @param {number} endTime
     * @return {?Array.<number>}
     */
    dividerOffsets: function(startTime, endTime)
    {
        var frames = this._frameModel.filteredFrames(this._model.minimumRecordTime() + startTime, this._model.minimumRecordTime() + endTime);
        if (frames.length > 30)
            return null;
        var offsets = [];
        for (var i = 0; i < frames.length; ++i)
            offsets.push(frames[i].startTimeOffset);
        return frames.length ? offsets : null;
    },

    reset: function()
    {
        this._timelineData = null;
    },

    /**
     * @param {!WebInspector.TimelineModel.Record} record
     */
    addRecord: function(record)
    {
        this._appendRecord(record, 0);
    },

    /**
     * @return {!WebInspector.FlameChart.TimelineData}
     */
    timelineData: function()
    {
        if (!this._timelineData) {
            this._resetData();
            var records = this._model.records();
            for (var i = 0; i < records.length; ++i)
                this._appendRecord(records[i], 0);
            this._zeroTime = this._model.minimumRecordTime();
        }
        return /** @type {!WebInspector.FlameChart.TimelineData} */(this._timelineData);
    },

    /**
     * @return {number}
     */
    zeroTime: function()
    {
        return this._zeroTime;
    },

    /**
     * @return {number}
     */
    totalTime: function()
    {
        return this._totalTime;
    },

    /**
     * @return {number}
     */
    maxStackDepth: function()
    {
        return this._maxStackDepth;
    },

    _resetData: function()
    {
        this._startTime = 0;
        this._endTime = 0;
        this._maxStackDepth = 5;
        this._totalTime = 1000;
        this._zeroTime = 0;

        /**
         * @type {?WebInspector.FlameChart.TimelineData}
         */
        this._timelineData = {
            entryLevels: [],
            entryTotalTimes: [],
            entryOffsets: [],
        };

        this._records = [];
        this._isSelfSegment = [];
    },

    /**
     * @param {!WebInspector.TimelineModel.Record} record
     * @param {number} level
     */
    _appendRecord: function(record, level)
    {
        var timelineData = this.timelineData();

        this._startTime = this._startTime ? Math.min(this._startTime, record.startTime) : record.startTime;
        this._zeroTime = this._startTime;
        var recordEndTime = record.endTime;
        this._endTime = Math.max(this._endTime, recordEndTime);
        this._totalTime = Math.max(1000, this._endTime - this._startTime);

        if (this._mainThread) {
            if (record.type === WebInspector.TimelineModel.RecordType.GPUTask || !!record.thread)
                return;
        } else {
            if (record.type === WebInspector.TimelineModel.RecordType.Program || !record.thread || record.thread === "gpu")
                return;
        }

        var recordIndex = this._pushRecord(record, true, level, record.startTime, record.endTime);
        var currentTime = record.startTime;
        for (var i = 0; i < record.children.length; ++i) {
            var childRecord = record.children[i];
            var childStartTime = childRecord.startTime;
            var childEndTime = childRecord.endTime;
            if (childStartTime === childEndTime) {
                this._appendRecord(childRecord, level + 1);
                continue;
            }

            if (currentTime !== childStartTime) {
                if (recordIndex !== -1) {
                    this._timelineData.entryTotalTimes[recordIndex] = childStartTime - record.startTime;
                    recordIndex = -1;
                } else {
                    this._pushRecord(record, true, level, currentTime, childStartTime);
                }
            }
            this._pushRecord(record, false, level, childStartTime, childEndTime);
            this._appendRecord(childRecord, level + 1);
            currentTime = childEndTime;
        }
        if (recordIndex === -1 && recordEndTime !== currentTime || record.children.length === 0)
            this._pushRecord(record, true, level, currentTime, recordEndTime);

        this._maxStackDepth = Math.max(this._maxStackDepth, level + 2);
    },

    /**
     * @param {!WebInspector.TimelineModel.Record} record
     * @param {boolean} isSelfSegment
     * @param {number} level
     * @param {number} startTime
     * @param {number} endTime
     * @return {number}
     */
    _pushRecord: function(record, isSelfSegment, level, startTime, endTime)
    {
        var index = this._records.length;
        this._records.push(record);
        this._timelineData.entryOffsets[index] = startTime - this._zeroTime;
        this._timelineData.entryLevels[index] = level;
        this._timelineData.entryTotalTimes[index] = endTime - startTime;
        this._isSelfSegment[index] = isSelfSegment;
        return index;
    },

    /**
     * @param {number} entryIndex
     * @return {?Array.<!{title: string, text: string}>}
     */
    prepareHighlightedEntryInfo: function(entryIndex)
    {
        return null;
    },

    /**
     * @param {number} entryIndex
     * @return {boolean}
     */
    canJumpToEntry: function(entryIndex)
    {
        return false;
    },

    /**
     * @param {number} entryIndex
     * @return {!string}
     */
    entryColor: function(entryIndex)
    {
        var category = WebInspector.TimelineUIUtils.categoryForRecord(this._records[entryIndex]);
        return this._isSelfSegment[entryIndex] ? category.fillColorStop1 : category.backgroundColor;
    },

    /**
     * @param {number} entryIndex
     * @return {!{startTimeOffset: number, endTimeOffset: number}}
     */
    highlightTimeRange: function(entryIndex)
    {
        var record = this._records[entryIndex];
        return {
            startTimeOffset: record.startTime - this._zeroTime,
            endTimeOffset: record.endTime - this._zeroTime
        };
    },

    /**
     * @param {number} entryIndex
     * @return {!string}
     */
    textColor: function(entryIndex)
    {
        return "white";
    }
}

/**
 * @constructor
 * @extends {WebInspector.View}
 * @implements {WebInspector.TimelineModeView}
 * @implements {WebInspector.FlameChartDelegate}
 * @param {!WebInspector.TimelineModeViewDelegate} delegate
 * @param {!WebInspector.TimelineModel} model
 * @param {!WebInspector.TimelineFrameModel} frameModel
 * @param {boolean} mainThread
 */
WebInspector.TimelineFlameChart = function(delegate, model, frameModel, mainThread)
{
    WebInspector.View.call(this);
    this.element.classList.add("timeline-flamechart");
    this.registerRequiredCSS("flameChart.css");
    this._delegate = delegate;
    this._model = model;
    this._dataProvider = new WebInspector.TimelineFlameChartDataProvider(model, frameModel, mainThread);
    this._mainView = new WebInspector.FlameChart.MainPane(this._dataProvider, this, true, true);
    this._mainView.show(this.element);
    this._model.addEventListener(WebInspector.TimelineModel.Events.RecordingStarted, this._onRecordingStarted, this);
    this._mainView.addEventListener(WebInspector.FlameChart.Events.EntrySelected, this._onEntrySelected, this);
}

WebInspector.TimelineFlameChart.prototype = {
    /**
     * @param {number} windowStartTime
     * @param {number} windowEndTime
     */
    requestWindowTimes: function(windowStartTime, windowEndTime)
    {
        this._delegate.requestWindowTimes(windowStartTime, windowEndTime);
    },

    /**
     * @param {?RegExp} textFilter
     */
    refreshRecords: function(textFilter)
    {
        this._dataProvider.reset();
        this._mainView.reset();
        this.setSelectedRecord(this._selectedRecord);
    },

    reset: function()
    {
        this._automaticallySizeWindow = true;
        this._dataProvider.reset();
        this._mainView.setWindowTimes(0, Infinity);
        delete this._selectedRecord;
    },

    _onRecordingStarted: function()
    {
        this._automaticallySizeWindow = true;
    },

    /**
     * @param {!WebInspector.TimelineModel.Record} record
     */
    addRecord: function(record)
    {
        this._dataProvider.addRecord(record);
        if (this._automaticallySizeWindow) {
            var minimumRecordTime = this._model.minimumRecordTime();
            if (record.startTime > (minimumRecordTime + 1000)) {
                this._automaticallySizeWindow = false;
                this._delegate.requestWindowTimes(minimumRecordTime, minimumRecordTime + 1000);
            }
            this._mainView._scheduleUpdate();
        }
    },

    /**
     * @param {number} startTime
     * @param {number} endTime
     */
    setWindowTimes: function(startTime, endTime)
    {
        this._startTime = startTime;
        this._endTime = endTime;
        this._updateWindow();
    },

    _updateWindow: function()
    {
        var minimumRecordTime = this._model.minimumRecordTime();
        var timeRange = this._model.maximumRecordTime() - minimumRecordTime;
        if (timeRange === 0)
            this._mainView.setWindowTimes(0, Infinity);
        else
            this._mainView.setWindowTimes(this._startTime, this._endTime);
    },

    /**
     * @param {number} width
     */
    setSidebarSize: function(width)
    {
    },

    /**
     * @param {?WebInspector.TimelineModel.Record} record
     * @param {string=} regex
     * @param {boolean=} selectRecord
     */
    highlightSearchResult: function(record, regex, selectRecord)
    {
    },

    /**
     * @param {?WebInspector.TimelineModel.Record} record
     */
    setSelectedRecord: function(record)
    {
        this._selectedRecord = record;
        var entryRecords = this._dataProvider._records;
        for (var entryIndex = 0; entryIndex < entryRecords.length; ++entryIndex) {
            if (entryRecords[entryIndex] === record) {
                this._mainView.setSelectedEntry(entryIndex);
                return;
            }
        }
        this._mainView.setSelectedEntry(-1);
        if (this._selectedElement) {
            this._selectedElement.remove();
            delete this._selectedElement;
        }
    },

    /**
     * @param {!WebInspector.Event} event
     */
    _onEntrySelected: function(event)
    {
        var entryIndex = event.data;
        var record = this._dataProvider._records[entryIndex];
        this._delegate.selectRecord(record);
    },

    __proto__: WebInspector.View.prototype
}
