/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "EventHandler.h"

#include "AXObjectCache.h"
#include "BlockExceptions.h"
#include "ChromeClient.h"
#include "ClipboardMac.h"
#include "DragController.h"
#include "EventNames.h"
#include "FocusController.h"
#include "FrameLoader.h"
#include "Frame.h"
#include "FrameView.h"
#include "KeyboardEvent.h"
#include "MouseEventWithHitTestResults.h"
#include "Page.h"
#include "PlatformKeyboardEvent.h"
#include "PlatformWheelEvent.h"
#include "RenderWidget.h"
#include "Scrollbar.h"
#include "Settings.h"
#include <objc/objc-runtime.h>
#include <wtf/StdLibExtras.h>

#if !(defined(OBJC_API_VERSION) && OBJC_API_VERSION > 0)
static inline IMP method_setImplementation(Method m, IMP i)
{
    IMP oi = m->method_imp;
    m->method_imp = i;
    return oi;
}
#endif

namespace WebCore {

const double EventHandler::TextDragDelay = 0.15;

static RetainPtr<NSEvent>& currentEvent()
{
    DEFINE_STATIC_LOCAL(RetainPtr<NSEvent>, event, ());
    return event;
}

NSEvent *EventHandler::currentNSEvent()
{
    return currentEvent().get();
}

bool EventHandler::wheelEvent(NSEvent *event)
{
    RetainPtr<NSEvent> oldCurrentEvent = currentEvent();
    currentEvent() = event;

    PlatformWheelEvent wheelEvent(event);
    handleWheelEvent(wheelEvent);

    ASSERT(currentEvent() == event);
    currentEvent() = oldCurrentEvent;

    return wheelEvent.isAccepted();
}

PassRefPtr<KeyboardEvent> EventHandler::currentKeyboardEvent() const
{
    NSEvent *event = [NSApp currentEvent];
    if (!event)
        return 0;
    switch ([event type]) {
        case NSKeyDown: {
            PlatformKeyboardEvent platformEvent(event);
            platformEvent.disambiguateKeyDownEvent(PlatformKeyboardEvent::RawKeyDown);
            return KeyboardEvent::create(platformEvent, m_frame->document()->defaultView());
        }
        case NSKeyUp:
            return KeyboardEvent::create(event, m_frame->document()->defaultView());
        default:
            return 0;
    }
}

static inline bool isKeyboardOptionTab(KeyboardEvent* event)
{
    return event
        && (event->type() == eventNames().keydownEvent || event->type() == eventNames().keypressEvent)
        && event->altKey()
        && event->keyIdentifier() == "U+0009";    
}

bool EventHandler::invertSenseOfTabsToLinks(KeyboardEvent* event) const
{
    return isKeyboardOptionTab(event);
}

bool EventHandler::tabsToAllControls(KeyboardEvent* event) const
{
    Page* page = m_frame->page();
    if (!page)
        return false;

    KeyboardUIMode keyboardUIMode = page->chrome()->client()->keyboardUIMode();
    bool handlingOptionTab = isKeyboardOptionTab(event);

    // If tab-to-links is off, option-tab always highlights all controls
    if ((keyboardUIMode & KeyboardAccessTabsToLinks) == 0 && handlingOptionTab)
        return true;
    
    // If system preferences say to include all controls, we always include all controls
    if (keyboardUIMode & KeyboardAccessFull)
        return true;
    
    // Otherwise tab-to-links includes all controls, unless the sense is flipped via option-tab.
    if (keyboardUIMode & KeyboardAccessTabsToLinks)
        return !handlingOptionTab;
    
    return handlingOptionTab;
}

bool EventHandler::needsKeyboardEventDisambiguationQuirks() const
{
    static BOOL checkedSafari = NO;
    static BOOL isSafari = NO;

    if (!checkedSafari) {
        isSafari = [[[NSBundle mainBundle] bundleIdentifier] isEqualToString:@"com.apple.Safari"];
        checkedSafari = YES;
    }
    
    Document* document = m_frame->document();

    // RSS view needs arrow key keypress events.
    if (isSafari && document->url().protocolIs("feed") || document->url().protocolIs("feeds"))
        return true;
    Settings* settings = m_frame->settings();
    if (!settings)
        return false;

#if ENABLE(DASHBOARD_SUPPORT)
    if (settings->usesDashboardBackwardCompatibilityMode())
        return true;
#endif
        
    if (settings->needsKeyboardEventDisambiguationQuirks())
        return true;

    return false;
}

bool EventHandler::keyEvent(NSEvent *event)
{
    bool result;
    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    ASSERT([event type] == NSKeyDown || [event type] == NSKeyUp);

    RetainPtr<NSEvent> oldCurrentEvent = currentEvent();
    currentEvent() = event;

    result = keyEvent(PlatformKeyboardEvent(event));
    
    ASSERT(currentEvent() == event);
    currentEvent() = oldCurrentEvent;

    return result;

    END_BLOCK_OBJC_EXCEPTIONS;

    return false;
}

void EventHandler::focusDocumentView()
{
    Page* page = m_frame->page();
    if (!page)
        return;

    if (FrameView* frameView = m_frame->view()) {
        if (NSView *documentView = frameView->documentView())
            page->chrome()->focusNSView(documentView);
    }

    page->focusController()->setFocusedFrame(m_frame);
}

bool EventHandler::passWidgetMouseDownEventToWidget(const MouseEventWithHitTestResults& event)
{
    // Figure out which view to send the event to.
    RenderObject* target = event.targetNode() ? event.targetNode()->renderer() : 0;
    if (!target || !target->isWidget())
        return false;
    
    // Double-click events don't exist in Cocoa. Since passWidgetMouseDownEventToWidget will
    // just pass currentEvent down to the widget, we don't want to call it for events that
    // don't correspond to Cocoa events.  The mousedown/ups will have already been passed on as
    // part of the pressed/released handling.
    return passMouseDownEventToWidget(static_cast<RenderWidget*>(target)->widget());
}

bool EventHandler::passWidgetMouseDownEventToWidget(RenderWidget* renderWidget)
{
    return passMouseDownEventToWidget(renderWidget->widget());
}

static bool lastEventIsMouseUp()
{
    // Many AK widgets run their own event loops and consume events while the mouse is down.
    // When they finish, currentEvent is the mouseUp that they exited on.  We need to update
    // the khtml state with this mouseUp, which khtml never saw.  This method lets us detect
    // that state.

    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    NSEvent *currentEventAfterHandlingMouseDown = [NSApp currentEvent];
    if (currentEvent() != currentEventAfterHandlingMouseDown &&
        [currentEventAfterHandlingMouseDown type] == NSLeftMouseUp &&
        [currentEventAfterHandlingMouseDown timestamp] >= [currentEvent().get() timestamp])
            return true;
    END_BLOCK_OBJC_EXCEPTIONS;

    return false;
}

bool EventHandler::passMouseDownEventToWidget(Widget* widget)
{
    // FIXME: this method always returns true

    if (!widget) {
        LOG_ERROR("hit a RenderWidget without a corresponding Widget, means a frame is half-constructed");
        return true;
    }

    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    
    NSView *nodeView = widget->platformWidget();
    ASSERT(nodeView);
    ASSERT([nodeView superview]);
    NSView *view = [nodeView hitTest:[[nodeView superview] convertPoint:[currentEvent().get() locationInWindow] fromView:nil]];
    if (!view) {
        // We probably hit the border of a RenderWidget
        return true;
    }
    
    Page* page = m_frame->page();
    if (!page)
        return true;

    if (page->chrome()->client()->firstResponder() != view) {
        // Normally [NSWindow sendEvent:] handles setting the first responder.
        // But in our case, the event was sent to the view representing the entire web page.
        if ([currentEvent().get() clickCount] <= 1 && [view acceptsFirstResponder] && [view needsPanelToBecomeKey])
            page->chrome()->client()->makeFirstResponder(view);
    }

    // We need to "defer loading" while tracking the mouse, because tearing down the
    // page while an AppKit control is tracking the mouse can cause a crash.
    
    // FIXME: In theory, WebCore now tolerates tear-down while tracking the
    // mouse. We should confirm that, and then remove the deferrsLoading
    // hack entirely.
    
    bool wasDeferringLoading = page->defersLoading();
    if (!wasDeferringLoading)
        page->setDefersLoading(true);

    ASSERT(!m_sendingEventToSubview);
    m_sendingEventToSubview = true;
    [view mouseDown:currentEvent().get()];
    m_sendingEventToSubview = false;
    
    if (!wasDeferringLoading)
        page->setDefersLoading(false);

    // Remember which view we sent the event to, so we can direct the release event properly.
    m_mouseDownView = view;
    m_mouseDownWasInSubframe = false;
    
    // Many AppKit widgets run their own event loops and consume events while the mouse is down.
    // When they finish, currentEvent is the mouseUp that they exited on.  We need to update
    // the EventHandler state with this mouseUp, which we never saw.
    // If this event isn't a mouseUp, we assume that the mouseUp will be coming later.  There
    // is a hole here if the widget consumes both the mouseUp and subsequent events.
    if (lastEventIsMouseUp())
        m_mousePressed = false;

    END_BLOCK_OBJC_EXCEPTIONS;

    return true;
}
    
// Note that this does the same kind of check as [target isDescendantOf:superview].
// There are two differences: This is a lot slower because it has to walk the whole
// tree, and this works in cases where the target has already been deallocated.
static bool findViewInSubviews(NSView *superview, NSView *target)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    NSEnumerator *e = [[superview subviews] objectEnumerator];
    NSView *subview;
    while ((subview = [e nextObject])) {
        if (subview == target || findViewInSubviews(subview, target)) {
            return true;
        }
    }
    END_BLOCK_OBJC_EXCEPTIONS;
    
