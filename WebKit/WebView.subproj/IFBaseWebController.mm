/*	
    IFBaseWebController.mm
	Copyright 2001, 2002 Apple, Inc. All rights reserved.
*/
#import <WebKit/IFBaseWebController.h>
#import <WebKit/IFBaseWebControllerPrivate.h>
#import <WebKit/IFWebViewPrivate.h>
#import <WebKit/IFWebDataSourcePrivate.h>
#import <WebKit/IFWebFrame.h>
#import <WebKit/IFWebFramePrivate.h>
#import <WebKit/IFDynamicScrollBarsView.h>
#import <WebKit/IFException.h>
#import <WebKit/IFWebController.h>
#import <WebKit/WebKitDebug.h>

#import <WebFoundation/IFError.h>

@implementation IFBaseWebController

- init
{
    return [self initWithView: nil provisionalDataSource: nil];
}

- initWithView: (IFWebView *)view provisionalDataSource: (IFWebDataSource *)dataSource
{
    [super init];
    
    _private = [[IFBaseWebControllerPrivate alloc] init];
    _private->mainFrame = [[IFWebFrame alloc] initWithName: @"_top" view: view provisionalDataSource: dataSource controller: self];

    return self;
}

- (void)dealloc
{
    [_private release];
    [super dealloc];
}

- (void)setDirectsAllLinksToSystemBrowser: (BOOL)flag
{
    [NSException raise:IFMethodNotYetImplemented format:@"IFBaseWebController::setDirectsAllLinksToSystemBrowser: is not implemented"];
}

- (BOOL)directsAllLinksToSystemBrowser
{
    [NSException raise:IFMethodNotYetImplemented format:@"IFBaseWebController::directsAllLinksToSystemBrowser is not implemented"];
    return NO;
}

- (IFWebFrame *)createFrameNamed: (NSString *)fname for: (IFWebDataSource *)childDataSource inParent: (IFWebDataSource *)parentDataSource inScrollView: (BOOL)inScrollView
{
    IFWebView *childView;
    IFWebFrame *newFrame;
    NSScrollView *scrollView;

    childView = [[[IFWebView alloc] initWithFrame: NSMakeRect (0,0,0,0)] autorelease];

    newFrame = [[[IFWebFrame alloc] initWithName: fname view: childView provisionalDataSource: childDataSource controller: self] autorelease];

    [parentDataSource addFrame: newFrame];

    [childView _setController: self];
    [childDataSource _setController: self];

    if (inScrollView == YES){
        scrollView  = [[[IFDynamicScrollBarsView alloc] initWithFrame: NSMakeRect(0,0,0,0)] autorelease];
        [scrollView setHasVerticalScroller: NO];
        [scrollView setHasHorizontalScroller: NO];
        [childView _setFrameScrollView: scrollView];
    }
        
    return newFrame;
}



// ---------------------------------------------------------------------
// IFScriptContextHandler
// ---------------------------------------------------------------------
- (void)setStatusText: (NSString *)text forDataSource: (IFWebDataSource *)dataSource
{
    [NSException raise:IFMethodNotYetImplemented format:@"IFBaseWebController::setStatusText:forDataSource: is not implemented"];
}


- (NSString *)statusTextForDataSource: (IFWebDataSource *)dataSource
{
    [NSException raise:IFMethodNotYetImplemented format:@"IFBaseWebController::statusTextForDataSource: is not implemented"];
    return nil;
}



// ---------------------------------------------------------------------
// IFLoadHandler
// ---------------------------------------------------------------------
- (void)receivedProgress: (IFLoadProgress *)progress forResource: (NSString *)resourceDescription fromDataSource: (IFWebDataSource *)dataSource
{
    // Do nothing.  Subclasses typically override this method.
}

- (void)receivedError: (IFError *)error forResource: (NSString *)resourceDescription partialProgress: (IFLoadProgress *)progress fromDataSource: (IFWebDataSource *)dataSource
{
    // Do nothing.  Subclasses typically override this method.
}

// ---------------------------------------------------------------------
// IFLocationChangeHandler
// ---------------------------------------------------------------------
- (id <IFLocationChangeHandler>)provideLocationChangeHandlerForFrame: (IFWebFrame *)frame
{
    return nil;
}


- (void)receivedPageTitle: (NSString *)title forDataSource: (IFWebDataSource *)dataSource
{
    // Do nothing.  Subclasses typically override this method.
}


- (void)serverRedirectTo: (NSURL *)url forDataSource: (IFWebDataSource *)dataSource
{
    // Do nothing.  Subclasses typically override this method.
}

