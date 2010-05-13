/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */
#if USE(PLUGIN_HOST_PROCESS)

#import "WebHostedNetscapePluginView.h"

#import "HostedNetscapePluginStream.h"
#import "NetscapePluginInstanceProxy.h"
#import "NetscapePluginHostManager.h"
#import "NetscapePluginHostProxy.h"
#import "WebTextInputWindowController.h"
#import "WebFrameInternal.h"
#import "WebView.h"
#import "WebViewInternal.h"
#import "WebUIDelegate.h"

#import <CoreFoundation/CoreFoundation.h>
#import <WebCore/Bridge.h>
#import <WebCore/Frame.h>
#import <WebCore/FrameLoaderTypes.h>
#import <WebCore/FrameView.h>
#import <WebCore/HTMLPlugInElement.h>
#import <WebCore/RenderEmbeddedObject.h>
#import <WebCore/WebCoreObjCExtras.h>
#import <WebCore/runtime_root.h>
#import <runtime/InitializeThreading.h>
#import <wtf/Assertions.h>
#import <wtf/Threading.h>

using namespace WebCore;
using namespace WebKit;

extern "C" {
#include "WebKitPluginClientServer.h"
#include "WebKitPluginHost.h"
}

@implementation WebHostedNetscapePluginView

+ (void)initialize
{
    JSC::initializeThreading();
    WTF::initializeMainThreadToProcessMainThread();
#ifndef BUILDING_ON_TIGER
    WebCoreObjCFinalizeOnMainThread(self);
#endif
    WKSendUserChangeNotifications();
}

- (id)initWithFrame:(NSRect)frame
      pluginPackage:(WebNetscapePluginPackage *)pluginPackage
                URL:(NSURL *)URL
            baseURL:(NSURL *)baseURL
           MIMEType:(NSString *)MIME
      attributeKeys:(NSArray *)keys
    attributeValues:(NSArray *)values
       loadManually:(BOOL)loadManually
            element:(PassRefPtr<WebCore::HTMLPlugInElement>)element
{
    self = [super initWithFrame:frame pluginPackage:pluginPackage URL:URL baseURL:baseURL MIMEType:MIME attributeKeys:keys attributeValues:values loadManually:loadManually element:element];
    if (!self)
        return nil;
    
    return self;
}    

- (void)handleMouseMoved:(NSEvent *)event
{
    if (_isStarted && _proxy)
        _proxy->mouseEvent(self, event, NPCocoaEventMouseMoved);
}

- (void)setAttributeKeys:(NSArray *)keys andValues:(NSArray *)values
{
    ASSERT(!_attributeKeys);
    ASSERT(!_attributeValues);
    
    _attributeKeys.adoptNS([keys copy]);
    _attributeValues.adoptNS([values copy]);
}    

- (BOOL)createPlugin
{
    ASSERT(!_proxy);

    NSString *userAgent = [[self webView] userAgentForURL:_baseURL.get()];
    BOOL accleratedCompositingEnabled = false;
#if USE(ACCELERATED_COMPOSITING)
    accleratedCompositingEnabled = [[[self webView] preferences] acceleratedCompositingEnabled];
#endif
    
    _proxy = NetscapePluginHostManager::shared().instantiatePlugin(_pluginPackage.get(), self, _MIMEType.get(), _attributeKeys.get(), _attributeValues.get(), userAgent, _sourceURL.get(), 
                                                                   _mode == NP_FULL, _isPrivateBrowsingEnabled, accleratedCompositingEnabled);
    if (!_proxy) 
        return NO;

    if (_proxy->rendererType() == UseSoftwareRenderer)
        _softwareRenderer = WKSoftwareCARendererCreate(_proxy->renderContextID());
    else {
        _pluginLayer = WKMakeRenderLayer(_proxy->renderContextID());

        if (accleratedCompositingEnabled && _proxy->rendererType() == UseAcceleratedCompositing) {
            // Eagerly enter compositing mode, since we know we'll need it. This avoids firing setNeedsStyleRecalc()
            // for iframes that contain composited plugins at bad times. https://bugs.webkit.org/show_bug.cgi?id=39033
            core([self webFrame])->view()->enterCompositingMode();
            [self element]->setNeedsStyleRecalc(SyntheticStyleChange);
        } else
            self.wantsLayer = YES;
    }
    
    // Update the window frame.
    _proxy->windowFrameChanged([[self window] frame]);
    
    return YES;
}