    return false;
}

NSView *EventHandler::mouseDownViewIfStillGood()
{
    // Since we have no way of tracking the lifetime of m_mouseDownView, we have to assume that
    // it could be deallocated already. We search for it in our subview tree; if we don't find
    // it, we set it to nil.
    NSView *mouseDownView = m_mouseDownView;
    if (!mouseDownView) {
        return nil;
    }
    FrameView* topFrameView = m_frame->view();
    NSView *topView = topFrameView ? topFrameView->platformWidget() : nil;
    if (!topView || !findViewInSubviews(topView, mouseDownView)) {
        m_mouseDownView = nil;
        return nil;
    }
    return mouseDownView;
}

bool EventHandler::eventActivatedView(const PlatformMouseEvent& event) const
{
    return m_activationEventNumber == event.eventNumber();
}

bool EventHandler::eventLoopHandleMouseDragged(const MouseEventWithHitTestResults&)
{
    NSView *view = mouseDownViewIfStillGood();
    
    if (!view)
        return false;
    
    if (!m_mouseDownWasInSubframe) {
        m_sendingEventToSubview = true;
        BEGIN_BLOCK_OBJC_EXCEPTIONS;
        [view mouseDragged:currentEvent().get()];
        END_BLOCK_OBJC_EXCEPTIONS;
        m_sendingEventToSubview = false;
    }
    
    return true;
}
    
