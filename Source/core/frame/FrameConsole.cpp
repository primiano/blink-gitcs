/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "core/frame/FrameConsole.h"

#include "bindings/core/v8/ScriptCallStackFactory.h"
#include "core/frame/FrameHost.h"
#include "core/inspector/ConsoleAPITypes.h"
#include "core/inspector/ConsoleMessage.h"
#include "core/inspector/ConsoleMessageStorage.h"
#include "core/inspector/InspectorConsoleInstrumentation.h"
#include "core/inspector/ScriptCallStack.h"
#include "core/page/Chrome.h"
#include "core/page/ChromeClient.h"
#include "core/page/Page.h"
#include "core/workers/WorkerGlobalScopeProxy.h"
#include "platform/network/ResourceResponse.h"
#include "wtf/text/StringBuilder.h"

namespace blink {

namespace {

int muteCount = 0;

}

FrameConsole::FrameConsole(LocalFrame& frame)
    : m_frame(frame)
{
}

DEFINE_EMPTY_DESTRUCTOR_WILL_BE_REMOVED(FrameConsole);

void FrameConsole::addMessage(PassRefPtrWillBeRawPtr<ConsoleMessage> prpConsoleMessage)
{
    RefPtrWillBeRawPtr<ConsoleMessage> consoleMessage = prpConsoleMessage;
    if (muteCount && consoleMessage->source() != ConsoleAPIMessageSource)
        return;

    // FIXME: This should not need to reach for the main-frame.
    // Inspector code should just take the current frame and know how to walk itself.
    ExecutionContext* context = m_frame.document();
    if (!context)
        return;

    String messageURL;
    unsigned lineNumber = 0;
    if (consoleMessage->callStack() && consoleMessage->callStack()->size()) {
        lineNumber = consoleMessage->callStack()->at(0).lineNumber();
        messageURL = consoleMessage->callStack()->at(0).sourceURL();
    } else {
        lineNumber = consoleMessage->lineNumber();
        messageURL = consoleMessage->url();
    }

    messageStorage()->reportMessage(consoleMessage);

    if (consoleMessage->source() == CSSMessageSource || consoleMessage->source() == NetworkMessageSource)
        return;

    RefPtrWillBeRawPtr<ScriptCallStack> reportedCallStack = nullptr;
    if (consoleMessage->source() != ConsoleAPIMessageSource) {
        if (consoleMessage->callStack() && m_frame.chromeClient().shouldReportDetailedMessageForSource(messageURL))
            reportedCallStack = consoleMessage->callStack();
    } else {
        if (!m_frame.host() || (consoleMessage->scriptArguments() && consoleMessage->scriptArguments()->argumentCount() == 0))
            return;

        MessageType type = consoleMessage->type();
        if (type == StartGroupMessageType || type == EndGroupMessageType || type == StartGroupCollapsedMessageType)
            return;

        if (m_frame.chromeClient().shouldReportDetailedMessageForSource(messageURL))
            reportedCallStack = createScriptCallStack(ScriptCallStack::maxCallStackSizeToCapture);
    }

    String stackTrace;
    if (reportedCallStack)
        stackTrace = FrameConsole::formatStackTraceString(consoleMessage->message(), reportedCallStack);
    m_frame.chromeClient().addMessageToConsole(&m_frame, consoleMessage->source(), consoleMessage->level(), consoleMessage->message(), lineNumber, messageURL, stackTrace);
}

void FrameConsole::reportResourceResponseReceived(DocumentLoader* loader, unsigned long requestIdentifier, const ResourceResponse& response)
{
    if (!loader)
        return;
    if (response.httpStatusCode() < 400)
        return;
    String message = "Failed to load resource: the server responded with a status of " + String::number(response.httpStatusCode()) + " (" + response.httpStatusText() + ')';
    RefPtrWillBeRawPtr<ConsoleMessage> consoleMessage = ConsoleMessage::create(NetworkMessageSource, ErrorMessageLevel, message, response.url().string());
    consoleMessage->setRequestIdentifier(requestIdentifier);
    addMessage(consoleMessage.release());
}

String FrameConsole::formatStackTraceString(const String& originalMessage, PassRefPtrWillBeRawPtr<ScriptCallStack> callStack)
{
    StringBuilder stackTrace;
    for (size_t i = 0; i < callStack->size(); ++i) {
        const ScriptCallFrame& frame = callStack->at(i);
        stackTrace.append("\n    at " + (frame.functionName().length() ? frame.functionName() : "(anonymous function)"));
        stackTrace.appendLiteral(" (");
        stackTrace.append(frame.sourceURL());
        stackTrace.append(':');
        stackTrace.append(String::number(frame.lineNumber()));
        stackTrace.append(':');
        stackTrace.append(String::number(frame.columnNumber()));
        stackTrace.append(')');
    }

    return stackTrace.toString();
}

void FrameConsole::mute()
{
    muteCount++;
}

void FrameConsole::unmute()
{
    ASSERT(muteCount > 0);
    muteCount--;
}

ConsoleMessageStorage* FrameConsole::messageStorage()
{
    LocalFrame* curFrame = &m_frame;
    Frame* topFrame = curFrame->tree().top();
    ASSERT(topFrame->isLocalFrame());
    LocalFrame* localTopFrame = toLocalFrame(topFrame);
    if (localTopFrame != curFrame)
        return localTopFrame->console().messageStorage();
    if (!m_consoleMessageStorage)
        m_consoleMessageStorage = ConsoleMessageStorage::createForFrame(&m_frame);
    return m_consoleMessageStorage.get();
}

void FrameConsole::adoptWorkerMessagesAfterTermination(WorkerGlobalScopeProxy* proxy)
{
    ConsoleMessageStorage* storage = messageStorage();
    size_t messageCount = storage->size();
    for (size_t i = 0; i < messageCount; ++i) {
        if (storage->at(i)->workerGlobalScopeProxy() == proxy)
            storage->at(i)->setWorkerGlobalScopeProxy(nullptr);
    }
}

void FrameConsole::trace(Visitor* visitor)
{
    visitor->trace(m_consoleMessageStorage);
}

} // namespace blink
