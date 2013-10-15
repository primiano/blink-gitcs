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

#include "config.h"
#include "core/workers/SharedWorkerRepository.h"

#include "WebContentSecurityPolicy.h"
#include "WebFrameClient.h"
#include "WebFrameImpl.h"
#include "WebKit.h"
#include "WebSharedWorker.h"
#include "WebSharedWorkerRepository.h"
#include "bindings/v8/ExceptionMessages.h"
#include "bindings/v8/ExceptionState.h"
#include "core/dom/ExceptionCode.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/MessagePortChannel.h"
#include "core/events/Event.h"
#include "core/events/ThreadLocalEventNames.h"
#include "core/frame/ContentSecurityPolicy.h"
#include "core/inspector/InspectorInstrumentation.h"
#include "core/platform/network/ResourceResponse.h"
#include "core/workers/SharedWorker.h"
#include "core/workers/WorkerScriptLoader.h"
#include "core/workers/WorkerScriptLoaderClient.h"

#include "public/platform/Platform.h"
#include "public/platform/WebMessagePortChannel.h"
#include "public/platform/WebString.h"
#include "public/platform/WebURL.h"

namespace WebKit {

WebSharedWorkerRepository* s_sharedWorkerRepository = 0;

void setSharedWorkerRepository(WebSharedWorkerRepository* repository)
{
    s_sharedWorkerRepository = repository;
}

static WebSharedWorkerRepository* sharedWorkerRepository()
{
    // Will only be non-zero if the embedder has set the shared worker repository upon initialization. Nothing in WebKit sets this.
    return s_sharedWorkerRepository;
}

}

namespace WebCore {

class Document;
using WebKit::WebFrameImpl;
using WebKit::WebMessagePortChannel;
using WebKit::WebSharedWorker;
using WebKit::WebSharedWorkerRepository;

// Callback class that keeps the SharedWorker and WebSharedWorker objects alive while loads are potentially happening, and also translates load errors into error events on the worker.
class SharedWorkerScriptLoader : private WorkerScriptLoaderClient, private WebSharedWorker::ConnectListener {
public:
    SharedWorkerScriptLoader(PassRefPtr<SharedWorker> worker, const KURL& url, const String& name, PassRefPtr<MessagePortChannel> channel, PassOwnPtr<WebSharedWorker> webWorker)
        : m_worker(worker)
        , m_url(url)
        , m_name(name)
        , m_webWorker(webWorker)
        , m_channel(channel)
        , m_scriptLoader(WorkerScriptLoader::create())
        , m_loading(false)
        , m_responseAppCacheID(0)
    {
        m_scriptLoader->setTargetType(ResourceRequest::TargetIsSharedWorker);
    }

    ~SharedWorkerScriptLoader();
    void load();
    static void stopAllLoadersForContext(ExecutionContext*);

private:
    // WorkerScriptLoaderClient callbacks
    virtual void didReceiveResponse(unsigned long identifier, const ResourceResponse&);
    virtual void notifyFinished();

    virtual void connected();

    const ExecutionContext* loadingContext() { return m_worker->executionContext(); }

    void sendConnect();

