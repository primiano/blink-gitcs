/*	
    WebMainResourceClient.mm
    Copyright (c) 2001, 2002, Apple Computer, Inc. All rights reserved.
*/

#import <WebKit/WebDocument.h>
#import <WebKit/WebDownloadHandler.h>
#import <WebKit/WebLoadProgress.h>
#import <WebKit/WebLocationChangeHandler.h>
#import <WebKit/WebMainResourceClient.h>
#import <WebKit/WebController.h>
#import <WebKit/WebControllerPrivate.h>
#import <WebKit/WebDataSource.h>
#import <WebKit/WebDataSourcePrivate.h>
#import <WebKit/WebFrame.h>
#import <WebKit/WebFramePrivate.h>
#import <WebKit/WebView.h>
#import <WebKit/WebBridge.h>
#import <WebKit/WebKitLogging.h>

#import <WebFoundation/WebError.h>
#import <WebFoundation/WebFileTypeMappings.h>
#import <WebFoundation/WebResourceHandle.h>
#import <WebFoundation/WebResourceResponse.h>

// FIXME: This is quite similar to WebSubresourceClient; they should share code.

@implementation WebMainResourceClient

- initWithDataSource:(WebDataSource *)ds
{
    self = [super init];
    
    if (self) {
        dataSource = [ds retain];
        resourceData = [[NSMutableData alloc] init];
        isFirstChunk = YES;
    }

    return self;
}

- (void)didStartLoadingWithURL:(NSURL *)URL
{
    ASSERT(currentURL == nil);
    currentURL = [URL retain];
    [[dataSource controller] _didStartLoading:currentURL];
}

- (void)didStopLoading
{
    ASSERT(currentURL != nil);
    [[dataSource controller] _didStopLoading:currentURL];
    [currentURL release];
    currentURL = nil;
}

- (void)dealloc
{
    ASSERT(currentURL == nil);
    ASSERT(downloadHandler == nil);
    
    [downloadProgressHandler release];
    [resourceData release];
    [dataSource release];
    
    [super dealloc];
}

- (NSData *)resourceData
{
    return resourceData;
}

- (WebDownloadHandler *)downloadHandler
{
    return downloadHandler;
}

- (void)receivedProgressWithHandle:(WebResourceHandle *)handle complete:(BOOL)isComplete
{
    WebLoadProgress *progress = [WebLoadProgress progressWithResourceHandle:handle];
    WebContentAction contentAction = [[dataSource contentPolicy] policyAction];

    if (contentAction == WebContentPolicySaveAndOpenExternally || contentAction == WebContentPolicySave) {
        if (isComplete) {
            [dataSource _setPrimaryLoadComplete:YES];
        }
        [downloadProgressHandler receivedProgress:progress forResourceHandle:handle 
            fromDataSource:dataSource complete:isComplete];
    } else {
        [[dataSource controller] _mainReceivedProgress:progress forResourceHandle:handle 
            fromDataSource:dataSource complete:isComplete];
    }
}

- (void)receivedError:(WebError *)error forHandle:(WebResourceHandle *)handle
{
    WebLoadProgress *progress = [WebLoadProgress progressWithResourceHandle:handle];
    WebContentAction contentAction = [[dataSource contentPolicy] policyAction];

    if (contentAction == WebContentPolicySaveAndOpenExternally || contentAction == WebContentPolicySave) {
        [downloadProgressHandler receivedError:error forResourceHandle:handle 
            partialProgress:progress fromDataSource:dataSource];
    } else {
        [[dataSource controller] _mainReceivedError:error forResourceHandle:handle 
            partialProgress:progress fromDataSource:dataSource];
    }
}

- (NSString *)handleWillUseUserAgent:(WebResourceHandle *)handle forURL:(NSURL *)URL
{
    ASSERT([dataSource controller]);
    return [[dataSource controller] userAgentForURL:URL];
}

- (void)didCancelWithHandle:(WebResourceHandle *)handle
{
    if (currentURL == nil) {
        return;
    }
    
    LOG(Loading, "URL = %@", [handle URL]);
    
    // Calling receivedError will likely result in a call to release, so we must retain.
    [self retain];
    
    // FIXME: Maybe we should be passing the URL from the handle here, not from the dataSource.
    WebError *error = [[WebError alloc] initWithErrorCode:WebResultCancelled 
        inDomain:WebErrorDomainWebFoundation failingURL:[[dataSource originalURL] absoluteString]];
    [self receivedError:error forHandle:handle];
    [error release];
    
    [downloadHandler cancel];
    [downloadHandler release];
    downloadHandler = nil;

    [self didStopLoading];
    
    [self release];
}

