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

#include "config.h"
#include "core/html/imports/HTMLImportsController.h"

#include "core/dom/Document.h"
#include "core/dom/StyleEngine.h"
#include "core/fetch/ResourceFetcher.h"
#include "core/frame/LocalFrame.h"
#include "core/html/imports/HTMLImportChild.h"
#include "core/html/imports/HTMLImportChildClient.h"
#include "core/html/imports/HTMLImportLoader.h"

namespace WebCore {

void HTMLImportsController::provideTo(Document& master)
{
    DEFINE_STATIC_LOCAL(const char*, name, ("HTMLImportsController"));
    OwnPtrWillBeRawPtr<HTMLImportsController> controller = adoptPtrWillBeNoop(new HTMLImportsController(master));
    master.setImportsController(controller.get());
    DocumentSupplement::provideTo(master, name, controller.release());
}

HTMLImportsController::HTMLImportsController(Document& master)
    : HTMLImport(HTMLImport::Sync)
    , m_master(&master)
    , m_recalcTimer(this, &HTMLImportsController::recalcTimerFired)
{
    recalcTreeState(this); // This recomputes initial state.
}

HTMLImportsController::~HTMLImportsController()
{
    ASSERT(!m_master);
}

void HTMLImportsController::clear()
{
    for (size_t i = 0; i < m_imports.size(); ++i)
        m_imports[i]->importDestroyed();
    m_imports.clear();

    for (size_t i = 0; i < m_loaders.size(); ++i)
        m_loaders[i]->importDestroyed();
    m_loaders.clear();

    if (m_master)
        m_master->setImportsController(0);
    m_master = 0;

    m_recalcTimer.stop();
}

static bool makesCycle(HTMLImport* parent, const KURL& url)
{
    for (HTMLImport* ancestor = parent; ancestor; ancestor = ancestor->parent()) {
        if (!ancestor->isRoot() && equalIgnoringFragmentIdentifier(toHTMLImportChild(parent)->url(), url))
            return true;
    }

    return false;
}

HTMLImportChild* HTMLImportsController::createChild(const KURL& url, HTMLImport* parent, HTMLImportChildClient* client)
{
    HTMLImport::SyncMode mode = client->isSync() && !makesCycle(parent, url) ? HTMLImport::Sync : HTMLImport::Async;
    OwnPtr<HTMLImportChild> loader = adoptPtr(new HTMLImportChild(*m_master, url, mode));
    loader->setClient(client);
    parent->appendImport(loader.get());
    m_imports.append(loader.release());
    return m_imports.last().get();
}

HTMLImportChild* HTMLImportsController::load(HTMLImport* parent, HTMLImportChildClient* client, FetchRequest request)
{
    ASSERT(!request.url().isEmpty() && request.url().isValid());
    ASSERT(parent == this || toHTMLImportChild(parent)->loader()->isFirstImport(toHTMLImportChild(parent)));

    if (findLinkFor(request.url())) {
        HTMLImportChild* child = createChild(request.url(), parent, client);
        child->wasAlreadyLoaded();
        return child;
    }

    bool sameOriginRequest = securityOrigin()->canRequest(request.url());
    request.setCrossOriginAccessControl(
        securityOrigin(), sameOriginRequest ? AllowStoredCredentials : DoNotAllowStoredCredentials,
        ClientDidNotRequestCredentials);
    ResourcePtr<RawResource> resource = parent->document()->fetcher()->fetchImport(request);
    if (!resource)
        return 0;

    HTMLImportChild* child = createChild(request.url(), parent, client);
    // We set resource after the import tree is built since
    // Resource::addClient() immediately calls back to feed the bytes when the resource is cached.
    child->startLoading(resource);

    return child;
}

void HTMLImportsController::showSecurityErrorMessage(const String& message)
{
    m_master->addConsoleMessage(JSMessageSource, ErrorMessageLevel, message);
}

HTMLImportChild* HTMLImportsController::findLinkFor(const KURL& url, HTMLImport* excluding) const
{
    for (size_t i = 0; i < m_imports.size(); ++i) {
        HTMLImportChild* candidate = m_imports[i].get();
        if (candidate != excluding && equalIgnoringFragmentIdentifier(candidate->url(), url) && candidate->loader())
            return candidate;
    }

    return 0;
}

SecurityOrigin* HTMLImportsController::securityOrigin() const
{
    return m_master->securityOrigin();
}

ResourceFetcher* HTMLImportsController::fetcher() const
{
    return m_master->fetcher();
}

LocalFrame* HTMLImportsController::frame() const
{
    return m_master->frame();
}

Document* HTMLImportsController::document() const
{
    return m_master;
}

bool HTMLImportsController::shouldBlockScriptExecution(const Document& document) const
{
    ASSERT(document.importsController() == this);
    if (HTMLImportLoader* loader = loaderFor(document))
        return loader->shouldBlockScriptExecution();
    return state().shouldBlockScriptExecution();
}

void HTMLImportsController::wasDetachedFrom(const Document& document)
{
    ASSERT(document.importsController() == this);
    if (m_master == &document)
        clear();
}

bool HTMLImportsController::isDone() const
{
    return !m_master->parsing() && m_master->styleEngine()->haveStylesheetsLoaded();
}

void HTMLImportsController::stateWillChange()
{
    scheduleRecalcState();
}

void HTMLImportsController::stateDidChange()
{
    HTMLImport::stateDidChange();

    if (!state().isReady())
        return;
    if (LocalFrame* frame = m_master->frame())
        frame->loader().checkCompleted();
}

void HTMLImportsController::scheduleRecalcState()
{
    if (m_recalcTimer.isActive() || !m_master)
        return;
    m_recalcTimer.startOneShot(0, FROM_HERE);
}

void HTMLImportsController::recalcTimerFired(Timer<HTMLImportsController>*)
{
    ASSERT(m_master);

    do {
        m_recalcTimer.stop();
        HTMLImport::recalcTreeState(this);
    } while (m_recalcTimer.isActive());
}

HTMLImportLoader* HTMLImportsController::createLoader()
{
    m_loaders.append(HTMLImportLoader::create(this));
    return m_loaders.last().get();
}

HTMLImportLoader* HTMLImportsController::loaderFor(const Document& document) const
{
    for (size_t i = 0; i < m_loaders.size(); ++i) {
        if (m_loaders[i]->document() == &document)
            return m_loaders[i].get();
    }

    return 0;
}

} // namespace WebCore
