/*	
    WebBaseResourceHandleDelegate.m
    Copyright (c) 2002, Apple Computer, Inc. All rights reserved.
*/

#import <WebKit/WebBaseResourceHandleDelegate.h>

#import <WebFoundation/NSURLConnection.h>
#import <WebFoundation/NSURLConnectionAuthenticationChallenge.h>
#import <WebFoundation/NSURLConnectionPrivate.h>
#import <WebFoundation/NSURLRequest.h>
#import <WebFoundation/NSURLRequestPrivate.h>
#import <WebFoundation/NSURLResponse.h>
#import <WebFoundation/NSURLResponsePrivate.h>
#import <WebFoundation/WebAssertions.h>
#import <WebFoundation/WebNSErrorExtras.h>

#import <WebKit/WebAuthenticationChallenge.h>
#import <WebKit/WebAuthenticationChallengeInternal.h>
#import <WebKit/WebDataProtocol.h>
#import <WebKit/WebDataSourcePrivate.h>
#import <WebKit/WebDefaultResourceLoadDelegate.h>
#import <WebKit/WebKitErrors.h>
#import <WebKit/WebResourceLoadDelegate.h>
#import <WebKit/WebViewPrivate.h>

@implementation WebBaseResourceHandleDelegate

- (void)_releaseResources
{
    // It's possible that when we release the handle, it will be
    // deallocated and release the last reference to this object.
    // We need to retain to avoid accessing the object after it
    // has been deallocated and also to avoid reentering this method.
    
    [self retain];
    
    [identifier release];
    identifier = nil;

    [connection release];
    connection = nil;

    [controller release];
    controller = nil;
    
    [dataSource release];
    dataSource = nil;
    
    [resourceLoadDelegate release];
    resourceLoadDelegate = nil;

    [downloadDelegate release];
    downloadDelegate = nil;
    
    reachedTerminalState = YES;
    
    [self release];
}

- (void)dealloc
{
    [self _releaseResources];
    [request release];
    [response release];
    [super dealloc];
}

- (BOOL)loadWithRequest:(NSURLRequest *)r
{
    ASSERT(connection == nil);
    
    r = [self connection:connection willSendRequest:r redirectResponse:nil];
    connection = [[NSURLConnection alloc] initWithRequest:r delegate:self];
    if (defersCallbacks) {
        [connection setDefersCallbacks:YES];
    }

    return YES;
}

- (void)setDefersCallbacks:(BOOL)defers
{
    defersCallbacks = defers;
    [connection setDefersCallbacks:defers];
}

- (BOOL)defersCallbacks
{
    return defersCallbacks;
}

