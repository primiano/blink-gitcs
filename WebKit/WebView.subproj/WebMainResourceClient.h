/*	
    WebMainResourceClient.h

    Private header.
    
    Copyright 2001, 2002 Apple Computer Inc. All rights reserved.
*/

#import <Foundation/Foundation.h>

#import <WebKit/WebControllerPolicyHandler.h>

@class WebDownloadHandler;
@class WebDataSource;
@class WebResourceHandle;
@protocol WebResourceClient;
@protocol WebResourceProgressHandler;

@interface WebMainResourceClient : NSObject <WebResourceClient>
{
    NSURL *currentURL;
    WebDataSource *dataSource;
    BOOL isFirstChunk;
    BOOL suppressErrors;
    WebDownloadHandler *downloadHandler;
    id <WebResourceProgressHandler> downloadProgressHandler;
    WebContentAction policyAction;
    NSMutableData *resourceData;
}

- initWithDataSource:(WebDataSource *)dataSource;
- (WebDownloadHandler *)downloadHandler;
- (NSData *)resourceData;

- (void)didStartLoadingWithURL:(NSURL *)URL;
- (void)didCancelWithHandle:(WebResourceHandle *)handle;

@end
