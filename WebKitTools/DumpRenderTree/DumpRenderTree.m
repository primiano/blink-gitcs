/*
 * Copyright (C) 2005, 2006, 2007 Apple, Inc.  All rights reserved.
 *           (C) 2007 Graham Dennis (graham.dennis@gmail.com)
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
 
#import "DumpRenderTree.h"

#import "EditingDelegate.h"
#import "EventSendingController.h"
#import "FrameLoadDelegate.h"
#import "NavigationController.h"
#import "ObjCPlugin.h"
#import "ObjCPluginFunction.h"
#import "ResourceLoadDelegate.h"
#import "UIDelegate.h"
#import <ApplicationServices/ApplicationServices.h> // for CMSetDefaultProfileBySpace
#import <CoreFoundation/CoreFoundation.h>
#import <JavaScriptCore/JavaScriptCore.h>
#import <WebKit/DOMElementPrivate.h>
#import <WebKit/DOMExtensions.h>
#import <WebKit/DOMRange.h>
#import <WebKit/WebBackForwardList.h>
#import <WebKit/WebCoreStatistics.h>
#import <WebKit/WebDataSource.h>
#import <WebKit/WebDocumentPrivate.h>
#import <WebKit/WebEditingDelegate.h>
#import <WebKit/WebFramePrivate.h>
#import <WebKit/WebFrameView.h>
#import <WebKit/WebHTMLViewPrivate.h>
#import <WebKit/WebHistory.h>
#import <WebKit/WebHistoryItemPrivate.h>
#import <WebKit/WebNSURLExtras.h>
#import <WebKit/WebPluginDatabase.h>
#import <WebKit/WebPreferences.h>
#import <WebKit/WebPreferencesPrivate.h>
#import <WebKit/WebResourceLoadDelegate.h>
#import <WebKit/WebViewPrivate.h>
#import <JavaScriptCore/Assertions.h>
#import <getopt.h>
#import <mach-o/getsect.h>
#import <malloc/malloc.h>
#import <objc/objc-runtime.h>                       // for class_poseAs
#import <pthread.h>

#define COMMON_DIGEST_FOR_OPENSSL
#import <CommonCrypto/CommonDigest.h>               // for MD5 functions

@interface DumpRenderTreeWindow : NSWindow
@end

@interface DumpRenderTreePasteboard : NSPasteboard
- (int)declareType:(NSString *)type owner:(id)newOwner;
@end

@interface DumpRenderTreeEvent : NSEvent
@end

@interface LocalPasteboard : NSPasteboard
{
    NSMutableArray *typesArray;
    NSMutableSet *typesSet;
    NSMutableDictionary *dataByType;
    int changeCount;
}
@end

BOOL windowIsKey = YES;
WebFrame *mainFrame = 0;
BOOL shouldDumpEditingCallbacks;
BOOL shouldDumpResourceLoadCallbacks;
BOOL shouldDumpFrameLoadCallbacks;
NSMutableSet *disallowedURLs = 0;
BOOL waitToDump;     // TRUE if waitUntilDone() has been called, but notifyDone() has not yet been called
BOOL canOpenWindows;
BOOL closeWebViews;
BOOL closeRemainingWindowsWhenComplete = YES;
BOOL addFileToPasteboardOnDrag = NO;

static void runTest(const char *pathOrURL);
static NSString *md5HashStringForBitmap(CGImageRef bitmap);
static void displayWebView();

volatile BOOL done;
NavigationController *navigationController = nil;

// Delegates
static FrameLoadDelegate *frameLoadDelegate;
static UIDelegate *uiDelegate;
static EditingDelegate *editingDelegate;
static ResourceLoadDelegate *resourceLoadDelegate;

// Deciding when it's OK to dump out the state is a bit tricky.  All these must be true:
// - There is no load in progress
// - There is no work queued up (see workQueue var, below)
// - waitToDump==NO.  This means either waitUntilDone was never called, or it was called
//       and notifyDone was called subsequently.
// Note that the call to notifyDone and the end of the load can happen in either order.

// This is the topmost frame that is loading, during a given load, or nil when no load is 
// in progress.  Usually this is the same as the main frame, but not always.  In the case
// where a frameset is loaded, and then new content is loaded into one of the child frames,
// that child frame is the "topmost frame that is loading".
WebFrame *topLoadingFrame = nil;     // !nil iff a load is in progress

static BOOL dumpAsText;
static BOOL dumpDOMAsWebArchive;
static BOOL dumpSourceAsWebArchive;
static BOOL dumpSelectionRect;
BOOL dumpTitleChanges = NO;
static BOOL dumpBackForwardList;
static BOOL dumpChildFrameScrollPositions;
static int dumpPixels;
static int paint;
static int dumpAllPixels;
static int threaded;
static BOOL readFromWindow;
static int testRepaintDefault;
static BOOL testRepaint;
static int repaintSweepHorizontallyDefault;
static BOOL repaintSweepHorizontally;
static int dumpTree = YES;
static BOOL printSeparators;
static NSString *currentTest = nil;
static NSMutableDictionary *localPasteboards;
static WebHistoryItem *prevTestBFItem = nil;  // current b/f item at the end of the previous test
static unsigned char* screenCaptureBuffer;
static CGColorSpaceRef sharedColorSpace;
// a queue of NSInvocations, queued by callouts from the test, to be exec'ed when the load is done
NSMutableArray *workQueue = nil;
// to prevent infinite loops, only the first page of a test can add to a work queue
// (since we may well come back to that same page)
BOOL workQueueFrozen = NO;
const unsigned maxViewHeight = 600;
const unsigned maxViewWidth = 800;

static CFMutableArrayRef allWindowsRef;

static pthread_mutex_t javaScriptThreadsMutex = PTHREAD_MUTEX_INITIALIZER;
static BOOL javaScriptThreadsShouldTerminate;

static const int javaScriptThreadsCount = 4;
static CFMutableDictionaryRef javaScriptThreads()
{
    assert(pthread_mutex_trylock(&javaScriptThreadsMutex) == EBUSY);
    static CFMutableDictionaryRef staticJavaScriptThreads;
    if (!staticJavaScriptThreads)
        staticJavaScriptThreads = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, NULL, NULL);
    return staticJavaScriptThreads;
}

// Loops forever, running a script and randomly respawning, until 
// javaScriptThreadsShouldTerminate becomes true.
void* runJavaScriptThread(void* arg)
{
    const char* const script =
        "var array = [];"
        "for (var i = 0; i < 10; i++) {"
        "    array.push(String(i));"
        "}";

    while(1) {
        JSGlobalContextRef ctx = JSGlobalContextCreate(NULL);
        JSStringRef scriptRef = JSStringCreateWithUTF8CString(script);

        JSValueRef exception = NULL;
        JSEvaluateScript(ctx, scriptRef, NULL, NULL, 0, &exception);
        assert(!exception);
        
        JSGlobalContextRelease(ctx);
        JSStringRelease(scriptRef);
        
        JSGarbageCollect(ctx);

        pthread_mutex_lock(&javaScriptThreadsMutex);

        // Check for cancellation.
        if (javaScriptThreadsShouldTerminate) {
            pthread_mutex_unlock(&javaScriptThreadsMutex);
            return 0;
        }

        // Respawn probabilistically.
        if (random() % 5 == 0) {
            pthread_t pthread;
            pthread_create(&pthread, NULL, &runJavaScriptThread, NULL);
            pthread_detach(pthread);

            CFDictionaryRemoveValue(javaScriptThreads(), pthread_self());
            CFDictionaryAddValue(javaScriptThreads(), pthread, NULL);

            pthread_mutex_unlock(&javaScriptThreadsMutex);
            return 0;
        }

        pthread_mutex_unlock(&javaScriptThreadsMutex);
    }
}

static void startJavaScriptThreads(void)
{
    pthread_mutex_lock(&javaScriptThreadsMutex);

    for (int i = 0; i < javaScriptThreadsCount; i++) {
        pthread_t pthread;
        pthread_create(&pthread, NULL, &runJavaScriptThread, NULL);
        pthread_detach(pthread);
        CFDictionaryAddValue(javaScriptThreads(), pthread, NULL);
    }

    pthread_mutex_unlock(&javaScriptThreadsMutex);
}

static void stopJavaScriptThreads(void)
{
    pthread_mutex_lock(&javaScriptThreadsMutex);

    javaScriptThreadsShouldTerminate = YES;

    const pthread_t pthreads[javaScriptThreadsCount];
    assert(CFDictionaryGetCount(javaScriptThreads()) == javaScriptThreadsCount);
    CFDictionaryGetKeysAndValues(javaScriptThreads(), (const void**)pthreads, NULL);

    pthread_mutex_unlock(&javaScriptThreadsMutex);

    for (int i = 0; i < javaScriptThreadsCount; i++) {
        pthread_t pthread = pthreads[i];
        pthread_join(pthread, NULL);
    }
}

static BOOL shouldIgnoreWebCoreNodeLeaks(CFStringRef URLString)
{
    static CFStringRef const ignoreSet[] = {
        // Keeping this infrastructure around in case we ever need it again.
    };
    static const int ignoreSetCount = sizeof(ignoreSet) / sizeof(CFStringRef);
    
    for (int i = 0; i < ignoreSetCount; i++) {
        CFStringRef ignoreString = ignoreSet[i];
        CFRange range = CFRangeMake(0, CFStringGetLength(URLString));
        CFOptionFlags flags = kCFCompareAnchored | kCFCompareBackwards | kCFCompareCaseInsensitive;
        if (CFStringFindWithOptions(URLString, ignoreString, range, flags, NULL))
            return YES;
    }
    return NO;
}

static CMProfileRef currentColorProfile = 0;
static void restoreColorSpace(int ignored)
{
    if (currentColorProfile) {
        int error = CMSetDefaultProfileByUse(cmDisplayUse, currentColorProfile);
        if (error)
            fprintf(stderr, "Failed to retore previous color profile!  You may need to open System Preferences : Displays : Color and manually restore your color settings.  (Error: %i)", error);
        currentColorProfile = 0;
    }
}

static void crashHandler(int sig)
{
    fprintf(stderr, "%s\n", strsignal(sig));
    restoreColorSpace(0);
    exit(128 + sig);
}

static void activateAhemFont(void)
{    
    unsigned long fontDataLength;
    char* fontData = getsectdata("__DATA", "Ahem", &fontDataLength);
    if (!fontData) {
        fprintf(stderr, "Failed to locate the Ahem font.\n");
        exit(1);
    }

    ATSFontContainerRef fontContainer;
    OSStatus status = ATSFontActivateFromMemory(fontData, fontDataLength, kATSFontContextLocal, kATSFontFormatUnspecified, NULL, kATSOptionFlagsDefault, &fontContainer);

    if (status != noErr) {
        fprintf(stderr, "Failed to activate the Ahem font.\n");
        exit(1);
    }
}

static void setDefaultColorProfileToRGB(void)
{
    CMProfileRef genericProfile = [[NSColorSpace genericRGBColorSpace] colorSyncProfile];
    CMProfileRef previousProfile;
    int error = CMGetDefaultProfileByUse(cmDisplayUse, &previousProfile);
    if (error) {
        fprintf(stderr, "Failed to get current color profile.  I will not be able to restore your current profile, thus I'm not changing it.  Many pixel tests may fail as a result.  (Error: %i)\n", error);
        return;
    }
    if (previousProfile == genericProfile)
        return;
    CFStringRef previousProfileName;
    CFStringRef genericProfileName;
    char previousProfileNameString[1024];
    char genericProfileNameString[1024];
    CMCopyProfileDescriptionString(previousProfile, &previousProfileName);
    CMCopyProfileDescriptionString(genericProfile, &genericProfileName);
    CFStringGetCString(previousProfileName, previousProfileNameString, sizeof(previousProfileNameString), kCFStringEncodingUTF8);
    CFStringGetCString(genericProfileName, genericProfileNameString, sizeof(previousProfileNameString), kCFStringEncodingUTF8);
    CFRelease(genericProfileName);
    CFRelease(previousProfileName);
    
    fprintf(stderr, "\n\nWARNING: Temporarily changing your system color profile from \"%s\" to \"%s\".\n", previousProfileNameString, genericProfileNameString);
    fprintf(stderr, "This allows the WebKit pixel-based regression tests to have consistent color values across all machines.\n");
    fprintf(stderr, "The colors on your screen will change for the duration of the testing.\n\n");
    
    if ((error = CMSetDefaultProfileByUse(cmDisplayUse, genericProfile)))
        fprintf(stderr, "Failed to set color profile to \"%s\"! Many pixel tests will fail as a result.  (Error: %i)",
            genericProfileNameString, error);
    else {
        currentColorProfile = previousProfile;
        signal(SIGINT, restoreColorSpace);
        signal(SIGHUP, restoreColorSpace);
        signal(SIGTERM, restoreColorSpace);
    }
}

static void* (*savedMalloc)(malloc_zone_t*, size_t);
static void* (*savedRealloc)(malloc_zone_t*, void*, size_t);

static void* checkedMalloc(malloc_zone_t* zone, size_t size)
{
    if (size >= 0x10000000)
        return 0;
    return savedMalloc(zone, size);
}

static void* checkedRealloc(malloc_zone_t* zone, void* ptr, size_t size)
{
    if (size >= 0x10000000)
        return 0;
    return savedRealloc(zone, ptr, size);
}

static void makeLargeMallocFailSilently(void)
{
    malloc_zone_t* zone = malloc_default_zone();
    savedMalloc = zone->malloc;
    savedRealloc = zone->realloc;
    zone->malloc = checkedMalloc;
    zone->realloc = checkedRealloc;
}

WebView *createWebView()
{
    NSRect rect = NSMakeRect(0, 0, maxViewWidth, maxViewHeight);
    WebView *webView = [[WebView alloc] initWithFrame:rect];
        
    [webView setUIDelegate:uiDelegate];
    [webView setFrameLoadDelegate:frameLoadDelegate];
    [webView setEditingDelegate:editingDelegate];
    [webView setResourceLoadDelegate:resourceLoadDelegate];
    
    // The back/forward cache is causing problems due to layouts during transition from one page to another.
    // So, turn it off for now, but we might want to turn it back on some day.
    [[webView backForwardList] setPageCacheSize:0];
    
    [webView setContinuousSpellCheckingEnabled:YES];
    
    // To make things like certain NSViews, dragging, and plug-ins work, put the WebView a window, but put it off-screen so you don't see it.
    // Put it at -10000, -10000 in "flipped coordinates", since WebCore and the DOM use flipped coordinates.
    NSRect windowRect = NSOffsetRect(rect, -10000, [[[NSScreen screens] objectAtIndex:0] frame].size.height - rect.size.height + 10000);
    NSWindow *window = [[NSWindow alloc] initWithContentRect:windowRect styleMask:NSBorderlessWindowMask backing:NSBackingStoreBuffered defer:YES];
    [[window contentView] addSubview:webView];
    [window orderBack:nil];
    [window setAutodisplay:NO];
    
    // For reasons that are not entirely clear, the following pair of calls makes WebView handle its
    // dynamic scrollbars properly. Without it, every frame will always have scrollbars.
    NSBitmapImageRep *imageRep = [webView bitmapImageRepForCachingDisplayInRect:[webView bounds]];
    [webView cacheDisplayInRect:[webView bounds] toBitmapImageRep:imageRep];
        
    return webView;
}

void testStringByEvaluatingJavaScriptFromString()
{
    // maps expected result <= JavaScript expression
    NSDictionary *expressions = [NSDictionary dictionaryWithObjectsAndKeys:
        @"", @"", 
        @"", @"''", 
        @"", @"new String()", 
        @"", @"throw 1", 
        @"", @"{ }", 
        @"", @"[ ]", 
        @"", @"//", 
        @"", @"a.b.c", 
        @"", @"(function() { throw 'error'; })()", 
        @"0", @"new String('0')", 
        @"0", @"0", 
        @"0", @"'0'", 
        @"true", @"true",
        @"false", @"false",
        @"null", @"null",
        @"undefined", @"undefined",
        nil
    ];

    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    WebView *webView = [[WebView alloc] initWithFrame:NSZeroRect frameName:@"" groupName:@""];

    NSEnumerator *enumerator = [expressions keyEnumerator];
    id expression;
    while ((expression = [enumerator nextObject])) {
        NSString *expectedResult = [expressions objectForKey:expression];
        NSString *result = [webView stringByEvaluatingJavaScriptFromString:expression];
        assert([result isEqualToString:expectedResult]);
    }

    [webView close];
    [webView release];
    [pool release];
}

void dumpRenderTree(int argc, const char *argv[])
{    
    [NSApplication sharedApplication];

    class_poseAs(objc_getClass("DumpRenderTreePasteboard"), objc_getClass("NSPasteboard"));
    class_poseAs(objc_getClass("DumpRenderTreeWindow"), objc_getClass("NSWindow"));
    class_poseAs(objc_getClass("DumpRenderTreeEvent"), objc_getClass("NSEvent"));

    struct option options[] = {
        {"dump-all-pixels", no_argument, &dumpAllPixels, YES},
        {"horizontal-sweep", no_argument, &repaintSweepHorizontallyDefault, YES},
        {"notree", no_argument, &dumpTree, NO},
        {"pixel-tests", no_argument, &dumpPixels, YES},
        {"paint", no_argument, &paint, YES},
        {"repaint", no_argument, &testRepaintDefault, YES},
        {"tree", no_argument, &dumpTree, YES},
        {"threaded", no_argument, &threaded, YES},
        {NULL, 0, NULL, 0}
    };

    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    [defaults setObject:@"DoubleMax" forKey:@"AppleScrollBarVariant"];
    [defaults setInteger:4 forKey:@"AppleAntiAliasingThreshold"];
    // 2 is the "Medium" font smoothing mode
    [defaults setInteger:2 forKey:@"AppleFontSmoothing"];

    [defaults setInteger:1 forKey:@"AppleAquaColorVariant"];
    [defaults setObject:@"0.709800 0.835300 1.000000" forKey:@"AppleHighlightColor"];
    [defaults setObject:@"0.500000 0.500000 0.500000" forKey:@"AppleOtherHighlightColor"];
    
    [defaults setObject:[NSArray arrayWithObject:@"en"] forKey:@"AppleLanguages"];
    
    WebPreferences *preferences = [WebPreferences standardPreferences];
    
    [preferences setStandardFontFamily:@"Times"];
    [preferences setFixedFontFamily:@"Courier"];
    [preferences setSerifFontFamily:@"Times"];
    [preferences setSansSerifFontFamily:@"Helvetica"];
    [preferences setCursiveFontFamily:@"Apple Chancery"];
    [preferences setFantasyFontFamily:@"Papyrus"];
    [preferences setDefaultFontSize:16];
    [preferences setDefaultFixedFontSize:13];
    [preferences setMinimumFontSize:1];
    [preferences setJavaEnabled:NO];
    [preferences setJavaScriptCanOpenWindowsAutomatically:YES];
    [preferences setEditableLinkBehavior:WebKitEditableLinkOnlyLiveWithShiftKey];
    [preferences setTabsToLinks:NO];
    [preferences setDOMPasteAllowed:YES];
    
    int option;
    while ((option = getopt_long(argc, (char * const *)argv, "", options, NULL)) != -1)
        switch (option) {
            case '?':   // unknown or ambiguous option
            case ':':   // missing argument
                exit(1);
                break;
        }

    activateAhemFont();

    if (dumpPixels) {
        setDefaultColorProfileToRGB();
        screenCaptureBuffer = malloc(maxViewHeight * maxViewWidth * 4);
        sharedColorSpace = CGColorSpaceCreateDeviceRGB();
    }
    
    localPasteboards = [[NSMutableDictionary alloc] init];
    navigationController = [[NavigationController alloc] init];
    frameLoadDelegate = [[FrameLoadDelegate alloc] init];
    uiDelegate = [[UIDelegate alloc] init];
    editingDelegate = [[EditingDelegate alloc] init];    
    resourceLoadDelegate = [[ResourceLoadDelegate alloc] init];
    
    NSString *pwd = [[NSString stringWithUTF8String:argv[0]] stringByDeletingLastPathComponent];
    [WebPluginDatabase setAdditionalWebPlugInPaths:[NSArray arrayWithObject:pwd]];
    [[WebPluginDatabase sharedDatabase] refresh];
    
    WebView *webView = createWebView();    
    mainFrame = [webView mainFrame];
    NSWindow *window = [webView window];
    
    workQueue = [[NSMutableArray alloc] init];

    makeLargeMallocFailSilently();

    signal(SIGILL, crashHandler);    /* 4:   illegal instruction (not reset when caught) */
    signal(SIGTRAP, crashHandler);   /* 5:   trace trap (not reset when caught) */
    signal(SIGEMT, crashHandler);    /* 7:   EMT instruction */
    signal(SIGFPE, crashHandler);    /* 8:   floating point exception */
    signal(SIGBUS, crashHandler);    /* 10:  bus error */
    signal(SIGSEGV, crashHandler);   /* 11:  segmentation violation */
    signal(SIGSYS, crashHandler);    /* 12:  bad argument to system call */
    signal(SIGPIPE, crashHandler);   /* 13:  write on a pipe with no reader */
    signal(SIGXCPU, crashHandler);   /* 24:  exceeded CPU time limit */
    signal(SIGXFSZ, crashHandler);   /* 25:  exceeded file size limit */
    
    [[NSURLCache sharedURLCache] removeAllCachedResponses];
    
    // <rdar://problem/5222911>
    testStringByEvaluatingJavaScriptFromString();

    if (threaded)
        startJavaScriptThreads();
    
    if (argc == optind+1 && strcmp(argv[optind], "-") == 0) {
        char filenameBuffer[2048];
        printSeparators = YES;
        while (fgets(filenameBuffer, sizeof(filenameBuffer), stdin)) {
            char *newLineCharacter = strchr(filenameBuffer, '\n');
            if (newLineCharacter)
                *newLineCharacter = '\0';
            
            if (strlen(filenameBuffer) == 0)
                continue;
                
            runTest(filenameBuffer);
        }
    } else {
        printSeparators = (optind < argc-1 || (dumpPixels && dumpTree));
        for (int i = optind; i != argc; ++i)
            runTest(argv[i]);
    }

    if (threaded)
        stopJavaScriptThreads();
    
    [workQueue release];

    [WebCoreStatistics emptyCache]; // Otherwise SVGImages trigger false positives for Frame/Node counts    
    [webView close];
    mainFrame = nil;

    // Work around problem where registering drag types leaves an outstanding
    // "perform selector" on the window, which retains the window. It's a bit
    // inelegant and perhaps dangerous to just blow them all away, but in practice
    // it probably won't cause any trouble (and this is just a test tool, after all).
    [NSObject cancelPreviousPerformRequestsWithTarget:window];
    
    [window close]; // releases when closed
    [webView release];
    [frameLoadDelegate release];
    [editingDelegate release];
    [resourceLoadDelegate release];
    [uiDelegate release];
    
    [localPasteboards release];
    localPasteboards = nil;
    
    [navigationController release];
    navigationController = nil;
    
    [disallowedURLs release];
    disallowedURLs = nil;
    
    if (dumpPixels)
        restoreColorSpace(0);
}