- (void)handleDidFinishLoading:(WebResourceHandle *)handle
{
    LOG(Loading, "URL = %@", [handle URL]);
    
    ASSERT([currentURL isEqual:[handle URL]]);
    ASSERT([[handle response] statusCode] == WebResourceHandleStatusLoadComplete);

    // Calling receivedError will likely result in a call to release, so we must retain.
    [self retain];

    WebContentAction contentAction = [[dataSource contentPolicy] policyAction];
    
    // Don't retain data for downloaded files
    if(contentAction != WebContentPolicySave && contentAction != WebContentPolicySaveAndOpenExternally){
       [dataSource _setResourceData:resourceData];
    }

    if (contentAction == WebContentPolicyShow) {
        [[dataSource representation] finishedLoadingWithDataSource:dataSource];
    }
    
    // Either send a final error message or a final progress message.
    WebError *nonTerminalError = [[handle response] error];
    if (nonTerminalError) {
        [self receivedError:nonTerminalError forHandle:handle];
    } else {
        [self receivedProgressWithHandle:handle complete:YES];
    }
    
    [downloadHandler finishedLoading];
    [downloadHandler release];
    downloadHandler = nil;
    
    [self didStopLoading];
    
    [self release];
}

- (void)handleDidReceiveData:(WebResourceHandle *)handle data:(NSData *)data
{
    WebController *controller = [dataSource controller];
    NSString *contentType = [[handle response] contentType];
    WebFrame *frame = [dataSource webFrame];
    
    LOG(Loading, "URL = %@, data = %p, length %d", [handle URL], data, [data length]);
    
    ASSERT([currentURL isEqual:[handle URL]]);
    
    // Check the mime type and ask the client for the content policy.
    if(isFirstChunk){
        // Make assumption that if the contentType is the default 
        // and there is no extension, this is text/html
        if([contentType isEqualToString:@"application/octet-stream"] && [[[currentURL path] pathExtension] isEqualToString:@""])
            contentType = @"text/html";
        
        [dataSource _setContentType:contentType];
        [dataSource _setEncoding:[[handle response] characterSet]];
        
        // retain the downloadProgressHandler just in case this is a download.
        // Alexander releases the WebController if no window is created for it.
        // This happens in the cases mentioned in 2981866 and 2965312.
        downloadProgressHandler = [[[dataSource controller] downloadProgressHandler] retain];

        WebContentPolicy *contentPolicy = [dataSource contentPolicy];
        if(contentPolicy == nil){
            contentPolicy = [[controller policyHandler] contentPolicyForMIMEType: contentType URL:currentURL inFrame:frame];
            [dataSource _setContentPolicy:contentPolicy];
        }
        policyAction = [contentPolicy policyAction];
        
        LOG(Download, "main content type: %@", contentType);
    }

    switch (policyAction) {
    case WebContentPolicyShow:
        // only need to buffer data in this case
        [resourceData appendData:data];
        [dataSource _receivedData:data];
        break;
    case WebContentPolicySave:
    case WebContentPolicySaveAndOpenExternally:
        if (!downloadHandler) {
            [frame _setProvisionalDataSource:nil];
	    [[[dataSource controller] locationChangeHandler] locationChangeDone:nil forDataSource:dataSource];
            downloadHandler = [[WebDownloadHandler alloc] initWithDataSource:dataSource];
        }
        [downloadHandler receivedData:data];
        break;
    case WebContentPolicyIgnore:
        [handle cancelLoadInBackground];
        [self didCancelWithHandle:handle];
        [frame _setProvisionalDataSource:nil];
	[[[dataSource controller] locationChangeHandler] locationChangeDone:nil forDataSource:dataSource];
        break;
    default:
        [NSException raise:NSInvalidArgumentException format:@"contentPolicyForMIMEType:URL:inFrame: returned an invalid content policy."];
    }

    [self receivedProgressWithHandle:handle complete:NO];
    
    LOG(Download, "%d of %d", [handle contentLengthReceived], [handle contentLength]);
    isFirstChunk = NO;
}

- (void)handleDidFailLoading:(WebResourceHandle *)handle withError:(WebError *)result
{
    LOG(Loading, "URL = %@, result = %@", [handle URL], [result errorDescription]);

    ASSERT([currentURL isEqual:[handle URL]]);

    // Calling receivedError will likely result in a call to release, so we must retain.
    [self retain];

    [self receivedError:result forHandle:handle];
    
    [downloadHandler cancel];
    [downloadHandler release];
    downloadHandler = nil;

    [self didStopLoading];
    
    [self release];
}

- (void)handleDidRedirect:(WebResourceHandle *)handle toURL:(NSURL *)URL
{
    LOG(Redirect, "URL = %@", URL);

    ASSERT(currentURL != nil);
    ASSERT([URL isEqual:[handle URL]]);
    
    [dataSource _setURL:URL];

    [self didStopLoading];
    [self didStartLoadingWithURL:URL];
}

@end
