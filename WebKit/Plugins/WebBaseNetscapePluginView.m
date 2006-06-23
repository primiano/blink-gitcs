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

#import <WebKit/WebBaseNetscapePluginView.h>

#import <JavaScriptCore/Assertions.h>
#import <WebKit/WebFrameBridge.h>
#import <WebKit/WebDataSource.h>
#import <WebKit/WebDefaultUIDelegate.h>
#import <WebKit/WebFrameInternal.h> 
#import <WebKit/WebFrameView.h>
#import <WebKit/WebKitLogging.h>
#import <WebKit/WebKitNSStringExtras.h>
#import <WebKit/WebNetscapePluginStream.h>
#import <WebKit/WebNullPluginView.h>
#import <WebKit/WebNSDataExtras.h>
#import <WebKit/WebNSDictionaryExtras.h>
#import <WebKit/WebNSObjectExtras.h>
#import <WebKit/WebNSURLExtras.h>
#import <WebKit/WebNSURLRequestExtras.h>
#import <WebKit/WebNSViewExtras.h>
#import <WebKit/WebNetscapePluginPackage.h>
#import <WebKit/WebPreferences.h>
#import <WebKit/WebViewInternal.h>

#import <WebKit/WebUIDelegate.h>
#import <WebKitSystemInterface.h>
#import <JavaScriptCore/npruntime_impl.h>

#import <Carbon/Carbon.h>

#import <objc/objc-runtime.h>

// Send null events 50 times a second when active, so plug-ins like Flash get high frame rates.
#define NullEventIntervalActive 	0.02
#define NullEventIntervalNotActive	0.25

#define LoginWindowDidSwitchFromUserNotification    @"WebLoginWindowDidSwitchFromUserNotification"
#define LoginWindowDidSwitchToUserNotification      @"WebLoginWindowDidSwitchToUserNotification"

@interface WebBaseNetscapePluginView (Internal)
- (NSBitmapImageRep *)_printedPluginBitmap;
@end

static WebBaseNetscapePluginView *currentPluginView = nil;

typedef struct OpaquePortState* PortState;

#ifndef NP_NO_QUICKDRAW

// QuickDraw is not available in 64-bit

typedef struct {
    GrafPtr oldPort;
    Point oldOrigin;
    RgnHandle oldClipRegion;
    RgnHandle oldVisibleRegion;
    RgnHandle clipRegion;
    BOOL forUpdate;
} PortState_QD;

#endif /* NP_NO_QUICKDRAW */

typedef struct {
    CGContextRef context;
    BOOL forUpdate;
} PortState_CG;

@interface WebPluginRequest : NSObject
{
    NSURLRequest *_request;
    NSString *_frameName;
    void *_notifyData;
    BOOL _didStartFromUserGesture;
    BOOL _sendNotification;
}

- (id)initWithRequest:(NSURLRequest *)request frameName:(NSString *)frameName notifyData:(void *)notifyData sendNotification:(BOOL)sendNotification didStartFromUserGesture:(BOOL)currentEventIsUserGesture;

- (NSURLRequest *)request;
- (NSString *)frameName;
- (void *)notifyData;
- (BOOL)isCurrentEventUserGesture;
- (BOOL)sendNotification;

@end

@interface NSData (WebPluginDataExtras)
- (BOOL)_web_startsWithBlankLine;
- (WebNSInteger)_web_locationAfterFirstBlankLine;
@end

static OSStatus TSMEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void *pluginView);

@interface WebBaseNetscapePluginView (ForwardDeclarations)
- (void)setWindowIfNecessary;
@end

@implementation WebBaseNetscapePluginView

+ (void)initialize
{
	WKSendUserChangeNotifications();
}

#pragma mark EVENTS

+ (void)getCarbonEvent:(EventRecord *)carbonEvent
{
    carbonEvent->what = nullEvent;
    carbonEvent->message = 0;
    carbonEvent->when = TickCount();
    GetGlobalMouse(&carbonEvent->where);
    carbonEvent->modifiers = GetCurrentKeyModifiers();
    if (!Button())
        carbonEvent->modifiers |= btnState;
}

- (void)getCarbonEvent:(EventRecord *)carbonEvent
{
    [[self class] getCarbonEvent:carbonEvent];
}

- (EventModifiers)modifiersForEvent:(NSEvent *)event
{
    EventModifiers modifiers;
    unsigned int modifierFlags = [event modifierFlags];
    NSEventType eventType = [event type];
    
    modifiers = 0;
    
    if (eventType != NSLeftMouseDown && eventType != NSRightMouseDown)
        modifiers |= btnState;
    
    if (modifierFlags & NSCommandKeyMask)
        modifiers |= cmdKey;
    
    if (modifierFlags & NSShiftKeyMask)
        modifiers |= shiftKey;

    if (modifierFlags & NSAlphaShiftKeyMask)
        modifiers |= alphaLock;

    if (modifierFlags & NSAlternateKeyMask)
        modifiers |= optionKey;

    if (modifierFlags & NSControlKeyMask || eventType == NSRightMouseDown)
        modifiers |= controlKey;
    
    return modifiers;
}

- (void)getCarbonEvent:(EventRecord *)carbonEvent withEvent:(NSEvent *)cocoaEvent
{
    if (WKConvertNSEventToCarbonEvent(carbonEvent, cocoaEvent)) {
        return;
    }
    
    NSPoint where = [[cocoaEvent window] convertBaseToScreen:[cocoaEvent locationInWindow]];
        
    carbonEvent->what = nullEvent;
    carbonEvent->message = 0;
    carbonEvent->when = (UInt32)([cocoaEvent timestamp] * 60); // seconds to ticks
    carbonEvent->where.h = (short)where.x;
    carbonEvent->where.v = (short)(NSMaxY([[[NSScreen screens] objectAtIndex:0] frame]) - where.y);
    carbonEvent->modifiers = [self modifiersForEvent:cocoaEvent];
}

- (BOOL)superviewsHaveSuperviews
{
    NSView *contentView = [[self window] contentView];
    NSView *view;
    for (view = self; view != nil; view = [view superview]) { 
        if (view == contentView) {
            return YES;
        }
    }
    return NO;
}

#ifndef NP_NO_QUICKDRAW
// The WindowRef created by -[NSWindow windowRef] has a QuickDraw GrafPort that covers 
// the entire window frame (or structure region to use the Carbon term) rather then just the window content.
// We can remove this when <rdar://problem/4201099> is fixed.
- (void)fixWindowPort
{
    ASSERT(drawingModel == NPDrawingModelQuickDraw);
    
    NSWindow *currentWindow = [self currentWindow];
    if ([currentWindow isKindOfClass:objc_getClass("NSCarbonWindow")])
        return;
    
    float windowHeight = [currentWindow frame].size.height;
    NSView *contentView = [currentWindow contentView];
    NSRect contentRect = [contentView convertRect:[contentView frame] toView:nil]; // convert to window-relative coordinates
    
    CGrafPtr oldPort;
    GetPort(&oldPort);    
    SetPort(GetWindowPort([currentWindow windowRef]));
    
    MovePortTo(contentRect.origin.x, /* Flip Y */ windowHeight - NSMaxY(contentRect));
    PortSize(contentRect.size.width, contentRect.size.height);
    
    SetPort(oldPort);
}
#endif

