/*
    This file is part of the KDE libraries

    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller <mueller@kde.org>
    Copyright (C) 2004, 2005, 2006 Apple Computer, Inc.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.

    This class provides all functionality needed for loading images, style sheets and html
    pages from the web. It has a memory cache for these objects.
*/

#ifndef DocLoader_h
#define DocLoader_h

#include "Settings.h"
#include "CacheControl.h"
#include "DeprecatedPtrList.h"
#include "DeprecatedStringList.h"
#include "wtf/HashMap.h"
#include "StringHash.h"

class KURL;
class LoaderFunctions;

namespace WebCore {
    class CachedCSSStyleSheet;
    class CachedImage;
    class CachedObject;
    class CachedScript;
    class CachedXSLStyleSheet;
    class Document;
    class Frame;
    class HTMLImageLoader;
    
    /**
     * Manages the loading of scripts/images/stylesheets for a particular document
     */
    class DocLoader
    {
    public:
        DocLoader(Frame*, WebCore::Document*);
        ~DocLoader();

        CachedImage* requestImage(const String& url);
        CachedCSSStyleSheet* requestStyleSheet(const String& url, const DeprecatedString& charset);
        CachedScript* requestScript(const String& url, const DeprecatedString& charset);

#ifdef KHTML_XSLT
        CachedXSLStyleSheet* requestXSLStyleSheet(const String& url);
#endif
#ifndef KHTML_NO_XBL
        CachedXBLDocument* requestXBLDocument(const String &url);
#endif

        CachedObject* cachedObject(const String& url) const { return m_docObjects.get(url); }
        const HashMap<String, CachedObject*>& allCachedObjects() const { return m_docObjects; }

        bool autoloadImages() const { return m_bautoloadImages; }
        KIO::CacheControl cachePolicy() const { return m_cachePolicy; }
        time_t expireDate() const { return m_expireDate; }
        Frame* frame() const { return m_frame; }
        WebCore::Document* doc() const { return m_doc; }

        void setExpireDate(time_t);
        void setAutoloadImages(bool);
        void setCachePolicy(KIO::CacheControl cachePolicy);
        void removeCachedObject(CachedObject*) const;

        void setLoadInProgress(bool);
        bool loadInProgress() const { return m_loadInProgress; }

    private:
        bool needReload(const KURL &fullUrl);

        friend class Cache;
        friend class WebCore::Document;
        friend class WebCore::HTMLImageLoader;

        DeprecatedStringList m_reloadedURLs;
        mutable HashMap<String, CachedObject*> m_docObjects;
        time_t m_expireDate;
        KIO::CacheControl m_cachePolicy;
        bool m_bautoloadImages : 1;
        Frame* m_frame;
        WebCore::Document *m_doc;
        bool m_loadInProgress;
    };

}

#endif
