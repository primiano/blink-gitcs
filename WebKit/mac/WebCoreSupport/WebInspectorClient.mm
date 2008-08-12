/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
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

#import "WebInspectorClient.h"

#import "WebFrameInternal.h"
#import "WebInspector.h"
#import "WebLocalizableStrings.h"
#import "WebNodeHighlight.h"
#import "WebUIDelegate.h"
#import "WebViewInternal.h"

#import <WebCore/InspectorController.h>
#import <WebCore/Page.h>

#import <WebKit/DOMExtensions.h>

using namespace WebCore;

@interface WebInspectorWindowController : NSWindowController <NSWindowDelegate> {
@private
    WebView *_inspectedWebView;
    WebView *_webView;
    WebNodeHighlight *_currentHighlight;
    BOOL _attachedToInspectedWebView;
    BOOL _shouldAttach;
    BOOL _visible;
    BOOL _movingWindows;
}
- (id)initWithInspectedWebView:(WebView *)webView;
- (BOOL)inspectorVisible;
- (WebView *)webView;
- (void)attach;
- (void)detach;
- (void)highlightAndScrollToNode:(DOMNode *)node;
- (void)highlightNode:(DOMNode *)node;
- (void)hideHighlight;
@end

#pragma mark -

WebInspectorClient::WebInspectorClient(WebView *webView)
: m_webView(webView)
{
}

void WebInspectorClient::inspectorDestroyed()
{
    [[m_windowController.get() webView] close];
    delete this;
}

Page* WebInspectorClient::createPage()
{
    if (!m_windowController)
        m_windowController.adoptNS([[WebInspectorWindowController alloc] initWithInspectedWebView:m_webView]);

    return core([m_windowController.get() webView]);
}

String WebInspectorClient::localizedStringsURL()
{
    NSString *path = [[NSBundle bundleWithIdentifier:@"com.apple.WebCore"] pathForResource:@"localizedStrings" ofType:@"js"];
    if (path)
        return [[NSURL fileURLWithPath:path] absoluteString];
    return String();
}

void WebInspectorClient::showWindow()
{
    updateWindowTitle();
    [m_windowController.get() showWindow:nil];
}

void WebInspectorClient::closeWindow()
{
    [m_windowController.get() close];
}

void WebInspectorClient::attachWindow()
{
    [m_windowController.get() attach];
}

void WebInspectorClient::detachWindow()
{
    [m_windowController.get() detach];
}

void WebInspectorClient::highlight(Node* node)
{
    [m_windowController.get() highlightAndScrollToNode:kit(node)];
}

void WebInspectorClient::hideHighlight()
{
    [m_windowController.get() hideHighlight];
}

void WebInspectorClient::inspectedURLChanged(const String& newURL)
{
    m_inspectedURL = newURL;
    updateWindowTitle();
}

void WebInspectorClient::updateWindowTitle() const
{
    NSString *title = [NSString stringWithFormat:UI_STRING("Web Inspector — %@", "Web Inspector window title"), (NSString *)m_inspectedURL];
    [[m_windowController.get() window] setTitle:title];
}

#pragma mark -

#define WebKitInspectorAttachedViewHeightKey @"WebKitInspectorAttachedViewHeight"

@implementation WebInspectorWindowController
- (id)init
{
    if (![super initWithWindow:nil])
        return nil;

    // Keep preferences separate from the rest of the client, making sure we are using expected preference values.
    // One reason this is good is that it keeps the inspector out of history via "private browsing".

    WebPreferences *preferences = [[WebPreferences alloc] init];
    [preferences setAutosaves:NO];
    [preferences setPrivateBrowsingEnabled:YES];
    [preferences setLoadsImagesAutomatically:YES];
    [preferences setAuthorAndUserStylesEnabled:YES];
    [preferences setJavaScriptEnabled:YES];
    [preferences setAllowsAnimatedImages:YES];
    [preferences setPlugInsEnabled:NO];
    [preferences setJavaEnabled:NO];
    [preferences setUserStyleSheetEnabled:NO];
    [preferences setTabsToLinks:NO];
    [preferences setMinimumFontSize:0];
    [preferences setMinimumLogicalFontSize:9];

    _webView = [[WebView alloc] init];
    [_webView setPreferences:preferences];
    [_webView setDrawsBackground:NO];
    [_webView setProhibitsMainFrameScrolling:YES];
    [_webView setUIDelegate:self];

    [preferences release];

    NSString *path = [[NSBundle bundleWithIdentifier:@"com.apple.WebCore"] pathForResource:@"inspector" ofType:@"html" inDirectory:@"inspector"];
    NSURLRequest *request = [[NSURLRequest alloc] initWithURL:[NSURL fileURLWithPath:path]];
    [[_webView mainFrame] loadRequest:request];
    [request release];

    [self setWindowFrameAutosaveName:@"Web Inspector 2"];
    return self;
}