- (PortState)saveAndSetNewPortStateForUpdate:(BOOL)forUpdate
{
    ASSERT([self currentWindow] != nil);

    // A CoreGraphics plugin's window may only be set while the plugin view is being updated
    ASSERT(drawingModel != NPDrawingModelCoreGraphics || (forUpdate && [NSView focusView] == self));
     
#ifndef NP_NO_QUICKDRAW
    // If drawing with QuickDraw, fix the window port so that it has the same bounds as the NSWindow's
    // content view.  This makes it easier to convert between AppKit view and QuickDraw port coordinates.
    if (drawingModel == NPDrawingModelQuickDraw)
        [self fixWindowPort];
#endif
    
    WindowRef windowRef = [[self currentWindow] windowRef];
    ASSERT(windowRef);
    
    // Use AppKit to convert view coordinates to NSWindow coordinates.
    NSRect boundsInWindow = [self convertRect:[self bounds] toView:nil];
    NSRect visibleRectInWindow = [self convertRect:[self visibleRect] toView:nil];
    
    // Flip Y to convert NSWindow coordinates to top-left-based window coordinates.
    float borderViewHeight = [[self currentWindow] frame].size.height;
    boundsInWindow.origin.y = borderViewHeight - NSMaxY(boundsInWindow);
    visibleRectInWindow.origin.y = borderViewHeight - NSMaxY(visibleRectInWindow);
    
#ifndef NP_NO_QUICKDRAW
    // Look at the Carbon port to convert top-left-based window coordinates into top-left-based content coordinates.
    if (drawingModel == NPDrawingModelQuickDraw) {
        Rect portBounds;
        CGrafPtr port = GetWindowPort(windowRef);
        GetPortBounds(port, &portBounds);

        PixMap *pix = *GetPortPixMap(port);
        boundsInWindow.origin.x += pix->bounds.left - portBounds.left;
        boundsInWindow.origin.y += pix->bounds.top - portBounds.top;
        visibleRectInWindow.origin.x += pix->bounds.left - portBounds.left;
        visibleRectInWindow.origin.y += pix->bounds.top - portBounds.top;
    }
#endif
    
    window.x = (int32)boundsInWindow.origin.x; 
    window.y = (int32)boundsInWindow.origin.y;
    window.width = NSWidth(boundsInWindow);
    window.height = NSHeight(boundsInWindow);
    
    // "Clip-out" the plug-in when:
    // 1) it's not really in a window or off-screen or has no height or width.
    // 2) window.x is a "big negative number" which is how WebCore expresses off-screen widgets.
    // 3) the window is miniaturized or the app is hidden
    // 4) we're inside of viewWillMoveToWindow: with a nil window. In this case, superviews may already have nil 
    // superviews and nil windows and results from convertRect:toView: are incorrect.
    NSWindow *realWindow = [self window];
    if (window.width <= 0 || window.height <= 0 || window.x < -100000
            || realWindow == nil || [realWindow isMiniaturized]
            || [NSApp isHidden]
            || ![self superviewsHaveSuperviews]
            || [self isHiddenOrHasHiddenAncestor]) {
        // The following code tries to give plug-ins the same size they will eventually have.
        // The specifiedWidth and specifiedHeight variables are used to predict the size that
        // WebCore will eventually resize us to.

        // The QuickTime plug-in has problems if you give it a width or height of 0.
        // Since other plug-ins also might have the same sort of trouble, we make sure
        // to always give plug-ins a size other than 0,0.

        if (window.width <= 0) {
            window.width = specifiedWidth > 0 ? specifiedWidth : 100;
        }
        if (window.height <= 0) {
            window.height = specifiedHeight > 0 ? specifiedHeight : 100;
        }

        window.clipRect.bottom = window.clipRect.top;
        window.clipRect.left = window.clipRect.right;
    } else {
        window.clipRect.top = (uint16)visibleRectInWindow.origin.y;
        window.clipRect.left = (uint16)visibleRectInWindow.origin.x;
        window.clipRect.bottom = (uint16)(visibleRectInWindow.origin.y + visibleRectInWindow.size.height);
        window.clipRect.right = (uint16)(visibleRectInWindow.origin.x + visibleRectInWindow.size.width);        
    }

    window.type = NPWindowTypeWindow;
    
    // Save the port state, set up the port for entry into the plugin
    PortState portState;
    switch (drawingModel) {
#ifndef NP_NO_QUICKDRAW
        case NPDrawingModelQuickDraw:
        {
            // Set up NS_Port.
            Rect portBounds;
            CGrafPtr port = GetWindowPort(windowRef);
            GetPortBounds(port, &portBounds);
            nPort.qdPort.port = port;
            nPort.qdPort.portx = (int32)-boundsInWindow.origin.x;
            nPort.qdPort.porty = (int32)-boundsInWindow.origin.y;
            window.window = &nPort;

            PortState_QD *qdPortState = malloc(sizeof(PortState_QD));
            portState = (PortState)qdPortState;
            
            GetPort(&qdPortState->oldPort);    

            qdPortState->oldOrigin.h = portBounds.left;
            qdPortState->oldOrigin.v = portBounds.top;

            qdPortState->oldClipRegion = NewRgn();
            GetPortClipRegion(port, qdPortState->oldClipRegion);
            
            qdPortState->oldVisibleRegion = NewRgn();
            GetPortVisibleRegion(port, qdPortState->oldVisibleRegion);
            
            RgnHandle clipRegion = NewRgn();
            qdPortState->clipRegion = clipRegion;
            
            MacSetRectRgn(clipRegion,
                window.clipRect.left + nPort.qdPort.portx, window.clipRect.top + nPort.qdPort.porty,
                window.clipRect.right + nPort.qdPort.portx, window.clipRect.bottom + nPort.qdPort.porty);
            
            // Clip to dirty region when updating in "windowless" mode (transparent)
            if (forUpdate && isTransparent) {
                RgnHandle viewClipRegion = NewRgn();
                
                // Get list of dirty rects from the opaque ancestor -- WebKit does some tricks with invalidation and
                // display to enable z-ordering for NSViews; a side-effect of this is that only the WebHTMLView
                // knows about the true set of dirty rects.
                NSView *opaqueAncestor = [self opaqueAncestor];
                const NSRect *dirtyRects;
                int dirtyRectCount, dirtyRectIndex;
                [opaqueAncestor getRectsBeingDrawn:&dirtyRects count:&dirtyRectCount];

                for (dirtyRectIndex = 0; dirtyRectIndex < dirtyRectCount; dirtyRectIndex++) {
                    NSRect dirtyRect = [self convertRect:dirtyRects[dirtyRectIndex] fromView:opaqueAncestor];
                    if (!NSEqualSizes(dirtyRect.size, NSZeroSize)) {
                        // Create a region for this dirty rect
                        RgnHandle dirtyRectRegion = NewRgn();
                        SetRectRgn(dirtyRectRegion, NSMinX(dirtyRect), NSMinY(dirtyRect), NSMaxX(dirtyRect), NSMaxY(dirtyRect));
                        
                        // Union this dirty rect with the rest of the dirty rects
                        UnionRgn(viewClipRegion, dirtyRectRegion, viewClipRegion);
                        DisposeRgn(dirtyRectRegion);
                    }
                }
            
                // Intersect the dirty region with the clip region, so that we only draw over dirty parts
                SectRgn(clipRegion, viewClipRegion, clipRegion);
                DisposeRgn(viewClipRegion);
            }
            
            qdPortState->forUpdate = forUpdate;
            
            // Switch to the port and set it up.
            SetPort(port);

            PenNormal();
            ForeColor(blackColor);
            BackColor(whiteColor);
            
            SetOrigin(nPort.qdPort.portx, nPort.qdPort.porty);

            SetPortClipRegion(nPort.qdPort.port, clipRegion);

            if (forUpdate) {
                // AppKit may have tried to help us by doing a BeginUpdate.
                // But the invalid region at that level didn't include AppKit's notion of what was not valid.
                // We reset the port's visible region to counteract what BeginUpdate did.
                SetPortVisibleRegion(nPort.qdPort.port, clipRegion);

                // Some plugins do their own BeginUpdate/EndUpdate.
                // For those, we must make sure that the update region contains the area we want to draw.
                InvalWindowRgn(windowRef, clipRegion);
            }
        }
        break;
#endif /* NP_NO_QUICKDRAW */
        
        case NPDrawingModelCoreGraphics:
        {            
            ASSERT([NSView focusView] == self);

            PortState_CG *cgPortState = (PortState_CG *)malloc(sizeof(PortState_CG));
            portState = (PortState)cgPortState;
            cgPortState->forUpdate = YES;
            cgPortState->context = [[NSGraphicsContext currentContext] graphicsPort];
            
            // Update the plugin's window/context
            nPort.cgPort.window = windowRef;
            nPort.cgPort.context = cgPortState->context;
            window.window = &nPort.cgPort;

            // Save current graphics context's state; will be restored by -restorePortState:
            CGContextSaveGState(nPort.cgPort.context);
            
            // FIXME (4544971): Clip to dirty region when updating in "windowless" mode (transparent), like in the QD case
        }
        break;
        
        default:
            ASSERT_NOT_REACHED();
            portState = NULL;
        break;
    }
    
    return portState;
}

- (PortState)saveAndSetNewPortState
{
    return [self saveAndSetNewPortStateForUpdate:NO];
}

- (void)restorePortState:(PortState)portState
{
    ASSERT([self currentWindow]);
    ASSERT(portState);
    
    switch (drawingModel) {
#ifndef NP_NO_QUICKDRAW
        case NPDrawingModelQuickDraw:
        {
            PortState_QD *qdPortState = (PortState_QD *)portState;
            WindowRef windowRef = [[self currentWindow] windowRef];
            CGrafPtr port = GetWindowPort(windowRef);
            if (qdPortState->forUpdate)
                ValidWindowRgn(windowRef, qdPortState->clipRegion);
            
            SetOrigin(qdPortState->oldOrigin.h, qdPortState->oldOrigin.v);

            SetPortClipRegion(port, qdPortState->oldClipRegion);
            if (qdPortState->forUpdate)
                SetPortVisibleRegion(port, qdPortState->oldVisibleRegion);

            DisposeRgn(qdPortState->oldClipRegion);
            DisposeRgn(qdPortState->oldVisibleRegion);
            DisposeRgn(qdPortState->clipRegion);

            SetPort(qdPortState->oldPort);
        }
        break;
#endif /* NP_NO_QUICKDRAW */
        
        case NPDrawingModelCoreGraphics:
        {
            PortState_CG *cgPortState = (PortState_CG *)portState;
            if (cgPortState->forUpdate) {
                ASSERT([NSView focusView] == self);
                ASSERT(cgPortState->context == nPort.cgPort.context);
                CGContextRestoreGState(nPort.cgPort.context);
            }
        }
        break;
        
        default:
            ASSERT_NOT_REACHED();
        break;
    }
}