// FIXME: This method is an ideal candidate to move up to the base class
- (CALayer *)pluginLayer
{
    return _pluginLayer.get();
}

- (void)setLayer:(CALayer *)newLayer
{
    // FIXME: This should use the same implementation as WebNetscapePluginView (and move to the base class).
    [super setLayer:newLayer];
    
    if (_pluginLayer)
        [newLayer addSublayer:_pluginLayer.get()];
}

- (void)privateBrowsingModeDidChange
{
    if (_proxy)
        _proxy->privateBrowsingModeDidChange(_isPrivateBrowsingEnabled);
}

- (void)loadStream
{
}

- (void)updateAndSetWindow
{
    if (!_proxy)
        return;
    
    // Use AppKit to convert view coordinates to NSWindow coordinates.
    NSRect boundsInWindow = [self convertRect:[self bounds] toView:nil];
    NSRect visibleRectInWindow;
    
    // Core Animation plug-ins need to be updated (with a 0,0,0,0 clipRect) when
    // moved to a background tab. We don't do this for Core Graphics plug-ins as
    // older versions of Flash have historical WebKit-specific code that isn't
    // compatible with this behavior.    
    BOOL shouldClipOutPlugin = _pluginLayer && [self shouldClipOutPlugin];
    if (!shouldClipOutPlugin)
        visibleRectInWindow = [self convertRect:[self visibleRect] toView:nil];
    else
        visibleRectInWindow = NSZeroRect;
    
    // Flip Y to convert NSWindow coordinates to top-left-based window coordinates.
    float borderViewHeight = [[self currentWindow] frame].size.height;
    boundsInWindow.origin.y = borderViewHeight - NSMaxY(boundsInWindow);
        
    if (!shouldClipOutPlugin)
        visibleRectInWindow.origin.y = borderViewHeight - NSMaxY(visibleRectInWindow);

    _previousSize = boundsInWindow.size;
    
    _proxy->resize(boundsInWindow, visibleRectInWindow);
}

- (void)windowFocusChanged:(BOOL)hasFocus
{
    if (_proxy)
        _proxy->windowFocusChanged(hasFocus);
}

- (BOOL)shouldStop
{
    if (!_proxy)
        return YES;
    
    return _proxy->shouldStop();
}

- (void)destroyPlugin
{
    if (_proxy) {
        if (_softwareRenderer) {
            WKSoftwareCARendererDestroy(_softwareRenderer);
            _softwareRenderer = 0;
        }
        
        _proxy->destroy();
        _proxy = 0;
    }
    
    _pluginLayer = 0;
}

- (void)startTimers
{
    if (_proxy)
        _proxy->startTimers(_isCompletelyObscured);
}

- (void)stopTimers
{
    if (_proxy)
        _proxy->stopTimers();
}

- (void)focusChanged
{
    if (_proxy)
        _proxy->focusChanged(_hasFocus);
}

- (void)windowFrameDidChange:(NSNotification *)notification 
{
    if (_proxy && [self window])
        _proxy->windowFrameChanged([[self window] frame]);
}

- (void)addWindowObservers
{
    [super addWindowObservers];
    
    ASSERT([self window]);
    
    NSWindow *window = [self window];
    
    NSNotificationCenter *notificationCenter = [NSNotificationCenter defaultCenter];
    [notificationCenter addObserver:self selector:@selector(windowFrameDidChange:) 
                               name:NSWindowDidMoveNotification object:window];
    [notificationCenter addObserver:self selector:@selector(windowFrameDidChange:)
                               name:NSWindowDidResizeNotification object:window];    

    if (_proxy)
        _proxy->windowFrameChanged([window frame]);
    [self updateAndSetWindow];
}