- (id)initWithInspectedWebView:(WebView *)webView
{
    if (![self init])
        return nil;

    // Don't retain to avoid a circular reference
    _inspectedWebView = webView;
    return self;
}

- (void)dealloc
{
    ASSERT(!_currentHighlight);
    [_webView release];
    [super dealloc];
}

#pragma mark -

- (BOOL)inspectorVisible
{
    return _visible;
}

- (WebView *)webView
{
    return _webView;
}

- (NSWindow *)window
{
    NSWindow *window = [super window];
    if (window)
        return window;

    NSUInteger styleMask = (NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask);

#ifndef BUILDING_ON_TIGER
    styleMask |= NSTexturedBackgroundWindowMask;
#endif

    window = [[NSWindow alloc] initWithContentRect:NSMakeRect(60.0, 200.0, 750.0, 650.0) styleMask:styleMask backing:NSBackingStoreBuffered defer:NO];
    [window setDelegate:self];
    [window setMinSize:NSMakeSize(400.0, 400.0)];

#ifndef BUILDING_ON_TIGER
    [window setAutorecalculatesContentBorderThickness:NO forEdge:NSMaxYEdge];
    [window setContentBorderThickness:55. forEdge:NSMaxYEdge];
#endif

    [self setWindow:window];
    [window release];

    return window;
}

#pragma mark -

- (BOOL)windowShouldClose:(id)sender
{
    _visible = NO;

    [_inspectedWebView page]->inspectorController()->setWindowVisible(false);

    [self hideHighlight];

    return YES;
}

- (void)close
{
    if (!_visible)
        return;

    _visible = NO;

    if (!_movingWindows)
        [_inspectedWebView page]->inspectorController()->setWindowVisible(false);

    [self hideHighlight];

    if (_attachedToInspectedWebView) {
        if ([_inspectedWebView _isClosed])
            return;

        WebFrameView *frameView = [[_inspectedWebView mainFrame] frameView];

        NSRect frameViewRect = [frameView frame];
        frameViewRect = NSMakeRect(0, 0, NSWidth(frameViewRect), NSHeight([_inspectedWebView frame]));

        [frameView setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable)];
        [frameView setFrame:frameViewRect];

        [_inspectedWebView displayIfNeeded];
    } else
        [super close];
}

- (IBAction)showWindow:(id)sender
{
    if (_visible) {
        if (!_attachedToInspectedWebView)
            [super showWindow:sender]; // call super so the window will be ordered front if needed
        return;
    }

    _visible = YES;

    if (_shouldAttach) {
        WebFrameView *frameView = [[_inspectedWebView mainFrame] frameView];

        NSRect frameViewRect = [frameView frame];
        float attachedHeight = [[NSUserDefaults standardUserDefaults] integerForKey:WebKitInspectorAttachedViewHeightKey];
        attachedHeight = MAX(300.0, MIN(attachedHeight, (NSHeight(frameViewRect) * 0.6)));
        frameViewRect = NSMakeRect(0, attachedHeight, NSWidth(frameViewRect), NSHeight(frameViewRect) - attachedHeight);

        [_webView removeFromSuperview];
        [_inspectedWebView addSubview:_webView positioned:NSWindowBelow relativeTo:(NSView*)frameView];

        [_webView setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable | NSViewMaxYMargin)];
        [_webView setFrame:NSMakeRect(0, 0, NSWidth(frameViewRect), attachedHeight)];

        [frameView setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable | NSViewMinYMargin)];
        [frameView setFrame:frameViewRect];

        _attachedToInspectedWebView = YES;
    } else {
        _attachedToInspectedWebView = NO;

        NSView *contentView = [[self window] contentView];
        [_webView setFrame:[contentView frame]];
        [_webView setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable)];
        [_webView removeFromSuperview];
        [contentView addSubview:_webView];

        [super showWindow:nil];
    }

    [_inspectedWebView page]->inspectorController()->setWindowVisible(true);
}

#pragma mark -

- (void)attach
{
    if (_attachedToInspectedWebView)
        return;

    _shouldAttach = YES;
    _movingWindows = YES;

    [self close];
    [self showWindow:nil];

    _movingWindows = NO;
}