- (BOOL)sendEvent:(EventRecord *)event
{
    ASSERT([self window]);
    ASSERT(event);
   
    // If at any point the user clicks or presses a key from within a plugin, set the 
    // currentEventIsUserGesture flag to true. This is important to differentiate legitimate 
    // window.open() calls;  we still want to allow those.  See rdar://problem/4010765
    if (event->what == mouseDown || event->what == keyDown || event->what == mouseUp || event->what == autoKey)
        currentEventIsUserGesture = YES;
    
    suspendKeyUpEvents = NO;
    
    if (!isStarted)
        return NO;

    ASSERT(NPP_HandleEvent);
    
    // Make sure we don't call NPP_HandleEvent while we're inside NPP_SetWindow.
    // We probably don't want more general reentrancy protection; we are really
    // protecting only against this one case, which actually comes up when
    // you first install the SVG viewer plug-in.
    if (inSetWindow)
        return NO;

    BOOL defers = [[self webView] defersCallbacks];
    if (!defers)
        [[self webView] setDefersCallbacks:YES];

    // Can only send updateEvt to CoreGraphics plugins when actually drawing
    ASSERT(drawingModel != NPDrawingModelCoreGraphics || event->what != updateEvt || [NSView focusView] == self);
    
    BOOL updating = event->what == updateEvt;
    PortState portState;
    if (drawingModel != NPDrawingModelCoreGraphics || event->what == updateEvt) {
        // Can only save/set port state when updating in CoreGraphics mode.  We can save/set the port state
        // at any time in other modes.
        portState = [self saveAndSetNewPortStateForUpdate:updating];
        
        // We may have changed the window, so inform the plug-in.
        [self setWindowIfNecessary];
    } else
        portState = NULL;
    
#if !defined(NDEBUG) && !defined(NP_NO_QUICKDRAW)
    // Draw green to help debug.
    // If we see any green we know something's wrong.
    // Note that PaintRect() only works for QuickDraw plugins; otherwise the current QD port is undefined.
    if (drawingModel == NPDrawingModelQuickDraw && !isTransparent && event->what == updateEvt) {
        ForeColor(greenColor);
        const Rect bigRect = { -10000, -10000, 10000, 10000 };
        PaintRect(&bigRect);
        ForeColor(blackColor);
    }
#endif
    
    // Temporarily retain self in case the plug-in view is released while sending an event. 
    [[self retain] autorelease];
    
    BOOL acceptedEvent = NPP_HandleEvent(instance, event);
    
    currentEventIsUserGesture = NO;
    
    if (portState) {
        if ([self currentWindow])
            [self restorePortState:portState];
        free(portState);
    }

    if (!defers)
        [[self webView] setDefersCallbacks:NO];
            
    return acceptedEvent;
}

- (void)sendActivateEvent:(BOOL)activate
{
    EventRecord event;
    
    [self getCarbonEvent:&event];
    event.what = activateEvt;
    WindowRef windowRef = [[self window] windowRef];
    event.message = (unsigned long)windowRef;
    if (activate) {
        event.modifiers |= activeFlag;
    }
    
    BOOL acceptedEvent;
    acceptedEvent = [self sendEvent:&event]; 
    
    LOG(PluginEvents, "NPP_HandleEvent(activateEvent): %d  isActive: %d", acceptedEvent, activate);
}

- (BOOL)sendUpdateEvent
{
    EventRecord event;
    
    [self getCarbonEvent:&event];
    event.what = updateEvt;
    WindowRef windowRef = [[self window] windowRef];
    event.message = (unsigned long)windowRef;

    BOOL acceptedEvent = [self sendEvent:&event];

    LOG(PluginEvents, "NPP_HandleEvent(updateEvt): %d", acceptedEvent);

    return acceptedEvent;
}

-(void)sendNullEvent
{
    EventRecord event;

    [self getCarbonEvent:&event];

    // Plug-in should not react to cursor position when not active or when a menu is down.
    MenuTrackingData trackingData;
    OSStatus error = GetMenuTrackingData(NULL, &trackingData);

    // Plug-in should not react to cursor position when the actual window is not key.
    if (![[self window] isKeyWindow] || (error == noErr && trackingData.menu)) {
        // FIXME: Does passing a v and h of -1 really prevent it from reacting to the cursor position?
        event.where.v = -1;
        event.where.h = -1;
    }

    [self sendEvent:&event];
}

- (void)stopNullEvents
{
    [nullEventTimer invalidate];
    [nullEventTimer release];
    nullEventTimer = nil;
}

- (void)restartNullEvents
{
    ASSERT([self window]);
    
    if (nullEventTimer)
        [self stopNullEvents];
    
    if ([[self window] isMiniaturized])
        return;

    NSTimeInterval interval;

    // Send null events less frequently when the actual window is not key.  Also, allow the DB
    // to override this behavior and send full speed events to non key windows.
    // If the plugin is completely obscured (scrolled out of view, for example), then we will
    // send null events at a reduced rate.
    if (!isCompletelyObscured && ([[self window] isKeyWindow] || [[self webView] _dashboardBehavior:WebDashboardBehaviorAlwaysSendActiveNullEventsToPlugIns]))
        interval = NullEventIntervalActive;
    else
        interval = NullEventIntervalNotActive;
    
    nullEventTimer = [[NSTimer scheduledTimerWithTimeInterval:interval
                                                       target:self
                                                     selector:@selector(sendNullEvent)
                                                     userInfo:nil
                                                      repeats:YES] retain];
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (void)installKeyEventHandler
{
    static const EventTypeSpec sTSMEvents[] =
    {
    { kEventClassTextInput, kEventTextInputUnicodeForKeyEvent }
    };
    
    if (!keyEventHandler) {
        InstallEventHandler(GetWindowEventTarget([[self window] windowRef]),
                            NewEventHandlerUPP(TSMEventHandler),
                            GetEventTypeCount(sTSMEvents),
                            sTSMEvents,
                            self,
                            &keyEventHandler);
    }
}

- (void)removeKeyEventHandler
{
    if (keyEventHandler) {
        RemoveEventHandler(keyEventHandler);
        keyEventHandler = NULL;
    }
}

- (void)setHasFocus:(BOOL)flag
{
    if (hasFocus != flag) {
        hasFocus = flag;
        EventRecord event;
        [self getCarbonEvent:&event];
        BOOL acceptedEvent;
        if (hasFocus) {
            event.what = getFocusEvent;
            acceptedEvent = [self sendEvent:&event]; 
            LOG(PluginEvents, "NPP_HandleEvent(getFocusEvent): %d", acceptedEvent);
            [self installKeyEventHandler];
        } else {
            event.what = loseFocusEvent;
            acceptedEvent = [self sendEvent:&event]; 
            LOG(PluginEvents, "NPP_HandleEvent(loseFocusEvent): %d", acceptedEvent);
            [self removeKeyEventHandler];
        }
    }
}

- (BOOL)becomeFirstResponder
{
    [self setHasFocus:YES];
    return YES;
}

- (BOOL)resignFirstResponder
{
    [self setHasFocus:NO];    
    return YES;
}

// AppKit doesn't call mouseDown or mouseUp on right-click. Simulate control-click
// mouseDown and mouseUp so plug-ins get the right-click event as they do in Carbon (3125743).
- (void)rightMouseDown:(NSEvent *)theEvent
{
    [self mouseDown:theEvent];
}

- (void)rightMouseUp:(NSEvent *)theEvent
{
    [self mouseUp:theEvent];
}

- (void)mouseDown:(NSEvent *)theEvent
{
    EventRecord event;

    [self getCarbonEvent:&event withEvent:theEvent];
    event.what = mouseDown;

    BOOL acceptedEvent;
    acceptedEvent = [self sendEvent:&event]; 
    
    LOG(PluginEvents, "NPP_HandleEvent(mouseDown): %d pt.v=%d, pt.h=%d", acceptedEvent, event.where.v, event.where.h);
}

- (void)mouseUp:(NSEvent *)theEvent
{
    EventRecord event;
    
    [self getCarbonEvent:&event withEvent:theEvent];
    event.what = mouseUp;

    BOOL acceptedEvent;
    acceptedEvent = [self sendEvent:&event]; 
    
    LOG(PluginEvents, "NPP_HandleEvent(mouseUp): %d pt.v=%d, pt.h=%d", acceptedEvent, event.where.v, event.where.h);
}

- (void)mouseEntered:(NSEvent *)theEvent
{
    EventRecord event;
    
    [self getCarbonEvent:&event withEvent:theEvent];
    event.what = adjustCursorEvent;

    BOOL acceptedEvent;
    acceptedEvent = [self sendEvent:&event]; 
    
    LOG(PluginEvents, "NPP_HandleEvent(mouseEntered): %d", acceptedEvent);
}

- (void)mouseExited:(NSEvent *)theEvent
{
    EventRecord event;
        
    [self getCarbonEvent:&event withEvent:theEvent];
    event.what = adjustCursorEvent;

    BOOL acceptedEvent;
    acceptedEvent = [self sendEvent:&event]; 
    
    LOG(PluginEvents, "NPP_HandleEvent(mouseExited): %d", acceptedEvent);
    
    // Set cursor back to arrow cursor.
    [[NSCursor arrowCursor] set];
}

- (void)mouseDragged:(NSEvent *)theEvent
{
    // Do nothing so that other responders don't respond to the drag that initiated in this view.
}

- (UInt32)keyMessageForEvent:(NSEvent *)event
{
    NSData *data = [[event characters] dataUsingEncoding:CFStringConvertEncodingToNSStringEncoding(CFStringGetSystemEncoding())];
    if (!data) {
        return 0;
    }
    UInt8 characterCode;
    [data getBytes:&characterCode length:1];
    UInt16 keyCode = [event keyCode];
    return keyCode << 8 | characterCode;
}

- (void)keyUp:(NSEvent *)theEvent
{
	WKSendKeyEventToTSM(theEvent);
    
    // TSM won't send keyUp events so we have to send them ourselves.
    // Only send keyUp events after we receive the TSM callback because this is what plug-in expect from OS 9.
    if (!suspendKeyUpEvents) {
        EventRecord event;
        
        [self getCarbonEvent:&event withEvent:theEvent];
        event.what = keyUp;
        
        if (event.message == 0) {
            event.message = [self keyMessageForEvent:theEvent];
        }
        
        [self sendEvent:&event];
    }
}

- (void)keyDown:(NSEvent *)theEvent
{
    suspendKeyUpEvents = YES;
    WKSendKeyEventToTSM(theEvent);
}

static OSStatus TSMEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void *pluginView)
{    
    EventRef rawKeyEventRef;
    OSStatus status = GetEventParameter(inEvent, kEventParamTextInputSendKeyboardEvent, typeEventRef, NULL, sizeof(EventRef), NULL, &rawKeyEventRef);
    if (status != noErr) {
        LOG_ERROR("GetEventParameter failed with error: %d", status);
        return noErr;
    }
    
    // Two-pass read to allocate/extract Mac charCodes
    ByteCount numBytes;    
    status = GetEventParameter(rawKeyEventRef, kEventParamKeyMacCharCodes, typeChar, NULL, 0, &numBytes, NULL);
    if (status != noErr) {
        LOG_ERROR("GetEventParameter failed with error: %d", status);
        return noErr;
    }
    char *buffer = malloc(numBytes);
    status = GetEventParameter(rawKeyEventRef, kEventParamKeyMacCharCodes, typeChar, NULL, numBytes, NULL, buffer);
    if (status != noErr) {
        LOG_ERROR("GetEventParameter failed with error: %d", status);
        free(buffer);
        return noErr;
    }
    
    EventRef cloneEvent = CopyEvent(rawKeyEventRef);
    unsigned i;
    for (i = 0; i < numBytes; i++) {
        status = SetEventParameter(cloneEvent, kEventParamKeyMacCharCodes, typeChar, 1 /* one char code */, &buffer[i]);
        if (status != noErr) {
            LOG_ERROR("SetEventParameter failed with error: %d", status);
            free(buffer);
            return noErr;
        }
        
        EventRecord eventRec;
        if (ConvertEventRefToEventRecord(cloneEvent, &eventRec)) {
            BOOL acceptedEvent;
            acceptedEvent = [(WebBaseNetscapePluginView *)pluginView sendEvent:&eventRec];
            
            LOG(PluginEvents, "NPP_HandleEvent(keyDown): %d charCode:%c keyCode:%lu",
                acceptedEvent, (char) (eventRec.message & charCodeMask), (eventRec.message & keyCodeMask));
            
            // We originally thought that if the plug-in didn't accept this event,
            // we should pass it along so that keyboard scrolling, for example, will work.
            // In practice, this is not a good idea, because plug-ins tend to eat the event but return false.
            // MacIE handles each key event twice because of this, but we will emulate the other browsers instead.
        }
    }
    ReleaseEvent(cloneEvent);
    
    free(buffer);
    return noErr;
}

