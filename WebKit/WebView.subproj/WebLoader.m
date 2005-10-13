/*
 * Copyright (C) 2005 Apple Computer, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import <WebKit/WebLoader.h>

#import <Foundation/NSURLAuthenticationChallenge.h>
#import <Foundation/NSURLConnection.h>
#import <Foundation/NSURLRequest.h>
#import <Foundation/NSURLResponse.h>

#import <WebKit/WebAssertions.h>
#import <WebKit/WebDataProtocol.h>
#import <WebKit/WebDataSourcePrivate.h>
#import <WebKit/WebDefaultResourceLoadDelegate.h>
#import <WebKit/WebKitErrors.h>
#import <WebKit/WebKitErrorsPrivate.h>
#import <WebKit/WebNSURLRequestExtras.h>
#import <WebKit/WebKitNSStringExtras.h>
#import <WebKit/WebPreferences.h>
#import <WebKit/WebPreferencesPrivate.h>
#import <WebKit/WebResourceLoadDelegate.h>
#import <WebKit/WebResourcePrivate.h>
#import <WebKit/WebViewPrivate.h>
#import <WebKitSystemInterface.h>

static unsigned inNSURLConnectionCallback;
static BOOL NSURLConnectionSupportsBufferedData;

@interface NSURLConnection (NSURLConnectionTigerPrivate)
- (NSData *)_bufferedData;
@end

@interface WebLoader (WebNSURLAuthenticationChallengeSender) <NSURLAuthenticationChallengeSender>
@end

@implementation WebLoader (WebNSURLAuthenticationChallengeSender) 

- (void)useCredential:(NSURLCredential *)credential forAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge
{
    if (challenge == nil || challenge != currentWebChallenge) {
	return;
    }

    [[currentConnectionChallenge sender] useCredential:credential forAuthenticationChallenge:currentConnectionChallenge];

    [currentConnectionChallenge release];
    currentConnectionChallenge = nil;
    
    [currentWebChallenge release];
    currentWebChallenge = nil;
}

- (void)continueWithoutCredentialForAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge
{
    if (challenge == nil || challenge != currentWebChallenge) {
	return;
    }

    [[currentConnectionChallenge sender] continueWithoutCredentialForAuthenticationChallenge:currentConnectionChallenge];

    [currentConnectionChallenge release];
    currentConnectionChallenge = nil;
    
    [currentWebChallenge release];
    currentWebChallenge = nil;
}

- (void)cancelAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge
{
    if (challenge == nil || challenge != currentWebChallenge) {
	return;
    }

    [self cancel];
}

@end

// This declaration is only needed to ease the transition to a new SPI.  It can be removed
// moving forward beyond Tiger 8A416.
@interface NSURLProtocol (WebFoundationSecret) 
+ (void)_removePropertyForKey:(NSString *)key inRequest:(NSMutableURLRequest *)request;
@end

@implementation WebLoader

+ (void)initialize
{
    NSURLConnectionSupportsBufferedData = [NSURLConnection instancesRespondToSelector:@selector(_bufferedData)];
}

- (void)releaseResources
{
    ASSERT(!reachedTerminalState);
    
    // It's possible that when we release the handle, it will be
    // deallocated and release the last reference to this object.
    // We need to retain to avoid accessing the object after it
    // has been deallocated and also to avoid reentering this method.
    
    [self retain];
    
    [identifier release];
    identifier = nil;

    [connection release];
    connection = nil;

    [webView release];
    webView = nil;
    
    [dataSource release];
    dataSource = nil;
    
    [resourceLoadDelegate release];
    resourceLoadDelegate = nil;

    [downloadDelegate release];
    downloadDelegate = nil;
    
    [resource release];
    resource = nil;
    
    [resourceData release];
    resourceData = nil;
    
    reachedTerminalState = YES;
    
    [self release];
}

- (void)dealloc
{
    ASSERT(reachedTerminalState);
    [request release];
    [response release];
    [originalURL release];
    [super dealloc];
}

- (void)deliverResource
{
    ASSERT(resource);
    ASSERT(waitingToDeliverResource);
    
    if (!defersCallbacks) {
        [self didReceiveResponse:[resource _response]];
        NSData *data = [resource data];
        [self didReceiveData:data lengthReceived:[data length]];
        [self didFinishLoading];
        deliveredResource = YES;
        waitingToDeliverResource = NO;
    }
}

- (void)deliverResourceAfterDelay
{
    if (resource && !defersCallbacks && !waitingToDeliverResource && !deliveredResource) {
        [self performSelector:@selector(deliverResource) withObject:nil afterDelay:0];
        waitingToDeliverResource = YES;
    }
}

// The following 2 methods are copied from [NSHTTPURLProtocol _cachedResponsePassesValidityChecks] and modified for our needs.
// FIXME: It would be nice to eventually to share this code somehow.
- (BOOL)_canUseResourceForRequest:(NSURLRequest *)theRequest
{
    NSURLRequestCachePolicy policy = [theRequest cachePolicy];
        
    if (policy == NSURLRequestReturnCacheDataElseLoad) {
        return YES;
    } else if (policy == NSURLRequestReturnCacheDataDontLoad) {
        return YES;
    } else if (policy == NSURLRequestReloadIgnoringCacheData) {
        return NO;
    } else if ([theRequest valueForHTTPHeaderField:@"must-revalidate"] != nil) {
        return NO;
    } else if ([theRequest valueForHTTPHeaderField:@"proxy-revalidate"] != nil) {
        return NO;
    } else if ([theRequest valueForHTTPHeaderField:@"If-Modified-Since"] != nil) {
        return NO;
    } else if ([theRequest valueForHTTPHeaderField:@"Cache-Control"] != nil) {
	return NO;
    } else if ([[theRequest HTTPMethod] _webkit_isCaseInsensitiveEqualToString:@"POST"]) {
        return NO;
    } else {
        return YES;
    }
}

- (BOOL)_canUseResourceWithResponse:(NSURLResponse *)theResponse
{
    if (WKGetNSURLResponseMustRevalidate(theResponse)) {
        return NO;
    } else if (WKGetNSURLResponseCalculatedExpiration(theResponse) - CFAbsoluteTimeGetCurrent() < 1) {
        return NO;
    } else {
        return YES;
    }
}

- (BOOL)loadWithRequest:(NSURLRequest *)r
{
    ASSERT(connection == nil);
    ASSERT(resource == nil);
    
    NSURL *URL = [[r URL] retain];
    [originalURL release];
    originalURL = URL;
    
    deliveredResource = NO;
    waitingToDeliverResource = NO;

    NSURLRequest *clientRequest = [self willSendRequest:r redirectResponse:nil];
    if (clientRequest == nil) {
        NSError *badURLError = [NSError _webKitErrorWithDomain:NSURLErrorDomain 
                                                          code:NSURLErrorCancelled
                                                           URL:[r URL]];
        [self didFailWithError:badURLError];
        return NO;
    }
    r = clientRequest;
    
    if ([[r URL] isEqual:originalURL] && [self _canUseResourceForRequest:r]) {
        resource = [dataSource subresourceForURL:originalURL];
        if (resource != nil) {
            if ([self _canUseResourceWithResponse:[resource _response]]) {
                [resource retain];
                // Deliver the resource after a delay because callers don't expect to receive callbacks while calling this method.
                [self deliverResourceAfterDelay];
                return YES;
            } else {
                resource = nil;
            }
        }
    }
    
#ifndef NDEBUG
    isInitializingConnection = YES;
#endif
    connection = [[NSURLConnection alloc] initWithRequest:r delegate:self];
#ifndef NDEBUG
    isInitializingConnection = NO;
#endif
    if (defersCallbacks) {
		WKSetNSURLConnectionDefersCallbacks(connection, YES);
    }

    return YES;
}

- (void)setDefersCallbacks:(BOOL)defers
{
    defersCallbacks = defers;
	WKSetNSURLConnectionDefersCallbacks(connection, defers);
    // Deliver the resource after a delay because callers don't expect to receive callbacks while calling this method.
    [self deliverResourceAfterDelay];
}

- (BOOL)defersCallbacks
{
    return defersCallbacks;
}

- (void)setDataSource:(WebDataSource *)d
{
    ASSERT(d);
    ASSERT([d _webView]);
    
    [d retain];
    [dataSource release];
    dataSource = d;

    [webView release];
    webView = [[dataSource _webView] retain];
    
    [resourceLoadDelegate release];
    resourceLoadDelegate = [[webView resourceLoadDelegate] retain];
    implementations = [webView _resourceLoadDelegateImplementations];

    [downloadDelegate release];
    downloadDelegate = [[webView downloadDelegate] retain];

    [self setDefersCallbacks:[webView defersCallbacks]];
}

- (WebDataSource *)dataSource
{
    return dataSource;
}

- resourceLoadDelegate
{
    return resourceLoadDelegate;
}

- downloadDelegate
{
    return downloadDelegate;
}

- (void)addData:(NSData *)data
{
    // Don't buffer data if we're loading it from a WebResource.
    if (resource == nil) {
        if (NSURLConnectionSupportsBufferedData) {
            // Buffer data only if the connection has handed us the data because is has stopped buffering it.
            if (resourceData != nil) {
                [resourceData appendData:data];
            }
        } else {
            if (resourceData == nil) {
                resourceData = [[NSMutableData alloc] init];
            }
            [resourceData appendData:data];
        }
    }
}

- (void)saveResource
{
    // Don't save data as a WebResource if it was loaded from a WebResource.
    if (resource == nil) {
        NSData *data = [self resourceData];
        if ([data length] > 0) {
            ASSERT(originalURL);
            ASSERT([response MIMEType]);
            WebResource *newResource = [[WebResource alloc] _initWithData:data URL:originalURL response:response];
            if (newResource != nil) {
                [dataSource addSubresource:newResource];
                [newResource release];
            } else {
                ASSERT_NOT_REACHED();
            }
        }
    }
}

- (NSData *)resourceData
{
    if (resource != nil) {
        return [resource data];
    }
    if (resourceData != nil) {
        // Retain and autorelease resourceData since releaseResources (which releases resourceData) may be called 
        // before the caller of this method has an opporuntity to retain the returned data (4070729).
        return [[resourceData retain] autorelease];
    }
    if (NSURLConnectionSupportsBufferedData) {
        return [connection _bufferedData];
    }
    return nil;
}

- (void)clearResourceData
{
    [resourceData setLength:0];
}

- (NSURLRequest *)willSendRequest:(NSURLRequest *)newRequest redirectResponse:(NSURLResponse *)redirectResponse
{
    ASSERT(!reachedTerminalState);
    NSMutableURLRequest *mutableRequest = [newRequest mutableCopy];
    NSURLRequest *clientRequest, *updatedRequest;
    BOOL haveDataSchemeRequest = NO;
    
    // retain/release self in this delegate method since the additional processing can do
    // anything including possibly releasing self; one example of this is 3266216
    [self retain];

    [mutableRequest _web_setHTTPUserAgent:[webView userAgentForURL:[newRequest URL]]];
    newRequest = [mutableRequest autorelease];

    clientRequest = [newRequest _webDataRequestExternalRequest];
    if(!clientRequest)
        clientRequest = newRequest;
    else
        haveDataSchemeRequest = YES;
    
    if (identifier == nil) {
        // The identifier is released after the last callback, rather than in dealloc
        // to avoid potential cycles.
        if (implementations.delegateImplementsIdentifierForRequest)
            identifier = [[resourceLoadDelegate webView: webView identifierForInitialRequest:clientRequest fromDataSource:dataSource] retain];
        else
            identifier = [[[WebDefaultResourceLoadDelegate sharedResourceLoadDelegate] webView:webView identifierForInitialRequest:clientRequest fromDataSource:dataSource] retain];
    }

    // If we have a special "applewebdata" scheme URL we send a fake request to the delegate.
    if (implementations.delegateImplementsWillSendRequest)
        updatedRequest = [resourceLoadDelegate webView:webView resource:identifier willSendRequest:clientRequest redirectResponse:redirectResponse fromDataSource:dataSource];
    else
        updatedRequest = [[WebDefaultResourceLoadDelegate sharedResourceLoadDelegate] webView:webView resource:identifier willSendRequest:clientRequest redirectResponse:redirectResponse fromDataSource:dataSource];
        
    if (!haveDataSchemeRequest)
        newRequest = updatedRequest;
    else {
        // If the delegate modified the request use that instead of
        // our applewebdata request, otherwise use the original
        // applewebdata request.
        if (![updatedRequest isEqual:clientRequest]) {
            newRequest = updatedRequest;
        
            // The respondsToSelector: check is only necessary for people building/running prior to Tier 8A416.
            if ([NSURLProtocol respondsToSelector:@selector(_removePropertyForKey:inRequest:)] &&
                [newRequest isKindOfClass:[NSMutableURLRequest class]]) {
                NSMutableURLRequest *mr = (NSMutableURLRequest *)newRequest;
                [NSURLProtocol _removePropertyForKey:[NSURLRequest _webDataRequestPropertyKey] inRequest:mr];
            }

        }
    }

    // Store a copy of the request.
    [request autorelease];

    // Client may return a nil request, indicating that the request should be aborted.
    if (newRequest){
        request = [newRequest copy];
    }
    else {
        request = nil;
    }

    [self release];
    return request;
}

- (void)didReceiveAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge
{
    ASSERT(!reachedTerminalState);
    ASSERT(!currentConnectionChallenge);
    ASSERT(!currentWebChallenge);

    // retain/release self in this delegate method since the additional processing can do
    // anything including possibly releasing self; one example of this is 3266216
    [self retain];
    currentConnectionChallenge = [challenge retain];;
    currentWebChallenge = [[NSURLAuthenticationChallenge alloc] initWithAuthenticationChallenge:challenge sender:self];

    if (implementations.delegateImplementsDidReceiveAuthenticationChallenge) {
        [resourceLoadDelegate webView:webView resource:identifier didReceiveAuthenticationChallenge:currentWebChallenge fromDataSource:dataSource];
    } else {
        [[WebDefaultResourceLoadDelegate sharedResourceLoadDelegate] webView:webView resource:identifier didReceiveAuthenticationChallenge:currentWebChallenge fromDataSource:dataSource];
    }
    [self release];
}

- (void)didCancelAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge
{
    ASSERT(!reachedTerminalState);
    ASSERT(currentConnectionChallenge);
    ASSERT(currentWebChallenge);
    ASSERT(currentConnectionChallenge = challenge);

    // retain/release self in this delegate method since the additional processing can do
    // anything including possibly releasing self; one example of this is 3266216
    [self retain];
    if (implementations.delegateImplementsDidCancelAuthenticationChallenge) {
        [resourceLoadDelegate webView:webView resource:identifier didCancelAuthenticationChallenge:currentWebChallenge fromDataSource:dataSource];
    } else {
        [[WebDefaultResourceLoadDelegate sharedResourceLoadDelegate] webView:webView resource:identifier didCancelAuthenticationChallenge:currentWebChallenge fromDataSource:dataSource];
    }
    [self release];
}

- (void)didReceiveResponse:(NSURLResponse *)r
{
    ASSERT(!reachedTerminalState);

    // retain/release self in this delegate method since the additional processing can do
    // anything including possibly releasing self; one example of this is 3266216
    [self retain]; 

    // If the URL is one of our whacky applewebdata URLs then
    // fake up a substitute URL to present to the delegate.
    if([WebDataProtocol _webIsDataProtocolURL:[r URL]]) {
        r = [[[NSURLResponse alloc] initWithURL:[request _webDataRequestExternalURL] MIMEType:[r MIMEType] expectedContentLength:[r expectedContentLength] textEncodingName:[r textEncodingName]] autorelease];
    }

    [r retain];
    [response release];
    response = r;

    [dataSource _addResponse: r];

    [webView _incrementProgressForConnectionDelegate:self response:r];
        
    if (implementations.delegateImplementsDidReceiveResponse)
        [resourceLoadDelegate webView:webView resource:identifier didReceiveResponse:r fromDataSource:dataSource];
    else
        [[WebDefaultResourceLoadDelegate sharedResourceLoadDelegate] webView:webView resource:identifier didReceiveResponse:r fromDataSource:dataSource];
    [self release];
}

- (void)didReceiveData:(NSData *)data lengthReceived:(long long)lengthReceived
{
    // The following assertions are not quite valid here, since a subclass
    // might override didReceiveData: in a way that invalidates them. This
    // happens with the steps listed in 3266216
    // ASSERT(con == connection);
    // ASSERT(!reachedTerminalState);

    // retain/release self in this delegate method since the additional processing can do
    // anything including possibly releasing self; one example of this is 3266216
    [self retain];
    
    [self addData:data];
    
    [webView _incrementProgressForConnectionDelegate:self data:data];

    if (implementations.delegateImplementsDidReceiveContentLength)
        [resourceLoadDelegate webView:webView resource:identifier didReceiveContentLength:(WebNSUInt)lengthReceived fromDataSource:dataSource];
    else
        [[WebDefaultResourceLoadDelegate sharedResourceLoadDelegate] webView:webView resource:identifier didReceiveContentLength:(WebNSUInt)lengthReceived fromDataSource:dataSource];
    [self release];
}

- (void)willStopBufferingData:(NSData *)data
{
    ASSERT(resourceData == nil);
    resourceData = [data mutableCopy];
}

- (void)signalFinish
{
    signalledFinish = YES;

    [webView _completeProgressForConnectionDelegate:self];    
    
    if (implementations.delegateImplementsDidFinishLoadingFromDataSource)
        [resourceLoadDelegate webView:webView resource:identifier didFinishLoadingFromDataSource:dataSource];
    else
        [[WebDefaultResourceLoadDelegate sharedResourceLoadDelegate] webView:webView resource:identifier didFinishLoadingFromDataSource:dataSource];
}

- (void)didFinishLoading
{
    // If load has been cancelled after finishing (which could happen with a 
    // javascript that changes the window location), do nothing.
    if (cancelledFlag) {
        return;
    }
    
    ASSERT(!reachedTerminalState);

    [self saveResource];
    
    if (!signalledFinish)
        [self signalFinish];

    [self releaseResources];
}

- (void)didFailWithError:(NSError *)error
{
    ASSERT(!reachedTerminalState);

    // retain/release self in this delegate method since the additional processing can do
    // anything including possibly releasing self; one example of this is 3266216
    [self retain];
    [webView _completeProgressForConnectionDelegate:self];

    [[webView _resourceLoadDelegateForwarder] webView:webView resource:identifier didFailLoadingWithError:error fromDataSource:dataSource];

    [self releaseResources];
    [self release];
}

- (NSCachedURLResponse *)willCacheResponse:(NSCachedURLResponse *)cachedResponse
{
    // When in private browsing mode, prevent caching to disk
    if ([cachedResponse storagePolicy] == NSURLCacheStorageAllowed && [[webView preferences] privateBrowsingEnabled]) {
        cachedResponse = [[[NSCachedURLResponse alloc] initWithResponse:[cachedResponse response]
                                                                   data:[cachedResponse data]
                                                               userInfo:[cachedResponse userInfo]
                                                          storagePolicy:NSURLCacheStorageAllowedInMemoryOnly] autorelease];
    }
    return cachedResponse;
}

- (NSURLRequest *)connection:(NSURLConnection *)con willSendRequest:(NSURLRequest *)newRequest redirectResponse:(NSURLResponse *)redirectResponse
{
    ASSERT(con == connection);
    ++inNSURLConnectionCallback;
    NSURLRequest *result = [self willSendRequest:newRequest redirectResponse:redirectResponse];
    --inNSURLConnectionCallback;
    return result;
}

- (void)connection:(NSURLConnection *)con didReceiveAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge
{
    ASSERT(con == connection);
    ++inNSURLConnectionCallback;
    [self didReceiveAuthenticationChallenge:challenge];
    --inNSURLConnectionCallback;
}

- (void)connection:(NSURLConnection *)con didCancelAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge
{
    ASSERT(con == connection);
    ++inNSURLConnectionCallback;
    [self didCancelAuthenticationChallenge:challenge];
    --inNSURLConnectionCallback;
}

- (void)connection:(NSURLConnection *)con didReceiveResponse:(NSURLResponse *)r
{
    ASSERT(con == connection);
    ++inNSURLConnectionCallback;
    [self didReceiveResponse:r];
    --inNSURLConnectionCallback;
}

- (void)connection:(NSURLConnection *)con didReceiveData:(NSData *)data lengthReceived:(long long)lengthReceived
{
    ASSERT(con == connection);
    ++inNSURLConnectionCallback;
    [self didReceiveData:data lengthReceived:lengthReceived];
    --inNSURLConnectionCallback;
}

- (void)connection:(NSURLConnection *)con willStopBufferingData:(NSData *)data
{
    ASSERT(con == connection);
    ++inNSURLConnectionCallback;
    [self willStopBufferingData:data];
    --inNSURLConnectionCallback;
}

- (void)connectionDidFinishLoading:(NSURLConnection *)con
{
    // don't worry about checking connection consistency if this load
    // got cancelled while finishing.
    ASSERT(cancelledFlag || con == connection);
    ++inNSURLConnectionCallback;
    [self didFinishLoading];
    --inNSURLConnectionCallback;
}

- (void)connection:(NSURLConnection *)con didFailWithError:(NSError *)error
{
    ASSERT(con == connection);
    ++inNSURLConnectionCallback;
    [self didFailWithError:error];
    --inNSURLConnectionCallback;
}

- (NSCachedURLResponse *)connection:(NSURLConnection *)con willCacheResponse:(NSCachedURLResponse *)cachedResponse
{
#ifndef NDEBUG
    if (connection == nil && isInitializingConnection) {
        ERROR("connection:willCacheResponse: was called inside of [NSURLConnection initWithRequest:delegate:] (40676250)");
    }
#endif
    ++inNSURLConnectionCallback;
    NSCachedURLResponse *result = [self willCacheResponse:cachedResponse];
    --inNSURLConnectionCallback;
    return result;
}

- (void)cancelWithError:(NSError *)error
{
    ASSERT(!reachedTerminalState);

    // This flag prevents bad behvior when loads that finish cause the
    // load itself to be cancelled (which could happen with a javascript that 
    // changes the window location). This is used to prevent both the body
    // of this method and the body of connectionDidFinishLoading: running
    // for a single delegate. Cancelling wins.
    cancelledFlag = YES;
    
    [currentConnectionChallenge release];
    currentConnectionChallenge = nil;
    
    [currentWebChallenge release];
    currentWebChallenge = nil;

    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(deliverResource) object:nil];
    [connection cancel];

    [webView _completeProgressForConnectionDelegate:self];

    if (error) {
        [[webView _resourceLoadDelegateForwarder] webView:webView resource:identifier didFailLoadingWithError:error fromDataSource:dataSource];
    }

    [self releaseResources];
}

- (void)cancel
{
    if (!reachedTerminalState) {
        [self cancelWithError:[self cancelledError]];
    }
}

- (NSError *)cancelledError
{
    return [NSError _webKitErrorWithDomain:NSURLErrorDomain
                                      code:NSURLErrorCancelled
                                       URL:[request URL]];
}

- (void)setIdentifier: ident
{
    if (identifier != ident){
        [identifier release];
        identifier = [ident retain];
    }
}

- (NSURLResponse *)response
{
    return response;
}

+ (BOOL)inConnectionCallback
{
    return inNSURLConnectionCallback != 0;
}

- (void)setSupportsMultipartContent:(BOOL)flag
{
    supportsMultipartContent = flag;
}

@end