PassRefPtr<Clipboard> EventHandler::createDraggingClipboard() const 
{
    NSPasteboard *pasteboard = [NSPasteboard pasteboardWithName:NSDragPboard];
    // Must be done before ondragstart adds types and data to the pboard,
    // also done for security, as it erases data from the last drag
    [pasteboard declareTypes:[NSArray array] owner:nil];
    return ClipboardMac::create(true, pasteboard, ClipboardWritable, m_frame);
}
    
bool EventHandler::eventLoopHandleMouseUp(const MouseEventWithHitTestResults&)
{
    NSView *view = mouseDownViewIfStillGood();
    if (!view)
        return false;
    
    if (!m_mouseDownWasInSubframe) {
        m_sendingEventToSubview = true;
        BEGIN_BLOCK_OBJC_EXCEPTIONS;
        [view mouseUp:currentEvent().get()];
        END_BLOCK_OBJC_EXCEPTIONS;
        m_sendingEventToSubview = false;
    }
 
    return true;
}
    
bool EventHandler::passSubframeEventToSubframe(MouseEventWithHitTestResults& event, Frame* subframe, HitTestResult* hoveredNode)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    switch ([currentEvent().get() type]) {
        case NSLeftMouseDragged:
        case NSOtherMouseDragged:
        case NSRightMouseDragged:
            // This check is bogus and results in <rdar://6813830>, but removing it breaks a number of
            // layout tests.
            if (!m_mouseDownWasInSubframe)
                return false;
            if (subframe->page()->dragController()->didInitiateDrag())
                return false;
        case NSMouseMoved:
            // Since we're passing in currentEvent() here, we can call
            // handleMouseMoveEvent() directly, since the save/restore of
            // currentEvent() that mouseMoved() does would have no effect.
            subframe->eventHandler()->handleMouseMoveEvent(currentEvent().get(), hoveredNode);
            return true;
        
        case NSLeftMouseDown: {
            Node* node = event.targetNode();
            if (!node)
                return false;
            RenderObject* renderer = node->renderer();
            if (!renderer || !renderer->isWidget())
                return false;
            Widget* widget = static_cast<RenderWidget*>(renderer)->widget();
            if (!widget || !widget->isFrameView())
                return false;
            if (!passWidgetMouseDownEventToWidget(static_cast<RenderWidget*>(renderer)))
                return false;
            m_mouseDownWasInSubframe = true;
            return true;
        }
        case NSLeftMouseUp: {
            if (!m_mouseDownWasInSubframe)
                return false;
            NSView *view = mouseDownViewIfStillGood();
            if (!view)
                return false;
            ASSERT(!m_sendingEventToSubview);
            m_sendingEventToSubview = true;
            [view mouseUp:currentEvent().get()];
            m_sendingEventToSubview = false;
            return true;
        }
        default:
            return false;
    }
    END_BLOCK_OBJC_EXCEPTIONS;

    return false;
}