int main(int argc, const char *argv[])
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    dumpRenderTree(argc, argv);
    [WebCoreStatistics garbageCollectJavaScriptObjects];
    [pool release];
    return 0;
}

static int compareHistoryItems(id item1, id item2, void *context)
{
    return [[item1 target] caseInsensitiveCompare:[item2 target]];
}

static void dumpHistoryItem(WebHistoryItem *item, int indent, BOOL current)
{
    int start = 0;
    if (current) {
        printf("curr->");
        start = 6;
    }
    for (int i = start; i < indent; i++)
        putchar(' ');
    printf("%s", [[item URLString] UTF8String]);
    NSString *target = [item target];
    if (target && [target length] > 0)
        printf(" (in frame \"%s\")", [target UTF8String]);
    if ([item isTargetItem])
        printf("  **nav target**");
    putchar('\n');
    NSArray *kids = [item children];
    if (kids) {
        // must sort to eliminate arbitrary result ordering which defeats reproducible testing
        kids = [kids sortedArrayUsingFunction:&compareHistoryItems context:nil];
        for (unsigned i = 0; i < [kids count]; i++)
            dumpHistoryItem([kids objectAtIndex:i], indent+4, NO);
    }
}

static void dumpFrameScrollPosition(WebFrame *f)
{
    NSPoint scrollPosition = [[[[f frameView] documentView] superview] bounds].origin;
    if (ABS(scrollPosition.x) > 0.00000001 || ABS(scrollPosition.y) > 0.00000001) {
        if ([f parentFrame] != nil)
            printf("frame '%s' ", [[f name] UTF8String]);
        printf("scrolled to %.f,%.f\n", scrollPosition.x, scrollPosition.y);
    }

    if (dumpChildFrameScrollPositions) {
        NSArray *kids = [f childFrames];
        if (kids)
            for (unsigned i = 0; i < [kids count]; i++)
                dumpFrameScrollPosition([kids objectAtIndex:i]);
    }
}

