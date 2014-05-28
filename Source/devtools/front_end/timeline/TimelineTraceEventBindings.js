// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @constructor
 */
WebInspector.TimelineTraceEventBindings = function()
{
    this._reset();
}

WebInspector.TimelineTraceEventBindings.RecordType = {
    Program: "Program",
    EventDispatch: "EventDispatch",

    GPUTask: "GPUTask",

    RequestMainThreadFrame: "RequestMainThreadFrame",
    BeginFrame: "BeginFrame",
    BeginMainThreadFrame: "BeginMainThreadFrame",
    ActivateLayerTree: "ActivateLayerTree",
    DrawFrame: "DrawFrame",
    ScheduleStyleRecalculation: "ScheduleStyleRecalculation",
    RecalculateStyles: "RecalculateStyles",
    InvalidateLayout: "InvalidateLayout",
    Layout: "Layout",
    UpdateLayerTree: "UpdateLayerTree",
    PaintSetup: "PaintSetup",
    Paint: "Paint",
    Rasterize: "Rasterize",
    RasterTask: "RasterTask",
    ScrollLayer: "ScrollLayer",
    DecodeImage: "DecodeImage",
    ResizeImage: "ResizeImage",
    CompositeLayers: "CompositeLayers",

    ParseHTML: "ParseHTML",

    TimerInstall: "TimerInstall",
    TimerRemove: "TimerRemove",
    TimerFire: "TimerFire",

    XHRReadyStateChange: "XHRReadyStateChange",
    XHRLoad: "XHRLoad",
    EvaluateScript: "EvaluateScript",

    MarkLoad: "MarkLoad",
    MarkDOMContent: "MarkDOMContent",
    MarkFirstPaint: "MarkFirstPaint",

    TimeStamp: "TimeStamp",
    ConsoleTime: "ConsoleTime",

    ResourceSendRequest: "ResourceSendRequest",
    ResourceReceiveResponse: "ResourceReceiveResponse",
    ResourceReceivedData: "ResourceReceivedData",
    ResourceFinish: "ResourceFinish",

    FunctionCall: "FunctionCall",
    GCEvent: "GCEvent",
    JSFrame: "JSFrame",

    UpdateCounters: "UpdateCounters",

    RequestAnimationFrame: "RequestAnimationFrame",
    CancelAnimationFrame: "CancelAnimationFrame",
    FireAnimationFrame: "FireAnimationFrame",

    WebSocketCreate : "WebSocketCreate",
    WebSocketSendHandshakeRequest : "WebSocketSendHandshakeRequest",
    WebSocketReceiveHandshakeResponse : "WebSocketReceiveHandshakeResponse",
    WebSocketDestroy : "WebSocketDestroy",

    EmbedderCallback : "EmbedderCallback",

    CallStack: "CallStack",
    SetLayerTreeId: "SetLayerTreeId",
    TracingStartedInPage: "TracingStartedInPage",

    LayerTreeHostImplSnapshot: "cc::LayerTreeHostImpl"
};


