/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Matt Lilek <webkit@mattlilek.com>
 * Copyright (C) 2009 Google Inc. All rights reserved.
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
#include "InspectorResource.h"

#include "CachedResource.h"
#include "DocLoader.h"
#include "DocumentLoader.h"
#include "Frame.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include "ScriptFunctionCall.h"
#include "ScriptObject.h"
#include "TextEncoding.h"

namespace WebCore {

InspectorResource::InspectorResource(long long identifier, DocumentLoader* loader)
    : m_identifier(identifier)
    , m_loader(loader)
    , m_frame(loader->frame())
    , m_scriptObjectCreated(false)
    , m_expectedContentLength(0)
    , m_cached(false)
    , m_finished(false)
    , m_failed(false)
    , m_length(0)
    , m_responseStatusCode(0)
    , m_startTime(-1.0)
    , m_responseReceivedTime(-1.0)
    , m_endTime(-1.0)
    , m_isMainResource(false)
{
}

InspectorResource::~InspectorResource()
{
}

PassRefPtr<InspectorResource> InspectorResource::createCached(long long identifier, DocumentLoader* loader, const CachedResource* cachedResource)
{
    PassRefPtr<InspectorResource> resource = create(identifier, loader);

    resource->m_finished = true;

    resource->m_requestURL = KURL(cachedResource->url());
    resource->updateResponse(cachedResource->response());

    resource->m_length = cachedResource->encodedSize();
    resource->m_cached = true;
    resource->m_startTime = currentTime();
    resource->m_responseReceivedTime = resource->m_startTime;
    resource->m_endTime = resource->m_startTime;

    resource->m_changes.setAll();

    return resource;
}

void InspectorResource::updateRequest(const ResourceRequest& request)
{
    m_requestHeaderFields = request.httpHeaderFields();
    m_requestURL = request.url();

    m_changes.set(RequestChange);
}

void InspectorResource::updateResponse(const ResourceResponse& response)
{
    m_expectedContentLength = response.expectedContentLength();
    m_mimeType = response.mimeType();
    m_responseHeaderFields = response.httpHeaderFields();
    m_responseStatusCode = response.httpStatusCode();
    m_suggestedFilename = response.suggestedFilename();

    m_changes.set(ResponseChange);
    m_changes.set(TypeChange);
}

static void populateHeadersObject(ScriptState* scriptState, ScriptObject* object, const HTTPHeaderMap& headers)
{
    HTTPHeaderMap::const_iterator end = headers.end();
    for (HTTPHeaderMap::const_iterator it = headers.begin(); it != end; ++it) {
        object->set(scriptState, it->first.string(), it->second);
    }
}

void InspectorResource::createScriptObject(ScriptState* scriptState, const ScriptObject& webInspector)
{
    if (!m_scriptObjectCreated) {
        bool hadException = false;

        ScriptObject scriptObject = ScriptObject::createNew(scriptState);
        ScriptObject requestHeaders = ScriptObject::createNew(scriptState);
        populateHeadersObject(scriptState, &requestHeaders, m_requestHeaderFields);
        scriptObject.set(scriptState, "requestHeaders", requestHeaders);
        scriptObject.set(scriptState, "requestURL", requestURL());
        scriptObject.set(scriptState, "host", m_requestURL.host());
        scriptObject.set(scriptState, "path", m_requestURL.path());
        scriptObject.set(scriptState, "lastPathComponent", m_requestURL.lastPathComponent());
        scriptObject.set(scriptState, "isMainResource", m_isMainResource);
        scriptObject.set(scriptState, "cached", m_cached);
        
        ScriptFunctionCall addResource(scriptState, webInspector, "addResource");
        addResource.appendArgument(m_identifier);
        addResource.appendArgument(scriptObject);
        addResource.call(hadException);
        if (hadException)
            return;

        m_scriptObjectCreated = true;
        m_changes.clear(RequestChange);
    }
    updateScriptObject(scriptState, webInspector);
}

void InspectorResource::updateScriptObject(ScriptState* scriptState, const ScriptObject& webInspector)
{
    if (!m_scriptObjectCreated)
        return;

    if (m_changes.hasChange(NoChange))
        return;

    bool hadException = false;

    ScriptObject scriptObject = ScriptObject::createNew(scriptState);
    if (m_changes.hasChange(RequestChange)) {
        scriptObject.set(scriptState, "url", requestURL());
        scriptObject.set(scriptState, "domain", m_requestURL.host());
        scriptObject.set(scriptState, "path", m_requestURL.path());
        scriptObject.set(scriptState, "lastPathComponent", m_requestURL.lastPathComponent());
        ScriptObject requestHeaders = ScriptObject::createNew(scriptState);
        populateHeadersObject(scriptState, &requestHeaders, m_requestHeaderFields);
        scriptObject.set(scriptState, "requestHeaders", requestHeaders);
        scriptObject.set(scriptState, "mainResource", m_isMainResource);
        scriptObject.set(scriptState, "didRequestChange", true);
    }

    if (m_changes.hasChange(ResponseChange)) {
        scriptObject.set(scriptState, "mimeType", m_mimeType);
        scriptObject.set(scriptState, "suggestedFilename", m_suggestedFilename);
        scriptObject.set(scriptState, "expectedContentLength", m_expectedContentLength);
        scriptObject.set(scriptState, "statusCode", m_responseStatusCode);
        scriptObject.set(scriptState, "suggestedFilename", m_suggestedFilename);
        ScriptObject responseHeaders = ScriptObject::createNew(scriptState);
        populateHeadersObject(scriptState, &responseHeaders, m_responseHeaderFields);
        scriptObject.set(scriptState, "responseHeaders", responseHeaders);
        scriptObject.set(scriptState, "didResponseChange", true);
    }

    if (m_changes.hasChange(TypeChange)) {
        scriptObject.set(scriptState, "type", static_cast<int>(type()));
        scriptObject.set(scriptState, "didTypeChange", true);
    }
    
    if (m_changes.hasChange(LengthChange)) {
        scriptObject.set(scriptState, "contentLength", m_length);
        scriptObject.set(scriptState, "didLengthChange", true);
    }

    if (m_changes.hasChange(CompletionChange)) {
        scriptObject.set(scriptState, "failed", m_failed);
        scriptObject.set(scriptState, "finished", m_finished);
        scriptObject.set(scriptState, "didCompletionChange", true);
    }

    if (m_changes.hasChange(TimingChange)) {
        if (m_startTime > 0)
            scriptObject.set(scriptState, "startTime", m_startTime);
        if (m_responseReceivedTime > 0)
            scriptObject.set(scriptState, "responseReceivedTime", m_responseReceivedTime);
        if (m_endTime > 0)
            scriptObject.set(scriptState, "endTime", m_endTime);
        scriptObject.set(scriptState, "didTimingChange", true);
    }
    
    ScriptFunctionCall updateResource(scriptState, webInspector, "updateResource");
    updateResource.appendArgument(m_identifier);
    updateResource.appendArgument(scriptObject);
    updateResource.call(hadException);
    if (hadException)
        return;

    m_changes.clearAll();
}

void InspectorResource::releaseScriptObject(ScriptState* scriptState, const ScriptObject& webInspector, bool callRemoveResource)
{
    if (!m_scriptObjectCreated)
        return;

    m_scriptObjectCreated = false;
    m_changes.setAll();

    if (!callRemoveResource)
        return;

    ScriptFunctionCall removeResource(scriptState, webInspector, "removeResource");
    removeResource.appendArgument(m_identifier);
    removeResource.call();
}

InspectorResource::Type InspectorResource::type() const
{
    if (!m_xmlHttpResponseText.isNull())
        return XHR;

    if (m_requestURL == m_loader->requestURL())
        return Doc;

    if (m_loader->frameLoader() && m_requestURL == m_loader->frameLoader()->iconURL())
        return Image;

    CachedResource* cachedResource = m_frame->document()->docLoader()->cachedResource(requestURL());
    if (!cachedResource)
        return Other;

    switch (cachedResource->type()) {
        case CachedResource::ImageResource:
            return Image;
        case CachedResource::FontResource:
            return Font;
        case CachedResource::CSSStyleSheet:
#if ENABLE(XSLT)
        case CachedResource::XSLStyleSheet:
#endif
            return Stylesheet;
        case CachedResource::Script:
            return Script;
        default:
            return Other;
    }
}

void InspectorResource::setXMLHttpResponseText(const ScriptString& data)
{
    m_xmlHttpResponseText = data;
    m_changes.set(TypeChange);
}

String InspectorResource::sourceString() const
{
    if (!m_xmlHttpResponseText.isNull())
        return String(m_xmlHttpResponseText);

    RefPtr<SharedBuffer> buffer;
    String textEncodingName;

    if (m_requestURL == m_loader->requestURL()) {
        buffer = m_loader->mainResourceData();
        textEncodingName = m_frame->document()->inputEncoding();
    } else {
        CachedResource* cachedResource = m_frame->document()->docLoader()->cachedResource(requestURL());
        if (!cachedResource)
            return String();

        if (cachedResource->isPurgeable()) {
            // If the resource is purgeable then make it unpurgeable to get
            // get its data. This might fail, in which case we return an
            // empty String.
            // FIXME: should we do something else in the case of a purged
            // resource that informs the user why there is no data in the
            // inspector?
            if (!cachedResource->makePurgeable(false))
                return String();
        }

        buffer = cachedResource->data();
        textEncodingName = cachedResource->encoding();
    }

    if (!buffer)
        return String();

    TextEncoding encoding(textEncodingName);
    if (!encoding.isValid())
        encoding = WindowsLatin1Encoding();
    return encoding.decode(buffer->data(), buffer->size());
}

void InspectorResource::startTiming()
{
    m_startTime = currentTime();
    m_changes.set(TimingChange);
}

void InspectorResource::markResponseReceivedTime()
{
    m_responseReceivedTime = currentTime();
    m_changes.set(TimingChange);
}

void InspectorResource::endTiming()
{
    m_endTime = currentTime();
    m_finished = true;
    m_changes.set(TimingChange);
    m_changes.set(CompletionChange);
}

void InspectorResource::markFailed()
{
    m_failed = true;
    m_changes.set(CompletionChange);
}

void InspectorResource::addLength(int lengthReceived)
{
    m_length += lengthReceived;
    m_changes.set(LengthChange);
}

} // namespace WebCore