static void convertWebResourceDataToString(NSMutableDictionary *resource)
{
    NSString *mimeType = [resource objectForKey:@"WebResourceMIMEType"];
    if ([mimeType hasPrefix:@"text/"] || [mimeType isEqualToString:@"application/x-javascript"]) {
        NSData *data = [resource objectForKey:@"WebResourceData"];
        NSString *dataAsString = [[[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding] autorelease];
        [resource setObject:dataAsString forKey:@"WebResourceData"];
    }
}

static void normalizeWebResourceURL(NSMutableString *webResourceURL, NSString *oldURLBase)
{
    [webResourceURL replaceOccurrencesOfString:oldURLBase
                                    withString:@"file://"
                                       options:NSLiteralSearch
                                         range:NSMakeRange(0, [webResourceURL length])];
}

static void normalizeWebResourceResponse(NSMutableDictionary *propertyList, NSString *oldURLBase)
{
    NSURLResponse *response = nil;
    NSData *responseData = [propertyList objectForKey:@"WebResourceResponse"]; // WebResourceResponseKey in WebResource.m
    if ([responseData isKindOfClass:[NSData class]]) {
        // Decode NSURLResponse
        NSKeyedUnarchiver *unarchiver = [[NSKeyedUnarchiver alloc] initForReadingWithData:responseData];
        response = [unarchiver decodeObjectForKey:@"WebResourceResponse"]; // WebResourceResponseKey in WebResource.m
        [unarchiver finishDecoding];
        [unarchiver release];

        // Create replacement NSURLReponse
        NSMutableString *URL = [[[NSMutableString alloc] initWithContentsOfURL:[response URL]] autorelease];
        normalizeWebResourceURL(URL, oldURLBase);
        NSURLResponse *newResponse = [[NSURLResponse alloc] initWithURL:[[[NSURL alloc] initWithString:URL] autorelease]
                                                               MIMEType:[response MIMEType]
                                                  expectedContentLength:[response expectedContentLength]
                                                       textEncodingName:[response textEncodingName]];
        [newResponse autorelease];

        // Encode replacement NSURLResponse
        NSMutableData *newResponseData = [[NSMutableData alloc] init];
        NSKeyedArchiver *archiver = [[NSKeyedArchiver alloc] initForWritingWithMutableData:newResponseData];
        [archiver encodeObject:newResponse forKey:@"WebResourceResponse"];
        [archiver finishEncoding];
        [archiver release];
        [propertyList setObject:newResponseData forKey:@"WebResourceResponse"]; // WebResourceResponseKey in WebResource.m
        [newResponseData release];
    }
}

static NSString *serializeWebArchiveToXML(WebArchive *webArchive)
{
    NSString *errorString;
    NSMutableDictionary *propertyList = [NSPropertyListSerialization propertyListFromData:[webArchive data]
                                                                         mutabilityOption:NSPropertyListMutableContainersAndLeaves
                                                                                   format:NULL
                                                                         errorDescription:&errorString];
    if (!propertyList)
        return errorString;

    // Normalize WebResourceResponse and WebResourceURL values in plist for testing
    NSString *cwdURL = [@"file://" stringByAppendingString:[[[NSFileManager defaultManager] currentDirectoryPath] stringByExpandingTildeInPath]];

    NSMutableArray *resources = [NSMutableArray arrayWithCapacity:1];
    [resources addObject:propertyList];

    while ([resources count]) {
        NSMutableDictionary *resourcePropertyList = [resources objectAtIndex:0];
        [resources removeObjectAtIndex:0];

        NSMutableDictionary *mainResource = [resourcePropertyList objectForKey:@"WebMainResource"];
        normalizeWebResourceURL([mainResource objectForKey:@"WebResourceURL"], cwdURL);
        convertWebResourceDataToString(mainResource);

        // Add subframeArchives to list for processing
        NSMutableArray *subframeArchives = [resourcePropertyList objectForKey:@"WebSubframeArchives"]; // WebSubframeArchivesKey in WebArchive.m
        if (subframeArchives)
            [resources addObjectsFromArray:subframeArchives];

        NSMutableArray *subresources = [resourcePropertyList objectForKey:@"WebSubresources"]; // WebSubresourcesKey in WebArchive.m
        NSEnumerator *enumerator = [subresources objectEnumerator];
        NSMutableDictionary *subresourcePropertyList;
        while ((subresourcePropertyList = [enumerator nextObject])) {
            normalizeWebResourceURL([subresourcePropertyList objectForKey:@"WebResourceURL"], cwdURL);
            normalizeWebResourceResponse(subresourcePropertyList, cwdURL);
            convertWebResourceDataToString(subresourcePropertyList);
        }
    }

    NSData *xmlData = [NSPropertyListSerialization dataFromPropertyList:propertyList
                                                                 format:NSPropertyListXMLFormat_v1_0
                                                       errorDescription:&errorString];
    if (!xmlData)
        return errorString;

    NSMutableString *string = [[[NSMutableString alloc] initWithData:xmlData encoding:NSUTF8StringEncoding] autorelease];

    // Replace "Apple Computer" with "Apple" in the DTD declaration.
    NSRange range = [string rangeOfString:@"-//Apple Computer//"];
    if (range.location != NSNotFound)
        [string replaceCharactersInRange:range withString:@"-//Apple//"];
    
    return string;
}

static void dumpBackForwardListForWebView(WebView *view)
{
    printf("\n============== Back Forward List ==============\n");
    WebBackForwardList *bfList = [view backForwardList];

    // Print out all items in the list after prevTestBFItem, which was from the previous test
    // Gather items from the end of the list, the print them out from oldest to newest
    NSMutableArray *itemsToPrint = [[NSMutableArray alloc] init];
    for (int i = [bfList forwardListCount]; i > 0; i--) {
        WebHistoryItem *item = [bfList itemAtIndex:i];
        // something is wrong if the item from the last test is in the forward part of the b/f list
        assert(item != prevTestBFItem);
        [itemsToPrint addObject:item];
    }
            
    assert([bfList currentItem] != prevTestBFItem);
    [itemsToPrint addObject:[bfList currentItem]];
    int currentItemIndex = [itemsToPrint count] - 1;

    for (int i = -1; i >= -[bfList backListCount]; i--) {
        WebHistoryItem *item = [bfList itemAtIndex:i];
        if (item == prevTestBFItem)
            break;
        [itemsToPrint addObject:item];
    }

    for (int i = [itemsToPrint count]-1; i >= 0; i--) {
        dumpHistoryItem([itemsToPrint objectAtIndex:i], 8, i == currentItemIndex);
    }
    [itemsToPrint release];
    printf("===============================================\n");
}

void dump(void)
{
    if (dumpTree) {
        NSString *result = nil;

        dumpAsText |= [[[[mainFrame dataSource] response] MIMEType] isEqualToString:@"text/plain"];
        if (dumpAsText) {
            DOMElement *documentElement = [[mainFrame DOMDocument] documentElement];
            result = [[(DOMElement *)documentElement innerText] stringByAppendingString:@"\n"];
        } else if (dumpDOMAsWebArchive) {
            WebArchive *webArchive = [[mainFrame DOMDocument] webArchive];
            result = serializeWebArchiveToXML(webArchive);
        } else if (dumpSourceAsWebArchive) {
            WebArchive *webArchive = [[mainFrame dataSource] webArchive];
            result = serializeWebArchiveToXML(webArchive);
        } else {
            bool isSVGW3CTest = ([currentTest rangeOfString:@"svg/W3C-SVG-1.1"].length);
            if (isSVGW3CTest)
                [[mainFrame webView] setFrameSize:NSMakeSize(480, 360)];
            else 
                [[mainFrame webView] setFrameSize:NSMakeSize(maxViewWidth, maxViewHeight)];
            result = [mainFrame renderTreeAsExternalRepresentation];
        }

        if (!result) {
            const char *errorMessage;
            if (dumpAsText)
                errorMessage = "[documentElement innerText]";
            else if (dumpDOMAsWebArchive)
                errorMessage = "[[mainFrame DOMDocument] webArchive]";
            else if (dumpSourceAsWebArchive)
                errorMessage = "[[mainFrame dataSource] webArchive]";
            else
                errorMessage = "[mainFrame renderTreeAsExternalRepresentation]";
            printf("ERROR: nil result from %s", errorMessage);
        } else {
            fputs([result UTF8String], stdout);
            if (!dumpAsText && !dumpDOMAsWebArchive && !dumpSourceAsWebArchive)
                dumpFrameScrollPosition(mainFrame);
        }

        if (dumpBackForwardList) {
            unsigned count = [(NSArray *)allWindowsRef count];
            for (unsigned i = 0; i < count; i++) {
                NSWindow *window = [(NSArray *)allWindowsRef objectAtIndex:i];
                WebView *webView = [[[window contentView] subviews] objectAtIndex:0];
                dumpBackForwardListForWebView(webView);
            }
        }

        if (printSeparators)
            puts("#EOF");
    }
    
    if (dumpPixels) {
        if (!dumpAsText && !dumpDOMAsWebArchive && !dumpSourceAsWebArchive) {
            // grab a bitmap from the view
            WebView* view = [mainFrame webView];
            NSSize webViewSize = [view frame].size;

            CGContextRef cgContext = CGBitmapContextCreate(screenCaptureBuffer, webViewSize.width, webViewSize.height, 8, webViewSize.width * 4, sharedColorSpace, kCGBitmapByteOrder32Host | kCGImageAlphaPremultipliedLast);

            NSGraphicsContext* savedContext = [[[NSGraphicsContext currentContext] retain] autorelease];
            NSGraphicsContext* nsContext = [NSGraphicsContext graphicsContextWithGraphicsPort:cgContext flipped:NO];
            [NSGraphicsContext setCurrentContext:nsContext];

            if (readFromWindow) {
                NSBitmapImageRep *imageRep;
                [view displayIfNeeded];
                [view lockFocus];
                imageRep = [[NSBitmapImageRep alloc] initWithFocusedViewRect:[view frame]];
                [view unlockFocus];
                [imageRep draw];
                [imageRep release];
            } else if (!testRepaint)
                [view displayRectIgnoringOpacity:NSMakeRect(0, 0, webViewSize.width, webViewSize.height) inContext:nsContext];
            else if (!repaintSweepHorizontally) {
                NSRect line = NSMakeRect(0, 0, webViewSize.width, 1);
                while (line.origin.y < webViewSize.height) {
                    [view displayRectIgnoringOpacity:line inContext:nsContext];
                    line.origin.y++;
                }
            } else {
                NSRect column = NSMakeRect(0, 0, 1, webViewSize.height);
                while (column.origin.x < webViewSize.width) {
                    [view displayRectIgnoringOpacity:column inContext:nsContext];
                    column.origin.x++;
                }
            }
            if (dumpSelectionRect) {
                NSView *documentView = [[mainFrame frameView] documentView];
                if ([documentView conformsToProtocol:@protocol(WebDocumentSelection)]) {
                    [[NSColor redColor] set];
                    [NSBezierPath strokeRect:[documentView convertRect:[(id <WebDocumentSelection>)documentView selectionRect] fromView:nil]];
                }
            }

            [NSGraphicsContext setCurrentContext:savedContext];
            
            CGImageRef bitmapImage = CGBitmapContextCreateImage(cgContext);
            CGContextRelease(cgContext);

            // compute the actual hash to compare to the expected image's hash
            NSString *actualHash = md5HashStringForBitmap(bitmapImage);
            printf("\nActualHash: %s\n", [actualHash UTF8String]);

            BOOL dumpImage;
            if (dumpAllPixels)
                dumpImage = YES;
            else {
                // FIXME: It's unfortunate that we hardcode the file naming scheme here.
                // At one time, the perl script had all the knowledge about file layout.
                // Some day we should restore that setup by passing in more parameters to this tool.
                NSString *baseTestPath = [currentTest stringByDeletingPathExtension];
                NSString *baselineHashPath = [baseTestPath stringByAppendingString:@"-expected.checksum"];
                NSString *baselineHash = [NSString stringWithContentsOfFile:baselineHashPath encoding:NSUTF8StringEncoding error:nil];
                NSString *baselineImagePath = [baseTestPath stringByAppendingString:@"-expected.png"];

                printf("BaselineHash: %s\n", [baselineHash UTF8String]);

                /// send the image to stdout if the hash mismatches or if there's no file in the file system
                dumpImage = ![baselineHash isEqualToString:actualHash] || access([baselineImagePath fileSystemRepresentation], F_OK) != 0;
            }
            
            if (dumpImage) {
                CFMutableDataRef imageData = CFDataCreateMutable(0, 0);
                CGImageDestinationRef imageDest = CGImageDestinationCreateWithData(imageData, CFSTR("public.png"), 1, 0);
                CGImageDestinationAddImage(imageDest, bitmapImage, 0);
                CGImageDestinationFinalize(imageDest);
                CFRelease(imageDest);
                printf("Content-length: %lu\n", CFDataGetLength(imageData));
                fwrite(CFDataGetBytePtr(imageData), 1, CFDataGetLength(imageData), stdout);
                CFRelease(imageData);
            }

            CGImageRelease(bitmapImage);
        }

        printf("#EOF\n");
    }
    
    fflush(stdout);

    if (paint)
        displayWebView();
    
    done = YES;
}

@implementation LayoutTestController

+ (BOOL)isSelectorExcludedFromWebScript:(SEL)aSelector
{
    if (0
            || aSelector == @selector(accessStoredWebScriptObject)
            || aSelector == @selector(addDisallowedURL:)    
            || aSelector == @selector(addFileToPasteboardOnDrag)
            || aSelector == @selector(clearBackForwardList)
            || aSelector == @selector(decodeHostName:)
            || aSelector == @selector(display)
            || aSelector == @selector(dumpAsText)
            || aSelector == @selector(dumpBackForwardList)
            || aSelector == @selector(dumpChildFrameScrollPositions)
            || aSelector == @selector(dumpDOMAsWebArchive)
            || aSelector == @selector(dumpEditingCallbacks)
            || aSelector == @selector(dumpFrameLoadCallbacks)
            || aSelector == @selector(dumpResourceLoadCallbacks)
            || aSelector == @selector(dumpSelectionRect)
            || aSelector == @selector(dumpSourceAsWebArchive)
            || aSelector == @selector(dumpTitleChanges)
            || aSelector == @selector(encodeHostName:)
            || aSelector == @selector(keepWebHistory)
            || aSelector == @selector(notifyDone)
            || aSelector == @selector(objCClassNameOf:)
            || aSelector == @selector(objCIdentityIsEqual::)
            || aSelector == @selector(objCObjectOfClass:)
            || aSelector == @selector(objCLongLongRoundTrip:)
            || aSelector == @selector(objCUnsignedLongLongRoundTrip:)
            || aSelector == @selector(queueBackNavigation:)
            || aSelector == @selector(queueForwardNavigation:)
            || aSelector == @selector(queueLoad:target:)
            || aSelector == @selector(queueReload)
            || aSelector == @selector(queueScript:)
            || aSelector == @selector(repaintSweepHorizontally)
            || aSelector == @selector(setAcceptsEditing:)
            || aSelector == @selector(setCallCloseOnWebViews:)
            || aSelector == @selector(setCanOpenWindows)
            || aSelector == @selector(setCloseRemainingWindowsWhenComplete:)
            || aSelector == @selector(setMainFrameIsFirstResponder:)
            || aSelector == @selector(setTabKeyCyclesThroughElements:)
            || aSelector == @selector(setUseDashboardCompatibilityMode:)
            || aSelector == @selector(setUserStyleSheetEnabled:)
            || aSelector == @selector(setUserStyleSheetLocation:)
            || aSelector == @selector(setWindowIsKey:)
            || aSelector == @selector(storeWebScriptObject:)
            || aSelector == @selector(testRepaint)
            || aSelector == @selector(testWrapperRoundTripping:)
            || aSelector == @selector(waitUntilDone)
            || aSelector == @selector(windowCount)
        )
        return NO;
    return YES;
}

+ (NSString *)webScriptNameForSelector:(SEL)aSelector
{
    if (aSelector == @selector(setWindowIsKey:))
        return @"setWindowIsKey";
    if (aSelector == @selector(setMainFrameIsFirstResponder:))
        return @"setMainFrameIsFirstResponder";
    if (aSelector == @selector(queueBackNavigation:))
        return @"queueBackNavigation";
    if (aSelector == @selector(queueForwardNavigation:))
        return @"queueForwardNavigation";
    if (aSelector == @selector(queueScript:))
        return @"queueScript";
    if (aSelector == @selector(queueLoad:target:))
        return @"queueLoad";
    if (aSelector == @selector(setAcceptsEditing:))
        return @"setAcceptsEditing";
    if (aSelector == @selector(setTabKeyCyclesThroughElements:))
        return @"setTabKeyCyclesThroughElements";
    if (aSelector == @selector(storeWebScriptObject:))
        return @"storeWebScriptObject";
    if (aSelector == @selector(testWrapperRoundTripping:))
        return @"testWrapperRoundTripping";
    if (aSelector == @selector(setUserStyleSheetLocation:))
        return @"setUserStyleSheetLocation";
    if (aSelector == @selector(setUserStyleSheetEnabled:))
        return @"setUserStyleSheetEnabled";
    if (aSelector == @selector(objCClassNameOf:))
        return @"objCClassName";
    if (aSelector == @selector(objCObjectOfClass:))
        return @"objCObjectOfClass";
    if (aSelector == @selector(objCIdentityIsEqual::))
        return @"objCIdentityIsEqual";
    if (aSelector == @selector(addDisallowedURL:))
        return @"addDisallowedURL";
    if (aSelector == @selector(setCallCloseOnWebViews:))
        return @"setCallCloseOnWebViews";
    if (aSelector == @selector(setCloseRemainingWindowsWhenComplete:))
        return @"setCloseRemainingWindowsWhenComplete";
    if (aSelector == @selector(setUseDashboardCompatibilityMode:))
        return @"setUseDashboardCompatiblityMode";
    if (aSelector == @selector(encodeHostName:))
        return @"encodeHostName";
    if (aSelector == @selector(decodeHostName:))
        return @"decodeHostName";    
    if (aSelector == @selector(objCLongLongRoundTrip:))
        return @"objCLongLongRoundTrip";
    if (aSelector == @selector(objCUnsignedLongLongRoundTrip:))
        return @"objCUnsignedLongLongRoundTrip";
    
    return nil;
}

- (void)clearBackForwardList
{
    WebBackForwardList *backForwardList = [[mainFrame webView] backForwardList];
    WebHistoryItem *item = [[backForwardList currentItem] retain];

    // We clear the history by setting the back/forward list's capacity to 0
    // then restoring it back and adding back the current item.
    int capacity = [backForwardList capacity];
    [backForwardList setCapacity:0];
    [backForwardList setCapacity:capacity];
    [backForwardList addItem:item];
    [backForwardList goToItem:item];
    [item release];
}

- (void)setUseDashboardCompatibilityMode:(BOOL)flag
{
    [[mainFrame webView] _setDashboardBehavior:WebDashboardBehaviorUseBackwardCompatibilityMode to:flag];
}

- (void)setCloseRemainingWindowsWhenComplete:(BOOL)closeWindows
{
    closeRemainingWindowsWhenComplete = closeWindows;
}

- (void)keepWebHistory
{
    if (![WebHistory optionalSharedHistory]) {
        WebHistory *history = [[WebHistory alloc] init];
        [WebHistory setOptionalSharedHistory:history];
        [history release];
    }
}

- (void)setCallCloseOnWebViews:(BOOL)callClose
{
    closeWebViews = callClose;
}

- (void)setCanOpenWindows
{
    canOpenWindows = YES;
}

- (void)waitUntilDone 
{
    waitToDump = YES;
}

- (void)notifyDone
{
    if (waitToDump && !topLoadingFrame && [workQueue count] == 0)
        dump();
    waitToDump = NO;
}

- (void)dumpAsText
{
    dumpAsText = YES;
}

- (void)addFileToPasteboardOnDrag
{
    addFileToPasteboardOnDrag = YES;
}

- (void)addDisallowedURL:(NSString *)urlString
{
    if (!disallowedURLs)
        disallowedURLs = [[NSMutableSet alloc] init];
    
    
    // Canonicalize the URL
    NSURLRequest* request = [NSURLRequest requestWithURL:[NSURL URLWithString:urlString]];
    request = [NSURLProtocol canonicalRequestForRequest:request];
    
    [disallowedURLs addObject:[request URL]];
}

- (void)setUserStyleSheetLocation:(NSString *)path
{
    NSURL *url = [NSURL URLWithString:path];
    [[WebPreferences standardPreferences] setUserStyleSheetLocation:url];
}

- (void)setUserStyleSheetEnabled:(BOOL)flag
{
    [[WebPreferences standardPreferences] setUserStyleSheetEnabled:flag];
}

- (void)dumpDOMAsWebArchive
{
    dumpDOMAsWebArchive = YES;
}

- (void)dumpSourceAsWebArchive
{
    dumpSourceAsWebArchive = YES;
}

- (void)dumpSelectionRect
{
    dumpSelectionRect = YES;
}

- (void)dumpTitleChanges
{
    dumpTitleChanges = YES;
}

- (void)dumpBackForwardList
{
    dumpBackForwardList = YES;
}

- (int)windowCount
{
    return CFArrayGetCount(allWindowsRef);
}

- (void)dumpChildFrameScrollPositions
{
    dumpChildFrameScrollPositions = YES;
}

- (void)dumpEditingCallbacks
{
    shouldDumpEditingCallbacks = YES;
}

- (void)dumpResourceLoadCallbacks
{
    shouldDumpResourceLoadCallbacks = YES;
}

- (void)dumpFrameLoadCallbacks
{
    shouldDumpFrameLoadCallbacks = YES;
}

- (void)setWindowIsKey:(BOOL)flag
{
    windowIsKey = flag;
    NSView *documentView = [[mainFrame frameView] documentView];
    if ([documentView isKindOfClass:[WebHTMLView class]])
        [(WebHTMLView *)documentView _updateActiveState];
}

- (void)setMainFrameIsFirstResponder:(BOOL)flag
{
    NSView *documentView = [[mainFrame frameView] documentView];
    
    NSResponder *firstResponder = flag ? documentView : nil;
    [[[mainFrame webView] window] makeFirstResponder:firstResponder];
        
    if ([documentView isKindOfClass:[WebHTMLView class]])
        [(WebHTMLView *)documentView _updateActiveState];
}

- (void)display
{
    displayWebView();
}

- (void)testRepaint
{
    testRepaint = YES;
}

- (void)repaintSweepHorizontally
{
    repaintSweepHorizontally = YES;
}

- (id)invokeUndefinedMethodFromWebScript:(NSString *)name withArguments:(NSArray *)args
{
    return nil;
}

- (void)_addWorkForTarget:(id)target selector:(SEL)selector arg1:(id)arg1 arg2:(id)arg2
{
    if (workQueueFrozen)
        return;
    NSMethodSignature *sig = [target methodSignatureForSelector:selector];
    NSInvocation *work = [NSInvocation invocationWithMethodSignature:sig];
    [work retainArguments];
    [work setTarget:target];
    [work setSelector:selector];
    if (arg1) {
        [work setArgument:&arg1 atIndex:2];
        if (arg2)
            [work setArgument:&arg2 atIndex:3];
    }
    [workQueue addObject:work];
}

- (void)_doLoad:(NSURL *)url target:(NSString *)target
{
    WebFrame *targetFrame;
    if (target && ![target isKindOfClass:[WebUndefined class]])
        targetFrame = [mainFrame findFrameNamed:target];
    else
        targetFrame = mainFrame;
    [targetFrame loadRequest:[NSURLRequest requestWithURL:url]];
}

- (void)_doBackOrForwardNavigation:(NSNumber *)index
{
    int bfIndex = [index intValue];
    if (bfIndex == 1)
        [[mainFrame webView] goForward];
    if (bfIndex == -1)
        [[mainFrame webView] goBack];
    else {        
        WebBackForwardList *bfList = [[mainFrame webView] backForwardList];
        [[mainFrame webView] goToBackForwardItem:[bfList itemAtIndex:bfIndex]];
    }
}

- (void)queueBackNavigation:(int)howFarBack
{
    [self _addWorkForTarget:self selector:@selector(_doBackOrForwardNavigation:) arg1:[NSNumber numberWithInt:-howFarBack] arg2:nil];
}

- (void)queueForwardNavigation:(int)howFarForward
{
    [self _addWorkForTarget:self selector:@selector(_doBackOrForwardNavigation:) arg1:[NSNumber numberWithInt:howFarForward] arg2:nil];
}

- (void)queueReload
{
    [self _addWorkForTarget:[mainFrame webView] selector:@selector(reload:) arg1:self arg2:nil];
}

- (void)queueScript:(NSString *)script
{
    [self _addWorkForTarget:[mainFrame webView] selector:@selector(stringByEvaluatingJavaScriptFromString:) arg1:script arg2:nil];
}

- (void)queueLoad:(NSString *)URLString target:(NSString *)target
{
    NSURL *URL = [NSURL URLWithString:URLString relativeToURL:[[[mainFrame dataSource] response] URL]];
    [self _addWorkForTarget:self selector:@selector(_doLoad:target:) arg1:URL arg2:target];
}

- (void)setAcceptsEditing:(BOOL)newAcceptsEditing
{
    [(EditingDelegate *)[[mainFrame webView] editingDelegate] setAcceptsEditing:newAcceptsEditing];
}

- (void)setTabKeyCyclesThroughElements:(BOOL)newTabKeyCyclesThroughElements
{
    [[mainFrame webView] setTabKeyCyclesThroughElements:newTabKeyCyclesThroughElements];
}

- (void)storeWebScriptObject:(WebScriptObject *)webScriptObject
{
    if (webScriptObject == storedWebScriptObject)
        return;

    [storedWebScriptObject release];
    storedWebScriptObject = [webScriptObject retain];
}

- (void)accessStoredWebScriptObject
{
    JSObjectRef jsObject = [storedWebScriptObject JSObject];
    ASSERT(!jsObject);

    [storedWebScriptObject callWebScriptMethod:@"" withArguments:nil];
    [storedWebScriptObject evaluateWebScript:@""];
    [storedWebScriptObject setValue:[WebUndefined undefined] forKey:@"key"];
    [storedWebScriptObject valueForKey:@"key"];
    [storedWebScriptObject removeWebScriptKey:@"key"];
    [storedWebScriptObject stringRepresentation];
    [storedWebScriptObject webScriptValueAtIndex:0];
    [storedWebScriptObject setWebScriptValueAtIndex:0 value:[WebUndefined undefined]];
    [storedWebScriptObject setException:@"exception"];
}

- (BOOL)testWrapperRoundTripping:(WebScriptObject *)webScriptObject
{
    JSObjectRef jsObject = [webScriptObject JSObject];

    if (!jsObject)
        return false;

    if (!webScriptObject)
        return false;

    if ([[webScriptObject evaluateWebScript:@"({ })"] class] != [webScriptObject class])
        return false;

    [webScriptObject setValue:[NSNumber numberWithInt:666] forKey:@"key"];
    if (![[webScriptObject valueForKey:@"key"] isKindOfClass:[NSNumber class]] ||
        ![[webScriptObject valueForKey:@"key"] isEqualToNumber:[NSNumber numberWithInt:666]])
        return false;

    [webScriptObject removeWebScriptKey:@"key"];
    @try {
        if ([webScriptObject valueForKey:@"key"])
            return false;
    } @catch(NSException *exception) {
        // NSObject throws an exception if the key doesn't exist.
    }

    [webScriptObject setWebScriptValueAtIndex:0 value:webScriptObject];
    if ([webScriptObject webScriptValueAtIndex:0] != webScriptObject)
        return false;

    if ([[webScriptObject stringRepresentation] isEqualToString:@"[Object object]"])
        return false;

    if ([webScriptObject callWebScriptMethod:@"returnThis" withArguments:nil] != webScriptObject)
        return false;

    return true;
}

- (void)dealloc
{
    [storedWebScriptObject release];
    [super dealloc];
}

- (NSString *)objCClassNameOf:(id)object
{
    if (!object)
        return @"nil";
    return NSStringFromClass([object class]);
}

- (id)objCObjectOfClass:(NSString *)aClass
{
    if ([aClass isEqualToString:@"NSNull"])
        return [NSNull null];
    if ([aClass isEqualToString:@"WebUndefined"])
        return [WebUndefined undefined];
    if ([aClass isEqualToString:@"NSCFBoolean"])
        return [NSNumber numberWithBool:true];
    if ([aClass isEqualToString:@"NSCFNumber"])
        return [NSNumber numberWithInt:1];
    if ([aClass isEqualToString:@"NSCFString"])
        return @"";
    if ([aClass isEqualToString:@"WebScriptObject"])
        return self;
    if ([aClass isEqualToString:@"NSArray"])
        return [NSArray array];

    return nil;
}

- (BOOL)objCIdentityIsEqual:(WebScriptObject *)a :(WebScriptObject *)b
{
    return a == b;
}

- (NSString*)decodeHostName:(NSString*)name
{
    return [name _web_decodeHostName];
}

- (NSString*)encodeHostName:(NSString*)name
{
    return [name _web_encodeHostName];
}

- (long long)objCLongLongRoundTrip:(long long)num
{
    return num;
}

- (unsigned long long)objCUnsignedLongLongRoundTrip:(unsigned long long)num
{
    return num;
}

@end

static bool shouldLogFrameLoadDelegates(const char *pathOrURL)
{
    return strstr(pathOrURL, "loading/");
}    

static void runTest(const char *pathOrURL)
{
    CFStringRef pathOrURLString = CFStringCreateWithCString(NULL, pathOrURL, kCFStringEncodingUTF8);
    if (!pathOrURLString) {
        fprintf(stderr, "can't parse filename as UTF-8\n");
        return;
    }
    
    CFURLRef URL;
    if (CFStringHasPrefix(pathOrURLString, CFSTR("http://")))
        URL = CFURLCreateWithString(NULL, pathOrURLString, NULL);
    else
        URL = CFURLCreateWithFileSystemPath(NULL, pathOrURLString, kCFURLPOSIXPathStyle, FALSE);
    
    if (!URL) {
        CFRelease(pathOrURLString);
        fprintf(stderr, "can't turn %s into a CFURL\n", pathOrURL);
        return;
    }

    [(EditingDelegate *)[[mainFrame webView] editingDelegate] setAcceptsEditing:YES];
    [[mainFrame webView] setTabKeyCyclesThroughElements: YES];
    done = NO;
    topLoadingFrame = nil;
    waitToDump = NO;
    dumpAsText = NO;
    dumpDOMAsWebArchive = NO;
    dumpSourceAsWebArchive = NO;
    dumpChildFrameScrollPositions = NO;
    shouldDumpEditingCallbacks = NO;
    shouldDumpResourceLoadCallbacks = NO;
    shouldDumpFrameLoadCallbacks = NO;
    dumpSelectionRect = NO;
    dumpTitleChanges = NO;
    dumpBackForwardList = NO;
    readFromWindow = NO;
    canOpenWindows = NO;
    closeWebViews = YES;
    addFileToPasteboardOnDrag = NO;
    [[mainFrame webView] _setDashboardBehavior:WebDashboardBehaviorUseBackwardCompatibilityMode to:NO];
    testRepaint = testRepaintDefault;
    repaintSweepHorizontally = repaintSweepHorizontallyDefault;
    if ([WebHistory optionalSharedHistory])
        [WebHistory setOptionalSharedHistory:nil];
    lastMousePosition = NSMakePoint(0, 0);
    [disallowedURLs removeAllObjects];
    
    if (currentTest != nil)
        CFRelease(currentTest);
    currentTest = (NSString *)pathOrURLString;
    [prevTestBFItem release];
    prevTestBFItem = [[[[mainFrame webView] backForwardList] currentItem] retain];
    [workQueue removeAllObjects];
    workQueueFrozen = NO;

    BOOL _shouldIgnoreWebCoreNodeLeaks = shouldIgnoreWebCoreNodeLeaks(CFURLGetString(URL));
    if (_shouldIgnoreWebCoreNodeLeaks)
        [WebCoreStatistics startIgnoringWebCoreNodeLeaks];

    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    if (shouldLogFrameLoadDelegates(pathOrURL))
        shouldDumpFrameLoadCallbacks = YES;
    [mainFrame loadRequest:[NSURLRequest requestWithURL:(NSURL *)URL]];
    CFRelease(URL);
    [pool release];
    while (!done) {
        pool = [[NSAutoreleasePool alloc] init];
        [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantPast]];
        [pool release];
    }
    pool = [[NSAutoreleasePool alloc] init];
    [EventSendingController clearSavedEvents];
    [[mainFrame webView] setSelectedDOMRange:nil affinity:NSSelectionAffinityDownstream];
    
    if (closeRemainingWindowsWhenComplete) {
        NSArray* array = [(NSArray *)allWindowsRef copy];
        
        unsigned count = [array count];
        for (unsigned i = 0; i < count; i++) {
            NSWindow *window = [array objectAtIndex:i];

            // Don't try to close the main window
            if (window == [[mainFrame webView] window])
                continue;
            
            WebView *webView = [[[window contentView] subviews] objectAtIndex:0];

            [webView close];
            [window close];
        }
        [array release];
    }
    
    [pool release];
    
    // We should only have our main window left when we're done
    ASSERT(CFArrayGetCount(allWindowsRef) == 1);
    ASSERT(CFArrayGetValueAtIndex(allWindowsRef, 0) == [[mainFrame webView] window]);
    
    if (_shouldIgnoreWebCoreNodeLeaks)
        [WebCoreStatistics stopIgnoringWebCoreNodeLeaks];
}

