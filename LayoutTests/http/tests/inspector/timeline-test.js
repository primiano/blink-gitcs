var initialize_Timeline = function() {

// Scrub values when printing out these properties in the record or data field.
InspectorTest.timelinePropertyFormatters = {
    children: "formatAsTypeName",
    endTime: "formatAsTypeName",
    requestId: "formatAsTypeName",
    startTime: "formatAsTypeName",
    stackTrace: "formatAsTypeName",
    url: "formatAsURL",
    scriptName: "formatAsTypeName",
    scriptId: "formatAsTypeName",
    usedHeapSizeDelta: "skip",
    mimeType: "formatAsTypeName",
    id: "formatAsTypeName",
    timerId: "formatAsTypeName",
    scriptLine: "formatAsTypeName",
    layerId: "formatAsTypeName",
    lineNumber: "formatAsTypeName",
    columnNumber: "formatAsTypeName",
    frameId: "formatAsTypeName",
    encodedDataLength: "formatAsTypeName",
    identifier: "formatAsTypeName",
    clip: "formatAsTypeName",
    root: "formatAsTypeName",
    backendNodeId: "formatAsTypeName",
    networkTime: "formatAsTypeName",
    thread: "formatAsTypeName"
};

InspectorTest.timelinePresentationModel = function()
{
    return WebInspector.panels.timeline._currentViews[0]._presentationModel;
}

InspectorTest.timelineModel = function()
{
    return WebInspector.panels.timeline._model;
}

InspectorTest.startTimeline = function(callback)
{
    InspectorTest._timelineRecords = [];
    WebInspector.inspectorView.panel("timeline").toggleTimelineButton.toggled = true;
    WebInspector.inspectorView.panel("timeline")._model._collectionEnabled = true;
    TimelineAgent.start(5, false, undefined, true, false, callback);
    function addRecord(record)
    {
        InspectorTest._timelineRecords.push(record);
        for (var i = 0; record.children && i < record.children.length; ++i)
            addRecord(record.children[i]);
    }
    InspectorTest._addTimelineEvent = function(event)
    {
        addRecord(event.data);
    }
    WebInspector.timelineManager.addEventListener(WebInspector.TimelineManager.EventTypes.TimelineEventRecorded, InspectorTest._addTimelineEvent);
};


InspectorTest.waitForRecordType = function(recordType, callback)
{
    WebInspector.timelineManager.addEventListener(WebInspector.TimelineManager.EventTypes.TimelineEventRecorded, addEvent);

    function addEvent(event)
    {
        addRecord(event.data);
    }
    function addRecord(record)
    {
        if (record.type !== WebInspector.TimelineModel.RecordType[recordType]) {
            for (var i = 0; record.children && i < record.children.length; ++i)
                addRecord(record.children[i]);
            return;
        }
        WebInspector.timelineManager.removeEventListener(WebInspector.TimelineManager.EventTypes.TimelineEventRecorded, addEvent);
        callback(record);
    }
}

InspectorTest.stopTimeline = function(callback)
{
    function didStop()
    {
        WebInspector.timelineManager.removeEventListener(WebInspector.TimelineManager.EventTypes.TimelineEventRecorded, InspectorTest._addTimelineEvent);
        WebInspector.inspectorView.panel("timeline").toggleTimelineButton.toggled = false;
        WebInspector.inspectorView.panel("timeline")._model._collectionEnabled = false;
        callback(InspectorTest._timelineRecords);
    }
    TimelineAgent.stop(didStop);
};

InspectorTest.evaluateWithTimeline = function(actions, doneCallback)
{
    InspectorTest.startTimeline(step1);
    function step1()
    {
        InspectorTest.evaluateInPage(actions, step2);
    }

    function step2()
    {
        InspectorTest.stopTimeline(doneCallback);
    }
}

InspectorTest.invokeAsyncWithTimeline = function(functionName, doneCallback)
{
    InspectorTest.startTimeline(step1);
    function step1()
    {
        InspectorTest.invokePageFunctionAsync(functionName, step2);
    }

    function step2()
    {
       InspectorTest.stopTimeline(doneCallback);
    }
}

InspectorTest.loadTimelineRecords = function(records)
{
    var model = WebInspector.inspectorView.showPanel("timeline")._model;
    model.reset();
    records.forEach(model._addRecord, model);
}

InspectorTest.performActionsAndPrint = function(actions, typeName, includeTimeStamps)
{
    function callback()
    {
        InspectorTest.printTimelineRecords(typeName);
        if (includeTimeStamps) {
            InspectorTest.addResult("Timestamp records: ");
            InspectorTest.printTimestampRecords(typeName);
        }
        InspectorTest.completeTest();
    }
    InspectorTest.evaluateWithTimeline(actions, callback);
};

InspectorTest.printTimelineRecords = function(typeName, formatter)
{
    InspectorTest.innerPrintTimelineRecords(InspectorTest._timelineRecords, typeName, formatter);
};

InspectorTest.printTimelinePresentationRecords = function(typeName, formatter)
{
    InspectorTest.innerPrintTimelinePresentationRecords(WebInspector.panels.timeline._model.records(), typeName, formatter);
};

InspectorTest.printTimestampRecords = function(typeName, formatter)
{
    InspectorTest.innerPrintTimelineRecords(InspectorTest.timelineModel().eventDividerRecords(), typeName, formatter);
};

InspectorTest.innerPrintTimelineRecords = function(records, typeName, formatter)
{
    for (var i = 0; i < records.length; ++i) {
        var recordTypeName = (typeof records[i].type === "string") ? records[i].type : records[i].type();
        if (typeName && recordTypeName === WebInspector.TimelineModel.RecordType[typeName])
            InspectorTest.printTimelineRecordProperties(records[i]);
        if (formatter)
            formatter(records[i]);
    }
};

InspectorTest.innerPrintTimelinePresentationRecords = function(records, typeName, formatter)
{
    for (var i = 0; i < records.length; ++i) {
        if (typeName && records[i].type() === WebInspector.TimelineModel.RecordType[typeName])
            InspectorTest.printTimelineRecordProperties(records[i]);
        if (formatter)
            formatter(records[i]);
        InspectorTest.innerPrintTimelinePresentationRecords(records[i].children(), typeName, formatter);
    }
};

// Dump just the record name, indenting output on separate lines for subrecords
InspectorTest.dumpTimelineRecord = function(record, detailsCallback, level, filterTypes)
{
    if (typeof level !== "number")
        level = 0;
    var prefix = "";
    var suffix = "";
    for (var i = 0; i < level ; ++i)
        prefix = "----" + prefix;
    if (level > 0)
        prefix = prefix + "> ";
    if (record.type === WebInspector.TimelineModel.RecordType.TimeStamp
        || record.type === WebInspector.TimelineModel.RecordType.ConsoleTime) {
        suffix = " : " + record.data.message;
    }
    if (detailsCallback)
        suffix += " " + detailsCallback(record);
    InspectorTest.addResult(prefix + InspectorTest._timelineAgentTypeToString(record.type) + suffix);

    var numChildren = record.children ? record.children.length : 0;
    for (var i = 0; i < numChildren; ++i) {
        if (filterTypes && filterTypes.indexOf(record.children[i].type) == -1)
            continue;
        InspectorTest.dumpTimelineRecord(record.children[i], detailsCallback, level + 1, filterTypes);
    }
}

// Dump just the record name, indenting output on separate lines for subrecords
InspectorTest.dumpPresentationRecord = function(presentationRecord, detailsCallback, level, filterTypes)
{
    var record = !presentationRecord.presentationParent() ? null : presentationRecord.record();
    if (typeof level !== "number")
        level = 0;
    var prefix = "";
    var suffix = "";
    for (var i = 0; i < level ; ++i)
        prefix = "----" + prefix;
    if (level > 0)
        prefix = prefix + "> ";
    if (presentationRecord.coalesced()) {
        suffix = " x " + presentationRecord.presentationChildren().length;
    } else if (record && (record.type() === WebInspector.TimelineModel.RecordType.TimeStamp
        || record.type() === WebInspector.TimelineModel.RecordType.ConsoleTime)) {
        suffix = " : " + record.data().message;
    }
    if (detailsCallback)
        suffix += " " + detailsCallback(presentationRecord);
    var typeString = record ? InspectorTest._timelineAgentTypeToString(record.type()) : "Root";
    InspectorTest.addResult(prefix + typeString + suffix);

    var numChildren = presentationRecord.presentationChildren() ? presentationRecord.presentationChildren().length : 0;
    for (var i = 0; i < numChildren; ++i) {
        if (filterTypes && filterTypes.indexOf(presentationRecord.presentationChildren()[i].record().type()) == -1)
            continue;
        InspectorTest.dumpPresentationRecord(presentationRecord.presentationChildren()[i], detailsCallback, level + 1, filterTypes);
    }
}

InspectorTest.dumpTimelineRecords = function(timelineRecords)
{
    for (var i = 0; i < timelineRecords.length; ++i)
        InspectorTest.dumpTimelineRecord(timelineRecords[i], 0);
};

InspectorTest.printTimelineRecordProperties = function(record)
{
    var recordType = (typeof record.type === "string") ? record.type : record.type();
    InspectorTest.addResult(InspectorTest._timelineAgentTypeToString(recordType) + " Properties:");
    // Use this recursive routine to print the properties
    if (record instanceof WebInspector.TimelineModel.RecordImpl)
        record = record._record;
    InspectorTest.addObject(record, InspectorTest.timelinePropertyFormatters);
};

InspectorTest._timelineAgentTypeToString = function(numericType)
{
    for (var prop in WebInspector.TimelineModel.RecordType) {
        if (WebInspector.TimelineModel.RecordType[prop] === numericType)
            return prop;
    }
    return undefined;
};

InspectorTest.findPresentationRecord = function(type)
{
    var result;
    function findByType(record)
    {
        if (record.type() !== type)
            return false;
        result = record;
        return true;
    }
    WebInspector.inspectorView.panel("timeline")._model.forAllRecords(findByType);
    return result;
}

InspectorTest.FakeFileReader = function(input, delegate, callback)
{
    this._delegate = delegate;
    this._callback = callback;
    this._input = input;
    this._loadedSize = 0;
    this._fileSize = input.length;
};

InspectorTest.dumpFrame = function(frame)
{
    var fieldsToDump = ["cpuTime", "duration", "startTime", "endTime", "id", "mainThreadFrameId", "timeByCategory", "other", "scripting", "painting", "rendering", "committedFrom"];
    function formatFields(object)
    {
        var result = {};
        for (var key in object) {
            if (fieldsToDump.indexOf(key) < 0)
                continue;
            var value = object[key];
            if (typeof value === "number")
                value = Number(value.toFixed(7));
            else if (typeof value === "object" && value)
                value = formatFields(value);
            result[key] = value;
        }
        return result;
    }
    InspectorTest.addObject(formatFields(frame));
}

InspectorTest.FakeFileReader.prototype = {
    start: function(output)
    {
        this._delegate.onTransferStarted(this);

        var length = this._input.length;
        var half = (length + 1) >> 1;

        var chunk = this._input.substring(0, half);
        this._loadedSize += chunk.length;
        output.write(chunk);
        this._delegate.onChunkTransferred(this);

        chunk = this._input.substring(half);
        this._loadedSize += chunk.length;
        output.write(chunk);
        this._delegate.onChunkTransferred(this);

        output.close();
        this._delegate.onTransferFinished(this);

        this._callback();
    },

    cancel: function() { },

    loadedSize: function()
    {
        return this._loadedSize;
    },

    fileSize: function()
    {
        return this._fileSize;
    },

    fileName: function()
    {
        return "fakeFile";
    }
};

};