static IMP originalNSScrollViewScrollWheel;
static bool _nsScrollViewScrollWheelShouldRetainSelf;
static void selfRetainingNSScrollViewScrollWheel(NSScrollView *, SEL, NSEvent *);

static bool nsScrollViewScrollWheelShouldRetainSelf()
{
    ASSERT(isMainThread());

    return _nsScrollViewScrollWheelShouldRetainSelf;
}

static void setNSScrollViewScrollWheelShouldRetainSelf(bool shouldRetain)
{
    ASSERT(isMainThread());

    if (!originalNSScrollViewScrollWheel) {
        Method method = class_getInstanceMethod(objc_getRequiredClass("NSScrollView"), @selector(scrollWheel:));
        originalNSScrollViewScrollWheel = method_setImplementation(method, reinterpret_cast<IMP>(selfRetainingNSScrollViewScrollWheel));
    }

    _nsScrollViewScrollWheelShouldRetainSelf = shouldRetain;
}

static void selfRetainingNSScrollViewScrollWheel(NSScrollView *self, SEL selector, NSEvent *event)
{
    bool shouldRetainSelf = isMainThread() && nsScrollViewScrollWheelShouldRetainSelf();

    if (shouldRetainSelf)
        [self retain];
    originalNSScrollViewScrollWheel(self, selector, event);
    if (shouldRetainSelf)
        [self release];
}