/* Hashes a bitmap and returns a text string for comparison and saving to a file */
static NSString *md5HashStringForBitmap(CGImageRef bitmap)
{
    MD5_CTX md5Context;
    unsigned char hash[16];
    
    unsigned bitsPerPixel = CGImageGetBitsPerPixel(bitmap);
    assert(bitsPerPixel == 32); // ImageDiff assumes 32 bit RGBA, we must as well.
    unsigned bytesPerPixel = bitsPerPixel / 8;
    unsigned pixelsHigh = CGImageGetHeight(bitmap);
    unsigned pixelsWide = CGImageGetWidth(bitmap);
    unsigned bytesPerRow = CGImageGetBytesPerRow(bitmap);
    assert(bytesPerRow >= (pixelsWide * bytesPerPixel));
    
    MD5_Init(&md5Context);
    unsigned char *bitmapData = screenCaptureBuffer;
    for (unsigned row = 0; row < pixelsHigh; row++) {
        MD5_Update(&md5Context, bitmapData, pixelsWide * bytesPerPixel);
        bitmapData += bytesPerRow;
    }
    MD5_Final(hash, &md5Context);
    
    char hex[33] = "";
    for (int i = 0; i < 16; i++) {
       snprintf(hex, 33, "%s%02x", hex, hash[i]);
    }

    return [NSString stringWithUTF8String:hex];
}