- (void)detach
{
    if (!_attachedToInspectedWebView)
        return;

    _shouldAttach = NO;
    _movingWindows = YES;

    [self close];
    [self showWindow:nil];

    _movingWindows = NO;
}

#pragma mark -

- (void)highlightAndScrollToNode:(DOMNode *)node
{
    NSRect bounds = [node boundingBox];
    if (!NSIsEmptyRect(bounds)) {
        // FIXME: this needs to use the frame the node coordinates are in
        NSRect visible = [[[[_inspectedWebView mainFrame] frameView] documentView] visibleRect];
        BOOL needsScroll = !NSContainsRect(visible, bounds) && !NSContainsRect(bounds, visible);

        // only scroll if the bounds isn't in the visible rect and dosen't contain the visible rect
        if (needsScroll) {
            // scroll to the parent element if we aren't focused on an element
            DOMElement *element;
            if ([node isKindOfClass:[DOMElement class]])
                element = (DOMElement *)node;
            else
                element = (DOMElement *)[node parentNode];
            [element scrollIntoViewIfNeeded:YES];

            // give time for the scroll to happen
            [self performSelector:@selector(highlightNode:) withObject:node afterDelay:0.25];
        } else
            [self highlightNode:node];
    }
}

- (void)highlightNode:(DOMNode *)node
{
    // The scrollview's content view stays around between page navigations, so target it
    NSView *view = [[[[[_inspectedWebView mainFrame] frameView] documentView] enclosingScrollView] contentView];
    if (![view window])
        return; // skip the highlight if we have no window (e.g. hidden tab)

    if (!_currentHighlight) {
        _currentHighlight = [[WebNodeHighlight alloc] initWithTargetView:view inspectorController:[_inspectedWebView page]->inspectorController()];
        [_currentHighlight setDelegate:self];
        [_currentHighlight attach];
    }

    // FIXME: this is a hack until we hook up a didDraw and didScroll call in WebHTMLView
    [[_currentHighlight highlightView] setNeedsDisplay:YES];
}

- (void)hideHighlight
{
    [_currentHighlight detach];
    [_currentHighlight setDelegate:nil];
    [_currentHighlight release];
    _currentHighlight = nil;
}

#pragma mark -
#pragma mark UI delegate

- (NSUInteger)webView:(WebView *)sender dragDestinationActionMaskForDraggingInfo:(id <NSDraggingInfo>)draggingInfo
{
    return WebDragDestinationActionNone;
}

#pragma mark -

// These methods can be used by UI elements such as menu items and toolbar buttons when the inspector is the key window.

// This method is really only implemented to keep any UI elements enabled.
- (void)showWebInspector:(id)sender
{
    [[_inspectedWebView inspector] show:sender];
}

- (void)showErrorConsole:(id)sender
{
    [[_inspectedWebView inspector] showConsole:sender];
}

- (void)toggleDebuggingJavaScript:(id)sender
{
    [[_inspectedWebView inspector] toggleDebuggingJavaScript:sender];
}

- (void)toggleProfilingJavaScript:(id)sender
{
    [[_inspectedWebView inspector] toggleProfilingJavaScript:sender];
}

- (BOOL)validateUserInterfaceItem:(id <NSValidatedUserInterfaceItem>)item
{
    BOOL isMenuItem = [(id)item isKindOfClass:[NSMenuItem class]];
    if ([item action] == @selector(toggleDebuggingJavaScript:) && isMenuItem) {
        NSMenuItem *menuItem = (NSMenuItem *)item;
        if ([[_inspectedWebView inspector] isDebuggingJavaScript])
            [menuItem setTitle:UI_STRING("Stop Debugging JavaScript", "title for Stop Debugging JavaScript menu item")];
        else
            [menuItem setTitle:UI_STRING("Start Debugging JavaScript", "title for Start Debugging JavaScript menu item")];
    } else if ([item action] == @selector(toggleProfilingJavaScript:) && isMenuItem) {
        NSMenuItem *menuItem = (NSMenuItem *)item;
        if ([[_inspectedWebView inspector] isProfilingJavaScript])
            [menuItem setTitle:UI_STRING("Stop Profiling JavaScript", "title for Stop Profiling JavaScript menu item")];
        else
            [menuItem setTitle:UI_STRING("Start Profiling JavaScript", "title for Start Profiling JavaScript menu item")];
    }

    return YES;
}

@end
