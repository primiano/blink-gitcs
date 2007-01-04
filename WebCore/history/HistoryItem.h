/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef HistoryItem_H
#define HistoryItem_H

#include "FormData.h"
#include "IntPoint.h"
#include "KURL.h"
#include "PlatformString.h"
#include "Shared.h"
#include "StringHash.h"
#include <wtf/HashMap.h>
#include <wtf/OwnPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

#if PLATFORM(MAC)
#import "RetainPtr.h"
typedef struct objc_object* id;
#endif

namespace WebCore {

class Image;
class KURL;
class PageCache;
class ResourceRequest;

class HistoryItem;
typedef Vector<RefPtr<HistoryItem> > HistoryItemVector;

extern void (*notifyHistoryItemChanged)();

class HistoryItem : public Shared<HistoryItem> {
    friend class HistoryItemTimer;
public: 
    HistoryItem();
    HistoryItem(const String& urlString, const String& title, double lastVisited);
    HistoryItem(const KURL& url, const String& title);
    HistoryItem(const KURL& url, const String& target, const String& parent, const String& title);
    
    ~HistoryItem();
    
    PassRefPtr<HistoryItem> copy() const;
    
    const String& originalURLString() const;
    const String& urlString() const;
    const String& title() const;
    
    double lastVisitedTime() const;
    
    void setAlternateTitle(const String& alternateTitle);
    const String& alternateTitle() const;
    
    Image* icon() const;
    
    void retainIconInDatabase(bool retain);
    static void releaseAllPendingPageCaches();
    bool hasPageCache() const;
    void setHasPageCache(bool);
    PageCache* pageCache();

    const String& parent() const;
    KURL url() const;
    KURL originalURL() const;
    const String& target() const;
    bool isTargetItem() const;
    
    FormData* formData();
    String formContentType() const;
    String formReferrer() const;
    String rssFeedReferrer() const;
    
    int visitCount() const;

    void mergeAutoCompleteHints(HistoryItem* otherItem);
    
    const IntPoint& scrollPoint() const;
    void setScrollPoint(const IntPoint&);
    void clearScrollPoint();
    const Vector<String>& documentState() const;
    void setDocumentState(const Vector<String>&);
    void clearDocumentState();

    void setURL(const KURL&);
    void setURLString(const String&);
    void setOriginalURLString(const String&);
    void setTarget(const String&);
    void setParent(const String&);
    void setTitle(const String&);
    void setIsTargetItem(bool);
    
    void setFormInfoFromRequest(const ResourceRequest&);

    void setRSSFeedReferrer(const String&);
    void setVisitCount(int);

    void addChildItem(PassRefPtr<HistoryItem>);
    HistoryItem* childItemWithName(const String&) const;
    HistoryItem* targetItem();
    HistoryItem* recurseToFindTargetItem();
    const HistoryItemVector& children() const;
    bool hasChildren() const;

    void setAlwaysAttemptToUsePageCache(bool);
    bool alwaysAttemptToUsePageCache() const;

    // This should not be called directly for HistoryItems that are already included
    // in GlobalHistory. The WebKit api for this is to use -[WebHistory setLastVisitedTimeInterval:forItem:] instead.
    void setLastVisitedTime(double);
    
#if PLATFORM(MAC)
    id viewState() const;
    void setViewState(id);
    
    // Transient properties may be of any ObjC type.  They are intended to be used to store state per back/forward list entry.
    // The properties will not be persisted; when the history item is removed, the properties will be lost.
    id getTransientProperty(const String&) const;
    void setTransientProperty(const String&, id);
#endif

    void scheduleRelease();
    void cancelRelease();
    void releasePageCache();  
    
#ifndef NDEBUG
    void print() const;
#endif

private:
    HistoryItem(const HistoryItem&);
    static void releasePageCachesOrReschedule();
    
    String m_urlString;
    String m_originalURLString;
    String m_target;
    String m_parent;
    String m_title;
    String m_displayTitle;
    
    double m_lastVisitedTime;
    
    IntPoint m_scrollPoint;
    Vector<String> m_documentState;
    
    HistoryItemVector m_subItems;
    bool m_pageCacheIsPendingRelease;
    RefPtr<PageCache> m_pageCache;
    
    bool m_isTargetItem;
    bool m_alwaysAttemptToUsePageCache;
    int m_visitCount;
    
    // info used to repost form data
    RefPtr<FormData> m_formData;
    String m_formContentType;
    String m_formReferrer;
    
    // info used to support RSS feeds
    String m_rssFeedReferrer;
    
#if PLATFORM(MAC)
    RetainPtr<id> m_viewState;
    OwnPtr<HashMap<String, RetainPtr<id> > > m_transientProperties;
#endif
}; //class HistoryItem

} //namespace WebCore

#endif // HISTORYITEM_H