- (IFWebFrame *)_frameForDataSource: (IFWebDataSource *)dataSource fromFrame: (IFWebFrame *)frame
{
    NSArray *frames;
    int i, count;
    IFWebFrame *result, *aFrame;
    
    if ([frame dataSource] == dataSource)
        return frame;
        
    if ([frame provisionalDataSource] == dataSource)
        return frame;
        
    frames = [[frame dataSource] children];
    count = [frames count];
    for (i = 0; i < count; i++){
        aFrame = [frames objectAtIndex: i];
        result = [self _frameForDataSource: dataSource fromFrame: aFrame];
        if (result)
            return result;
    }

    frames = [[frame provisionalDataSource] children];
    count = [frames count];
    for (i = 0; i < count; i++){
        aFrame = [frames objectAtIndex: i];
        result = [self _frameForDataSource: dataSource fromFrame: aFrame];
        if (result)
            return result;
    }
    
    return nil;       
}


- (IFWebFrame *)frameForDataSource: (IFWebDataSource *)dataSource
{
    IFWebFrame *frame = [self mainFrame];
    
    return [self _frameForDataSource: dataSource fromFrame: frame];
}


- (IFWebFrame *)_frameForView: (NSView *)aView fromFrame: (IFWebFrame *)frame
{
    NSArray *frames;
    int i, count;
    IFWebFrame *result, *aFrame;
    
    if ([frame view] == aView)
        return frame;
        
    frames = [[frame dataSource] children];
    count = [frames count];
    for (i = 0; i < count; i++){
        aFrame = [frames objectAtIndex: i];
        result = [self _frameForView: aView fromFrame: aFrame];
        if (result)
            return result;
    }

    frames = [[frame provisionalDataSource] children];
    count = [frames count];
    for (i = 0; i < count; i++){
        aFrame = [frames objectAtIndex: i];
        result = [self _frameForView: aView fromFrame: aFrame];
        if (result)
            return result;
    }
    
    return nil;       
}


- (IFWebFrame *)frameForView: (NSView *)aView
{
    IFWebFrame *frame = [self mainFrame];
    
    return [self _frameForView: aView fromFrame: frame];
}


- (IFWebFrame *)_frameNamed: (NSString *)name fromFrame: (IFWebFrame *)frame
{
    if ([[frame name] isEqual: name])
        return frame;

    int i, count;
    IFWebFrame *aFrame;
    NSArray *children = [[frame dataSource] children];
    count = [children count];
    for (i = 0; i < count; i++){
        aFrame = [children objectAtIndex: i];
        if ([self _frameNamed: name fromFrame: aFrame])
            return aFrame;
    }
    return nil;
}

- (IFWebFrame *)frameNamed: (NSString *)name
{
    return [self _frameNamed: name fromFrame: [self mainFrame]];
}

- (IFWebFrame *)mainFrame
{
    return _private->mainFrame;
}

- (void)pluginNotFoundForMIMEType:(NSString *)mime pluginPageURL:(NSURL *)url
{
    // Do nothing.  Subclasses typically override this method.
}

- (id <IFLocationChangeHandler>)provideLocationChangeHandlerForFrame: (IFWebFrame *)frame andURL: (NSURL *)url
{
    return nil;
}

- (IFURLPolicy)URLPolicyForURL: (NSURL *)url
{
    // We need a factory method or something that tells us what schemes we handle
    if([[url scheme] isEqualToString:@"http"] || [[url scheme] isEqualToString:@"https"] || [[url scheme] isEqualToString:@"file"]){
        return IFURLPolicyUseContentPolicy;
    }else{
        return IFURLPolicyOpenExternally;
    }
}

- (void)unableToImplementURLPolicyForURL: (NSURL *)url error: (IFError *)error
{
}


- (void)haveContentPolicy: (IFContentPolicy)policy andPath: (NSString *)path forLocationChangeHandler: (id <IFLocationChangeHandler>)handler
{
    IFWebDataSource *mainDataSource, *mainProvisionalDataSource, *dataSource;
    
    mainProvisionalDataSource = [_private->mainFrame provisionalDataSource];
    mainDataSource = [_private->mainFrame dataSource];
    
    dataSource = [mainDataSource _recursiveDataSourceForLocationChangeHandler:handler];
    if(!dataSource)
        dataSource = [mainProvisionalDataSource _recursiveDataSourceForLocationChangeHandler:handler];
        
    if(dataSource){
        [dataSource _setContentPolicy:policy];
        [dataSource _setDownloadPath:path];
    }
}

@end
