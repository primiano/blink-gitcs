/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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

#import <Cocoa/Cocoa.h>

#import <WebKit/WebFramePrivate.h>

#import <WebKit/WebDocumentLoadState.h>

@class WebArchive;
@class WebDataSource;
@class WebMainResourceLoader;
@class WebLoader;
@class WebResource;
@class WebFrame;
@class WebPolicyDecisionListener;

typedef enum {
    WebFrameStateProvisional,
    
    // This state indicates we are ready to commit to a page,
    // which means the view will transition to use the new data source.
    WebFrameStateCommittedPage,
    
    WebFrameStateComplete
} WebFrameState;

typedef enum {
    WebPolicyUse,
    WebPolicyDownload,
    WebPolicyIgnore,
} WebPolicyAction;

@interface WebFrameLoader : NSObject
{
@public
    WebMainResourceLoader *mainResourceLoader;
    
    NSMutableArray *subresourceLoaders;
    NSMutableArray *plugInStreamLoaders;
    
    WebFrame *webFrame;
    WebDocumentLoadState *documentLoadState;
    WebDocumentLoadState *provisionalDocumentLoadState;
    WebDocumentLoadState *policyDocumentLoadState;
        
    WebFrameState state;
    
    WebPolicyDecisionListener *listener;
    
    NSMutableDictionary *pendingArchivedResources;
}

- (id)initWithWebFrame:(WebFrame *)wf;
- (void)addPlugInStreamLoader:(WebLoader *)loader;
- (void)removePlugInStreamLoader:(WebLoader *)loader;
- (void)setDefersCallbacks:(BOOL)defers;
- (void)stopLoadingPlugIns;
- (BOOL)isLoadingMainResource;
- (BOOL)isLoadingSubresources;
- (BOOL)isLoading;
- (void)stopLoadingSubresources;
- (void)addSubresourceLoader:(WebLoader *)loader;
- (void)removeSubresourceLoader:(WebLoader *)loader;
- (NSData *)mainResourceData;
- (void)releaseMainResourceLoader;
- (void)cancelMainResourceLoad;
- (BOOL)startLoadingMainResourceWithRequest:(NSMutableURLRequest *)request identifier:(id)identifier;
- (void)stopLoadingWithError:(NSError *)error;
- (void)clearProvisionalLoad;
- (void)stopLoading;
- (void)markLoadComplete;
- (void)commitProvisionalLoad;
- (void)startLoading;
- (void)startProvisionalLoad:(WebDataSource *)dataSource;
- (WebDataSource *)dataSource;
- (WebDataSource *)provisionalDataSource;
- (WebDataSource *)policyDataSource;
- (WebDataSource *)activeDataSource;
- (WebDocumentLoadState *)activeDocumentLoadState;
- (WebDocumentLoadState *)documentLoadState;
- (WebDocumentLoadState *)provisionalDocumentLoadState;
- (void)_setPolicyDocumentLoadState:(WebDocumentLoadState *)loadState;
- (WebFrameState)state;
- (void)clearDataSource;
- (void)setupForReplace;
+ (CFAbsoluteTime)timeOfLastCompletedLoad;