WebInspector.TimelineTraceEventBindings.prototype = {
    /**
     * @return {!Array.<!WebInspector.TracingModel.Event>}
     */
    mainThreadEvents: function()
    {
        return this._mainThreadEvents;
    },

    _reset: function()
    {
        this._resetProcessingState();
        this._mainThreadEvents = [];
    },

    _resetProcessingState: function()
    {
        this._sendRequestEvents = {};
        this._timerEvents = {};
        this._requestAnimationFrameEvents = {};
        this._layoutInvalidate = {};
        this._lastScheduleStyleRecalculation = {};
        this._webSocketCreateEvents = {};

        this._lastRecalculateStylesEvent = null;
        this._currentScriptEvent = null;
        this._eventStack = [];
    },

    /**
     * @param {!Array.<!WebInspector.TracingModel.Event>} events
     */
    setEvents: function(events)
    {
        this._resetProcessingState();
        for (var i = 0, length = events.length; i < length; i++)
            this._processEvent(events[i]);
        this._resetProcessingState();
    },

    /**
     * @param {!WebInspector.TracingModel.Event} event
     */
    _processEvent: function(event)
    {
        var recordTypes = WebInspector.TimelineTraceEventBindings.RecordType;

        var eventStack = this._eventStack;
        while (eventStack.length && eventStack.peekLast().endTime < event.startTime)
            eventStack.pop();
        var duration = event.duration;
        if (duration) {
            if (eventStack.length) {
                var parent = eventStack.peekLast();
                parent.selfTime -= duration;
            }
            event.selfTime = duration;
            eventStack.push(event);
        }

        if (this._currentScriptEvent && event.startTime > this._currentScriptEvent.endTime)
            this._currentScriptEvent = null;

        switch (event.name) {
        case recordTypes.CallStack:
            var lastMainThreadEvent = this._mainThreadEvents.peekLast();
            if (lastMainThreadEvent)
                lastMainThreadEvent.stackTrace = event.args.stack;
            break;

        case recordTypes.ResourceSendRequest:
            this._sendRequestEvents[event.args.data["requestId"]] = event;
            break;

        case recordTypes.ResourceReceiveResponse:
        case recordTypes.ResourceReceivedData:
        case recordTypes.ResourceFinish:
            event.initiator = this._sendRequestEvents[event.args.data["requestId"]];
            break;

        case recordTypes.TimerInstall:
            this._timerEvents[event.args.data["timerId"]] = event;
            break;

        case recordTypes.TimerFire:
            event.initiator = this._timerEvents[event.args.data["timerId"]];
            break;

        case recordTypes.RequestAnimationFrame:
            this._requestAnimationFrameEvents[event.args.data["id"]] = event;
            break;

        case recordTypes.FireAnimationFrame:
            event.initiator = this._requestAnimationFrameEvents[event.args.data["id"]];
            break;

        case recordTypes.ScheduleStyleRecalculation:
            this._lastScheduleStyleRecalculation[event.args.frame] = event;
            break;

        case recordTypes.RecalculateStyles:
            event.initiator = this._lastScheduleStyleRecalculation[event.args.frame];
            this._lastRecalculateStylesEvent = event;
            break;

        case recordTypes.InvalidateLayout:
            // Consider style recalculation as a reason for layout invalidation,
            // but only if we had no earlier layout invalidation records.
            var layoutInitator = event;
            var frameId = event.args.frame;
            if (!this._layoutInvalidate[frameId] && this._lastRecalculateStylesEvent && this._lastRecalculateStylesEvent.endTime >  event.startTime)
                layoutInitator = this._lastRecalculateStylesEvent.initiator;
            this._layoutInvalidate[frameId] = layoutInitator;
            break;

        case recordTypes.Layout:
            var frameId = event.args["beginData"]["frame"];
            event.initiator = this._layoutInvalidate[frameId];
            this._layoutInvalidate[frameId] = null;
            if (this._currentScriptEvent)
                event.warning = WebInspector.UIString("Forced synchronous layout is a possible performance bottleneck.");
            break;

        case recordTypes.WebSocketCreate:
            this._webSocketCreateEvents[event.args.data["identifier"]] = event;
            break;

        case recordTypes.WebSocketSendHandshakeRequest:
        case recordTypes.WebSocketReceiveHandshakeResponse:
        case recordTypes.WebSocketDestroy:
            event.initiator = this._webSocketCreateEvents[event.args.data["identifier"]];
            break;

        case recordTypes.EvaluateScript:
        case recordTypes.FunctionCall:
            if (!this._currentScriptEvent)
                this._currentScriptEvent = event;
            break;

        case recordTypes.SetLayerTreeId:
            this._inspectedTargetLayerTreeId = event.args["layerTreeId"];
            break;

        case recordTypes.TracingStartedInPage:
            this._mainThread = event.thread;
            break;
        }
        if (this._mainThread === event.thread)
            this._mainThreadEvents.push(event);
    }
}