- (void)setDataSource:(WebDataSource *)d
{
    ASSERT(d);
    ASSERT([d _controller]);
    
    [d retain];
    [dataSource release];
    dataSource = d;

    [controller release];
    controller = [[dataSource _controller] retain];
    
    [resourceLoadDelegate release];
    resourceLoadDelegate = [[controller resourceLoadDelegate] retain];
    implementations = [controller _resourceLoadDelegateImplementations];

    [downloadDelegate release];
    downloadDelegate = [[controller downloadDelegate] retain];
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

- (NSURLRequest *)connection:(NSURLConnection *)con willSendRequest:(NSURLRequest *)newRequest redirectResponse:(NSURLResponse *)redirectResponse
{
    ASSERT(con == connection);
    ASSERT(!reachedTerminalState);
    NSMutableURLRequest *mutableRequest = [newRequest mutableCopy];
    NSURLRequest *clientRequest, *updatedRequest;
    BOOL haveDataSchemeRequest = NO;
    
    [mutableRequest setHTTPUserAgent:[controller userAgentForURL:[newRequest URL]]];
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
            identifier = [[resourceLoadDelegate webView: controller identifierForInitialRequest:clientRequest fromDataSource:dataSource] retain];
        else
            identifier = [[[WebDefaultResourceLoadDelegate sharedResourceLoadDelegate] webView:controller identifierForInitialRequest:clientRequest fromDataSource:dataSource] retain];
    }

    // If we have a special "applewebdata" scheme URL we send a fake request to the delegate.
    if (implementations.delegateImplementsWillSendRequest)
        updatedRequest = [resourceLoadDelegate webView:controller resource:identifier willSendRequest:clientRequest redirectResponse:redirectResponse fromDataSource:dataSource];
    else
        updatedRequest = [[WebDefaultResourceLoadDelegate sharedResourceLoadDelegate] webView:controller resource:identifier willSendRequest:clientRequest redirectResponse:redirectResponse fromDataSource:dataSource];
        
    if (!haveDataSchemeRequest)
        newRequest = updatedRequest;
    else {
        // If the delegate modified the request use that instead of
        // our applewebdata request, otherwise use the original
        // applewebdata request.
        if (![updatedRequest isEqual:clientRequest])
            newRequest = updatedRequest;
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
    
    return request;
}

- (void)connection:(NSURLConnection *)con didReceiveAuthenticationChallenge:(NSURLConnectionAuthenticationChallenge *)challenge
{
    ASSERT(con == connection);
    ASSERT(!reachedTerminalState);

    ASSERT(!currentConnectionChallenge);
    ASSERT(!currentWebChallenge);

    currentConnectionChallenge = [challenge retain];;
    currentWebChallenge = [[WebAuthenticationChallenge alloc] _initWithAuthenticationChallenge:challenge delegate:self];

    if (implementations.delegateImplementsDidReceiveAuthenticationChallenge) {
        [resourceLoadDelegate webView:controller resource:identifier didReceiveAuthenticationChallenge:currentWebChallenge fromDataSource:dataSource];
    } else {
        [[WebDefaultResourceLoadDelegate sharedResourceLoadDelegate] webView:controller resource:identifier didReceiveAuthenticationChallenge:currentWebChallenge fromDataSource:dataSource];
    }
}

-(void)connection:(NSURLConnection *)con didCancelAuthenticationChallenge:(NSURLConnectionAuthenticationChallenge *)challenge
{
    ASSERT(con == connection);
    ASSERT(!reachedTerminalState);

    ASSERT(currentConnectionChallenge);
    ASSERT(currentWebChallenge);
    ASSERT(currentConnectionChallenge = challenge);

    if (implementations.delegateImplementsDidCancelAuthenticationChallenge) {
        [resourceLoadDelegate webView:controller resource:identifier didCancelAuthenticationChallenge:currentWebChallenge fromDataSource:dataSource];
    } else {
        [[WebDefaultResourceLoadDelegate sharedResourceLoadDelegate] webView:controller resource:identifier didCancelAuthenticationChallenge:currentWebChallenge fromDataSource:dataSource];
    }
}


-(void)useCredential:(NSURLCredential *)credential forAuthenticationChallenge:(WebAuthenticationChallenge *)challenge
{
    if (challenge == nil || challenge != currentWebChallenge) {
	return;
    }

    [[currentConnectionChallenge connection] useCredential:credential forAuthenticationChallenge:currentConnectionChallenge];

    [currentConnectionChallenge release];
    currentConnectionChallenge = nil;
    
    [currentWebChallenge release];
    currentWebChallenge = nil;
}

-(void)continueWithoutCredentialForAuthenticationChallenge:(WebAuthenticationChallenge *)challenge
{
    if (challenge == nil || challenge != currentWebChallenge) {
	return;
    }

    [[currentConnectionChallenge connection] continueWithoutCredentialForAuthenticationChallenge:currentConnectionChallenge];

    [currentConnectionChallenge release];
    currentConnectionChallenge = nil;
    
    [currentWebChallenge release];
    currentWebChallenge = nil;
}

- (void)connection:(NSURLConnection *)con didReceiveResponse:(NSURLResponse *)r
{
    ASSERT(con == connection);
    ASSERT(!reachedTerminalState);

    // If the URL is one of our whacky applewebdata URLs that
    // fake up a substitute URL to present to the delegate.
    if([WebDataProtocol _webIsDataProtocolURL:[r URL]]) {
        NSURL *baseURL = [request _webDataRequestBaseURL];
        if (baseURL)
            [r _setURL: baseURL];
        else
            [r _setURL: [NSURL URLWithString: @"about:blank"]];
    }

    [r retain];
    [response release];
    response = r;

    [dataSource _addResponse: r];
    
    if (implementations.delegateImplementsDidReceiveResponse)
        [resourceLoadDelegate webView:controller resource:identifier didReceiveResponse:r fromDataSource:dataSource];
    else
        [[WebDefaultResourceLoadDelegate sharedResourceLoadDelegate] webView:controller resource:identifier didReceiveResponse:r fromDataSource:dataSource];
}

- (void)connection:(NSURLConnection *)con didReceiveData:(NSData *)data
{
    ASSERT(con == connection);
    ASSERT(!reachedTerminalState);

    if (implementations.delegateImplementsDidReceiveContentLength)
        [resourceLoadDelegate webView:controller resource:identifier didReceiveContentLength:[data length] fromDataSource:dataSource];
    else
        [[WebDefaultResourceLoadDelegate sharedResourceLoadDelegate] webView:controller resource:identifier didReceiveContentLength:[data length] fromDataSource:dataSource];
}

- (void)connectionDidFinishLoading:(NSURLConnection *)con
{
    ASSERT(con == connection);
    ASSERT(!reachedTerminalState);

    if (implementations.delegateImplementsDidFinishLoadingFromDataSource)
        [resourceLoadDelegate webView:controller resource:identifier didFinishLoadingFromDataSource:dataSource];
    else
        [[WebDefaultResourceLoadDelegate sharedResourceLoadDelegate] webView:controller resource:identifier didFinishLoadingFromDataSource:dataSource];

    [self _releaseResources];
}

- (void)connection:(NSURLConnection *)con didFailLoadingWithError:(NSError *)result
{
    ASSERT(con == connection);
    ASSERT(!reachedTerminalState);
    
    [[controller _resourceLoadDelegateForwarder] webView:controller resource:identifier didFailLoadingWithError:result fromDataSource:dataSource];

    [self _releaseResources];
}

- (void)cancelWithError:(NSError *)error
{
    ASSERT(!reachedTerminalState);

    [currentConnectionChallenge release];
    currentConnectionChallenge = nil;
    
    [currentWebChallenge release];
    currentWebChallenge = nil;

    [connection cancel];
    
    if (error) {
        [[controller _resourceLoadDelegateForwarder] webView:controller resource:identifier didFailLoadingWithError:error fromDataSource:dataSource];
    }

    [self _releaseResources];
}

- (void)cancel
{
    if (!reachedTerminalState) {
        [self cancelWithError:[self cancelledError]];
    }
}

- (NSError *)cancelledError
{
    return [NSError _web_errorWithDomain:WebFoundationErrorDomain
                                    code:WebFoundationErrorCancelled
                              failingURL:[[request URL] absoluteString]];
}

- (void)setIdentifier: ident
{
    if (identifier != ident){
        [identifier release];
        identifier = [ident retain];
    }
}


@end