static void displayWebView()
{
    NSView *webView = [mainFrame webView];
    [webView display];
    [webView lockFocus];
    [[[NSColor blackColor] colorWithAlphaComponent:0.66] set];
    NSRectFillUsingOperation([webView frame], NSCompositeSourceOver);
    [webView unlockFocus];
    readFromWindow = YES;
}

@implementation DumpRenderTreePasteboard

// Return a local pasteboard so we don't disturb the real pasteboards when running tests.
+ (NSPasteboard *)_pasteboardWithName:(NSString *)name
{
    static int number = 0;
    if (!name)
        name = [NSString stringWithFormat:@"LocalPasteboard%d", ++number];
    LocalPasteboard *pasteboard = [localPasteboards objectForKey:name];
    if (pasteboard)
        return pasteboard;
    pasteboard = [[LocalPasteboard alloc] init];
    [localPasteboards setObject:pasteboard forKey:name];
    [pasteboard release];
    return pasteboard;
}

// Convenience method for JS so that it doesn't have to try and create a NSArray on the objc side instead
// of the usual WebScriptObject that is passed around
- (int)declareType:(NSString *)type owner:(id)newOwner
{
    return [self declareTypes:[NSArray arrayWithObject:type] owner:newOwner];
}

@end

@implementation LocalPasteboard

