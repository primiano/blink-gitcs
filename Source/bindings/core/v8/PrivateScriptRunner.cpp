// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"
#include "bindings/core/v8/PrivateScriptRunner.h"

#include "bindings/core/v8/DOMWrapperWorld.h"
#include "bindings/core/v8/ExceptionState.h"
#include "bindings/core/v8/V8Binding.h"
#include "bindings/core/v8/V8PerContextData.h"
#include "bindings/core/v8/V8ScriptRunner.h"
#include "core/PrivateScriptSources.h"
#ifndef NDEBUG
#include "core/PrivateScriptSourcesForTesting.h"
#endif
#include "core/dom/ExceptionCode.h"

namespace blink {

static void dumpV8Message(v8::Handle<v8::Message> message)
{
    if (message.IsEmpty())
        return;

    // FIXME: GetScriptOrigin() and GetLineNumber() return empty handles
    // when they are called at the first time if V8 has a pending exception.
    // So we need to call twice to get a correct ScriptOrigin and line number.
    // This is a bug of V8.
    message->GetScriptOrigin();
    message->GetLineNumber();

    v8::Handle<v8::Value> resourceName = message->GetScriptOrigin().ResourceName();
    String fileName = "Unknown JavaScript file";
    if (!resourceName.IsEmpty() && resourceName->IsString())
        fileName = toCoreString(v8::Handle<v8::String>::Cast(resourceName));
    int lineNumber = message->GetLineNumber();
    v8::Handle<v8::String> errorMessage = message->Get();
    fprintf(stderr, "%s (line %d): %s\n", fileName.utf8().data(), lineNumber, toCoreString(errorMessage).utf8().data());
}

static void dumpJSError(String exceptionName, v8::Handle<v8::Message> message)
{
    // FIXME: Set a ScriptOrigin of the private script and print a more informative message.
#ifndef NDEBUG
    fprintf(stderr, "Private script throws an exception: %s\n", exceptionName.utf8().data());
    dumpV8Message(message);
#endif
}

static v8::Handle<v8::Value> compileAndRunPrivateScript(v8::Isolate* isolate, String scriptClassName, const unsigned char* source, size_t size)
{
    v8::TryCatch block;
    String sourceString(reinterpret_cast<const char*>(source), size);
    String fileName = scriptClassName + ".js";
    v8::Handle<v8::Script> script = V8ScriptRunner::compileScript(v8String(isolate, sourceString), fileName, TextPosition::minimumPosition(), 0, isolate, NotSharableCrossOrigin, V8CacheOptionsOff);
    if (block.HasCaught()) {
        fprintf(stderr, "Private script error: Compile failed. (Class name = %s)\n", scriptClassName.utf8().data());
        dumpV8Message(block.Message());
        RELEASE_ASSERT_NOT_REACHED();
    }

    v8::Handle<v8::Value> result = V8ScriptRunner::runCompiledInternalScript(script, isolate);
    if (block.HasCaught()) {
        fprintf(stderr, "Private script error: installClass() failed. (Class name = %s)\n", scriptClassName.utf8().data());
        dumpV8Message(block.Message());
        RELEASE_ASSERT_NOT_REACHED();
    }
    return result;
}

// FIXME: If we have X.js, XPartial-1.js and XPartial-2.js, currently all of the JS files
// are compiled when any of the JS files is requested. Ideally we should avoid compiling
// unrelated JS files. For example, if a method in XPartial-1.js is requested, we just
// need to compile X.js and XPartial-1.js, and don't need to compile XPartial-2.js.
static void installPrivateScript(v8::Isolate* isolate, String className)
{
    int compiledScriptCount = 0;
    // |kPrivateScriptSourcesForTesting| is defined in V8PrivateScriptSources.h, which is auto-generated
    // by make_private_script_source.py.
#ifndef NDEBUG
    for (size_t index = 0; index < WTF_ARRAY_LENGTH(kPrivateScriptSourcesForTesting); index++) {
        if (className == kPrivateScriptSourcesForTesting[index].className) {
            compileAndRunPrivateScript(isolate, kPrivateScriptSourcesForTesting[index].scriptClassName, kPrivateScriptSourcesForTesting[index].source, kPrivateScriptSourcesForTesting[index].size);
            compiledScriptCount++;
        }
    }
#endif

    // |kPrivateScriptSources| is defined in V8PrivateScriptSources.h, which is auto-generated
    // by make_private_script_source.py.
    for (size_t index = 0; index < WTF_ARRAY_LENGTH(kPrivateScriptSources); index++) {
        if (className == kPrivateScriptSources[index].className) {
            compileAndRunPrivateScript(isolate, kPrivateScriptSources[index].scriptClassName, kPrivateScriptSources[index].source, kPrivateScriptSources[index].size);
            compiledScriptCount++;
        }
    }

    if (!compiledScriptCount) {
        fprintf(stderr, "Private script error: Target source code was not found. (Class name = %s)\n", className.utf8().data());
        RELEASE_ASSERT_NOT_REACHED();
    }
}

static v8::Handle<v8::Value> installPrivateScriptRunner(v8::Isolate* isolate)
{
    const String className = "PrivateScriptRunner";
    size_t index;
    // |kPrivateScriptSources| is defined in V8PrivateScriptSources.h, which is auto-generated
    // by make_private_script_source.py.
    for (index = 0; index < WTF_ARRAY_LENGTH(kPrivateScriptSources); index++) {
        if (className == kPrivateScriptSources[index].className)
            break;
    }
    if (index == WTF_ARRAY_LENGTH(kPrivateScriptSources)) {
        fprintf(stderr, "Private script error: Target source code was not found. (Class name = %s)\n", className.utf8().data());
        RELEASE_ASSERT_NOT_REACHED();
    }
    return compileAndRunPrivateScript(isolate, className, kPrivateScriptSources[index].source, kPrivateScriptSources[index].size);
}

static v8::Handle<v8::Object> classObjectOfPrivateScript(ScriptState* scriptState, String className)
{
    ASSERT(scriptState->perContextData());
    ASSERT(scriptState->executionContext());
    v8::Isolate* isolate = scriptState->isolate();
    v8::Handle<v8::Value> compiledClass = scriptState->perContextData()->compiledPrivateScript(className);
    if (compiledClass.IsEmpty()) {
        v8::Handle<v8::Value> installedClasses = scriptState->perContextData()->compiledPrivateScript("PrivateScriptRunner");
        if (installedClasses.IsEmpty()) {
            installedClasses = installPrivateScriptRunner(isolate);
            scriptState->perContextData()->setCompiledPrivateScript("PrivateScriptRunner", installedClasses);
        }
        RELEASE_ASSERT(!installedClasses.IsEmpty());
        RELEASE_ASSERT(installedClasses->IsObject());

        installPrivateScript(isolate, className);
        compiledClass = v8::Handle<v8::Object>::Cast(installedClasses)->Get(v8String(isolate, className));
        RELEASE_ASSERT(!compiledClass.IsEmpty());
        RELEASE_ASSERT(compiledClass->IsObject());
        scriptState->perContextData()->setCompiledPrivateScript(className, compiledClass);
    }
    return v8::Handle<v8::Object>::Cast(compiledClass);
}

static void initializeHolderIfNeeded(ScriptState* scriptState, v8::Handle<v8::Object> classObject, v8::Handle<v8::Value> holder)
{
    RELEASE_ASSERT(!holder.IsEmpty());
    RELEASE_ASSERT(holder->IsObject());
    v8::Handle<v8::Object> holderObject = v8::Handle<v8::Object>::Cast(holder);
    v8::Isolate* isolate = scriptState->isolate();
    v8::Handle<v8::Value> isInitialized = V8HiddenValue::getHiddenValue(isolate, holderObject, V8HiddenValue::privateScriptObjectIsInitialized(isolate));
    if (isInitialized.IsEmpty()) {
        v8::TryCatch block;
        v8::Handle<v8::Value> initializeFunction = classObject->Get(v8String(isolate, "initialize"));
        if (!initializeFunction.IsEmpty() && initializeFunction->IsFunction()) {
            v8::TryCatch block;
            V8ScriptRunner::callFunction(v8::Handle<v8::Function>::Cast(initializeFunction), scriptState->executionContext(), holder, 0, 0, isolate);
            if (block.HasCaught()) {
                fprintf(stderr, "Private script error: Object constructor threw an exception.\n");
                dumpV8Message(block.Message());
                RELEASE_ASSERT_NOT_REACHED();
            }
        }

        // Inject the prototype object of the private script into the prototype chain of the holder object.
        // This is necessary to let the holder object use properties defined on the prototype object
        // of the private script. (e.g., if the prototype object has |foo|, the holder object should be able
        // to use it with |this.foo|.)
        if (classObject->GetPrototype() != holderObject->GetPrototype())
            classObject->SetPrototype(holderObject->GetPrototype());
        holderObject->SetPrototype(classObject);

        isInitialized = v8Boolean(true, isolate);
        V8HiddenValue::setHiddenValue(isolate, holderObject, V8HiddenValue::privateScriptObjectIsInitialized(isolate), isInitialized);
    }
}

v8::Handle<v8::Value> PrivateScriptRunner::installClassIfNeeded(LocalFrame* frame, String className)
{
    if (!frame)
        return v8::Handle<v8::Value>();
    v8::HandleScope handleScope(toIsolate(frame));
    v8::Handle<v8::Context> context = toV8Context(frame, DOMWrapperWorld::privateScriptIsolatedWorld());
    if (context.IsEmpty())
        return v8::Handle<v8::Value>();
    ScriptState* scriptState = ScriptState::from(context);
    if (!scriptState->executionContext())
        return v8::Handle<v8::Value>();

    ScriptState::Scope scope(scriptState);
    return classObjectOfPrivateScript(scriptState, className);
}

v8::Handle<v8::Value> PrivateScriptRunner::runDOMAttributeGetter(ScriptState* scriptState, String className, String attributeName, v8::Handle<v8::Value> holder)
{
    v8::Handle<v8::Object> classObject = classObjectOfPrivateScript(scriptState, className);
    v8::Handle<v8::Value> descriptor = classObject->GetOwnPropertyDescriptor(v8String(scriptState->isolate(), attributeName));
    if (descriptor.IsEmpty() || !descriptor->IsObject()) {
        fprintf(stderr, "Private script error: Target DOM attribute getter was not found. (Class name = %s, Attribute name = %s)\n", className.utf8().data(), attributeName.utf8().data());
        RELEASE_ASSERT_NOT_REACHED();
    }
    v8::Handle<v8::Value> getter = v8::Handle<v8::Object>::Cast(descriptor)->Get(v8String(scriptState->isolate(), "get"));
    if (getter.IsEmpty() || !getter->IsFunction()) {
        fprintf(stderr, "Private script error: Target DOM attribute getter was not found. (Class name = %s, Attribute name = %s)\n", className.utf8().data(), attributeName.utf8().data());
        RELEASE_ASSERT_NOT_REACHED();
    }
    initializeHolderIfNeeded(scriptState, classObject, holder);
    return V8ScriptRunner::callFunction(v8::Handle<v8::Function>::Cast(getter), scriptState->executionContext(), holder, 0, 0, scriptState->isolate());
}

void PrivateScriptRunner::runDOMAttributeSetter(ScriptState* scriptState, String className, String attributeName, v8::Handle<v8::Value> holder, v8::Handle<v8::Value> v8Value)
{
    v8::Handle<v8::Object> classObject = classObjectOfPrivateScript(scriptState, className);
    v8::Handle<v8::Value> descriptor = classObject->GetOwnPropertyDescriptor(v8String(scriptState->isolate(), attributeName));
    if (descriptor.IsEmpty() || !descriptor->IsObject()) {
        fprintf(stderr, "Private script error: Target DOM attribute setter was not found. (Class name = %s, Attribute name = %s)\n", className.utf8().data(), attributeName.utf8().data());
        RELEASE_ASSERT_NOT_REACHED();
    }
    v8::Handle<v8::Value> setter = v8::Handle<v8::Object>::Cast(descriptor)->Get(v8String(scriptState->isolate(), "set"));
    if (setter.IsEmpty() || !setter->IsFunction()) {
        fprintf(stderr, "Private script error: Target DOM attribute setter was not found. (Class name = %s, Attribute name = %s)\n", className.utf8().data(), attributeName.utf8().data());
        RELEASE_ASSERT_NOT_REACHED();
    }
    initializeHolderIfNeeded(scriptState, classObject, holder);
    v8::Handle<v8::Value> argv[] = { v8Value };
    V8ScriptRunner::callFunction(v8::Handle<v8::Function>::Cast(setter), scriptState->executionContext(), holder, WTF_ARRAY_LENGTH(argv), argv, scriptState->isolate());
}

v8::Handle<v8::Value> PrivateScriptRunner::runDOMMethod(ScriptState* scriptState, String className, String methodName, v8::Handle<v8::Value> holder, int argc, v8::Handle<v8::Value> argv[])
{
    v8::Handle<v8::Object> classObject = classObjectOfPrivateScript(scriptState, className);
    v8::Handle<v8::Value> method = classObject->Get(v8String(scriptState->isolate(), methodName));
    if (method.IsEmpty() || !method->IsFunction()) {
        fprintf(stderr, "Private script error: Target DOM method was not found. (Class name = %s, Method name = %s)\n", className.utf8().data(), methodName.utf8().data());
        RELEASE_ASSERT_NOT_REACHED();
    }
    initializeHolderIfNeeded(scriptState, classObject, holder);
    return V8ScriptRunner::callFunction(v8::Handle<v8::Function>::Cast(method), scriptState->executionContext(), holder, argc, argv, scriptState->isolate());
}

bool PrivateScriptRunner::rethrowExceptionInPrivateScript(v8::Isolate* isolate, ExceptionState& exceptionState, v8::TryCatch& block)
{
    v8::Handle<v8::Value> exception = block.Exception();
    if (exception.IsEmpty() || !exception->IsObject())
        return false;

    v8::Handle<v8::Object> exceptionObject = v8::Handle<v8::Object>::Cast(exception);
    v8::Handle<v8::Value> name = exceptionObject->Get(v8String(isolate, "name"));
    if (name.IsEmpty() || !name->IsString())
        return false;

    v8::Handle<v8::Message> tryCatchMessage = block.Message();
    v8::Handle<v8::Value> message = exceptionObject->Get(v8String(isolate, "message"));
    String messageString;
    if (!message.IsEmpty() && message->IsString())
        messageString = toCoreString(v8::Handle<v8::String>::Cast(message));

    String exceptionName = toCoreString(v8::Handle<v8::String>::Cast(name));
    if (exceptionName == "DOMExceptionInPrivateScript") {
        v8::Handle<v8::Value> code = exceptionObject->Get(v8String(isolate, "code"));
        RELEASE_ASSERT(!code.IsEmpty() && code->IsInt32());
        exceptionState.throwDOMException(toInt32(code), messageString);
        exceptionState.throwIfNeeded();
        return true;
    }
    if (exceptionName == "Error") {
        exceptionState.throwDOMException(V8GeneralError, messageString);
        exceptionState.throwIfNeeded();
        dumpJSError(exceptionName, tryCatchMessage);
        return true;
    }
    if (exceptionName == "TypeError") {
        exceptionState.throwDOMException(V8TypeError, messageString);
        exceptionState.throwIfNeeded();
        dumpJSError(exceptionName, tryCatchMessage);
        return true;
    }
    if (exceptionName == "RangeError") {
        exceptionState.throwDOMException(V8RangeError, messageString);
        exceptionState.throwIfNeeded();
        dumpJSError(exceptionName, tryCatchMessage);
        return true;
    }
    if (exceptionName == "SyntaxError") {
        exceptionState.throwDOMException(V8SyntaxError, messageString);
        exceptionState.throwIfNeeded();
        dumpJSError(exceptionName, tryCatchMessage);
        return true;
    }
    if (exceptionName == "ReferenceError") {
        exceptionState.throwDOMException(V8ReferenceError, messageString);
        exceptionState.throwIfNeeded();
        dumpJSError(exceptionName, tryCatchMessage);
        return true;
    }
    return false;
}

} // namespace blink