    RefPtr<SharedWorker> m_worker;
    KURL m_url;
    String m_name;
    OwnPtr<WebSharedWorker> m_webWorker;
    RefPtr<MessagePortChannel> m_channel;
    RefPtr<WorkerScriptLoader> m_scriptLoader;
    bool m_loading;
    long long m_responseAppCacheID;
};

static Vector<SharedWorkerScriptLoader*>& pendingLoaders()
{
    AtomicallyInitializedStatic(Vector<SharedWorkerScriptLoader*>&, loaders = *new Vector<SharedWorkerScriptLoader*>);
    return loaders;
}

void SharedWorkerScriptLoader::stopAllLoadersForContext(ExecutionContext* context)
{
    // Walk our list of pending loaders and shutdown any that belong to this context.
    Vector<SharedWorkerScriptLoader*>& loaders = pendingLoaders();
    for (unsigned i = 0; i < loaders.size(); ) {
        SharedWorkerScriptLoader* loader = loaders[i];
        if (context == loader->loadingContext()) {
            loaders.remove(i);
            delete loader;
        } else
            i++;
    }
}

SharedWorkerScriptLoader::~SharedWorkerScriptLoader()
{
    if (m_loading)
        m_worker->unsetPendingActivity(m_worker.get());
}

void SharedWorkerScriptLoader::load()
{
    ASSERT(!m_loading);
    // If the shared worker is not yet running, load the script resource for it, otherwise just send it a connect event.
    if (m_webWorker->isStarted())
        sendConnect();
    else {
        // Keep the worker + JS wrapper alive until the resource load is complete in case we need to dispatch an error event.
        m_worker->setPendingActivity(m_worker.get());
        m_loading = true;

        m_scriptLoader->loadAsynchronously(m_worker->executionContext(), m_url, DenyCrossOriginRequests, this);
    }
}

void SharedWorkerScriptLoader::didReceiveResponse(unsigned long identifier, const ResourceResponse& response)
{
    m_responseAppCacheID = response.appCacheID();
    InspectorInstrumentation::didReceiveScriptResponse(m_worker->executionContext(), identifier);
}

void SharedWorkerScriptLoader::notifyFinished()
{
    if (m_scriptLoader->failed()) {
        m_worker->dispatchEvent(Event::createCancelable(EventTypeNames::error));
        delete this;
    } else {
        InspectorInstrumentation::scriptImported(m_worker->executionContext(), m_scriptLoader->identifier(), m_scriptLoader->script());
        // Pass the script off to the worker, then send a connect event.
        m_webWorker->startWorkerContext(m_url, m_name, m_worker->executionContext()->userAgent(m_url), m_scriptLoader->script(), m_worker->executionContext()->contentSecurityPolicy()->deprecatedHeader(), static_cast<WebKit::WebContentSecurityPolicyType>(m_worker->executionContext()->contentSecurityPolicy()->deprecatedHeaderType()), m_responseAppCacheID);
        sendConnect();
    }
}

void SharedWorkerScriptLoader::sendConnect()
{
    WebMessagePortChannel* webChannel = m_channel->webChannelRelease();
    m_channel.clear();
    // Send the connect event off, and linger until it is done sending.
    m_webWorker->connect(webChannel, this);
}

void SharedWorkerScriptLoader::connected()
{
    // Connect event has been sent, so free ourselves (this releases the SharedWorker so it can be freed as well if unreferenced).
    delete this;
}

bool SharedWorkerRepository::isAvailable()
{
    return WebKit::sharedWorkerRepository();
}

static WebSharedWorkerRepository::DocumentID getId(void* document)
{
    ASSERT(document);
    return reinterpret_cast<WebSharedWorkerRepository::DocumentID>(document);
}

void SharedWorkerRepository::connect(PassRefPtr<SharedWorker> worker, PassRefPtr<MessagePortChannel> port, const KURL& url, const String& name, ExceptionState& es)
{
    WebKit::WebSharedWorkerRepository* repository = WebKit::sharedWorkerRepository();

    // This should not be callable unless there's a SharedWorkerRepository for
    // this context (since isAvailable() should have returned null).
    ASSERT(repository);

    // No nested workers (for now) - connect() should only be called from document context.
    ASSERT(worker->executionContext()->isDocument());
    Document* document = toDocument(worker->executionContext());
    WebFrameImpl* webFrame = WebFrameImpl::fromFrame(document->frame());
    OwnPtr<WebSharedWorker> webWorker;
    webWorker = adoptPtr(webFrame->client()->createSharedWorker(webFrame, url, name, getId(document)));

    if (!webWorker) {
        // Existing worker does not match this url, so return an error back to the caller.
        es.throwDOMException(URLMismatchError, ExceptionMessages::failedToConstruct("SharedWorker", "The location of the SharedWorker named '" + name + "' does not exactly match the provided URL ('" + url.elidedString() + "')."));
        return;
    }

    repository->addSharedWorker(webWorker.get(), getId(document));

    // The loader object manages its own lifecycle (and the lifecycles of the two worker objects).
    // It will free itself once loading is completed.
    SharedWorkerScriptLoader* loader = new SharedWorkerScriptLoader(worker, url, name, port, webWorker.release());
    loader->load();
}

void SharedWorkerRepository::documentDetached(Document* document)
{
    WebKit::WebSharedWorkerRepository* repository = WebKit::sharedWorkerRepository();

    if (repository)
        repository->documentDetached(getId(document));

    // Stop the creation of any pending SharedWorkers for this context.
    // FIXME: Need a way to invoke this for WorkerGlobalScopes as well when we support for nested workers.
    SharedWorkerScriptLoader::stopAllLoadersForContext(document);
}

bool SharedWorkerRepository::hasSharedWorkers(Document* document)
{
    WebKit::WebSharedWorkerRepository* repository = WebKit::sharedWorkerRepository();

    return repository && repository->hasSharedWorkers(getId(document));
}

} // namespace WebCore
