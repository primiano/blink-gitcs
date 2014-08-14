/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 *           (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "core/dom/FullscreenElementStack.h"

#include "core/HTMLNames.h"
#include "core/dom/Document.h"
#include "core/events/Event.h"
#include "core/frame/FrameHost.h"
#include "core/frame/LocalFrame.h"
#include "core/frame/Settings.h"
#include "core/frame/UseCounter.h"
#include "core/html/HTMLIFrameElement.h"
#include "core/html/HTMLMediaElement.h"
#include "core/page/Chrome.h"
#include "core/page/ChromeClient.h"
#include "core/rendering/RenderFullScreen.h"
#include "platform/UserGestureIndicator.h"

namespace blink {

using namespace HTMLNames;

static bool fullscreenIsAllowedForAllOwners(const Document& document)
{
    for (const HTMLFrameOwnerElement* owner = document.ownerElement(); owner; owner = owner->document().ownerElement()) {
        if (!isHTMLIFrameElement(owner))
            return false;
        if (!owner->hasAttribute(allowfullscreenAttr))
            return false;
    }
    return true;
}

static bool fullscreenIsSupported(const Document& document)
{
    // Fullscreen is supported if there is no previously-established user preference,
    // security risk, or platform limitation.
    return !document.settings() || document.settings()->fullscreenSupported();
}

static bool fullscreenIsSupported(const Document& document, const Element& element)
{
    if (!document.settings() || (document.settings()->disallowFullscreenForNonMediaElements() && !isHTMLMediaElement(element)))
        return false;
    return fullscreenIsSupported(document);
}

static bool isPrefixed(const AtomicString& type)
{
    return type == EventTypeNames::webkitfullscreenchange || type == EventTypeNames::webkitfullscreenerror;
}

static PassRefPtrWillBeRawPtr<Event> createEvent(const AtomicString& type, EventTarget& target)
{
    EventInit initializer;
    initializer.bubbles = isPrefixed(type);
    RefPtrWillBeRawPtr<Event> event = Event::create(type, initializer);
    event->setTarget(&target);
    return event;
}

const char* FullscreenElementStack::supplementName()
{
    return "FullscreenElementStack";
}

FullscreenElementStack& FullscreenElementStack::from(Document& document)
{
    FullscreenElementStack* fullscreen = fromIfExists(document);
    if (!fullscreen) {
        fullscreen = new FullscreenElementStack(document);
        DocumentSupplement::provideTo(document, supplementName(), adoptPtrWillBeNoop(fullscreen));
    }

    return *fullscreen;
}

FullscreenElementStack* FullscreenElementStack::fromIfExistsSlow(Document& document)
{
    return static_cast<FullscreenElementStack*>(DocumentSupplement::from(document, supplementName()));
}

Element* FullscreenElementStack::fullscreenElementFrom(Document& document)
{
    if (FullscreenElementStack* found = fromIfExists(document))
        return found->fullscreenElement();
    return 0;
}

Element* FullscreenElementStack::currentFullScreenElementFrom(Document& document)
{
    if (FullscreenElementStack* found = fromIfExists(document))
        return found->webkitCurrentFullScreenElement();
    return 0;
}

bool FullscreenElementStack::isFullScreen(Document& document)
{
    if (FullscreenElementStack* found = fromIfExists(document))
        return found->webkitIsFullScreen();
    return false;
}

FullscreenElementStack::FullscreenElementStack(Document& document)
    : DocumentLifecycleObserver(&document)
    , m_areKeysEnabledInFullScreen(false)
    , m_fullScreenRenderer(nullptr)
    , m_eventQueueTimer(this, &FullscreenElementStack::eventQueueTimerFired)
{
    document.setHasFullscreenElementStack();
}

FullscreenElementStack::~FullscreenElementStack()
{
}

inline Document* FullscreenElementStack::document()
{
    return lifecycleContext();
}

void FullscreenElementStack::documentWasDetached()
{
    m_eventQueue.clear();

    if (m_fullScreenRenderer)
        m_fullScreenRenderer->destroy();

#if ENABLE(OILPAN)
    m_fullScreenElement = nullptr;
    m_fullScreenElementStack.clear();
#endif

}

#if !ENABLE(OILPAN)
void FullscreenElementStack::documentWasDisposed()
{
    // NOTE: the context dispose phase is not supported in oilpan. Please
    // consider using the detach phase instead.
    m_fullScreenElement = nullptr;
    m_fullScreenElementStack.clear();
}
#endif

bool FullscreenElementStack::elementReady(Element& element, RequestType requestType)
{
    // A fullscreen element ready check for an element |element| returns true if all of the
    // following are true, and false otherwise:

    // |element| is in a document.
    if (!element.inDocument())
        return false;

    // |element|'s node document's fullscreen enabled flag is set.
    if (!fullscreenIsAllowedForAllOwners(element.document())) {
        if (requestType == PrefixedVideoRequest)
            UseCounter::count(element.document(), UseCounter::VideoFullscreenAllowedExemption);
        else
            return false;
    }

    // |element|'s node document's fullscreen element stack is either empty or its top element is an
    // ancestor of |element|.
    if (Element* topElement = fullscreenElementFrom(element.document())) {
        if (!element.isDescendantOf(topElement))
            return false;
    }

    // |element| has no ancestor element whose local name is iframe and namespace is the HTML
    // namespace.
    if (Traversal<HTMLIFrameElement>::firstAncestor(element))
        return false;

    return true;
}

void FullscreenElementStack::requestFullscreen(Element& element, RequestType requestType)
{
    // Ignore this request if the document is not in a live frame.
    if (!document()->isActive())
        return;

    do {
        // 1. If any of the following conditions are true, terminate these steps and queue a task to fire
        // an event named fullscreenerror with its bubbles attribute set to true on the context object's
        // node document:

        // The fullscreen element ready check returns false.
        if (!elementReady(element, requestType))
            break;

        // This algorithm is not allowed to show a pop-up:
        //   An algorithm is allowed to show a pop-up if, in the task in which the algorithm is running, either:
        //   - an activation behavior is currently being processed whose click event was trusted, or
        //   - the event listener for a trusted click event is being handled.
        if (!UserGestureIndicator::processingUserGesture())
            break;

        // Fullscreen is not supported.
        if (!fullscreenIsSupported(element.document(), element))
            break;

        // 2. Let doc be element's node document. (i.e. "this")
        Document* currentDoc = document();

        // 3. Let docs be all doc's ancestor browsing context's documents (if any) and doc.
        Deque<Document*> docs;

        do {
            docs.prepend(currentDoc);
            currentDoc = currentDoc->ownerElement() ? &currentDoc->ownerElement()->document() : 0;
        } while (currentDoc);

        // 4. For each document in docs, run these substeps:
        Deque<Document*>::iterator current = docs.begin(), following = docs.begin();

        do {
            ++following;

            // 1. Let following document be the document after document in docs, or null if there is no
            // such document.
            Document* currentDoc = *current;
            Document* followingDoc = following != docs.end() ? *following : 0;

            // 2. If following document is null, push context object on document's fullscreen element
            // stack, and queue a task to fire an event named fullscreenchange with its bubbles attribute
            // set to true on the document.
            if (!followingDoc) {
                from(*currentDoc).pushFullscreenElementStack(element, requestType);
                enqueueChangeEvent(*currentDoc, requestType);
                continue;
            }

            // 3. Otherwise, if document's fullscreen element stack is either empty or its top element
            // is not following document's browsing context container,
            Element* topElement = fullscreenElementFrom(*currentDoc);
            if (!topElement || topElement != followingDoc->ownerElement()) {
                // ...push following document's browsing context container on document's fullscreen element
                // stack, and queue a task to fire an event named fullscreenchange with its bubbles attribute
                // set to true on document.
                from(*currentDoc).pushFullscreenElementStack(*followingDoc->ownerElement(), requestType);
                enqueueChangeEvent(*currentDoc, requestType);
                continue;
            }

            // 4. Otherwise, do nothing for this document. It stays the same.
        } while (++current != docs.end());

        // 5. Return, and run the remaining steps asynchronously.
        // 6. Optionally, perform some animation.
        m_areKeysEnabledInFullScreen = requestType != PrefixedMozillaRequest && requestType != PrefixedVideoRequest;
        document()->frameHost()->chrome().client().enterFullScreenForElement(&element);

        // 7. Optionally, display a message indicating how the user can exit displaying the context object fullscreen.
        return;
    } while (0);

    enqueueErrorEvent(element, requestType);
}

void FullscreenElementStack::fullyExitFullscreen()
{
    // "To fully exit fullscreen act as if the exitFullscreen() method was invoked on the top-level browsing
    // context's document and subsequently empty that document's fullscreen element stack."
    if (!fullscreenElementFrom(document()->topDocument()))
        return;

    // To achieve that aim, remove all the elements from the top document's stack except for the first before
    // calling exitFullscreen():
    WillBeHeapVector<std::pair<RefPtrWillBeMember<Element>, RequestType> > replacementFullscreenElementStack;
    FullscreenElementStack& topFullscreenElementStack = from(document()->topDocument());
    replacementFullscreenElementStack.append(topFullscreenElementStack.m_fullScreenElementStack.last());
    topFullscreenElementStack.m_fullScreenElementStack.swap(replacementFullscreenElementStack);
    topFullscreenElementStack.exitFullscreen();
}

void FullscreenElementStack::exitFullscreen()
{
    // The exitFullscreen() method must run these steps:

    // 1. Let doc be the context object. (i.e. "this")
    Document* currentDoc = document();
    if (!currentDoc->isActive())
        return;

    // 2. If doc's fullscreen element stack is empty, terminate these steps.
    if (m_fullScreenElementStack.isEmpty())
        return;

    // 3. Let descendants be all the doc's descendant browsing context's documents with a non-empty fullscreen
    // element stack (if any), ordered so that the child of the doc is last and the document furthest
    // away from the doc is first.
    WillBeHeapDeque<RefPtrWillBeMember<Document> > descendants;
    for (Frame* descendant = document()->frame() ? document()->frame()->tree().traverseNext() : 0; descendant; descendant = descendant->tree().traverseNext()) {
        if (!descendant->isLocalFrame())
            continue;
        ASSERT(toLocalFrame(descendant)->document());
        if (fullscreenElementFrom(*toLocalFrame(descendant)->document()))
            descendants.prepend(toLocalFrame(descendant)->document());
    }

    // 4. For each descendant in descendants, empty descendant's fullscreen element stack, and queue a
    // task to fire an event named fullscreenchange with its bubbles attribute set to true on descendant.
    for (WillBeHeapDeque<RefPtrWillBeMember<Document> >::iterator i = descendants.begin(); i != descendants.end(); ++i) {
        ASSERT(*i);
        RequestType requestType = from(**i).m_fullScreenElementStack.last().second;
        from(**i).clearFullscreenElementStack();
        enqueueChangeEvent(**i, requestType);
    }

    // 5. While doc is not null, run these substeps:
    Element* newTop = 0;
    while (currentDoc) {
        RequestType requestType = from(*currentDoc).m_fullScreenElementStack.last().second;

        // 1. Pop the top element of doc's fullscreen element stack.
        from(*currentDoc).popFullscreenElementStack();

        //    If doc's fullscreen element stack is non-empty and the element now at the top is either
        //    not in a document or its node document is not doc, repeat this substep.
        newTop = fullscreenElementFrom(*currentDoc);
        if (newTop && (!newTop->inDocument() || newTop->document() != currentDoc))
            continue;

        // 2. Queue a task to fire an event named fullscreenchange with its bubbles attribute set to true
        // on doc.
        enqueueChangeEvent(*currentDoc, requestType);

        // 3. If doc's fullscreen element stack is empty and doc's browsing context has a browsing context
        // container, set doc to that browsing context container's node document.
        if (!newTop && currentDoc->ownerElement()) {
            currentDoc = &currentDoc->ownerElement()->document();
            continue;
        }

        // 4. Otherwise, set doc to null.
        currentDoc = 0;
    }

    // 6. Return, and run the remaining steps asynchronously.
    // 7. Optionally, perform some animation.

    FrameHost* host = document()->frameHost();

    // Speculative fix for engaget.com/videos per crbug.com/336239.
    // FIXME: This check is wrong. We ASSERT(document->isActive()) above
    // so this should be redundant and should be removed!
    if (!host)
        return;

    // Only exit out of full screen window mode if there are no remaining elements in the
    // full screen stack.
    if (!newTop) {
        host->chrome().client().exitFullScreenForElement(m_fullScreenElement.get());
        return;
    }

    // Otherwise, notify the chrome of the new full screen element.
    host->chrome().client().enterFullScreenForElement(newTop);
}

bool FullscreenElementStack::fullscreenEnabled(Document& document)
{
    // 4. The fullscreenEnabled attribute must return true if the context object has its
    //    fullscreen enabled flag set and fullscreen is supported, and false otherwise.

    // Top-level browsing contexts are implied to have their allowFullScreen attribute set.
    return fullscreenIsAllowedForAllOwners(document) && fullscreenIsSupported(document);
}

void FullscreenElementStack::willEnterFullScreenForElement(Element* element)
{
    ASSERT(element);
    if (!document()->isActive())
        return;

    if (m_fullScreenRenderer)
        m_fullScreenRenderer->unwrapRenderer();

    m_fullScreenElement = element;

    // Create a placeholder block for a the full-screen element, to keep the page from reflowing
    // when the element is removed from the normal flow. Only do this for a RenderBox, as only
    // a box will have a frameRect. The placeholder will be created in setFullScreenRenderer()
    // during layout.
    RenderObject* renderer = m_fullScreenElement->renderer();
    bool shouldCreatePlaceholder = renderer && renderer->isBox();
    if (shouldCreatePlaceholder) {
        m_savedPlaceholderFrameRect = toRenderBox(renderer)->frameRect();
        m_savedPlaceholderRenderStyle = RenderStyle::clone(renderer->style());
    }

    if (m_fullScreenElement != document()->documentElement())
        RenderFullScreen::wrapRenderer(renderer, renderer ? renderer->parent() : 0, document());

    m_fullScreenElement->setContainsFullScreenElementOnAncestorsCrossingFrameBoundaries(true);

    // FIXME: This should not call updateStyleIfNeeded.
    document()->setNeedsStyleRecalc(SubtreeStyleChange);
    document()->updateRenderTreeIfNeeded();
}

void FullscreenElementStack::didEnterFullScreenForElement(Element*)
{
    if (!m_fullScreenElement)
        return;

    if (!document()->isActive())
        return;

    m_fullScreenElement->didBecomeFullscreenElement();

    m_eventQueueTimer.startOneShot(0, FROM_HERE);
}

void FullscreenElementStack::willExitFullScreenForElement(Element*)
{
    if (!m_fullScreenElement)
        return;

    if (!document()->isActive())
        return;

    m_fullScreenElement->willStopBeingFullscreenElement();
}

void FullscreenElementStack::didExitFullScreenForElement(Element*)
{
    if (!m_fullScreenElement)
        return;

    if (!document()->isActive())
        return;

    m_fullScreenElement->setContainsFullScreenElementOnAncestorsCrossingFrameBoundaries(false);

    m_areKeysEnabledInFullScreen = false;

    if (m_fullScreenRenderer)
        m_fullScreenRenderer->unwrapRenderer();

    m_fullScreenElement = nullptr;
    document()->setNeedsStyleRecalc(SubtreeStyleChange);

    // When fullyExitFullscreen is called, we call exitFullscreen on the topDocument(). That means
    // that the events will be queued there. So if we have no events here, start the timer on the
    // exiting document.
    Document* exitingDocument = document();
    if (m_eventQueue.isEmpty())
        exitingDocument = &document()->topDocument();
    ASSERT(exitingDocument);
    from(*exitingDocument).m_eventQueueTimer.startOneShot(0, FROM_HERE);
}

void FullscreenElementStack::setFullScreenRenderer(RenderFullScreen* renderer)
{
    if (renderer == m_fullScreenRenderer)
        return;

    if (renderer && m_savedPlaceholderRenderStyle) {
        renderer->createPlaceholder(m_savedPlaceholderRenderStyle.release(), m_savedPlaceholderFrameRect);
    } else if (renderer && m_fullScreenRenderer && m_fullScreenRenderer->placeholder()) {
        RenderBlock* placeholder = m_fullScreenRenderer->placeholder();
        renderer->createPlaceholder(RenderStyle::clone(placeholder->style()), placeholder->frameRect());
    }

    if (m_fullScreenRenderer)
        m_fullScreenRenderer->unwrapRenderer();
    ASSERT(!m_fullScreenRenderer);

    m_fullScreenRenderer = renderer;
}

void FullscreenElementStack::fullScreenRendererDestroyed()
{
    m_fullScreenRenderer = nullptr;
}

void FullscreenElementStack::enqueueChangeEvent(Document& document, RequestType requestType)
{
    RefPtrWillBeRawPtr<Event> event;
    if (requestType == UnprefixedRequest) {
        event = createEvent(EventTypeNames::fullscreenchange, document);
    } else {
        ASSERT(document.hasFullscreenElementStack());
        FullscreenElementStack& fullscreen = from(document);
        EventTarget* target = fullscreen.fullscreenElement();
        if (!target)
            target = fullscreen.webkitCurrentFullScreenElement();
        if (!target)
            target = &document;
        event = createEvent(EventTypeNames::webkitfullscreenchange, *target);
    }
    m_eventQueue.append(event);
    // NOTE: The timer is started in didEnterFullScreenForElement/didExitFullScreenForElement.
}

void FullscreenElementStack::enqueueErrorEvent(Element& element, RequestType requestType)
{
    RefPtrWillBeRawPtr<Event> event;
    if (requestType == UnprefixedRequest)
        event = createEvent(EventTypeNames::fullscreenerror, element.document());
    else
        event = createEvent(EventTypeNames::webkitfullscreenerror, element);
    m_eventQueue.append(event);
    m_eventQueueTimer.startOneShot(0, FROM_HERE);
}

void FullscreenElementStack::eventQueueTimerFired(Timer<FullscreenElementStack>*)
{
    // Since we dispatch events in this function, it's possible that the
    // document will be detached and GC'd. We protect it here to make sure we
    // can finish the function successfully.
    RefPtrWillBeRawPtr<Document> protectDocument(document());
    WillBeHeapDeque<RefPtrWillBeMember<Event> > eventQueue;
    m_eventQueue.swap(eventQueue);

    while (!eventQueue.isEmpty()) {
        RefPtrWillBeRawPtr<Event> event = eventQueue.takeFirst();
        Node* target = event->target()->toNode();

        // If the element was removed from our tree, also message the documentElement.
        if (!target->inDocument() && document()->documentElement()) {
            ASSERT(isPrefixed(event->type()));
            eventQueue.append(createEvent(event->type(), *document()->documentElement()));
        }

        target->dispatchEvent(event);
    }
}

void FullscreenElementStack::elementRemoved(Element& element)
{
    // If an element |element| in a fullscreen element stack is removed from a document |document|,
    // run these steps:

    // 1. If |element| was at the top of |document|'s fullscreen element stack, act as if the
    // exitFullscreen() method was invoked on that document.
    if (fullscreenElement() == &element) {
        exitFullscreen();
        return;
    }

    // 2. Otherwise, remove |element| from |document|'s fullscreen element stack.
    for (size_t i = 0; i < m_fullScreenElementStack.size(); ++i) {
        if (m_fullScreenElementStack[i].first.get() == &element) {
            m_fullScreenElementStack.remove(i);
            return;
        }
    }

    // NOTE: |element| was not in the fullscreen element stack.
}

void FullscreenElementStack::clearFullscreenElementStack()
{
    m_fullScreenElementStack.clear();
}

void FullscreenElementStack::popFullscreenElementStack()
{
    if (m_fullScreenElementStack.isEmpty())
        return;

    m_fullScreenElementStack.removeLast();
}

void FullscreenElementStack::pushFullscreenElementStack(Element& element, RequestType requestType)
{
    m_fullScreenElementStack.append(std::make_pair(&element, requestType));
}

void FullscreenElementStack::trace(Visitor* visitor)
{
    visitor->trace(m_fullScreenElement);
    visitor->trace(m_fullScreenElementStack);
    visitor->trace(m_fullScreenRenderer);
    visitor->trace(m_eventQueue);
    DocumentSupplement::trace(visitor);
}

} // namespace blink