// Fake up command-modified events so cut, copy, paste and select all menus work.
- (void)sendModifierEventWithKeyCode:(int)keyCode character:(char)character
{
    EventRecord event;
    [self getCarbonEvent:&event];
    event.what = keyDown;
    event.modifiers |= cmdKey;
    event.message = keyCode << 8 | character;
    [self sendEvent:&event];
}

- (void)cut:(id)sender
{
    [self sendModifierEventWithKeyCode:7 character:'x'];
}

- (void)copy:(id)sender
{
    [self sendModifierEventWithKeyCode:8 character:'c'];
}

- (void)paste:(id)sender
{
    [self sendModifierEventWithKeyCode:9 character:'v'];
}

- (void)selectAll:(id)sender
{
    [self sendModifierEventWithKeyCode:0 character:'a'];
}

#pragma mark WEB_NETSCAPE_PLUGIN

- (BOOL)isNewWindowEqualToOldWindow
{
    if (window.x != lastSetWindow.x)
        return NO;
    if (window.y != lastSetWindow.y)
        return NO;
    if (window.width != lastSetWindow.width)
        return NO;
    if (window.height != lastSetWindow.height)
        return NO;
    if (window.clipRect.top != lastSetWindow.clipRect.top)
        return NO;
    if (window.clipRect.left != lastSetWindow.clipRect.left)
        return NO;
    if (window.clipRect.bottom  != lastSetWindow.clipRect.bottom)
        return NO;
    if (window.clipRect.right != lastSetWindow.clipRect.right)
        return NO;
    if (window.type != lastSetWindow.type)
        return NO;
    
    switch (drawingModel) {
#ifndef NP_NO_QUICKDRAW
        case NPDrawingModelQuickDraw:
            if (nPort.qdPort.portx != lastSetPort.qdPort.portx)
                return NO;
            if (nPort.qdPort.porty != lastSetPort.qdPort.porty)
                return NO;
            if (nPort.qdPort.port != lastSetPort.qdPort.port)
                return NO;
        break;
#endif /* NP_NO_QUICKDRAW */
            
        case NPDrawingModelCoreGraphics:
            if (nPort.cgPort.window != lastSetPort.cgPort.window)
                return NO;
            if (nPort.cgPort.context != lastSetPort.cgPort.context)
                return NO;
        break;
            
        default:
            ASSERT_NOT_REACHED();
        break;
    }
    
    return YES;
}

- (void)updateAndSetWindow
{
    if (drawingModel == NPDrawingModelCoreGraphics) {
        // Can only update CoreGraphics plugins while redrawing the plugin view
        [self setNeedsDisplay:YES];
        return;
    }
    
    // Can't update the plugin if it has not started (or has been stopped)
    if (!isStarted)
        return;
        
    PortState portState = [self saveAndSetNewPortState];
    if (portState) {
        [self setWindowIfNecessary];
        [self restorePortState:portState];
        free(portState);
    }
}

- (void)setWindowIfNecessary
{
    if (!isStarted) {
        return;
    }
    
    if (![self isNewWindowEqualToOldWindow]) {        
        // Make sure we don't call NPP_HandleEvent while we're inside NPP_SetWindow.
        // We probably don't want more general reentrancy protection; we are really
        // protecting only against this one case, which actually comes up when
        // you first install the SVG viewer plug-in.
        NPError npErr;
        ASSERT(!inSetWindow);
        
        inSetWindow = YES;
        
        // A CoreGraphics plugin's window may only be set while the plugin is being updated
        ASSERT(drawingModel != NPDrawingModelCoreGraphics || [NSView focusView] == self);
        
        npErr = NPP_SetWindow(instance, &window);
        inSetWindow = NO;

#ifndef NDEBUG
        switch (drawingModel) {
#ifndef NP_NO_QUICKDRAW
            case NPDrawingModelQuickDraw:
                LOG(Plugins, "NPP_SetWindow (QuickDraw): %d, port=0x%08x, window.x:%d window.y:%d window.width:%d window.height:%d",
                npErr, (int)nPort.qdPort.port, (int)window.x, (int)window.y, (int)window.width, (int)window.height);
            break;
#endif /* NP_NO_QUICKDRAW */
            
            case NPDrawingModelCoreGraphics:
                LOG(Plugins, "NPP_SetWindow (CoreGraphics): %d, window=%p, context=%p, window.x:%d window.y:%d window.width:%d window.height:%d",
                npErr, nPort.cgPort.window, nPort.cgPort.context, (int)window.x, (int)window.y, (int)window.width, (int)window.height);
            break;
            
            default:
                ASSERT_NOT_REACHED();
            break;
        }
#endif /* !defined(NDEBUG) */
        
        lastSetWindow = window;
        lastSetPort = nPort;
    }
}

- (void)removeTrackingRect
{
    if (trackingTag) {
        [self removeTrackingRect:trackingTag];
        trackingTag = 0;

        // Must release the window to balance the retain in resetTrackingRect.
        // But must do it after setting trackingTag to 0 so we don't re-enter.
        [[self window] release];
    }
}

- (void)resetTrackingRect
{
    [self removeTrackingRect];
    if (isStarted) {
        // Must retain the window so that removeTrackingRect can work after the window is closed.
        [[self window] retain];
        trackingTag = [self addTrackingRect:[self bounds] owner:self userData:nil assumeInside:NO];
    }
}

+ (void)setCurrentPluginView:(WebBaseNetscapePluginView *)view
{
    currentPluginView = view;
}

+ (WebBaseNetscapePluginView *)currentPluginView
{
    return currentPluginView;
}

- (BOOL)canStart
{
    return YES;
}

- (void)didStart
{
    // Do nothing. Overridden by subclasses.
}

- (void)addWindowObservers
{
    ASSERT([self window]);

    NSWindow *theWindow = [self window];
    
    NSNotificationCenter *notificationCenter = [NSNotificationCenter defaultCenter];
    NSView *view;
    for (view = self; view; view = [view superview]) {
        [notificationCenter addObserver:self selector:@selector(viewHasMoved:)
                                   name:NSViewFrameDidChangeNotification object:view];
        [notificationCenter addObserver:self selector:@selector(viewHasMoved:)
                                   name:NSViewBoundsDidChangeNotification object:view];
    }
    [notificationCenter addObserver:self selector:@selector(windowWillClose:)
                               name:NSWindowWillCloseNotification object:theWindow];
    [notificationCenter addObserver:self selector:@selector(windowBecameKey:)
                               name:NSWindowDidBecomeKeyNotification object:theWindow];
    [notificationCenter addObserver:self selector:@selector(windowResignedKey:)
                               name:NSWindowDidResignKeyNotification object:theWindow];
    [notificationCenter addObserver:self selector:@selector(windowDidMiniaturize:)
                               name:NSWindowDidMiniaturizeNotification object:theWindow];
    [notificationCenter addObserver:self selector:@selector(windowDidDeminiaturize:)
                               name:NSWindowDidDeminiaturizeNotification object:theWindow];
    
    [notificationCenter addObserver:self selector:@selector(loginWindowDidSwitchFromUser:)
                               name:LoginWindowDidSwitchFromUserNotification object:nil];
    [notificationCenter addObserver:self selector:@selector(loginWindowDidSwitchToUser:)
                               name:LoginWindowDidSwitchToUserNotification object:nil];
}