bool EventHandler::passWheelEventToWidget(PlatformWheelEvent&, Widget* widget)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;
        
    if ([currentEvent().get() type] != NSScrollWheel || m_sendingEventToSubview || !widget) 
        return false;

    NSView* nodeView = widget->platformWidget();
    ASSERT(nodeView);
    ASSERT([nodeView superview]);
    NSView *view = [nodeView hitTest:[[nodeView superview] convertPoint:[currentEvent().get() locationInWindow] fromView:nil]];
    if (!view)
        // We probably hit the border of a RenderWidget
        return false;

    m_sendingEventToSubview = true;
    // Work around <rdar://problem/6806810> which can cause -[NSScrollView scrollWheel:] to
    // crash if the NSScrollView is released during timer or network callback dispatch
    // in the nested tracking runloop that -[NSScrollView scrollWheel:] runs.
    setNSScrollViewScrollWheelShouldRetainSelf(true);
    [view scrollWheel:currentEvent().get()];
    setNSScrollViewScrollWheelShouldRetainSelf(false);
    m_sendingEventToSubview = false;
    return true;
            
    END_BLOCK_OBJC_EXCEPTIONS;
    return false;
}

void EventHandler::mouseDown(NSEvent *event)
{
    FrameView* v = m_frame->view();
    if (!v || m_sendingEventToSubview)
        return;

    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    
    m_frame->loader()->resetMultipleFormSubmissionProtection();

    m_mouseDownView = nil;
    
    RetainPtr<NSEvent> oldCurrentEvent = currentEvent();
    currentEvent() = event;
    m_mouseDown = PlatformMouseEvent(event);
    
    handleMousePressEvent(event);
    
    ASSERT(currentEvent() == event);
    currentEvent() = oldCurrentEvent;

    END_BLOCK_OBJC_EXCEPTIONS;
}

void EventHandler::mouseDragged(NSEvent *event)
{
    FrameView* v = m_frame->view();
    if (!v || m_sendingEventToSubview)
        return;

    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    RetainPtr<NSEvent> oldCurrentEvent = currentEvent();
    currentEvent() = event;

    handleMouseMoveEvent(event);
    
    ASSERT(currentEvent() == event);
    currentEvent() = oldCurrentEvent;

    END_BLOCK_OBJC_EXCEPTIONS;
}

void EventHandler::mouseUp(NSEvent *event)
{
    FrameView* v = m_frame->view();
    if (!v || m_sendingEventToSubview)
        return;

    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    RetainPtr<NSEvent> oldCurrentEvent = currentEvent();
    currentEvent() = event;

    // Our behavior here is a little different that Qt. Qt always sends
    // a mouse release event, even for a double click. To correct problems
    // in khtml's DOM click event handling we do not send a release here
    // for a double click. Instead we send that event from FrameView's
    // handleMouseDoubleClickEvent. Note also that the third click of
    // a triple click is treated as a single click, but the fourth is then
    // treated as another double click. Hence the "% 2" below.
    int clickCount = [event clickCount];
    if (clickCount > 0 && clickCount % 2 == 0)
        handleMouseDoubleClickEvent(event);
    else
        handleMouseReleaseEvent(event);
    
    ASSERT(currentEvent() == event);
    currentEvent() = oldCurrentEvent;
    
    m_mouseDownView = nil;

    END_BLOCK_OBJC_EXCEPTIONS;
}

/*
 A hack for the benefit of AK's PopUpButton, which uses the Carbon menu manager, which thus
 eats all subsequent events after it is starts its modal tracking loop.  After the interaction
 is done, this routine is used to fix things up.  When a mouse down started us tracking in
 the widget, we post a fake mouse up to balance the mouse down we started with. When a 
 key down started us tracking in the widget, we post a fake key up to balance things out.
 In addition, we post a fake mouseMoved to get the cursor in sync with whatever we happen to 
 be over after the tracking is done.
 */
