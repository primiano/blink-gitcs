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

#ifndef WorkerScriptController_h
#define WorkerScriptController_h

#if ENABLE(WORKERS)

#include <wtf/OwnPtr.h>
#include <wtf/Threading.h>

namespace WebCore {

    class ScriptSourceCode;
    class ScriptValue;
    class WorkerContext;
    class WorkerContextExecutionProxy;

    class WorkerScriptController {
    public:
        WorkerScriptController(WorkerContext*);
        ~WorkerScriptController();

        WorkerContextExecutionProxy* proxy() { return m_executionForbidden ? 0 : m_proxy.get(); }

        ScriptValue evaluate(const ScriptSourceCode&);
        ScriptValue evaluate(const ScriptSourceCode&, ScriptValue* exception);

        void setException(ScriptValue);

        enum ForbidExecutionOption { TerminateRunningScript, LetRunningScriptFinish };
        void forbidExecution(ForbidExecutionOption);

        // Returns WorkerScriptController for the currently executing context. 0 will be returned if the current executing context is not the worker context.
        static WorkerScriptController* controllerForContext();

    private:
        WorkerContext* m_workerContext;
        OwnPtr<WorkerContextExecutionProxy> m_proxy;

        Mutex m_sharedDataMutex;
        bool m_executionForbidden;
    };

} // namespace WebCore

#endif // ENABLE(WORKERS)

#endif // WorkerScriptController_h