+ (id)alloc
{
    return NSAllocateObject(self, 0, 0);
}

- (id)init
{
    typesArray = [[NSMutableArray alloc] init];
    typesSet = [[NSMutableSet alloc] init];
    dataByType = [[NSMutableDictionary alloc] init];
    return self;
}

- (void)dealloc
{
    [typesArray release];
    [typesSet release];
    [dataByType release];
    [super dealloc];
}

- (NSString *)name
{
    return nil;
}

- (void)releaseGlobally
{
}

- (int)declareTypes:(NSArray *)newTypes owner:(id)newOwner
{
    [typesArray removeAllObjects];
    [typesSet removeAllObjects];
    [dataByType removeAllObjects];
    return [self addTypes:newTypes owner:newOwner];
}

- (int)addTypes:(NSArray *)newTypes owner:(id)newOwner
{
    unsigned count = [newTypes count];
    unsigned i;
    for (i = 0; i < count; ++i) {
        NSString *type = [newTypes objectAtIndex:i];
        NSString *setType = [typesSet member:type];
        if (!setType) {
            setType = [type copy];
            [typesArray addObject:setType];
            [typesSet addObject:setType];
            [setType release];
        }
        if (newOwner && [newOwner respondsToSelector:@selector(pasteboard:provideDataForType:)])
            [newOwner pasteboard:self provideDataForType:setType];
    }
    return ++changeCount;
}

