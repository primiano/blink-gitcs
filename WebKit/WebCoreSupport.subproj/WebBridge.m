/*	
    WebBridge.mm
    Copyright (c) 2002, Apple, Inc. All rights reserved.
*/

#import <WebKit/WebBridge.h>

#import <WebKit/WebBackForwardList.h>
#import <WebKit/WebControllerPrivate.h>
#import <WebKit/WebDataSourcePrivate.h>
#import <WebKit/WebFramePrivate.h>
#import <WebKit/WebHistoryItem.h>
#import <WebKit/WebHTMLRepresentationPrivate.h>
#import <WebKit/WebHTMLViewPrivate.h>
#import <WebKit/WebKitDebug.h>
#import <WebKit/WebKitStatisticsPrivate.h>
#import <WebKit/WebLoadProgress.h>
#import <WebKit/WebSubresourceClient.h>
#import <WebKit/WebViewPrivate.h>

#import <WebFoundation/WebNSStringExtras.h>
#import <WebFoundation/WebResourceHandle.h>

@interface NSApplication (DeclarationStolenFromAppKit)
- (void)_cycleWindowsReversed:(BOOL)reversed;
@end

@implementation WebBridge

- init
{
    ++WebBridgeCount;
    
    return [super init];
}

- (void)dealloc
{
    --WebBridgeCount;
    
    [super dealloc];
}

- (WebCoreBridge *)parent
{
    return [[[self dataSource] parent] _bridge];
}

- (NSArray *)childFrames
{
    NSArray *frames = [[self dataSource] children];
    NSEnumerator *e = [frames objectEnumerator];
    NSMutableArray *frameBridges = [NSMutableArray arrayWithCapacity:[frames count]];
    WebFrame *childFrame;
    while ((childFrame = [e nextObject])) {
        id frameBridge = [childFrame _bridge];
        if (frameBridge)
            [frameBridges addObject:frameBridge];
    }
    return frameBridges;
}

- (WebCoreBridge *)descendantFrameNamed:(NSString *)name
{
    WEBKIT_ASSERT(frame != nil);
    return [[frame frameNamed:name] _bridge];
}

- (BOOL)createChildFrameNamed:(NSString *)frameName
    withURL:(NSURL *)URL renderPart:(KHTMLRenderPart *)childRenderPart
    allowsScrolling:(BOOL)allowsScrolling marginWidth:(int)width marginHeight:(int)height
{
    WEBKIT_ASSERT(frame != nil);
    WebFrame *newFrame = [[frame controller] createFrameNamed:frameName for:nil inParent:[self dataSource] allowsScrolling:allowsScrolling];
    if (newFrame == nil) {
        return NO;
    }
    
    [[newFrame _bridge] setRenderPart:childRenderPart];
    
    [[newFrame webView] _setMarginWidth:width];
    [[newFrame webView] _setMarginHeight:height];
    
    [[newFrame _bridge] loadURL:URL attributes:nil flags:0 withParent:[self dataSource]];
    
    // Set the load type so this load doesn't end up in the back
    // forward list.
    [newFrame _setLoadType:WebFrameLoadTypeInternal];

    return YES;
}

- (WebCoreBridge *)openNewWindowWithURL:(NSURL *)url
{
    WEBKIT_ASSERT(frame != nil);

    WebController *newController = [[[frame controller] windowContext] openNewWindowWithURL:url];
    WebFrame *newFrame = [newController mainFrame];

    return [newFrame _bridge];
}

- (BOOL)areToolbarsVisible
{
    WEBKIT_ASSERT(frame != nil);
    return [[[frame controller] windowContext] areToolbarsVisible];
}

- (void)setToolbarsVisible:(BOOL)visible
{
    WEBKIT_ASSERT(frame != nil);
    [[[frame controller] windowContext] setToolbarsVisible:visible];
}

- (BOOL)areScrollbarsVisible
{
    WEBKIT_ASSERT(frame != nil);
    return [[frame webView] allowsScrolling];
}

- (void)setScrollbarsVisible:(BOOL)visible
{
    WEBKIT_ASSERT(frame != nil);
    return [[frame webView] setAllowsScrolling:visible];
}

- (BOOL)isStatusBarVisible
{
    WEBKIT_ASSERT(frame != nil);
    return [[[frame controller] windowContext] isStatusBarVisible];
}

- (void)setStatusBarVisible:(BOOL)visible
{
    WEBKIT_ASSERT(frame != nil);
    [[[frame controller] windowContext] setStatusBarVisible:visible];
}

- (void)setWindowFrame:(NSRect)frameRect
{
    WEBKIT_ASSERT(frame != nil);
    [[[frame controller] windowContext] setFrame:frameRect];
}

- (NSWindow *)window
{
    WEBKIT_ASSERT(frame != nil);
    return [[[frame controller] windowContext] window];
}

- (void)setTitle:(NSString *)title
{
    [[self dataSource] _setTitle:[title _web_stringByCollapsingNonPrintingCharacters]];
}

- (void)setStatusText:(NSString *)status
{
    WEBKIT_ASSERT(frame != nil);
    [[[frame controller] windowContext] setStatusText:status];
}

- (WebCoreBridge *)mainFrame
{
    WEBKIT_ASSERT(frame != nil);
    return [[[frame controller] mainFrame] _bridge];
}

- (WebCoreBridge *)frameNamed:(NSString *)name
{
    WEBKIT_ASSERT(frame != nil);
    return [[[frame controller] frameNamed:name] _bridge];
}

