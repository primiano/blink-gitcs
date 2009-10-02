/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef PageGroup_h
#define PageGroup_h

#include <wtf/HashSet.h>
#include <wtf/Noncopyable.h>
#include "LinkHash.h"
#include "StringHash.h"
#include "UserScript.h"
#include "UserStyleSheet.h"

namespace WebCore {

    class KURL;
    class Page;
    class StorageNamespace;

    class PageGroup : public Noncopyable {
    public:
        PageGroup(const String& name);
        PageGroup(Page*);
        ~PageGroup();

        static PageGroup* pageGroup(const String& groupName);
        static void closeLocalStorage();
        
        const HashSet<Page*>& pages() const { return m_pages; }

        void addPage(Page*);
        void removePage(Page*);

        bool isLinkVisited(LinkHash);

        void addVisitedLink(const KURL&);
        void addVisitedLink(const UChar*, size_t);
        void removeVisitedLinks();

        static void setShouldTrackVisitedLinks(bool);
        static void removeAllVisitedLinks();

        const String& name() { return m_name; }
        unsigned identifier() { return m_identifier; }

#if ENABLE(DOM_STORAGE)
        StorageNamespace* localStorage();
        bool hasLocalStorage() { return m_localStorage; }
#endif

        void addUserScript(const String& source, const KURL&, 
                           PassOwnPtr<Vector<String> > whitelist, PassOwnPtr<Vector<String> > blacklist,
                           unsigned worldID, UserScriptInjectionTime);
        const UserScriptMap* userScripts() const { return m_userScripts.get(); }
        
        void addUserStyleSheet(const String& source, const KURL&,
                               PassOwnPtr<Vector<String> > whitelist, PassOwnPtr<Vector<String> > blacklist,
                               unsigned worldID);
        const UserStyleSheetMap* userStyleSheets() const { return m_userStyleSheets.get(); }
        
        void removeUserContentForWorld(unsigned);
        void removeUserContentWithURLForWorld(const KURL&, unsigned);
        void removeAllUserContent();
        
    private:
        void addVisitedLink(LinkHash stringHash);

        String m_name;

        HashSet<Page*> m_pages;

        HashSet<LinkHash, LinkHashHash> m_visitedLinkHashes;
        bool m_visitedLinksPopulated;

        unsigned m_identifier;
#if ENABLE(DOM_STORAGE)
        RefPtr<StorageNamespace> m_localStorage;
#endif

        OwnPtr<UserScriptMap> m_userScripts;
        OwnPtr<UserStyleSheetMap> m_userStyleSheets;
    };

} // namespace WebCore
    
#endif // PageGroup_h