- (int)changeCount
{
    return changeCount;
}

- (NSArray *)types
{
    return typesArray;
}

- (NSString *)availableTypeFromArray:(NSArray *)types
{
    unsigned count = [types count];
    unsigned i;
    for (i = 0; i < count; ++i) {
        NSString *type = [types objectAtIndex:i];
        NSString *setType = [typesSet member:type];
        if (setType)
            return setType;
    }
    return nil;
}

- (BOOL)setData:(NSData *)data forType:(NSString *)dataType
{
    if (data == nil)
        data = [NSData data];
    if (![typesSet containsObject:dataType])
        return NO;
    [dataByType setObject:data forKey:dataType];
    ++changeCount;
    return YES;
}

- (NSData *)dataForType:(NSString *)dataType
{
    return [dataByType objectForKey:dataType];
}

- (BOOL)setPropertyList:(id)propertyList forType:(NSString *)dataType;
{
    CFDataRef data = NULL;
    if (propertyList)
        data = CFPropertyListCreateXMLData(NULL, propertyList);
    BOOL result = [self setData:(NSData *)data forType:dataType];
    if (data)
        CFRelease(data);
    return result;
}

- (BOOL)setString:(NSString *)string forType:(NSString *)dataType
{
    CFDataRef data = NULL;
    if (string) {
        if ([string length] == 0)
            data = CFDataCreate(NULL, NULL, 0);
        else
            data = CFStringCreateExternalRepresentation(NULL, (CFStringRef)string, kCFStringEncodingUTF8, 0);
    }
    BOOL result = [self setData:(NSData *)data forType:dataType];
    if (data)
        CFRelease(data);
    return result;
}