- (void)removeWindowObservers
{
    [super removeWindowObservers];
    
    NSNotificationCenter *notificationCenter = [NSNotificationCenter defaultCenter];
    [notificationCenter removeObserver:self name:NSWindowDidMoveNotification object:nil];
    [notificationCenter removeObserver:self name:NSWindowDidResizeNotification object:nil];
}

- (void)mouseDown:(NSEvent *)event
{
    if (_isStarted && _proxy)
        _proxy->mouseEvent(self, event, NPCocoaEventMouseDown);
}

- (void)mouseUp:(NSEvent *)event
{
    if (_isStarted && _proxy)
        _proxy->mouseEvent(self, event, NPCocoaEventMouseUp);
}

- (void)mouseDragged:(NSEvent *)event
{
    if (_isStarted && _proxy)
        _proxy->mouseEvent(self, event, NPCocoaEventMouseDragged);
}

- (void)handleMouseEntered:(NSEvent *)event
{
    // Set cursor to arrow. Plugins often handle cursor internally, but those that don't will just get this default one.
    [[NSCursor arrowCursor] set];

    if (_isStarted && _proxy)
        _proxy->mouseEvent(self, event, NPCocoaEventMouseEntered);
}

- (void)handleMouseExited:(NSEvent *)event
{
    if (_isStarted && _proxy)
        _proxy->mouseEvent(self, event, NPCocoaEventMouseExited);

    // Set cursor back to arrow cursor.  Because NSCursor doesn't know about changes that the plugin made, we could get confused about what we think the
    // current cursor is otherwise.  Therefore we have no choice but to unconditionally reset the cursor when the mouse exits the plugin.
    // FIXME: This should be job of plugin host, see <rdar://problem/7654434>.
    [[NSCursor arrowCursor] set];
}

- (void)scrollWheel:(NSEvent *)event
{
    bool processedEvent = false;
    
    if (_isStarted && _proxy)
        processedEvent = _proxy->wheelEvent(self, event);
    
    if (!processedEvent)
        [super scrollWheel:event];
}

- (NSTextInputContext *)inputContext
{
    return [[WebTextInputWindowController sharedTextInputWindowController] inputContext];
}

- (void)keyDown:(NSEvent *)event
{
    if (!_isStarted || !_proxy)
        return;
    
    NSString *string = nil;
    if ([[WebTextInputWindowController sharedTextInputWindowController] interpretKeyEvent:event string:&string]) {
        if (string)
            _proxy->insertText(string);
        return;
    }
    
    _proxy->keyEvent(self, event, NPCocoaEventKeyDown);
}

- (void)keyUp:(NSEvent *)event
{
    if (_isStarted && _proxy)
        _proxy->keyEvent(self, event, NPCocoaEventKeyUp);
}

- (void)flagsChanged:(NSEvent *)event
{
    if (_isStarted && _proxy)
        _proxy->flagsChanged(event);
}

- (void)sendModifierEventWithKeyCode:(int)keyCode character:(char)character
{
    if (_isStarted && _proxy)
        _proxy->syntheticKeyDownWithCommandModifier(keyCode, character);
}

- (void)pluginHostDied
{
    RenderEmbeddedObject* renderer = toRenderEmbeddedObject(_element->renderer());
    if (renderer)
        renderer->setShowsCrashedPluginIndicator();

    _pluginLayer = nil;
    _proxy = 0;
    
    // No need for us to be layer backed anymore
    self.wantsLayer = NO;
    
    [self invalidatePluginContentRect:[self bounds]];
}

- (void)visibleRectDidChange
{
    [super visibleRectDidChange];
    WKSyncSurfaceToView(self);
}