- (void)receivedData:(NSData *)data withDataSource:(WebDataSource *)withDataSource
{
    WEBKIT_ASSERT([self dataSource] == withDataSource);

    [self addData:data withEncoding:[withDataSource encoding]];
}

- (WebResourceHandle *)startLoadingResource:(id <WebCoreResourceLoader>)resourceLoader withURL:(NSURL *)URL
{
    return [WebSubresourceClient startLoadingResource:resourceLoader withURL:URL dataSource:[self dataSource]];
}

- (void)objectLoadedFromCache:(NSURL *)URL size:(unsigned)bytes
{
    WEBKIT_ASSERT(frame != nil);

    WebResourceHandle *handle = [[WebResourceHandle alloc] initWithURL:URL];

    WebLoadProgress *loadProgress = [[WebLoadProgress alloc] initWithBytesSoFar:bytes totalToLoad:bytes];
    [[frame controller] _receivedProgress:loadProgress forResourceHandle:handle fromDataSource:[self dataSource] complete:YES];
    [loadProgress release];
    [handle release];
}

- (void)reportClientRedirectTo:(NSURL *)URL delay:(NSTimeInterval)seconds fireDate:(NSDate *)date
{
    [[[frame controller] locationChangeHandler]
        clientRedirectTo:URL delay:seconds fireDate:date forDataSource:[self dataSource]];
}

- (void)reportClientRedirectCancelled
{
    [[[frame controller] locationChangeHandler]
        clientRedirectCancelledForDataSource:[self dataSource]];
}

- (void)setFrame:(WebFrame *)webFrame
{
    WEBKIT_ASSERT(webFrame != nil);

    if (frame == nil) {
	// FIXME: non-retained because data source owns representation owns bridge
	frame = webFrame;
        [self setTextSizeMultiplier:[[frame controller] textSizeMultiplier]];
    } else {
	WEBKIT_ASSERT(frame == webFrame);
    }
}

- (void)dataSourceChanged
{
    [self openURL:[[self dataSource] redirectedURL] == nil
        ? [[self dataSource] inputURL] : [[self dataSource] redirectedURL]];
}

- (WebDataSource *)dataSource
{
    WEBKIT_ASSERT(frame != nil);
    WebDataSource *dataSource = [frame dataSource];

    WEBKIT_ASSERT(dataSource != nil);
    WEBKIT_ASSERT([dataSource _isCommitted]);

    return dataSource;
}

- (BOOL)openedByScript
{
    WEBKIT_ASSERT(frame != nil);
    return [[frame controller] _openedByScript];
}

- (void)setOpenedByScript:(BOOL)openedByScript
{
    WEBKIT_ASSERT(frame != nil);
    [[frame controller] _setOpenedByScript:openedByScript];
}

- (void)unfocusWindow
{
    if ([[self window] isKeyWindow] || [[[self window] attachedSheet] isKeyWindow]) {
	[NSApp _cycleWindowsReversed:FALSE];
    }
}

- (BOOL)modifierTrackingEnabled
{
    return [WebHTMLView _modifierTrackingEnabled];
}

- (void)setIconURL:(NSURL *)url
{
    [[self dataSource] _setIconURL:url];
}

- (void)setIconURL:(NSURL *)url withType:(NSString *)type
{
    [[self dataSource] _setIconURL:url withType:type];
}

- (void)loadURL:(NSURL *)URL attributes:(NSDictionary *)attributes flags:(unsigned)flags withParent:(WebDataSource *)parent
{
    WebDataSource *newDataSource = [[WebDataSource alloc] initWithURL:URL attributes:attributes flags:flags];
    [newDataSource _setParent:parent];
    if ([frame setProvisionalDataSource:newDataSource]) {
        [frame startLoading];
    }
    [newDataSource release];
}

- (void)loadURL:(NSURL *)URL
{
    [self loadURL:URL attributes:nil flags:0 withParent:[[frame dataSource] parent]];
}

- (void)postWithURL:(NSURL *)URL data:(NSData *)data
{
    // When posting, use the WebResourceHandleFlagLoadFromOrigin load flag. 
    // This prevents a potential bug which may cause a page
    // with a form that uses itself as an action to be returned 
    // from the cache without submitting.
    NSDictionary *attributes = [[NSDictionary alloc] initWithObjectsAndKeys:
        data, WebHTTPResourceHandleRequestData,
        @"POST", WebHTTPResourceHandleRequestMethod,
        nil];
    [self loadURL:URL attributes:attributes flags:WebResourceHandleFlagLoadFromOrigin withParent:[[frame dataSource] parent]];
    [attributes release];
}

- (void)reportError:(WebError *)error
{
    [[frame controller] _receivedError:error
                     forResourceHandle:nil
                       partialProgress:nil
                        fromDataSource:[self dataSource]];
}

- (void)addBackForwardItemWithURL:(NSURL *)url anchor:(NSString *)anchor;
{
    WebHistoryItem *backForwardItem;
    WebFrame *parentFrame = [[frame controller] frameForDataSource:[[frame dataSource] parent]]; 

    backForwardItem = [[WebHistoryItem alloc] initWithURL:url target:[frame name] parent:[parentFrame name] title:[[frame dataSource] pageTitle] image:nil];
    [backForwardItem setAnchor:anchor];
    [[[frame controller] backForwardList] addEntry:backForwardItem];
    [backForwardItem release];
}

@end