@end

static CFArrayCallBacks NonRetainingArrayCallbacks = {
    0,
    NULL,
    NULL,
    CFCopyDescription,
    CFEqual
};

@implementation DumpRenderTreeWindow

- (id)initWithContentRect:(NSRect)contentRect styleMask:(unsigned int)styleMask backing:(NSBackingStoreType)bufferingType defer:(BOOL)deferCreation
{
    if (!allWindowsRef)
        allWindowsRef = CFArrayCreateMutable(NULL, 0, &NonRetainingArrayCallbacks);

    CFArrayAppendValue(allWindowsRef, self);
            
    return [super initWithContentRect:contentRect styleMask:styleMask backing:bufferingType defer:deferCreation];
}

- (void)dealloc
{
    CFRange arrayRange = CFRangeMake(0, CFArrayGetCount(allWindowsRef));
    CFIndex i = CFArrayGetFirstIndexOfValue(allWindowsRef, arrayRange, self);
    assert(i != -1);

    CFArrayRemoveValueAtIndex(allWindowsRef, i);
    [super dealloc];
}

- (BOOL)isKeyWindow
{
    return windowIsKey;
}

- (void)keyDown:(id)sender
{
    // Do nothing, avoiding the beep we'd otherwise get from NSResponder,
    // once we get to the end of the responder chain.
}

@end

@implementation DumpRenderTreeEvent

+ (NSPoint)mouseLocation
{
    return [[[mainFrame webView] window] convertBaseToScreen:lastMousePosition];
}

@end
