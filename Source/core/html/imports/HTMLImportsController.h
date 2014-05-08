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

#ifndef HTMLImportsController_h
#define HTMLImportsController_h

#include "core/fetch/RawResource.h"
#include "core/html/LinkResource.h"
#include "core/html/imports/HTMLImport.h"
#include "platform/Supplementable.h"
#include "platform/Timer.h"
#include "wtf/FastAllocBase.h"
#include "wtf/PassOwnPtr.h"
#include "wtf/Vector.h"

namespace WebCore {

class FetchRequest;
class ExecutionContext;
class ResourceFetcher;
class HTMLImportChild;
class HTMLImportChildClient;
class HTMLImportLoader;

class HTMLImportsController FINAL : public NoBaseWillBeGarbageCollectedFinalized<HTMLImportsController>, public HTMLImport, public DocumentSupplement {
    WILL_BE_USING_GARBAGE_COLLECTED_MIXIN(HTMLImportsController);
    WTF_MAKE_FAST_ALLOCATED_WILL_BE_REMOVED;
public:
    static void provideTo(Document&);

    explicit HTMLImportsController(Document&);
    virtual ~HTMLImportsController();

    bool isMaster(const Document& document) const { return m_master == &document; }
    bool shouldBlockScriptExecution(const Document&) const;
    void wasDetachedFrom(const Document&);

    // HTMLImport
    virtual Document* document() const OVERRIDE;
    virtual bool isDone() const OVERRIDE;
    virtual void stateWillChange() OVERRIDE;
    virtual void stateDidChange() OVERRIDE;

    HTMLImportChild* load(HTMLImport* parent, HTMLImportChildClient*, FetchRequest);
    void showSecurityErrorMessage(const String&);

    SecurityOrigin* securityOrigin() const;
    ResourceFetcher* fetcher() const;
    LocalFrame* frame() const;
    Document* master() const { return m_master; }

    void recalcTimerFired(Timer<HTMLImportsController>*);

    HTMLImportLoader* createLoader();

    size_t loaderCount() const { return m_loaders.size(); }
    HTMLImportLoader* loaderAt(size_t i) const { return m_loaders[i].get(); }
    HTMLImportLoader* loaderFor(const Document&) const;

    void scheduleRecalcState();
    HTMLImportChild* findLinkFor(const KURL&, HTMLImport* excluding = 0) const;

private:
    HTMLImportChild* createChild(const KURL&, HTMLImport* parent, HTMLImportChildClient*);
    void clear();

    Document* m_master;
    Timer<HTMLImportsController> m_recalcTimer;

    // List of import which has been loaded or being loaded.
    typedef Vector<OwnPtr<HTMLImportChild> > ImportList;
    ImportList m_imports;

    typedef Vector<RefPtr<HTMLImportLoader> > LoaderList;
    LoaderList m_loaders;
};

DEFINE_TYPE_CASTS(HTMLImportsController, HTMLImport, import, import->isRoot(), import.isRoot());

} // namespace WebCore

#endif // HTMLImportsController_h
