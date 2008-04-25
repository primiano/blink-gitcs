/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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

#include "config.h"
#include "ApplicationCacheGroup.h"

#if ENABLE(OFFLINE_WEB_APPLICATIONS)

#include "ApplicationCache.h"
#include "ApplicationCacheResource.h"
#include "DocumentLoader.h"
#include "DOMApplicationCache.h"
#include "DOMWindow.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "MainResourceLoader.h"
#include "ManifestParser.h"
#include "Page.h"
#include "Settings.h"
#include <wtf/HashMap.h>

namespace WebCore {

typedef HashMap<String, ApplicationCacheGroup*> CacheGroupMap;

CacheGroupMap& cacheGroupMap()
{
    static CacheGroupMap cacheGroupMap;
    
    return cacheGroupMap;
}

// In order to quickly determinate if a given resource exists in an application cache,
// we keep a hash set of the hosts of the manifest URLs of all cache groups.
typedef HashSet<unsigned, AlreadyHashed> CacheHostSet;
    
CacheHostSet& cacheHostSet() {
    static CacheHostSet cacheHostSet;
    
    return cacheHostSet;
}

ApplicationCacheGroup::ApplicationCacheGroup(const KURL& manifestURL)
    : m_manifestURL(manifestURL)
    , m_status(Idle)
    , m_newestCache(0)
    , m_frame(0)
{
}

static unsigned urlHostHash(const KURL& url)
{
    unsigned hostStart = url.hostStart();
    unsigned hostEnd = url.hostEnd();
    
    return AlreadyHashed::avoidDeletedValue(StringImpl::computeHash(url.string().characters() + hostStart, hostEnd - hostStart));
}
    
ApplicationCache* ApplicationCacheGroup::cacheForMainRequest(const ResourceRequest& request, DocumentLoader* loader)
{
    if (!ApplicationCache::requestIsHTTPOrHTTPSGet(request))
        return 0;
 
    ASSERT(loader->frame());
    ASSERT(loader->frame()->page());
    if (loader->frame() != loader->frame()->page()->mainFrame())
        return 0;

    const KURL& url = request.url();
    if (!cacheHostSet().contains(urlHostHash(url)))
        return 0;
    
    CacheGroupMap::const_iterator end = cacheGroupMap().end();
    for (CacheGroupMap::const_iterator it = cacheGroupMap().begin(); it != end; ++it) {
        ApplicationCacheGroup* group = it->second;
        
        if (!protocolHostAndPortAreEqual(url, group->manifestURL()))
            continue;
        
        if (ApplicationCache* cache = group->newestCache()) {
            if (cache->resourceForURL(url))
                return cache;
        }
    }
    
    return 0;
}
    
void ApplicationCacheGroup::selectCache(Frame* frame, const KURL& manifestURL)
{
    ASSERT(frame && frame->page());
    
    if (!frame->settings()->offlineWebApplicationCacheEnabled())
        return;
    
    DocumentLoader* documentLoader = frame->loader()->documentLoader();
    ASSERT(!documentLoader->applicationCache());

    if (manifestURL.isNull()) {
        selectCacheWithoutManifestURL(frame);        
        return;
    }
    
    ApplicationCache* mainResourceCache = documentLoader->mainResourceApplicationCache();
    
    // Check if the main resource is being loaded as part of navigation of the main frame
    bool isMainFrame = frame->page()->mainFrame() == frame;
    
    if (!isMainFrame) {
        if (mainResourceCache && manifestURL != mainResourceCache->group()->manifestURL()) {
            ApplicationCacheResource* resource = mainResourceCache->resourceForURL(documentLoader->originalURL());
            ASSERT(resource);
            
            resource->addType(ApplicationCacheResource::Foreign);
        }

        return;
    }
    
    if (mainResourceCache) {
        if (manifestURL == mainResourceCache->group()->m_manifestURL) {
            mainResourceCache->group()->associateDocumentLoaderWithCache(documentLoader, mainResourceCache);
            mainResourceCache->group()->update(frame);
        } else {
            // FIXME: If the resource being loaded was loaded from an application cache and the URI of 
            // that application cache's manifest is not the same as the manifest URI with which the algorithm was invoked
            // then we should "undo" the navigation.
        }
        
        return;
    }
    
    // The resource was loaded from the network, check if it is a HTTP/HTTPS GET.    
    const ResourceRequest& request = frame->loader()->activeDocumentLoader()->request();

    if (!ApplicationCache::requestIsHTTPOrHTTPSGet(request)) {
        selectCacheWithoutManifestURL(frame);
        return;
    }

    // Check that the resource URL has the same scheme/host/port as the manifest URL.
    if (!protocolHostAndPortAreEqual(manifestURL, request.url())) {
        selectCacheWithoutManifestURL(frame);
        return;
    }
            
    ApplicationCache* cache = 0;
    ApplicationCacheGroup* group = 0;
    std::pair<CacheGroupMap::iterator, bool> result = cacheGroupMap().add(manifestURL, 0);
    if (result.second) {
        group = new ApplicationCacheGroup(manifestURL);
        result.first->second = group;
        cacheHostSet().add(urlHostHash(manifestURL));
    } else {
        group = result.first->second;
        cache = group->newestCache();
    }
            
    if (cache) {
        ASSERT(cache->manifestResource());
                
        group->associateDocumentLoaderWithCache(frame->loader()->documentLoader(), cache);
              
        if (!frame->loader()->documentLoader()->isLoadingMainResource())
            group->finishedLoadingMainResource(frame->loader()->documentLoader());

        group->update(frame);
    } else {
        bool isUpdating = group->m_cacheBeingUpdated;
                
        if (!isUpdating) {
            group->m_cacheBeingUpdated = ApplicationCache::create(group);
            group->m_caches.add(group->m_cacheBeingUpdated.get());
        }
                
        documentLoader->setCandidateApplicationCacheGroup(group);
        group->m_cacheCandidates.add(documentLoader);

        const KURL& url = frame->loader()->documentLoader()->originalURL();
                
        unsigned type = 0;

        // If the resource has already been downloaded, remove it so that it will be replaced with the implicit resource
        if (isUpdating)
            type = group->m_cacheBeingUpdated->removeResource(url);
               
        // Add the main resource URL as an implicit entry.
        group->addEntry(url, type | ApplicationCacheResource::Implicit);

        if (!frame->loader()->documentLoader()->isLoadingMainResource())
            group->finishedLoadingMainResource(frame->loader()->documentLoader());
                
        if (!isUpdating)
            group->update(frame);                
    }               
}

void ApplicationCacheGroup::selectCacheWithoutManifestURL(Frame* frame)
{
    if (!frame->settings()->offlineWebApplicationCacheEnabled())
        return;

    DocumentLoader* documentLoader = frame->loader()->documentLoader();
    ASSERT(!documentLoader->applicationCache());

    ApplicationCache* mainResourceCache = documentLoader->mainResourceApplicationCache();
    bool isMainFrame = frame->page()->mainFrame() == frame;

    if (isMainFrame && mainResourceCache) {
        mainResourceCache->group()->associateDocumentLoaderWithCache(documentLoader, mainResourceCache);
        mainResourceCache->group()->update(frame);
    }    
}

void ApplicationCacheGroup::finishedLoadingMainResource(DocumentLoader* loader)
{
    const KURL& url = loader->originalURL();
    
    if (ApplicationCache* cache = loader->applicationCache()) {
        RefPtr<ApplicationCacheResource> resource = ApplicationCacheResource::create(url, loader->response(), ApplicationCacheResource::Implicit, loader->mainResourceData());
        cache->addResource(resource.release());
        
        if (!m_cacheBeingUpdated)
            return;
    }
    
    ASSERT(m_pendingEntries.contains(url));
 
    EntryMap::iterator it = m_pendingEntries.find(url);
    ASSERT(it->second & ApplicationCacheResource::Implicit);

    RefPtr<ApplicationCacheResource> resource = ApplicationCacheResource::create(url, loader->response(), it->second, loader->mainResourceData());

    ASSERT(m_cacheBeingUpdated);
    m_cacheBeingUpdated->addResource(resource.release());
    
    m_pendingEntries.remove(it);
    
    checkIfLoadIsComplete();
}

void ApplicationCacheGroup::documentLoaderDestroyed(DocumentLoader* loader)
{
    HashSet<DocumentLoader*>::iterator it = m_associatedDocumentLoaders.find(loader);
    
    if (it != m_associatedDocumentLoaders.end()) {
        ASSERT(!m_cacheCandidates.contains(loader));
    
        m_associatedDocumentLoaders.remove(it);
        return;
    }
    
    ASSERT(m_cacheCandidates.contains(loader));
    m_cacheCandidates.remove(loader);
    
    // FIXME: Delete the group if the set of cache candidates is empty.
}
    

void ApplicationCacheGroup::cacheDestroyed(ApplicationCache* cache)
{
    ASSERT(m_caches.contains(cache));
    
    m_caches.remove(cache);
}
    
void ApplicationCacheGroup::update(Frame* frame)
{
    if (m_status == Checking || m_status == Downloading) 
        return;

    ASSERT(!m_frame);
    m_frame = frame;

    m_status = Checking;

    callListenersOnAssociatedDocuments(&DOMApplicationCache::callCheckingListener);
    
    ASSERT(!m_manifestHandle);
    ASSERT(!m_manifestResource);
    
    // FIXME: Handle defer loading
    m_manifestHandle = ResourceHandle::create(m_manifestURL, this, frame, false, true, false);
}
 
void ApplicationCacheGroup::didReceiveResponse(ResourceHandle* handle, const ResourceResponse& response)
{
    if (handle == m_manifestHandle) {
        didReceiveManifestResponse(response);
        return;
    }
    
    ASSERT(handle == m_currentHandle);
    
    int statusCode = response.httpStatusCode() / 100;
    if (statusCode == 4 || statusCode == 5) {
        cacheUpdateFailed();
        m_currentHandle = 0;
        return;
    }
    
    const KURL& url = handle->request().url();
    
    ASSERT(!m_currentResource);
    ASSERT(m_pendingEntries.contains(url));
    
    unsigned type = m_pendingEntries.get(url);
    
    // If this is an initial cache attempt, we should not get implicit resources delivered here.
    if (!m_newestCache)
        ASSERT(!(type & ApplicationCacheResource::Implicit));
    
    m_currentResource = ApplicationCacheResource::create(url, response, type);
}

void ApplicationCacheGroup::didReceiveData(ResourceHandle* handle, const char* data, int length, int lengthReceived)
{
    if (handle == m_manifestHandle) {
        didReceiveManifestData(data, length);
        return;
    }
    
    ASSERT(handle == m_currentHandle);
    
    ASSERT(m_currentResource);
    m_currentResource->data()->append(data, length);
}

void ApplicationCacheGroup::didFinishLoading(ResourceHandle* handle)
{
    if (handle == m_manifestHandle) {
        didFinishLoadingManifest();
        return;
    }
 
    ASSERT(m_currentHandle == handle);
    ASSERT(m_pendingEntries.contains(handle->request().url()));
    
    m_pendingEntries.remove(handle->request().url());
    
    ASSERT(m_cacheBeingUpdated);

    m_cacheBeingUpdated->addResource(m_currentResource.release());
    m_currentHandle = 0;
    
    // Load the next file.
    if (!m_pendingEntries.isEmpty()) {
        startLoadingEntry();
        return;
    }
    
    checkIfLoadIsComplete();
}

void ApplicationCacheGroup::didFail(ResourceHandle* handle, const ResourceError&)
{
    if (handle == m_manifestHandle) {
        didFailToLoadManifest();
        return;
    }
    
    cacheUpdateFailed();
    m_currentHandle = 0;
}

void ApplicationCacheGroup::didReceiveManifestResponse(const ResourceResponse& response)
{
    int statusCode = response.httpStatusCode() / 100;

    if (statusCode == 4 || statusCode == 5 || 
        !equalIgnoringCase(response.mimeType(), "text/cache-manifest")) {
        didFailToLoadManifest();
        return;
    }
    
    ASSERT(!m_manifestResource);
    ASSERT(m_manifestHandle);
    m_manifestResource = ApplicationCacheResource::create(m_manifestHandle->request().url(), response, 
                                                          ApplicationCacheResource::Manifest);
}

void ApplicationCacheGroup::didReceiveManifestData(const char* data, int length)
{
    ASSERT(m_manifestResource);
    m_manifestResource->data()->append(data, length);
}

void ApplicationCacheGroup::didFinishLoadingManifest()
{
    if (!m_manifestResource) {
        didFailToLoadManifest();
        return;
    }

    bool isUpgradeAttempt = m_newestCache;
    
    m_manifestHandle = 0;

    // Check if the manifest is byte-for-byte identical.
    if (isUpgradeAttempt) {
        ApplicationCacheResource* newestManifest = m_newestCache->manifestResource();
        ASSERT(newestManifest);
    
        if (newestManifest->data()->size() == m_manifestResource->data()->size() &&
            !memcmp(newestManifest->data()->data(), m_manifestResource->data()->data(), newestManifest->data()->size())) {
            
            callListenersOnAssociatedDocuments(&DOMApplicationCache::callNoUpdateListener);
         
            m_status = Idle;
            m_frame = 0;
            m_manifestResource = 0;
            return;
        }
    }
    
    Manifest manifest;
    if (!parseManifest(m_manifestURL, m_manifestResource->data()->data(), m_manifestResource->data()->size(), manifest)) {
        didFailToLoadManifest();
        return;
    }
        
    // FIXME: Add the opportunistic caching namespaces and their fallbacks.
    
    // We have the manifest, now download the resources.
    m_status = Downloading;
    
    callListenersOnAssociatedDocuments(&DOMApplicationCache::callDownloadingListener);

#ifndef NDEBUG
    // We should only have implicit entries.
    {
        EntryMap::const_iterator end = m_pendingEntries.end();
        for (EntryMap::const_iterator it = m_pendingEntries.begin(); it != end; ++it)
            ASSERT(it->second & ApplicationCacheResource::Implicit);
    }
#endif
    
    if (isUpgradeAttempt) {
        ASSERT(!m_cacheBeingUpdated);
        
        m_cacheBeingUpdated = ApplicationCache::create(this);
        m_caches.add(m_cacheBeingUpdated.get());
        
        ApplicationCache::ResourceMap::const_iterator end = m_newestCache->end();
        for (ApplicationCache::ResourceMap::const_iterator it = m_newestCache->begin(); it != end; ++it) {
            unsigned type = it->second->type();
            if (type & (ApplicationCacheResource::Opportunistic | ApplicationCacheResource::Implicit | ApplicationCacheResource::Dynamic))
                addEntry(it->first, type);
        }
    }
    
    HashSet<String>::const_iterator end = manifest.explicitURLs.end();
    for (HashSet<String>::const_iterator it = manifest.explicitURLs.begin(); it != end; ++it)
        addEntry(*it, ApplicationCacheResource::Explicit);
    
    m_cacheBeingUpdated->setOnlineWhitelist(manifest.onlineWhitelistedURLs);
    
    startLoadingEntry();
}

void ApplicationCacheGroup::cacheUpdateFailed()
{
    callListenersOnAssociatedDocuments(&DOMApplicationCache::callErrorListener);

    m_pendingEntries.clear();
    m_cacheBeingUpdated = 0;
    m_manifestResource = 0;

    bool isUpgradeAttempt = m_newestCache;
    if (!isUpgradeAttempt) {
        // If the cache attempt failed, setting m_cacheBeingUpdated to 0 should cause
        // it to be deleted.
        ASSERT(m_caches.isEmpty());
    }

    while (!m_cacheCandidates.isEmpty()) {
        HashSet<DocumentLoader*>::iterator it = m_cacheCandidates.begin();
        
        ASSERT((*it)->candidateApplicationCacheGroup() == this);
        (*it)->setCandidateApplicationCacheGroup(0);
        m_cacheCandidates.remove(it);
    }
    
    m_status = Idle;    
    m_frame = 0;
}
    
    
void ApplicationCacheGroup::didFailToLoadManifest()
{
    cacheUpdateFailed();
    m_manifestHandle = 0;
}

void ApplicationCacheGroup::checkIfLoadIsComplete()
{
    ASSERT(m_cacheBeingUpdated);
    
    if (m_manifestHandle)
        return;
    
    if (!m_pendingEntries.isEmpty())
        return;
    
    // We're done    
    bool isUpgradeAttempt = m_newestCache;
    
    m_cacheBeingUpdated->setManifestResource(m_manifestResource.release());
    
    m_status = Idle;
    m_frame = 0;
    
    Vector<RefPtr<DocumentLoader> > documentLoaders;
    
    if (isUpgradeAttempt) {
        ASSERT(m_cacheCandidates.isEmpty());
        
        copyToVector(m_associatedDocumentLoaders, documentLoaders);
    } else {
        while (!m_cacheCandidates.isEmpty()) {
            HashSet<DocumentLoader*>::iterator it = m_cacheCandidates.begin();
            
            DocumentLoader* loader = *it;
            ASSERT(!loader->applicationCache());
            ASSERT(loader->candidateApplicationCacheGroup() == m_cacheBeingUpdated->group());
            
            associateDocumentLoaderWithCache(loader, m_cacheBeingUpdated.get());
    
            documentLoaders.append(loader);

            m_cacheCandidates.remove(it);
        }
    }
    
    m_newestCache = m_cacheBeingUpdated.release();
    
    callListeners(isUpgradeAttempt ? &DOMApplicationCache::callUpdateReadyListener : &DOMApplicationCache::callCachedListener, 
                  documentLoaders);
}

void ApplicationCacheGroup::startLoadingEntry()
{
    ASSERT(m_cacheBeingUpdated);

    EntryMap::const_iterator it = m_pendingEntries.begin();

    // If this is an initial cache attempt, we do not want to fetch any implicit entries,
    // since those are fed to us by the normal loader machinery.
    if (!m_newestCache) {
        // Get the first URL in the entry table that is not implicit
        EntryMap::const_iterator end = m_pendingEntries.end();
    
        while (it->second & ApplicationCacheResource::Implicit) {
            ++it;

            if (it == end)
                return;
        }
    }
    
    // FIXME: Fire progress event.
    // FIXME: If this is an upgrade attempt, the newest cache should be used as an HTTP cache.
    
    ASSERT(!m_currentHandle);
    m_currentHandle = ResourceHandle::create(it->first, this, m_frame, false, true, false);
}

void ApplicationCacheGroup::addEntry(const String& url, unsigned type)
{
    ASSERT(m_cacheBeingUpdated);
    
    // Don't add the url if we already have an implicit resource in the cache
    if (ApplicationCacheResource* resource = m_cacheBeingUpdated->resourceForURL(url)) {
        ASSERT(resource->type() & ApplicationCacheResource::Implicit);
    
        resource->addType(type);
        return;
    }
    
    pair<EntryMap::iterator, bool> result = m_pendingEntries.add(url, type);
    
    if (!result.second)
        result.first->second |= type;
}

void ApplicationCacheGroup::associateDocumentLoaderWithCache(DocumentLoader* loader, ApplicationCache* cache)
{
    loader->setApplicationCache(cache);
    
    ASSERT(!m_associatedDocumentLoaders.contains(loader));
    m_associatedDocumentLoaders.add(loader);
}
 
void ApplicationCacheGroup::callListenersOnAssociatedDocuments(ListenerFunction listenerFunction)
{
    Vector<RefPtr<DocumentLoader> > loaders;
    copyToVector(m_associatedDocumentLoaders, loaders);

    callListeners(listenerFunction, loaders);
}
    
void ApplicationCacheGroup::callListeners(ListenerFunction listenerFunction, const Vector<RefPtr<DocumentLoader> >& loaders)
{
    for (unsigned i = 0; i < loaders.size(); i++) {
        Frame* frame = loaders[i]->frame();
        if (!frame)
            continue;
        
        ASSERT(frame->loader()->documentLoader() == loaders[i]);
        DOMWindow* window = frame->domWindow();
        
        if (DOMApplicationCache* domCache = window->optionalApplicationCache())
            (domCache->*listenerFunction)();
    }    
}

}

#endif // ENABLE(OFFLINE_WEB_APPLICATIONS)