void EventHandler::sendFakeEventsAfterWidgetTracking(NSEvent *initiatingEvent)
{
    FrameView* view = m_frame->view();
    if (!view)
        return;

    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    m_sendingEventToSubview = false;
    int eventType = [initiatingEvent type];
    if (eventType == NSLeftMouseDown || eventType == NSKeyDown) {
        NSEvent *fakeEvent = nil;
        if (eventType == NSLeftMouseDown) {
            fakeEvent = [NSEvent mouseEventWithType:NSLeftMouseUp
                                           location:[initiatingEvent locationInWindow]
                                      modifierFlags:[initiatingEvent modifierFlags]
                                          timestamp:[initiatingEvent timestamp]
                                       windowNumber:[initiatingEvent windowNumber]
                                            context:[initiatingEvent context]
                                        eventNumber:[initiatingEvent eventNumber]
                                         clickCount:[initiatingEvent clickCount]
                                           pressure:[initiatingEvent pressure]];
        
            [NSApp postEvent:fakeEvent atStart:YES];
        } else { // eventType == NSKeyDown
            fakeEvent = [NSEvent keyEventWithType:NSKeyUp
                                         location:[initiatingEvent locationInWindow]
                                    modifierFlags:[initiatingEvent modifierFlags]
                                        timestamp:[initiatingEvent timestamp]
                                     windowNumber:[initiatingEvent windowNumber]
                                          context:[initiatingEvent context]
                                       characters:[initiatingEvent characters] 
                      charactersIgnoringModifiers:[initiatingEvent charactersIgnoringModifiers] 
                                        isARepeat:[initiatingEvent isARepeat] 
                                          keyCode:[initiatingEvent keyCode]];
            [NSApp postEvent:fakeEvent atStart:YES];
        }
        // FIXME: We should really get the current modifierFlags here, but there's no way to poll
        // them in Cocoa, and because the event stream was stolen by the Carbon menu code we have
        // no up-to-date cache of them anywhere.
        fakeEvent = [NSEvent mouseEventWithType:NSMouseMoved
                                       location:[[view->platformWidget() window] convertScreenToBase:[NSEvent mouseLocation]]
                                  modifierFlags:[initiatingEvent modifierFlags]
                                      timestamp:[initiatingEvent timestamp]
                                   windowNumber:[initiatingEvent windowNumber]
                                        context:[initiatingEvent context]
                                    eventNumber:0
                                     clickCount:0
                                       pressure:0];
        [NSApp postEvent:fakeEvent atStart:YES];
    }
    
    END_BLOCK_OBJC_EXCEPTIONS;
}

void EventHandler::mouseMoved(NSEvent *event)
{
    // Reject a mouse moved if the button is down - screws up tracking during autoscroll
    // These happen because WebKit sometimes has to fake up moved events.
    if (!m_frame->view() || m_mousePressed || m_sendingEventToSubview)
        return;
    
    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    RetainPtr<NSEvent> oldCurrentEvent = currentEvent();
    currentEvent() = event;
    
    mouseMoved(PlatformMouseEvent(event));
    
    ASSERT(currentEvent() == event);
    currentEvent() = oldCurrentEvent;

    END_BLOCK_OBJC_EXCEPTIONS;
}

bool EventHandler::passMousePressEventToSubframe(MouseEventWithHitTestResults& mev, Frame* subframe)
{
    return passSubframeEventToSubframe(mev, subframe);
}

bool EventHandler::passMouseMoveEventToSubframe(MouseEventWithHitTestResults& mev, Frame* subframe, HitTestResult* hoveredNode)
{
    return passSubframeEventToSubframe(mev, subframe, hoveredNode);
}

bool EventHandler::passMouseReleaseEventToSubframe(MouseEventWithHitTestResults& mev, Frame* subframe)
{
    return passSubframeEventToSubframe(mev, subframe);
}

unsigned EventHandler::accessKeyModifiers()
{
    // Control+Option key combinations are usually unused on Mac OS X, but not when VoiceOver is enabled.
    // So, we use Control in this case, even though it conflicts with Emacs-style key bindings.
    // See <https://bugs.webkit.org/show_bug.cgi?id=21107> for more detail.
    if (AXObjectCache::accessibilityEnhancedUserInterfaceEnabled())
        return PlatformKeyboardEvent::CtrlKey;

    return PlatformKeyboardEvent::CtrlKey | PlatformKeyboardEvent::AltKey;
}

}
