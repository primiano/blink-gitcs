/*
 * Copyright (C) 2003 Apple Computer, Inc.  All rights reserved.
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

#import "KWQLoader.h"

#import "KWQExceptions.h"
#import "KWQKJobClasses.h"
#import "KWQLogging.h"
#import "KWQResourceLoader.h"
#import "WebCoreBridge.h"
#import "khtml_part.h"
#import "loader.h"

using khtml::Cache;
using khtml::CachedObject;
using khtml::CachedImage;
using khtml::DocLoader;
using khtml::Loader;
using khtml::Request;
using KIO::TransferJob;

bool KWQServeRequest(Loader *loader, Request *request, TransferJob *job)
{
    LOG(Loading, "Serving request for base %s, url %s", 
        request->m_docLoader->part()->baseURL().url().latin1(),
        request->object->url().string().latin1());
    
    return KWQServeRequest(loader, request->m_docLoader, job);
}

bool KWQServeRequest(Loader *loader, DocLoader *docLoader, TransferJob *job)
{
    KWQKHTMLPart *part = static_cast<KWQKHTMLPart *>(docLoader->part());
    WebCoreBridge *bridge = part->bridge();

    part->didTellBridgeAboutLoad(job->url().url());

    KWQ_BLOCK_EXCEPTIONS;
    KWQResourceLoader *resourceLoader = [[KWQResourceLoader alloc] initWithJob:job];

    id <WebCoreResourceHandle> handle;

    if (job->method() == "POST") {
	NSData *postData = [NSData dataWithBytesNoCopy:job->postData().data() length:job->postData().size() freeWhenDone:NO];
	handle = [bridge startLoadingResource:resourceLoader withURL:job->url().getNSURL() postData:postData];
    } else {
	handle = [bridge startLoadingResource:resourceLoader withURL:job->url().getNSURL()];
    }
    [resourceLoader setHandle:handle];
    [resourceLoader release];
    return handle != nil;
    KWQ_UNBLOCK_EXCEPTIONS;

    return true;
}

int KWQNumberOfPendingOrLoadingRequests(khtml::DocLoader *dl)
{
    return Cache::loader()->numRequests(dl);
}

bool KWQCheckIfReloading(DocLoader *loader)
{
    KWQ_BLOCK_EXCEPTIONS;
    return [static_cast<KWQKHTMLPart *>(loader->part())->bridge() isReloading];
    KWQ_UNBLOCK_EXCEPTIONS;

    return false;
}

void KWQCheckCacheObjectStatus(DocLoader *loader, CachedObject *cachedObject)
{
    // Return from the function for objects that we didn't load from the cache.
    if (!cachedObject)
        return;
    switch (cachedObject->status()) {
    case CachedObject::Persistent:
    case CachedObject::Cached:
    case CachedObject::Uncacheable:
        break;
    case CachedObject::NotCached:
    case CachedObject::Unknown:
    case CachedObject::New:
    case CachedObject::Pending:
        return;
    }
    
    ASSERT(cachedObject->response());
    
    // Notify the caller that we "loaded".
    KWQKHTMLPart *part = static_cast<KWQKHTMLPart *>(loader->part());

    QString urlString = cachedObject->url().string();

    if (!part->haveToldBridgeAboutLoad(urlString)) {
	WebCoreBridge *bridge = part->bridge();
	CachedImage *cachedImage = dynamic_cast<CachedImage *>(cachedObject);

	KWQ_BLOCK_EXCEPTIONS;
	[bridge objectLoadedFromCacheWithURL:KURL(cachedObject->url().string()).getNSURL()
	        response:(id)cachedObject->response()
	        size:cachedImage ? cachedImage->dataSize() : cachedObject->size()];
	KWQ_UNBLOCK_EXCEPTIONS;

	part->didTellBridgeAboutLoad(urlString);
    }
}

void KWQRetainResponse(void *response)
{
    // There's no way a retain can raise
    [(id)response retain];
}

void KWQReleaseResponse(void *response)
{
    // A release could raise if it deallocs, though...
    KWQ_BLOCK_EXCEPTIONS;
    [(id)response release];
    KWQ_UNBLOCK_EXCEPTIONS;
}

@interface NSObject (WebPrivateResponse)
- (NSString *)MIMEType;
@end

void *KWQResponseMIMEType(void *response)
{
    KWQ_BLOCK_EXCEPTIONS;
    return [(id)response MIMEType];
    KWQ_UNBLOCK_EXCEPTIONS;

    return NULL;
}


KWQLoader::KWQLoader(Loader *loader)
    : _requestStarted(loader, SIGNAL(requestStarted(khtml::DocLoader *, khtml::CachedObject *)))
    , _requestDone(loader, SIGNAL(requestDone(khtml::DocLoader *, khtml::CachedObject *)))
    , _requestFailed(loader, SIGNAL(requestFailed(khtml::DocLoader *, khtml::CachedObject *)))
{
}