- (void)removeWindowObservers
{
    NSNotificationCenter *notificationCenter = [NSNotificationCenter defaultCenter];
    [notificationCenter removeObserver:self name:NSViewFrameDidChangeNotification     object:nil];
    [notificationCenter removeObserver:self name:NSViewBoundsDidChangeNotification    object:nil];
    [notificationCenter removeObserver:self name:NSWindowWillCloseNotification        object:nil];
    [notificationCenter removeObserver:self name:NSWindowDidBecomeKeyNotification     object:nil];
    [notificationCenter removeObserver:self name:NSWindowDidResignKeyNotification     object:nil];
    [notificationCenter removeObserver:self name:NSWindowDidMiniaturizeNotification   object:nil];
    [notificationCenter removeObserver:self name:NSWindowDidDeminiaturizeNotification object:nil];
    [notificationCenter removeObserver:self name:LoginWindowDidSwitchFromUserNotification   object:nil];
    [notificationCenter removeObserver:self name:LoginWindowDidSwitchToUserNotification     object:nil];
}

- (BOOL)start
{
    ASSERT([self currentWindow]);
    
    if (isStarted) {
        return YES;
    }

    if (![self canStart]) {
        return NO;
    }
    
    ASSERT([self webView]);
    
    if (![[[self webView] preferences] arePlugInsEnabled]) {
        return NO;
    }

    ASSERT(NPP_New);

    // Initialize drawingModel to an invalid value so that we can detect when the plugin does not specify a drawingModel
    drawingModel = -1;
    
    [[self class] setCurrentPluginView:self];
    NPError npErr = NPP_New((char *)[MIMEType cString], instance, mode, argsCount, cAttributes, cValues, NULL);
    [[self class] setCurrentPluginView:nil];

    if (drawingModel == (NPDrawingModel)-1) {
#ifndef NP_NO_QUICKDRAW
        // Default to QuickDraw if the plugin did not specify a drawing model.
        drawingModel = NPDrawingModelQuickDraw;
#else
        // QuickDraw is not available, so we can't default to it.  We could default to CoreGraphics instead, but
        // if the plugin did not specify the CoreGraphics drawing model then it must be one of the old QuickDraw
        // plugins.  Thus, the plugin is unsupported and should not be started.  Destroy it here and bail out.
        LOG(Plugins, "Plugin only supports QuickDraw, but QuickDraw is unavailable: %@", plugin);
        NPP_Destroy(instance, NULL);
        instance->pdata = NULL;
        return NO;
#endif
    }
        
    LOG(Plugins, "NPP_New: %d", npErr);
    if (npErr != NPERR_NO_ERROR) {
        LOG_ERROR("NPP_New failed with error: %d", npErr);
        return NO;
    }
    
    isStarted = YES;
        
    [self updateAndSetWindow];

    if ([self window]) {
        [self addWindowObservers];
        if ([[self window] isKeyWindow]) {
            [self sendActivateEvent:YES];
        }
        [self restartNullEvents];
    }

    [self resetTrackingRect];
    
    [self didStart];
    
    return YES;
}

- (void)stop
{
    [self removeTrackingRect];

    if (!isStarted) {
        return;
    }
    
    isStarted = NO;
    
    // Stop any active streams
    [streams makeObjectsPerformSelector:@selector(stop)];
    
    // Stop the null events
    [self stopNullEvents];

    // Set cursor back to arrow cursor
    [[NSCursor arrowCursor] set];
    
    // Stop notifications and callbacks.
    [self removeWindowObservers];
    [[pendingFrameLoads allKeys] makeObjectsPerformSelector:@selector(_setInternalLoadDelegate:) withObject:nil];
    [NSObject cancelPreviousPerformRequestsWithTarget:self];

    // Setting the window type to 0 ensures that NPP_SetWindow will be called if the plug-in is restarted.
    lastSetWindow.type = 0;
    
    NPError npErr;
    npErr = NPP_Destroy(instance, NULL);
    LOG(Plugins, "NPP_Destroy: %d", npErr);

    instance->pdata = NULL;
    
    // We usually remove the key event handler in resignFirstResponder but it is possible that resignFirstResponder 
    // may never get called so we can't completely rely on it.
    [self removeKeyEventHandler];
}

- (BOOL)isStarted
{
    return isStarted;
}

- (WebDataSource *)dataSource
{
    // Do nothing. Overridden by subclasses.
    return nil;
}

- (WebFrame *)webFrame
{
    return [[self dataSource] webFrame];
}

- (WebView *)webView
{
    return [[self webFrame] webView];
}

- (NSWindow *)currentWindow
{
    return [self window] ? [self window] : [[self webView] hostWindow];
}

- (NPP)pluginPointer
{
    return instance;
}

- (WebNetscapePluginPackage *)plugin
{
    return plugin;
}

- (void)setPlugin:(WebNetscapePluginPackage *)thePlugin;
{
    [thePlugin retain];
    [plugin release];
    plugin = thePlugin;

    NPP_New = 		[plugin NPP_New];
    NPP_Destroy = 	[plugin NPP_Destroy];
    NPP_SetWindow = 	[plugin NPP_SetWindow];
    NPP_NewStream = 	[plugin NPP_NewStream];
    NPP_WriteReady = 	[plugin NPP_WriteReady];
    NPP_Write = 	[plugin NPP_Write];
    NPP_StreamAsFile = 	[plugin NPP_StreamAsFile];
    NPP_DestroyStream = [plugin NPP_DestroyStream];
    NPP_HandleEvent = 	[plugin NPP_HandleEvent];
    NPP_URLNotify = 	[plugin NPP_URLNotify];
    NPP_GetValue = 	[plugin NPP_GetValue];
    NPP_SetValue = 	[plugin NPP_SetValue];
    NPP_Print = 	[plugin NPP_Print];
}

- (void)setMIMEType:(NSString *)theMIMEType
{
    NSString *type = [theMIMEType copy];
    [MIMEType release];
    MIMEType = type;
}

- (void)setBaseURL:(NSURL *)theBaseURL
{
    [theBaseURL retain];
    [baseURL release];
    baseURL = theBaseURL;
}

- (void)setAttributeKeys:(NSArray *)keys andValues:(NSArray *)values;
{
    ASSERT([keys count] == [values count]);
    
    // Convert the attributes to 2 C string arrays.
    // These arrays are passed to NPP_New, but the strings need to be
    // modifiable and live the entire life of the plugin.

    // The Java plug-in requires the first argument to be the base URL
    if ([MIMEType isEqualToString:@"application/x-java-applet"]) {
        cAttributes = (char **)malloc(([keys count] + 1) * sizeof(char *));
        cValues = (char **)malloc(([values count] + 1) * sizeof(char *));
        cAttributes[0] = strdup("DOCBASE");
        cValues[0] = strdup([baseURL _web_URLCString]);
        argsCount++;
    } else {
        cAttributes = (char **)malloc([keys count] * sizeof(char *));
        cValues = (char **)malloc([values count] * sizeof(char *));
    }

    BOOL isWMP = [[[plugin bundle] bundleIdentifier] isEqualToString:@"com.microsoft.WMP.defaultplugin"];
    
    unsigned i;
    unsigned count = [keys count];
    for (i = 0; i < count; i++) {
        NSString *key = [keys objectAtIndex:i];
        NSString *value = [values objectAtIndex:i];
        if ([key _webkit_isCaseInsensitiveEqualToString:@"height"]) {
            specifiedHeight = [value intValue];
        } else if ([key _webkit_isCaseInsensitiveEqualToString:@"width"]) {
            specifiedWidth = [value intValue];
        }
        // Avoid Window Media Player crash when these attributes are present.
        if (isWMP && ([key _webkit_isCaseInsensitiveEqualToString:@"SAMIStyle"] || [key _webkit_isCaseInsensitiveEqualToString:@"SAMILang"])) {
            continue;
        }
        cAttributes[argsCount] = strdup([key UTF8String]);
        cValues[argsCount] = strdup([value UTF8String]);
        LOG(Plugins, "%@ = %@", key, value);
        argsCount++;
    }
}

- (void)setMode:(int)theMode
{
    mode = theMode;
}

#pragma mark NSVIEW

- initWithFrame:(NSRect)frame
{
    [super initWithFrame:frame];

    instance = &instanceStruct;
    instance->ndata = self;
    streams = [[NSMutableArray alloc] init];
    pendingFrameLoads = [[NSMutableDictionary alloc] init];

    return self;
}

- (void)freeAttributeKeysAndValues
{
    unsigned i;
    for (i = 0; i < argsCount; i++) {
        free(cAttributes[i]);
        free(cValues[i]);
    }
    free(cAttributes);
    free(cValues);
}

- (void)dealloc
{
    ASSERT(!isStarted);

    [plugin release];
    [streams release];
    [MIMEType release];
    [baseURL release];
    [pendingFrameLoads release];

    [self freeAttributeKeysAndValues];

    [super dealloc];
}

- (void)finalize
{
    ASSERT(!isStarted);

    [self freeAttributeKeysAndValues];

    [super finalize];
}

- (void)drawRect:(NSRect)rect
{
    if (!isStarted) {
        return;
    }
    
    if ([NSGraphicsContext currentContextDrawingToScreen])
        [self sendUpdateEvent];
    else {
        NSBitmapImageRep *printedPluginBitmap = [self _printedPluginBitmap];
        if (printedPluginBitmap) {
            // Flip the bitmap before drawing because the QuickDraw port is flipped relative
            // to this view.
            CGContextRef cgContext = [[NSGraphicsContext currentContext] graphicsPort];
            CGContextSaveGState(cgContext);
            NSRect bounds = [self bounds];
            CGContextTranslateCTM(cgContext, 0.0, NSHeight(bounds));
            CGContextScaleCTM(cgContext, 1.0, -1.0);
            [printedPluginBitmap drawInRect:bounds];
            CGContextRestoreGState(cgContext);
        }
    }
}

