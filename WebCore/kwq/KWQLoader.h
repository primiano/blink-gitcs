/*
 * Copyright (C) 2004 Apple Computer, Inc.  All rights reserved.
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

#if __OBJC__
@class NSDictionary;
@class NSString;
@class NSURLResponse;

@interface NSDictionary (WebCore_Extras)
+ (id)_webcore_dictionaryWithHeaderString:(NSString *)string;
@end

#else
class NSDictionary;
class NSString;
class NSURLResponse;
#endif

class KURL;
class QString;

namespace WebCore {
    class CachedObject;
    class DocLoader;
    class TransferJob;
    class Loader;
    class Request;
    class String;

    template <typename T> class Array;

    typedef Array<char> ByteArray;
}

WebCore::ByteArray KWQServeSynchronousRequest(WebCore::Loader*, WebCore::DocLoader*, WebCore::TransferJob*, KURL& finalURL, QString& headers);

void KWQCheckCacheObjectStatus(WebCore::DocLoader*, WebCore::CachedObject*);
bool KWQCheckIfReloading(WebCore::DocLoader*);
bool KWQIsResponseURLEqualToURL(NSURLResponse*, const WebCore::String& m_url);
QString KWQResponseURL(NSURLResponse *response);
QString KWQResponseMIMEType(NSURLResponse *response);
bool KWQResponseIsMultipart(NSURLResponse *response);
int KWQNumberOfPendingOrLoadingRequests(WebCore::DocLoader*);
time_t KWQCacheObjectExpiresTime(WebCore::DocLoader*, NSURLResponse*);
NSString* KWQHeaderStringFromDictionary(NSDictionary* headers, int statusCode);
