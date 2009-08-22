/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Matt Lilek <webkit@mattlilek.com>
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
#include "InspectorBackend.h"

#if ENABLE(DATABASE)
#include "Database.h"
#endif

#include "Element.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "HTMLFrameOwnerElement.h"
#include "InspectorClient.h"
#include "InspectorController.h"
#include "InspectorDOMAgent.h"
#include "InspectorFrontend.h"
#include "InspectorResource.h"

#if ENABLE(DOM_STORAGE)
#include "Storage.h"
#endif

#if ENABLE(JAVASCRIPT_DEBUGGER)
#include "JavaScriptCallFrame.h"
#include "JavaScriptDebugServer.h"
using namespace JSC;
#endif

#include <wtf/RefPtr.h>
#include <wtf/StdLibExtras.h>

using namespace std;

namespace WebCore {

InspectorBackend::InspectorBackend(InspectorController* inspectorController, InspectorClient* client)
    : m_inspectorController(inspectorController)
    , m_client(client)
{
}

InspectorBackend::~InspectorBackend()
{
}

void InspectorBackend::hideDOMNodeHighlight()
{
    if (m_inspectorController)
        m_inspectorController->hideHighlight();
}

String InspectorBackend::localizedStringsURL()
{
    return m_client->localizedStringsURL();
}

String InspectorBackend::hiddenPanels()
{
    return m_client->hiddenPanels();
}

void InspectorBackend::windowUnloading()
{
    if (m_inspectorController)
        m_inspectorController->close();
}

bool InspectorBackend::isWindowVisible()
{
    if (m_inspectorController)
        return m_inspectorController->windowVisible();
    return false;
}

void InspectorBackend::addResourceSourceToFrame(long identifier, Node* frame)
{
    if (!m_inspectorController)
        return;
    RefPtr<InspectorResource> resource = m_inspectorController->resources().get(identifier);
    if (resource) {
        String sourceString = resource->sourceString();
        if (!sourceString.isEmpty())
            addSourceToFrame(resource->mimeType(), sourceString, frame);
    }
}

bool InspectorBackend::addSourceToFrame(const String& mimeType, const String& source, Node* frameNode)
{
    ASSERT_ARG(frameNode, frameNode);

    if (!frameNode)
        return false;

    if (!frameNode->attached()) {
        ASSERT_NOT_REACHED();
        return false;
    }

    ASSERT(frameNode->isElementNode());
    if (!frameNode->isElementNode())
        return false;

    Element* element = static_cast<Element*>(frameNode);
    ASSERT(element->isFrameOwnerElement());
    if (!element->isFrameOwnerElement())
        return false;

    HTMLFrameOwnerElement* frameOwner = static_cast<HTMLFrameOwnerElement*>(element);
    ASSERT(frameOwner->contentFrame());
    if (!frameOwner->contentFrame())
        return false;

    FrameLoader* loader = frameOwner->contentFrame()->loader();

    loader->setResponseMIMEType(mimeType);
    loader->begin();
    loader->write(source);
    loader->end();

    return true;
}

void InspectorBackend::clearMessages(bool clearUI)
{
    if (m_inspectorController)
        m_inspectorController->clearConsoleMessages(clearUI);
}

void InspectorBackend::toggleNodeSearch()
{
    if (m_inspectorController)
        m_inspectorController->toggleSearchForNodeInPage();
}

void InspectorBackend::attach()
{
    if (m_inspectorController)
        m_inspectorController->attachWindow();
}

void InspectorBackend::detach()
{
    if (m_inspectorController)
        m_inspectorController->detachWindow();
}

void InspectorBackend::setAttachedWindowHeight(unsigned height)
{
    if (m_inspectorController)
        m_inspectorController->setAttachedWindowHeight(height);
}

void InspectorBackend::storeLastActivePanel(const String& panelName)
{
    if (m_inspectorController)
        m_inspectorController->storeLastActivePanel(panelName);
}

bool InspectorBackend::searchingForNode()
{
    if (m_inspectorController)
        return m_inspectorController->searchingForNodeInPage();
    return false;
}

void InspectorBackend::loaded()
{
    if (m_inspectorController)
        m_inspectorController->scriptObjectReady();
}

void InspectorBackend::enableResourceTracking(bool always)
{
    if (m_inspectorController)
        m_inspectorController->enableResourceTracking(always);
}

void InspectorBackend::disableResourceTracking(bool always)
{
    if (m_inspectorController)
        m_inspectorController->disableResourceTracking(always);
}

bool InspectorBackend::resourceTrackingEnabled() const
{
    if (m_inspectorController)
        return m_inspectorController->resourceTrackingEnabled();
    return false;
}

void InspectorBackend::moveWindowBy(float x, float y) const
{
    if (m_inspectorController)
        m_inspectorController->moveWindowBy(x, y);
}

void InspectorBackend::closeWindow()
{
    if (m_inspectorController)
        m_inspectorController->closeWindow();
}

const String& InspectorBackend::platform() const
{
#if PLATFORM(MAC)
#ifdef BUILDING_ON_TIGER
    DEFINE_STATIC_LOCAL(const String, platform, ("mac-tiger"));
#else
    DEFINE_STATIC_LOCAL(const String, platform, ("mac-leopard"));
#endif
#elif PLATFORM(WIN_OS)
    DEFINE_STATIC_LOCAL(const String, platform, ("windows"));
#elif PLATFORM(QT)
    DEFINE_STATIC_LOCAL(const String, platform, ("qt"));
#elif PLATFORM(GTK)
    DEFINE_STATIC_LOCAL(const String, platform, ("gtk"));
#elif PLATFORM(WX)
    DEFINE_STATIC_LOCAL(const String, platform, ("wx"));
#else
    DEFINE_STATIC_LOCAL(const String, platform, ("unknown"));
#endif

    return platform;
}

void InspectorBackend::getCookies(long callId)
{
    if (!m_inspectorController)
        return;
    if (!m_inspectorController->m_domAgent || !m_inspectorController->m_frontend)
        return;
    m_inspectorController->domAgent()->getCookies(callId);
}

void InspectorBackend::deleteCookie(const String& cookieName)
{
    if (!m_inspectorController)
        return;
    m_inspectorController->deleteCookie(cookieName);
}

#if ENABLE(JAVASCRIPT_DEBUGGER)
const ProfilesArray& InspectorBackend::profiles() const
{    
    if (m_inspectorController)
        return m_inspectorController->profiles();
    return m_emptyProfiles;
}

void InspectorBackend::startProfiling()
{
    if (m_inspectorController)
        m_inspectorController->startUserInitiatedProfiling();
}

void InspectorBackend::stopProfiling()
{
    if (m_inspectorController)
        m_inspectorController->stopUserInitiatedProfiling();
}

void InspectorBackend::enableProfiler(bool always)
{
    if (m_inspectorController)
        m_inspectorController->enableProfiler(always);
}

void InspectorBackend::disableProfiler(bool always)
{
    if (m_inspectorController)
        m_inspectorController->disableProfiler(always);
}

bool InspectorBackend::profilerEnabled()
{
    if (m_inspectorController)
        return m_inspectorController->profilerEnabled();
    return false;
}

void InspectorBackend::enableDebugger(bool always)
{
    if (m_inspectorController)
        m_inspectorController->enableDebuggerFromFrontend(always);
}

void InspectorBackend::disableDebugger(bool always)
{
    if (m_inspectorController)
        m_inspectorController->disableDebugger(always);
}

bool InspectorBackend::debuggerEnabled() const
{
    if (m_inspectorController)
        return m_inspectorController->debuggerEnabled();
    return false;
}

JavaScriptCallFrame* InspectorBackend::currentCallFrame() const
{
    return JavaScriptDebugServer::shared().currentCallFrame();
}

void InspectorBackend::addBreakpoint(const String& sourceID, unsigned lineNumber)
{
    intptr_t sourceIDValue = sourceID.toIntPtr();
    JavaScriptDebugServer::shared().addBreakpoint(sourceIDValue, lineNumber);
}

void InspectorBackend::removeBreakpoint(const String& sourceID, unsigned lineNumber)
{
    intptr_t sourceIDValue = sourceID.toIntPtr();
    JavaScriptDebugServer::shared().removeBreakpoint(sourceIDValue, lineNumber);
}

bool InspectorBackend::pauseOnExceptions()
{
    return JavaScriptDebugServer::shared().pauseOnExceptions();
}

void InspectorBackend::setPauseOnExceptions(bool pause)
{
    JavaScriptDebugServer::shared().setPauseOnExceptions(pause);
}

void InspectorBackend::pauseInDebugger()
{
    JavaScriptDebugServer::shared().pauseProgram();
}

void InspectorBackend::resumeDebugger()
{
    if (m_inspectorController)
        m_inspectorController->resumeDebugger();
}

void InspectorBackend::stepOverStatementInDebugger()
{
    JavaScriptDebugServer::shared().stepOverStatement();
}

void InspectorBackend::stepIntoStatementInDebugger()
{
    JavaScriptDebugServer::shared().stepIntoStatement();
}

void InspectorBackend::stepOutOfFunctionInDebugger()
{
    JavaScriptDebugServer::shared().stepOutOfFunction();
}

#endif

void InspectorBackend::getChildNodes(long callId, long nodeId)
{
    if (m_inspectorController)
        m_inspectorController->domAgent()->getChildNodes(callId, nodeId);
}

void InspectorBackend::setAttribute(long callId, long elementId, const String& name, const String& value)
{
    if (m_inspectorController)
        m_inspectorController->domAgent()->setAttribute(callId, elementId, name, value);
}

void InspectorBackend::removeAttribute(long callId, long elementId, const String& name)
{
    if (m_inspectorController)
        m_inspectorController->domAgent()->removeAttribute(callId, elementId, name);
}

void InspectorBackend::setTextNodeValue(long callId, long nodeId, const String& value)
{
    if (m_inspectorController)
        m_inspectorController->domAgent()->setTextNodeValue(callId, nodeId, value);
}

void InspectorBackend::highlight(long nodeId)
{
    if (m_inspectorController) {
        Node* node = m_inspectorController->domAgent()->nodeForId(nodeId);
        if (node)
            m_inspectorController->highlight(node);
    }
}

Node* InspectorBackend::nodeForId(long nodeId)
{
    if (m_inspectorController)
        return m_inspectorController->domAgent()->nodeForId(nodeId);
    return 0;
}

ScriptValue InspectorBackend::wrapObject(const ScriptValue& object)
{
    if (m_inspectorController)
        return m_inspectorController->wrapObject(object);
    return ScriptValue();
}

ScriptValue InspectorBackend::unwrapObject(const String& objectId)
{
    if (m_inspectorController)
        return m_inspectorController->unwrapObject(objectId);
    return ScriptValue();
}

long InspectorBackend::pushNodePathToFrontend(Node* node, bool selectInUI)
{
    if (!m_inspectorController)
        return 0;
    if (!m_inspectorController->m_domAgent || !m_inspectorController->m_frontend)
        return 0;
    long id = m_inspectorController->m_domAgent->pushNodePathToFrontend(node);
    if (selectInUI)
        m_inspectorController->m_frontend->updateFocusedNode(id);
    return id;
}

void InspectorBackend::addNodesToSearchResult(const String& nodeIds)
{
    if (m_inspectorController && m_inspectorController->m_frontend)
        m_inspectorController->m_frontend->addNodesToSearchResult(nodeIds);
}

#if ENABLE(DATABASE)
void InspectorBackend::selectDatabase(Database* database)
{
    if (!m_inspectorController)
        return;
    if (!m_inspectorController->m_frontend)
        return;
    m_inspectorController->m_frontend->selectDatabase(database);
}
#endif

#if ENABLE(DOM_STORAGE)
void InspectorBackend::selectDOMStorage(Storage* storage)
{
    if (!m_inspectorController)
        return;
    if (!m_inspectorController->m_frontend)
        return;
    m_inspectorController->m_frontend->selectDOMStorage(storage);
}
#endif

} // namespace WebCore