- (BOOL)isFlipped
{
    return YES;
}

#ifndef NP_NO_QUICKDRAW
-(void)tellQuickTimeToChill
{
    ASSERT(drawingModel == NPDrawingModelQuickDraw);
    
    // Make a call to the secret QuickDraw API that makes QuickTime calm down.
    WindowRef windowRef = [[self window] windowRef];
    if (!windowRef) {
        return;
    }
    CGrafPtr port = GetWindowPort(windowRef);
    Rect bounds;
    GetPortBounds(port, &bounds);
	WKCallDrawingNotification(port, &bounds);
}
#endif /* NP_NO_QUICKDRAW */

- (void)viewWillMoveToWindow:(NSWindow *)newWindow
{
#ifndef NP_NO_QUICKDRAW
    if (drawingModel == NPDrawingModelQuickDraw)
        [self tellQuickTimeToChill];
#endif

    // We must remove the tracking rect before we move to the new window.
    // Once we move to the new window, it will be too late.
    [self removeTrackingRect];
    [self removeWindowObservers];
    
    // Workaround for: <rdar://problem/3822871> resignFirstResponder is not sent to first responder view when it is removed from the window
    [self setHasFocus:NO];

    if (!newWindow) {
        if ([[self webView] hostWindow]) {
            // View will be moved out of the actual window but it still has a host window.
            [self stopNullEvents];
        } else {
            // View will have no associated windows.
            [self stop];

            // Stop observing WebPreferencesChangedNotification -- we only need to observe this when installed in the view hierarchy.
            // When not in the view hierarchy, -viewWillMoveToWindow: and -viewDidMoveToWindow will start/stop the plugin as needed.
            [[NSNotificationCenter defaultCenter] removeObserver:self name:WebPreferencesChangedNotification object:nil];
        }
    }
}

- (void)viewDidMoveToWindow
{
    [self resetTrackingRect];
    
    if ([self window]) {
        // While in the view hierarchy, observe WebPreferencesChangedNotification so that we can start/stop depending
        // on whether plugins are enabled.
        [[NSNotificationCenter defaultCenter] addObserver:self
                                              selector:@selector(preferencesHaveChanged:)
                                              name:WebPreferencesChangedNotification
                                              object:nil];

        // View moved to an actual window. Start it if not already started.
        [self start];
        [self restartNullEvents];
        [self addWindowObservers];
    } else if ([[self webView] hostWindow]) {
        // View moved out of an actual window, but still has a host window.
        // Call setWindow to explicitly "clip out" the plug-in from sight.
        // FIXME: It would be nice to do this where we call stopNullEvents in viewWillMoveToWindow.
        [self updateAndSetWindow];
    }
}

- (void)viewWillMoveToHostWindow:(NSWindow *)hostWindow
{
    if (!hostWindow && ![self window]) {
        // View will have no associated windows.
        [self stop];

        // Remove WebPreferencesChangedNotification observer -- we will observe once again when we move back into the window
        [[NSNotificationCenter defaultCenter] removeObserver:self name:WebPreferencesChangedNotification object:nil];
    }
}

- (void)viewDidMoveToHostWindow
{
    if ([[self webView] hostWindow]) {
        // View now has an associated window. Start it if not already started.
        [self start];
    }
}

#pragma mark NOTIFICATIONS

- (void)viewHasMoved:(NSNotification *)notification
{
#ifndef NP_NO_QUICKDRAW
    if (drawingModel == NPDrawingModelQuickDraw)
        [self tellQuickTimeToChill];
#endif
    [self updateAndSetWindow];
    [self resetTrackingRect];
    
    // Check to see if the plugin view is completely obscured (scrolled out of view, for example).
    // For performance reasons, we send null events at a lower rate to plugins which are obscured.
    BOOL oldIsObscured = isCompletelyObscured;
    isCompletelyObscured = NSEqualSizes([self visibleRect].size, NSZeroSize);
    if (isCompletelyObscured != oldIsObscured)
        [self restartNullEvents];
}

- (void)windowWillClose:(NSNotification *)notification
{
    [self stop];
}

- (void)windowBecameKey:(NSNotification *)notification
{
    [self sendActivateEvent:YES];
    [self setNeedsDisplay:YES];
    [self restartNullEvents];
    SetUserFocusWindow([[self window] windowRef]);
}

- (void)windowResignedKey:(NSNotification *)notification
{
    [self sendActivateEvent:NO];
    [self setNeedsDisplay:YES];
    [self restartNullEvents];
}

- (void)windowDidMiniaturize:(NSNotification *)notification
{
    [self stopNullEvents];
}

- (void)windowDidDeminiaturize:(NSNotification *)notification
{
    [self restartNullEvents];
}

- (void)loginWindowDidSwitchFromUser:(NSNotification *)notification
{
    [self stopNullEvents];
}

-(void)loginWindowDidSwitchToUser:(NSNotification *)notification
{
    [self restartNullEvents];
}

- (void)preferencesHaveChanged:(NSNotification *)notification
{
    WebPreferences *preferences = [[self webView] preferences];
    BOOL arePlugInsEnabled = [preferences arePlugInsEnabled];
    
    if ([notification object] == preferences && isStarted != arePlugInsEnabled) {
        if (arePlugInsEnabled) {
            if ([self currentWindow]) {
                [self start];
            }
        } else {
            [self stop];
            [self setNeedsDisplay:YES];
        }
    }
}

- (void *)pluginScriptableObject
{
    if (NPP_GetValue) {
        void *value = 0;
        NPError npErr = NPP_GetValue (instance, NPPVpluginScriptableNPObject, (void *)&value);
        if (npErr == NPERR_NO_ERROR) {
            return value;
        }
    }
    return (void *)0;
}


@end

@implementation WebBaseNetscapePluginView (WebNPPCallbacks)

- (NSMutableURLRequest *)requestWithURLCString:(const char *)URLCString
{
    if (!URLCString)
        return nil;
    
    CFStringRef string = CFStringCreateWithCString(kCFAllocatorDefault, URLCString, kCFStringEncodingISOLatin1);
    ASSERT(string); // All strings should be representable in ISO Latin 1
    
    NSString *URLString = [(NSString *)string _web_stringByStrippingReturnCharacters];
    NSURL *URL = [NSURL _web_URLWithDataAsString:URLString relativeToURL:baseURL];
    CFRelease(string);
    if (!URL)
        return nil;

    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:URL];
    [request _web_setHTTPReferrer:[[[self webFrame] _bridge] referrer]];
    return request;
}

- (void)evaluateJavaScriptPluginRequest:(WebPluginRequest *)JSPluginRequest
{
    // FIXME: Is this isStarted check needed here? evaluateJavaScriptPluginRequest should not be called
    // if we are stopped since this method is called after a delay and we call 
    // cancelPreviousPerformRequestsWithTarget inside of stop.
    if (!isStarted) {
        return;
    }
    
    NSURL *URL = [[JSPluginRequest request] URL];
    NSString *JSString = [URL _webkit_scriptIfJavaScriptURL];
    ASSERT(JSString);
    
    NSString *result = [[[self webFrame] _bridge] stringByEvaluatingJavaScriptFromString:JSString forceUserGesture:[JSPluginRequest isCurrentEventUserGesture]];
    
    // Don't continue if stringByEvaluatingJavaScriptFromString caused the plug-in to stop.
    if (!isStarted) {
        return;
    }
        
    if ([JSPluginRequest frameName] != nil) {
        // FIXME: If the result is a string, we probably want to put that string into the frame, just
        // like we do in KHTMLPartBrowserExtension::openURLRequest.
        if ([JSPluginRequest sendNotification]) {
            NPP_URLNotify(instance, [URL _web_URLCString], NPRES_DONE, [JSPluginRequest notifyData]);
        }
    } else if ([result length] > 0) {
        // Don't call NPP_NewStream and other stream methods if there is no JS result to deliver. This is what Mozilla does.
        NSData *JSData = [result dataUsingEncoding:NSUTF8StringEncoding];
        WebBaseNetscapePluginStream *stream = [[WebBaseNetscapePluginStream alloc] initWithRequestURL:URL
                                                                                        pluginPointer:instance
                                                                                           notifyData:[JSPluginRequest notifyData]
                                                                                     sendNotification:[JSPluginRequest sendNotification]];
        [stream startStreamResponseURL:URL
                 expectedContentLength:[JSData length]
                      lastModifiedDate:nil
                              MIMEType:@"text/plain"];
        [stream receivedData:JSData];
        [stream finishedLoadingWithData:JSData];
        [stream release];
    }
}

- (void)webFrame:(WebFrame *)webFrame didFinishLoadWithReason:(NPReason)reason
{
    ASSERT(isStarted);
    
    WebPluginRequest *pluginRequest = [pendingFrameLoads objectForKey:webFrame];
    ASSERT(pluginRequest != nil);
    ASSERT([pluginRequest sendNotification]);
        
    NPP_URLNotify(instance, [[[pluginRequest request] URL] _web_URLCString], reason, [pluginRequest notifyData]);
    
    [pendingFrameLoads removeObjectForKey:webFrame];
    [webFrame _setInternalLoadDelegate:nil];
}

- (void)webFrame:(WebFrame *)webFrame didFinishLoadWithError:(NSError *)error
{
    NPReason reason = NPRES_DONE;
    if (error != nil) {
        reason = [WebBaseNetscapePluginStream reasonForError:error];
    }    
    [self webFrame:webFrame didFinishLoadWithReason:reason];
}