- (void)drawRect:(NSRect)rect
{
    if (_proxy) {
        if (_softwareRenderer) {
            if ([NSGraphicsContext currentContextDrawingToScreen]) {
                WKSoftwareCARendererRender(_softwareRenderer, (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort], NSRectToCGRect(rect));
                _proxy->didDraw();
            } else
                _proxy->print(reinterpret_cast<CGContextRef>([[NSGraphicsContext currentContext] graphicsPort]), [self bounds].size.width, [self bounds].size.height);
        } else if ([self inFlatteningPaint] && [self supportsSnapshotting]) {
            _proxy->snapshot(reinterpret_cast<CGContextRef>([[NSGraphicsContext currentContext] graphicsPort]), [self bounds].size.width, [self bounds].size.height);
        }

        return;
    }
}

- (PassRefPtr<JSC::Bindings::Instance>)createPluginBindingsInstance:(PassRefPtr<JSC::Bindings::RootObject>)rootObject
{
    if (!_proxy)
        return 0;
    
    return _proxy->createBindingsInstance(rootObject);
}

- (void)pluginView:(NSView *)pluginView receivedResponse:(NSURLResponse *)response
{
    ASSERT(_loadManually);
    if (!_proxy)
        return;
    
    ASSERT(!_proxy->manualStream());

    _proxy->setManualStream(HostedNetscapePluginStream::create(_proxy.get(), core([self webFrame])->loader()));
    _proxy->manualStream()->startStreamWithResponse(response);
}

- (void)pluginView:(NSView *)pluginView receivedData:(NSData *)data
{
    ASSERT(_loadManually);
    if (!_proxy)
        return;
    
    if (HostedNetscapePluginStream* manualStream = _proxy->manualStream())
        manualStream->didReceiveData(0, static_cast<const char*>([data bytes]), [data length]);
}

- (void)pluginView:(NSView *)pluginView receivedError:(NSError *)error
{
    ASSERT(_loadManually);
    if (!_proxy)
        return;
    
    if (HostedNetscapePluginStream* manualStream = _proxy->manualStream())
        manualStream->didFail(0, error);
}

- (void)pluginViewFinishedLoading:(NSView *)pluginView
{
    ASSERT(_loadManually);
    if (!_proxy)
        return;
    
    if (HostedNetscapePluginStream* manualStream = _proxy->manualStream())
        manualStream->didFinishLoading(0);
}

- (void)_webPluginContainerCancelCheckIfAllowedToLoadRequest:(id)webPluginContainerCheck
{
    ASSERT([webPluginContainerCheck isKindOfClass:[WebPluginContainerCheck class]]);
    
    id contextInfo = [webPluginContainerCheck contextInfo];
    ASSERT([contextInfo isKindOfClass:[NSNumber class]]);

    if (!_proxy)
        return;

    uint32_t checkID = [(NSNumber *)contextInfo unsignedIntValue];
    _proxy->cancelCheckIfAllowedToLoadURL(checkID);
}

- (void)_containerCheckResult:(PolicyAction)policy contextInfo:(id)contextInfo
{
    ASSERT([contextInfo isKindOfClass:[NSNumber class]]);
    if (!_proxy)
        return;

    uint32_t checkID = [(NSNumber *)contextInfo unsignedIntValue];
    _proxy->checkIfAllowedToLoadURLResult(checkID, (policy == PolicyUse));
}

- (void)webFrame:(WebFrame *)webFrame didFinishLoadWithReason:(NPReason)reason
{
    if (_isStarted && _proxy)
        _proxy->webFrameDidFinishLoadWithReason(webFrame, reason);
}

- (void)webFrame:(WebFrame *)webFrame didFinishLoadWithError:(NSError *)error
{
    NPReason reason = NPRES_DONE;
    if (error)
        reason = HostedNetscapePluginStream::reasonForError(error);
    [self webFrame:webFrame didFinishLoadWithReason:reason];
}

@end

#endif
