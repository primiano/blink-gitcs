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
#include "core/html/imports/HTMLImportChild.h"

#include "core/dom/Document.h"
#include "core/dom/custom/CustomElement.h"
#include "core/dom/custom/CustomElementMicrotaskDispatcher.h"
#include "core/dom/custom/CustomElementMicrotaskImportStep.h"
#include "core/html/imports/HTMLImportChildClient.h"
#include "core/html/imports/HTMLImportLoader.h"
#include "core/html/imports/HTMLImportsController.h"

namespace WebCore {

HTMLImportChild::HTMLImportChild(const KURL& url, SyncMode sync)
    : HTMLImport(sync)
    , m_url(url)
    , m_weakFactory(this)
    , m_loader(0)
    , m_client(0)
{
}

HTMLImportChild::~HTMLImportChild()
{
    // importDestroyed() should be called before the destruction.
    ASSERT(!m_loader);

    if (m_client)
        m_client->importChildWasDestroyed(this);
}

void HTMLImportChild::wasAlreadyLoaded()
{
    ASSERT(!m_loader);
    ASSERT(m_client);
    shareLoader();
    stateWillChange();
}

void HTMLImportChild::startLoading(const ResourcePtr<RawResource>& resource)
{
    ASSERT(!m_loader);

    if (m_loader)
        return;

    m_loader = toHTMLImportsController(root())->createLoader();
    m_loader->addImport(this);
    m_loader->startLoading(resource);

    createCustomElementMicrotaskStepIfNeeded();
}

void HTMLImportChild::didFinish()
{
    if (m_client)
        m_client->didFinish();
}

void HTMLImportChild::didFinishLoading()
{
    stateWillChange();
    if (m_customElementMicrotaskStep)
        CustomElementMicrotaskDispatcher::instance().importDidFinish(m_customElementMicrotaskStep.get());
}

void HTMLImportChild::didFinishUpgradingCustomElements()
{
    stateWillChange();
    m_customElementMicrotaskStep.clear();
}

bool HTMLImportChild::isLoaded() const
{
    return m_loader && m_loader->isDone();
}

bool HTMLImportChild::isFirst() const
{
    return m_loader && m_loader->isFirstImport(this);
}

Document* HTMLImportChild::importedDocument() const
{
    if (!m_loader)
        return 0;
    return m_loader->importedDocument();
}

void HTMLImportChild::importDestroyed()
{
    if (parent())
        parent()->removeChild(this);
    if (m_loader) {
        m_loader->removeImport(this);
        m_loader = 0;
    }
}

Document* HTMLImportChild::document() const
{
    return m_loader ? m_loader->document() : 0;
}

void HTMLImportChild::stateWillChange()
{
    toHTMLImportsController(root())->scheduleRecalcState();
}

void HTMLImportChild::stateDidChange()
{
    HTMLImport::stateDidChange();

    if (state().isReady())
        didFinish();
}

void HTMLImportChild::createCustomElementMicrotaskStepIfNeeded()
{
    // HTMLImportChild::normalize(), which is called from HTMLImportLoader::addImport(),
    // can move import children to new parents. So their microtask steps should be updated as well,
    // to let the steps be in the new parent queues.This method handles such migration.
    // For implementation simplicity, outdated step objects that are owned by moved children
    // aren't removed from the (now wrong) queues. Instead, each step invalidates its content so that
    // it is removed from the wrong queue during the next traversal. See parentWasChanged() for the detail.

    if (m_customElementMicrotaskStep) {
        m_customElementMicrotaskStep->parentWasChanged();
        m_customElementMicrotaskStep.clear();
    }

    if (!isDone() && !formsCycle()) {
        m_customElementMicrotaskStep = CustomElement::didCreateImport(this)->weakPtr();
    }

    for (HTMLImport* child = firstChild(); child; child = child->next())
        toHTMLImportChild(child)->createCustomElementMicrotaskStepIfNeeded();
}

void HTMLImportChild::shareLoader()
{
    ASSERT(!m_loader);

    if (HTMLImportChild* childToShareWith = toHTMLImportsController(root())->findLinkFor(m_url, this)) {
        m_loader = childToShareWith->m_loader;
        m_loader->addImport(this);
    }

    createCustomElementMicrotaskStepIfNeeded();
}

bool HTMLImportChild::isDone() const
{
    return m_loader && m_loader->isDone() && !m_loader->microtaskQueue()->needsProcessOrStop() && !m_customElementMicrotaskStep;
}

bool HTMLImportChild::loaderHasError() const
{
    return m_loader && m_loader->hasError();
}


void HTMLImportChild::setClient(HTMLImportChildClient* client)
{
    ASSERT(client);
    ASSERT(!m_client);
    m_client = client;
}

void HTMLImportChild::clearClient()
{
    // Doesn't check m_client nullity because we allow
    // clearClient() to reenter.
    m_client = 0;
}

HTMLLinkElement* HTMLImportChild::link() const
{
    if (!m_client)
        return 0;
    return m_client->link();
}

// Ensuring following invariants against the import tree:
// - HTMLImportChild::firstImport() is the "first import" of the DFS order of the import tree.
// - The "first import" manages all the children that is loaded by the document.
void HTMLImportChild::normalize()
{
    if (!loader()->isFirstImport(this) && this->precedes(loader()->firstImport())) {
        HTMLImportChild* oldFirst = loader()->firstImport();
        loader()->moveToFirst(this);
        takeChildrenFrom(oldFirst);
    }

    for (HTMLImport* child = firstChild(); child; child = child->next())
        toHTMLImportChild(child)->normalize();
}

#if !defined(NDEBUG)
void HTMLImportChild::showThis()
{
    bool isFirst = loader() ? loader()->isFirstImport(this) : false;
    HTMLImport::showThis();
    fprintf(stderr, " loader=%p first=%d, step=%p sync=%s url=%s",
        m_loader,
        isFirst,
        m_customElementMicrotaskStep.get(),
        isSync() ? "Y" : "N",
        url().string().utf8().data());
}
#endif

} // namespace WebCore