- (void)loadPluginRequest:(WebPluginRequest *)pluginRequest
{
    NSURLRequest *request = [pluginRequest request];
    NSString *frameName = [pluginRequest frameName];
    WebFrame *frame = nil;
    
    NSURL *URL = [request URL];
    NSString *JSString = [URL _webkit_scriptIfJavaScriptURL];
    
    ASSERT(frameName || JSString);
    
    if (frameName) {
        // FIXME - need to get rid of this window creation which
        // bypasses normal targeted link handling
        frame = [[self webFrame] findFrameNamed:frameName];
    
        if (frame == nil) {
            WebView *newWebView = nil;
            WebView *currentWebView = [self webView];
            id wd = [currentWebView UIDelegate];
            if ([wd respondsToSelector:@selector(webView:createWebViewWithRequest:)]) {
                newWebView = [wd webView:currentWebView createWebViewWithRequest:nil];
            } else {
                newWebView = [[WebDefaultUIDelegate sharedUIDelegate] webView:currentWebView createWebViewWithRequest:nil];
            }
            frame = [newWebView mainFrame];
            [[frame _bridge] setName:frameName];
            [[newWebView _UIDelegateForwarder] webViewShow:newWebView];
        }
    }

    if (JSString) {
        ASSERT(frame == nil || [self webFrame] == frame);
        [self evaluateJavaScriptPluginRequest:pluginRequest];
    } else {
        [frame loadRequest:request];
        if ([pluginRequest sendNotification]) {
            // Check if another plug-in view or even this view is waiting for the frame to load.
            // If it is, tell it that the load was cancelled because it will be anyway.
            WebBaseNetscapePluginView *view = [frame _internalLoadDelegate];
            if (view != nil) {
                ASSERT([view isKindOfClass:[WebBaseNetscapePluginView class]]);
                [view webFrame:frame didFinishLoadWithReason:NPRES_USER_BREAK];
            }
            [pendingFrameLoads _webkit_setObject:pluginRequest forUncopiedKey:frame];
            [frame _setInternalLoadDelegate:self];
        }
    }
}

- (NPError)loadRequest:(NSMutableURLRequest *)request inTarget:(const char *)cTarget withNotifyData:(void *)notifyData sendNotification:(BOOL)sendNotification
{
    NSURL *URL = [request URL];

    if (!URL) {
        return NPERR_INVALID_URL;
    }
    
    NSString *JSString = [URL _webkit_scriptIfJavaScriptURL];
    if (JSString != nil) {
        if (![[[self webView] preferences] isJavaScriptEnabled]) {
            // Return NPERR_GENERIC_ERROR if JS is disabled. This is what Mozilla does.
            return NPERR_GENERIC_ERROR;
        } else if (cTarget == NULL && mode == NP_FULL) {
            // Don't allow a JavaScript request from a standalone plug-in that is self-targetted
            // because this can cause the user to be redirected to a blank page (3424039).
            return NPERR_INVALID_PARAM;
        }
    }
        
    if (cTarget || JSString) {
        // Make when targetting a frame or evaluating a JS string, perform the request after a delay because we don't
        // want to potentially kill the plug-in inside of its URL request.
        NSString *target = nil;
        if (cTarget) {
            // Find the frame given the target string.
            target = (NSString *)CFStringCreateWithCString(kCFAllocatorDefault, cTarget, kCFStringEncodingWindowsLatin1);
        }
        
        WebFrame *frame = [self webFrame];
        if (JSString != nil && target != nil && [frame findFrameNamed:target] != frame) {
            // For security reasons, only allow JS requests to be made on the frame that contains the plug-in.
            CFRelease(target);
            return NPERR_INVALID_PARAM;
        }
        
        WebPluginRequest *pluginRequest = [[WebPluginRequest alloc] initWithRequest:request frameName:target notifyData:notifyData sendNotification:sendNotification didStartFromUserGesture:currentEventIsUserGesture];
        [self performSelector:@selector(loadPluginRequest:) withObject:pluginRequest afterDelay:0];
        [pluginRequest release];
        if (target) {
            CFRelease(target);
        }
    } else {
        WebNetscapePluginStream *stream = [[WebNetscapePluginStream alloc] initWithRequest:request 
                                                                             pluginPointer:instance 
                                                                                notifyData:notifyData 
                                                                          sendNotification:sendNotification];
        if (!stream) {
            return NPERR_INVALID_URL;
        }
        [streams addObject:stream];
        [stream start];
        [stream release];
    }
    
    return NPERR_NO_ERROR;
}

-(NPError)getURLNotify:(const char *)URLCString target:(const char *)cTarget notifyData:(void *)notifyData
{
    LOG(Plugins, "NPN_GetURLNotify: %s target: %s", URLCString, cTarget);

    NSMutableURLRequest *request = [self requestWithURLCString:URLCString];
    return [self loadRequest:request inTarget:cTarget withNotifyData:notifyData sendNotification:YES];
}

-(NPError)getURL:(const char *)URLCString target:(const char *)cTarget
{
    LOG(Plugins, "NPN_GetURL: %s target: %s", URLCString, cTarget);

    NSMutableURLRequest *request = [self requestWithURLCString:URLCString];
    return [self loadRequest:request inTarget:cTarget withNotifyData:NULL sendNotification:NO];
}

- (NPError)_postURL:(const char *)URLCString
             target:(const char *)target
                len:(UInt32)len
                buf:(const char *)buf
               file:(NPBool)file
         notifyData:(void *)notifyData
   sendNotification:(BOOL)sendNotification
       allowHeaders:(BOOL)allowHeaders
{
    if (!URLCString || !len || !buf) {
        return NPERR_INVALID_PARAM;
    }
    
    NSData *postData = nil;

    if (file) {
        // If we're posting a file, buf is either a file URL or a path to the file.
        NSString *bufString = (NSString *)CFStringCreateWithCString(kCFAllocatorDefault, buf, kCFStringEncodingWindowsLatin1);
        if (!bufString) {
            return NPERR_INVALID_PARAM;
        }
        NSURL *fileURL = [NSURL _web_URLWithDataAsString:bufString];
        NSString *path;
        if ([fileURL isFileURL]) {
            path = [fileURL path];
        } else {
            path = bufString;
        }
        postData = [NSData dataWithContentsOfFile:[path _webkit_fixedCarbonPOSIXPath]];
        CFRelease(bufString);
        if (!postData) {
            return NPERR_FILE_NOT_FOUND;
        }
    } else {
        postData = [NSData dataWithBytes:buf length:len];
    }

    if ([postData length] == 0) {
        return NPERR_INVALID_PARAM;
    }

    NSMutableURLRequest *request = [self requestWithURLCString:URLCString];
    [request setHTTPMethod:@"POST"];
    
    if (allowHeaders) {
        if ([postData _web_startsWithBlankLine]) {
            postData = [postData subdataWithRange:NSMakeRange(1, [postData length] - 1)];
        } else {
            WebNSInteger location = [postData _web_locationAfterFirstBlankLine];
            if (location != NSNotFound) {
                // If the blank line is somewhere in the middle of postData, everything before is the header.
                NSData *headerData = [postData subdataWithRange:NSMakeRange(0, location)];
                NSMutableDictionary *header = [headerData _webkit_parseRFC822HeaderFields];
		unsigned dataLength = [postData length] - location;

		// Sometimes plugins like to set Content-Length themselves when they post,
		// but WebFoundation does not like that. So we will remove the header
		// and instead truncate the data to the requested length.
		NSString *contentLength = [header objectForKey:@"Content-Length"];

		if (contentLength != nil) {
		    dataLength = MIN((unsigned)[contentLength intValue], dataLength);
		}
		[header removeObjectForKey:@"Content-Length"];

                if ([header count] > 0) {
                    [request setAllHTTPHeaderFields:header];
                }
                // Everything after the blank line is the actual content of the POST.
                postData = [postData subdataWithRange:NSMakeRange(location, dataLength)];

            }
        }
        if ([postData length] == 0) {
            return NPERR_INVALID_PARAM;
        }
    }

    // Plug-ins expect to receive uncached data when doing a POST (3347134).
    [request setCachePolicy:NSURLRequestReloadIgnoringCacheData];
    [request setHTTPBody:postData];
    
    return [self loadRequest:request inTarget:target withNotifyData:notifyData sendNotification:sendNotification];
}

- (NPError)postURLNotify:(const char *)URLCString
                  target:(const char *)target
                     len:(UInt32)len
                     buf:(const char *)buf
                    file:(NPBool)file
              notifyData:(void *)notifyData
{
    LOG(Plugins, "NPN_PostURLNotify: %s", URLCString);
    return [self _postURL:URLCString target:target len:len buf:buf file:file notifyData:notifyData sendNotification:YES allowHeaders:YES];
}

-(NPError)postURL:(const char *)URLCString
           target:(const char *)target
              len:(UInt32)len
              buf:(const char *)buf
             file:(NPBool)file
{
    LOG(Plugins, "NPN_PostURL: %s", URLCString);        
    // As documented, only allow headers to be specified via NPP_PostURL when using a file.
    return [self _postURL:URLCString target:target len:len buf:buf file:file notifyData:NULL sendNotification:NO allowHeaders:file];
}

-(NPError)newStream:(NPMIMEType)type target:(const char *)target stream:(NPStream**)stream
{
    LOG(Plugins, "NPN_NewStream");
    return NPERR_GENERIC_ERROR;
}

-(NPError)write:(NPStream*)stream len:(SInt32)len buffer:(void *)buffer
{
    LOG(Plugins, "NPN_Write");
    return NPERR_GENERIC_ERROR;
}

