function initialize_TracingTest()
{

// FIXME: remove when tracing is out of experimental
WebInspector.inspectorView.showPanel("timeline");
InspectorTest.tracingModel = new WebInspector.TracingModel();
InspectorTest.tracingTimelineModel = new WebInspector.TracingTimelineModel(InspectorTest.tracingModel);

InspectorTest.invokeWithTracing = function(categoryFilter, functionName, callback)
{
    InspectorTest.tracingTimelineModel.addEventListener(WebInspector.TimelineModel.Events.RecordingStarted, onTracingStarted, this);
    InspectorTest.tracingTimelineModel._startRecordingWithCategories(categoryFilter);

    function onTracingStarted(event)
    {
        InspectorTest.tracingTimelineModel.removeEventListener(WebInspector.TimelineModel.Events.RecordingStarted, onTracingStarted, this);
        InspectorTest.invokePageFunctionAsync(functionName, onPageActionsDone);
    }

    function onPageActionsDone()
    {
        InspectorTest.tracingTimelineModel.addEventListener(WebInspector.TimelineModel.Events.RecordingStopped, onTracingComplete, this);
        InspectorTest.tracingTimelineModel.stopRecording();
    }

    function onTracingComplete(event)
    {
        InspectorTest.tracingTimelineModel.removeEventListener(WebInspector.TimelineModel.Events.RecordingStopped, onTracingComplete, this);
        callback();
    }
}

}
