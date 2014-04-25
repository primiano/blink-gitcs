function initialize_EditorTests()
{

InspectorTest.createTestEditor = function(clientHeight, textEditorDelegate)
{
    loadScript("source_frame/SourceFrame.js");
    var textEditor = new WebInspector.CodeMirrorTextEditor("", textEditorDelegate || new WebInspector.TextEditorDelegate());
    if (clientHeight)
        textEditor.element.style.height = clientHeight + "px";
    textEditor.element.style.flex = "none";
    textEditor.show(WebInspector.inspectorView.element);
    return textEditor;
};

InspectorTest.textWithSelection = function(text, selection)
{
    if (!selection)
        return text;

    function lineWithCursor(line, column, cursorChar)
    {
        return line.substring(0, column) + cursorChar + line.substring(column);
    }

    var lines = text.split("\n");
    selection = selection.normalize();
    var endCursorChar = selection.isEmpty() ? "|" : "<";
    lines[selection.endLine] = lineWithCursor(lines[selection.endLine], selection.endColumn, endCursorChar);
    if (!selection.isEmpty()) {
        lines[selection.startLine] = lineWithCursor(lines[selection.startLine], selection.startColumn, ">");
    }
    return lines.join("\n");
}

InspectorTest.dumpTextWithSelection = function(textEditor, dumpWhiteSpaces)
{
    var text = InspectorTest.textWithSelection(textEditor.text(), textEditor.selection());
    if (dumpWhiteSpaces)
        text = text.replace(/ /g, ".");
    InspectorTest.addResult(text);
}

InspectorTest.typeIn = function(editor, typeText, callback)
{
    var noop = new Function();
    for(var charIndex = 0; charIndex < typeText.length; ++charIndex) {
        // As soon as the last key event was processed, the whole text was processed.
        var iterationCallback = charIndex + 1 === typeText.length ? callback : noop;
        switch (typeText[charIndex]) {
        case "\n":
            InspectorTest.fakeKeyEvent(editor, "enter", null, iterationCallback);
            break;
        case "L":
            InspectorTest.fakeKeyEvent(editor, "leftArrow", null, iterationCallback);
            break;
        case "R":
            InspectorTest.fakeKeyEvent(editor, "rightArrow", null, iterationCallback);
            break;
        case "U":
            InspectorTest.fakeKeyEvent(editor, "upArrow", null, iterationCallback);
            break;
        case "D":
            InspectorTest.fakeKeyEvent(editor, "downArrow", null, iterationCallback);
            break;
        default:
            InspectorTest.fakeKeyEvent(editor, typeText[charIndex], null, iterationCallback);
        }
    }
}

var eventCodes = {
    enter: 13,
    home: 36,
    leftArrow: 37,
    upArrow: 38,
    rightArrow: 39,
    downArrow: 40
};

function createCodeMirrorFakeEvent(eventType, code, charCode, modifiers)
{
    function eventPreventDefault()
    {
        this._handled = true;
    }
    var event = {
        _handled: false,
        type: eventType,
        keyCode: code,
        charCode: charCode,
        preventDefault: eventPreventDefault,
        stopPropagation: function(){},
    };
    if (modifiers) {
        for (var i = 0; i < modifiers.length; ++i)
            event[modifiers[i]] = true;
    }
    return event;
}

function fakeCodeMirrorKeyEvent(editor, eventType, code, charCode, modifiers)
{
    var event = createCodeMirrorFakeEvent(eventType, code, charCode, modifiers);
    switch(eventType) {
    case "keydown":
        editor._codeMirror.triggerOnKeyDown(event);
        break;
    case "keypress":
        editor._codeMirror.triggerOnKeyPress(event);
        break;
    case "keyup":
        editor._codeMirror.triggerOnKeyUp(event);
        break;
    default:
        throw new Error("Unknown KeyEvent type");
    }
    return event._handled;
}

function fakeCodeMirrorInputEvent(editor, character)
{
    if (typeof character === "string")
        editor._codeMirror.display.input.value += character;
}

InspectorTest.fakeKeyEvent = function(editor, originalCode, modifiers, callback)
{
    modifiers = modifiers || [];
    var code;
    var charCode;
    if (originalCode === "(") {
        code = "9".charCodeAt(0);
        modifiers.push("shiftKey");
        charCode = originalCode.charCodeAt(0);
    }
    var code = code || eventCodes[originalCode] || originalCode;
    if (typeof code === "string")
        code = code.charCodeAt(0);
    if (fakeCodeMirrorKeyEvent(editor, "keydown", code, charCode, modifiers)) {
        callback();
        return;
    }
    if (fakeCodeMirrorKeyEvent(editor, "keypress", code, charCode, modifiers)) {
        callback();
        return;
    }
    fakeCodeMirrorInputEvent(editor, originalCode);
    fakeCodeMirrorKeyEvent(editor, "keyup", code, charCode, modifiers);

    function callbackWrapper()
    {
        editor._codeMirror.off("inputRead", callbackWrapper);
        callback();
    }
    editor._codeMirror.on("inputRead", callbackWrapper);
}

}