- (WebResource *)_archivedSubresourceForURL:(NSURL *)URL;
- (BOOL)defersCallbacks;
- (void)defersCallbacksChanged;
- (id)_identifierForInitialRequest:(NSURLRequest *)clientRequest;
- (NSURLRequest *)_willSendRequest:(NSMutableURLRequest *)clientRequest forResource:(id)identifier redirectResponse:(NSURLResponse *)redirectResponse;
- (void)_didReceiveAuthenticationChallenge:(NSURLAuthenticationChallenge *)currentWebChallenge forResource:(id)identifier;
- (void)_didCancelAuthenticationChallenge:(NSURLAuthenticationChallenge *)currentWebChallenge forResource:(id)identifier;
- (void)_didReceiveResponse:(NSURLResponse *)r forResource:(id)identifier;
- (void)_didReceiveData:(NSData *)data contentLength:(int)lengthReceived forResource:(id)identifier;
- (void)_didFinishLoadingForResource:(id)identifier;
- (void)_didFailLoadingWithError:(NSError *)error forResource:(id)identifier;
- (BOOL)_privateBrowsingEnabled;
- (void)_didFailLoadingWithError:(NSError *)error forResource:(id)identifier;
- (void)_finishedLoadingResource;
- (void)_receivedError:(NSError *)error;
- (NSURLRequest *)_originalRequest;
- (WebFrame *)webFrame;
- (void)_receivedMainResourceError:(NSError *)error complete:(BOOL)isComplete;
- (NSURLRequest *)initialRequest;
- (void)_receivedData:(NSData *)data;
- (void)_setRequest:(NSURLRequest *)request;
- (void)_downloadWithLoadingConnection:(NSURLConnection *)connection request:(NSURLRequest *)request response:(NSURLResponse *)r proxy:(id)proxy;
- (void)_handleFallbackContent;
- (BOOL)_isStopping;
- (void)_setupForReplaceByMIMEType:(NSString *)newMIMEType;
- (void)_setResponse:(NSURLResponse *)response;
- (void)_mainReceivedError:(NSError *)error complete:(BOOL)isComplete;
- (void)_finishedLoading;
- (NSURL *)_URL;

- (NSError *)cancelledErrorWithRequest:(NSURLRequest *)request;
- (NSError *)fileDoesNotExistErrorWithResponse:(NSURLResponse *)response;
- (BOOL)willUseArchiveForRequest:(NSURLRequest *)r originalURL:(NSURL *)originalURL loader:(WebLoader *)loader;
- (BOOL)archiveLoadPendingForLoader:(WebLoader *)loader;
- (void)deliverArchivedResourcesAfterDelay;
- (void)cancelPendingArchiveLoadForLoader:(WebLoader *)loader;
- (void)clearArchivedResources;
- (void)_addExtraFieldsToRequest:(NSMutableURLRequest *)request mainResource:(BOOL)mainResource alwaysFromRequest:(BOOL)f;
- (void)cannotShowMIMETypeForURL:(NSURL *)URL;
- (NSError *)interruptForPolicyChangeErrorWithRequest:(NSURLRequest *)request;
- (BOOL)isHostedByObjectElement;
- (BOOL)isLoadingMainFrame;
+ (BOOL)_canShowMIMEType:(NSString *)MIMEType;
+ (BOOL)_representationExistsForURLScheme:(NSString *)URLScheme;
+ (NSString *)_generatedMIMETypeForURLScheme:(NSString *)URLScheme;
- (void)_notifyIconChanged:(NSURL *)iconURL;
- (void)_checkNavigationPolicyForRequest:(NSURLRequest *)newRequest andCall:(id)obj withSelector:(SEL)sel;
- (void)_checkContentPolicyForMIMEType:(NSString *)MIMEType andCall:(id)obj withSelector:(SEL)sel;
- (void)cancelContentPolicy;
- (void)_loadRequest:(NSURLRequest *)request archive:(WebArchive *)archive;
- (void)reload;
- (void)_reloadAllowingStaleDataWithOverrideEncoding:(NSString *)encoding;
- (void)_loadRequest:(NSURLRequest *)request triggeringAction:(NSDictionary *)action loadType:(WebFrameLoadType)loadType formState:(WebFormState *)formState;

- (void)didReceiveServerRedirectForProvisionalLoadForFrame;
- (WebFrameBridge *)bridge;
- (void)finishedLoadingDocumentLoadState:(WebDocumentLoadState *)loadState;
- (void)commitProvisitionalLoad;
- (void)committedLoadWithDocumentLoadState:(WebDocumentLoadState *)loadState data:(NSData *)data;
- (BOOL)isReplacing;
- (void)setReplacing;
- (void)revertToProvisionalWithDocumentLoadState:(WebDocumentLoadState *)loadState;
- (void)documentLoadState:(WebDocumentLoadState *)loadState setMainDocumentError:(NSError *)error;
- (void)documentLoadState:(WebDocumentLoadState *)loadState mainReceivedCompleteError:(NSError *)error;
- (void)finalSetupForReplaceWithDocumentLoadState:(WebDocumentLoadState *)loadState;

@end
