/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#ifndef AsyncCallStackTracker_h
#define AsyncCallStackTracker_h

#include "platform/heap/Handle.h"
#include "wtf/Forward.h"
#include "wtf/HashMap.h"
#include "wtf/Noncopyable.h"

namespace blink {

class AsyncCallChain;
class Event;
class EventListener;
class EventTarget;
class ExecutionContext;
class ExecutionContextTask;
class FormData;
class HTTPHeaderMap;
class InspectorDebuggerAgent;
class KURL;
class MutationObserver;
class ScriptValue;
class ThreadableLoaderClient;
class XMLHttpRequest;

class AsyncCallStackTracker final : public NoBaseWillBeGarbageCollected<AsyncCallStackTracker> {
    WTF_MAKE_NONCOPYABLE(AsyncCallStackTracker);
    DECLARE_EMPTY_DESTRUCTOR_WILL_BE_REMOVED(AsyncCallStackTracker);
public:
    explicit AsyncCallStackTracker(InspectorDebuggerAgent*);

    void didInstallTimer(ExecutionContext*, int timerId, int timeout, bool singleShot);
    void didRemoveTimer(ExecutionContext*, int timerId);
    bool willFireTimer(ExecutionContext*, int timerId);
    void didFireTimer() { didFireAsyncCall(); };

    void didRequestAnimationFrame(ExecutionContext*, int callbackId);
    void didCancelAnimationFrame(ExecutionContext*, int callbackId);
    bool willFireAnimationFrame(ExecutionContext*, int callbackId);
    void didFireAnimationFrame() { didFireAsyncCall(); };

    void didEnqueueEvent(EventTarget*, Event*);
    void didRemoveEvent(EventTarget*, Event*);
    void willHandleEvent(EventTarget*, Event*, EventListener*, bool useCapture);
    void didHandleEvent() { didFireAsyncCall(); };

    void willLoadXHR(XMLHttpRequest*, ThreadableLoaderClient*, const AtomicString& method, const KURL&, bool async, PassRefPtr<FormData> body, const HTTPHeaderMap& headers, bool includeCrendentials);
    void didDispatchXHRLoadendEvent(XMLHttpRequest*);

    void didEnqueueMutationRecord(ExecutionContext*, MutationObserver*);
    void didClearAllMutationRecords(ExecutionContext*, MutationObserver*);
    void willDeliverMutationRecords(ExecutionContext*, MutationObserver*);
    void didDeliverMutationRecords() { didFireAsyncCall(); };

    void didPostExecutionContextTask(ExecutionContext*, ExecutionContextTask*);
    void didKillAllExecutionContextTasks(ExecutionContext*);
    void willPerformExecutionContextTask(ExecutionContext*, ExecutionContextTask*);
    void didPerformExecutionContextTask() { didFireAsyncCall(); };

    void didEnqueueV8AsyncTask(ExecutionContext*, const String& eventName, int id, const ScriptValue& callFrames);
    void willHandleV8AsyncTask(ExecutionContext*, const String& eventName, int id);

    int traceAsyncOperationStarting(ExecutionContext*, const String& operationName, int prevOperationId = 0);
    void traceAsyncOperationCompleted(ExecutionContext*, int operationId);
    void traceAsyncOperationCompletedCallbackStarting(ExecutionContext*, int operationId);
    void traceAsyncCallbackStarting(ExecutionContext*, int operationId);
    void traceAsyncCallbackCompleted() { didFireAsyncCall(); };

    void didFireAsyncCall();
    void reset();

    void trace(Visitor*);

    class ExecutionContextData;

private:
    template <class K> class AsyncCallChainMap;
    void willHandleXHREvent(XMLHttpRequest*, Event*);

    void setCurrentAsyncCallChain(ExecutionContext*, PassRefPtrWillBeRawPtr<AsyncCallChain>);
    bool validateCallFrames(const ScriptValue& callFrames);

    ExecutionContextData* createContextDataIfNeeded(ExecutionContext*);

    using ExecutionContextDataMap = WillBeHeapHashMap<RawPtrWillBeMember<ExecutionContext>, OwnPtrWillBeMember<ExecutionContextData>>;
    ExecutionContextDataMap m_executionContextDataMap;
    InspectorDebuggerAgent* m_debuggerAgent;
};

} // namespace blink

#endif // !defined(AsyncCallStackTracker_h)