-(NPError)destroyStream:(NPStream*)stream reason:(NPReason)reason
{
    LOG(Plugins, "NPN_DestroyStream");
    if (!stream->ndata) {
        return NPERR_INVALID_INSTANCE_ERROR;
    }
    WebBaseNetscapePluginStream *browserStream = (WebBaseNetscapePluginStream *)stream->ndata;
    [browserStream cancelLoadAndDestroyStreamWithError:[browserStream errorForReason:reason]];
    return NPERR_NO_ERROR;
}

- (const char *)userAgent
{
    return [[[self webView] userAgentForURL:baseURL] lossyCString];
}

-(void)status:(const char *)message
{    
    if (!message) {
        LOG_ERROR("NPN_Status passed a NULL status message");
        return;
    }

    CFStringRef status = CFStringCreateWithCString(NULL, message, kCFStringEncodingWindowsLatin1);
    LOG(Plugins, "NPN_Status: %@", status);
    WebView *wv = [self webView];
    [[wv _UIDelegateForwarder] webView:wv setStatusText:(NSString *)status];
    CFRelease(status);
}

-(void)invalidateRect:(NPRect *)invalidRect
{
    LOG(Plugins, "NPN_InvalidateRect");
    [self setNeedsDisplayInRect:NSMakeRect(invalidRect->left, invalidRect->top,
        (float)invalidRect->right - invalidRect->left, (float)invalidRect->bottom - invalidRect->top)];
}

- (void)invalidateRegion:(NPRegion)invalidRegion
{
    LOG(Plugins, "NPN_InvalidateRegion");
    NSRect invalidRect;
    switch (drawingModel) {
#ifndef NP_NO_QUICKDRAW
        case NPDrawingModelQuickDraw: {
            Rect qdRect;
            GetRegionBounds(invalidRegion, &qdRect);
            invalidRect = NSMakeRect(qdRect.left, qdRect.top, qdRect.right - qdRect.left, qdRect.bottom - qdRect.top);
        }
        break;
#endif /* NP_NO_QUICKDRAW */
        
        case NPDrawingModelCoreGraphics: {
            CGRect cgRect = CGPathGetBoundingBox((NPCGRegion)invalidRegion);
            invalidRect = *(NSRect *)&cgRect;
        }
        break;
    
        default:
            ASSERT_NOT_REACHED();
        break;
    }
    
    [self setNeedsDisplayInRect:invalidRect];
}

-(void)forceRedraw
{
    LOG(Plugins, "forceRedraw");
    [self setNeedsDisplay:YES];
    [[self window] displayIfNeeded];
}

- (NPError)getVariable:(NPNVariable)variable value:(void *)value
{
    switch (variable) {
        case NPNVWindowNPObject: {
            NPObject *windowScriptObject = [[[self webFrame] _bridge] windowScriptNPObject];

            // Return value is expected to be retained, as described here: <http://www.mozilla.org/projects/plugins/npruntime.html#browseraccess>
            if (windowScriptObject)
                _NPN_RetainObject(windowScriptObject);
            
            void **v = (void **)value;
            *v = windowScriptObject;

            return NPERR_NO_ERROR;
        }
        
        case NPNVpluginDrawingModel:
        {
            *(NPDrawingModel *)value = drawingModel;
            return NPERR_NO_ERROR;
        }

#ifndef NP_NO_QUICKDRAW
        case NPNVsupportsQuickDrawBool:
        {
            *(NPBool *)value = TRUE;
            return NPERR_NO_ERROR;
        }
#endif /* NP_NO_QUICKDRAW */
        
        case NPNVsupportsCoreGraphicsBool:
        {
            *(NPBool *)value = TRUE;
            return NPERR_NO_ERROR;
        }
        
        default:
            break;
    }

    return NPERR_GENERIC_ERROR;
}

- (NPError)setVariable:(NPPVariable)variable value:(void *)value
{
    switch (variable) {
        case NPPVpluginTransparentBool:
        {
            BOOL newTransparent = (value != 0);
            
            // Redisplay if transparency is changing
            if (isTransparent != newTransparent)
                [self setNeedsDisplay:YES];
            
            isTransparent = newTransparent;
            
            return NPERR_NO_ERROR;
        }
        
        case NPNVpluginDrawingModel:
        {
            // Can only set drawing model inside NPP_New()
            if (self != [[self class] currentPluginView])
                return NPERR_GENERIC_ERROR;
            
            // Check for valid, supported drawing model
            NPDrawingModel newDrawingModel = (NPDrawingModel)value;
            switch (newDrawingModel) {
                // Supported drawing models:
#ifndef NP_NO_QUICKDRAW
                case NPDrawingModelQuickDraw:
#endif
                case NPDrawingModelCoreGraphics:
                    drawingModel = newDrawingModel;
                    return NPERR_NO_ERROR;
                
                // Unsupported (or unknown) drawing models:
                default:
                    LOG(Plugins, "Plugin %@ uses unsupported drawing model: %d", plugin, drawingModel);
                    return NPERR_GENERIC_ERROR;
            }
        }
        
        default:
            return NPERR_GENERIC_ERROR;
    }
}

@end

@implementation WebPluginRequest

- (id)initWithRequest:(NSURLRequest *)request frameName:(NSString *)frameName notifyData:(void *)notifyData sendNotification:(BOOL)sendNotification didStartFromUserGesture:(BOOL)currentEventIsUserGesture
{
    [super init];
    _didStartFromUserGesture = currentEventIsUserGesture;
    _request = [request retain];
    _frameName = [frameName retain];
    _notifyData = notifyData;
    _sendNotification = sendNotification;
    return self;
}

- (void)dealloc
{
    [_request release];
    [_frameName release];
    [super dealloc];
}

- (NSURLRequest *)request
{
    return _request;
}

- (NSString *)frameName
{
    return _frameName;
}

- (BOOL)isCurrentEventUserGesture
{
    return _didStartFromUserGesture;
}

- (BOOL)sendNotification
{
    return _sendNotification;
}

- (void *)notifyData
{
    return _notifyData;
}

@end

@implementation WebBaseNetscapePluginView (Internal)

- (NSBitmapImageRep *)_printedPluginBitmap
{
#ifdef NP_NO_QUICKDRAW
    return nil;
#else
    // Cannot print plugins that do not implement NPP_Print
    if (!NPP_Print)
        return nil;

    // This NSBitmapImageRep will share its bitmap buffer with a GWorld that the plugin will draw into.
    // The bitmap is created in 32-bits-per-pixel ARGB format, which is the default GWorld pixel format.
    NSBitmapImageRep *bitmap = [[[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
                                                         pixelsWide:window.width
                                                         pixelsHigh:window.height
                                                         bitsPerSample:8
                                                         samplesPerPixel:4
                                                         hasAlpha:YES
                                                         isPlanar:NO
                                                         colorSpaceName:NSDeviceRGBColorSpace
                                                         bitmapFormat:NSAlphaFirstBitmapFormat
                                                         bytesPerRow:0
                                                         bitsPerPixel:0] autorelease];
    ASSERT(bitmap);
    
    // Create a GWorld with the same underlying buffer into which the plugin can draw
    Rect printGWorldBounds;
    SetRect(&printGWorldBounds, 0, 0, window.width, window.height);
    GWorldPtr printGWorld;
    if (NewGWorldFromPtr(&printGWorld,
                         k32ARGBPixelFormat,
                         &printGWorldBounds,
                         NULL,
                         NULL,
                         0,
                         (Ptr)[bitmap bitmapData],
                         [bitmap bytesPerRow]) != noErr) {
        LOG_ERROR("Could not create GWorld for printing");
        return nil;
    }
    
    /// Create NPWindow for the GWorld
    NPWindow printNPWindow;
    printNPWindow.window = &printGWorld; // Normally this is an NP_Port, but when printing it is the actual CGrafPtr
    printNPWindow.x = 0;
    printNPWindow.y = 0;
    printNPWindow.width = window.width;
    printNPWindow.height = window.height;
    printNPWindow.clipRect.top = 0;
    printNPWindow.clipRect.left = 0;
    printNPWindow.clipRect.right = window.width;
    printNPWindow.clipRect.bottom = window.height;
    printNPWindow.type = NPWindowTypeDrawable; // Offscreen graphics port as opposed to a proper window
    
    // Create embed-mode NPPrint
    NPPrint npPrint;
    npPrint.mode = NP_EMBED;
    npPrint.print.embedPrint.window = printNPWindow;
    npPrint.print.embedPrint.platformPrint = printGWorld;
    
    // Tell the plugin to print into the GWorld
    NPP_Print(instance, &npPrint);

    // Don't need the GWorld anymore
    DisposeGWorld(printGWorld);
        
    return bitmap;
#endif
}

@end

@implementation NSData (PluginExtras)

- (BOOL)_web_startsWithBlankLine
{
    return [self length] > 0 && ((const char *)[self bytes])[0] == '\n';
}


- (WebNSInteger)_web_locationAfterFirstBlankLine
{
    const char *bytes = (const char *)[self bytes];
    unsigned length = [self length];
    
    unsigned i;
    for (i = 0; i < length - 4; i++) {
        
        //  Support for Acrobat. It sends "\n\n".
        if (bytes[i] == '\n' && bytes[i+1] == '\n') {
            return i+2;
        }
        
        // Returns the position after 2 CRLF's or 1 CRLF if it is the first line.
        if (bytes[i] == '\r' && bytes[i+1] == '\n') {
            i += 2;
            if (i == 2) {
                return i;
            } else if (bytes[i] == '\n') {
                // Support for Director. It sends "\r\n\n" (3880387).
                return i+1;
            } else if (bytes[i] == '\r' && bytes[i+1] == '\n') {
                // Support for Flash. It sends "\r\n\r\n" (3758113).
                return i+2;
            }
        }
    }
    return NSNotFound;
}

@end
