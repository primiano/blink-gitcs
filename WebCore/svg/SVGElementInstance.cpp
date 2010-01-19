/*
    Copyright (C) 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"

#if ENABLE(SVG)
#include "SVGElementInstance.h"

#include "ContainerNodeAlgorithms.h"
#include "Event.h"
#include "EventException.h"
#include "EventListener.h"
#include "EventNames.h"
#include "FrameView.h"
#include "SVGElementInstanceList.h"
#include "SVGUseElement.h"

#include <wtf/RefCountedLeakCounter.h>

namespace WebCore {

#ifndef NDEBUG
static WTF::RefCountedLeakCounter instanceCounter("WebCoreSVGElementInstance");
#endif

static EventTargetData& dummyEventTargetData()
{
    DEFINE_STATIC_LOCAL(EventTargetData, dummyEventTargetData, ());
    dummyEventTargetData.eventListenerMap.clear();
    return dummyEventTargetData;
}

SVGElementInstance::SVGElementInstance(SVGUseElement* useElement, PassRefPtr<SVGElement> originalElement)
    : m_useElement(useElement)
    , m_element(originalElement)
    , m_previousSibling(0)
    , m_nextSibling(0)
    , m_firstChild(0)
    , m_lastChild(0)
{
    ASSERT(m_useElement);
    ASSERT(m_element);

    // Register as instance for passed element.
    m_element->mapInstanceToElement(this);

#ifndef NDEBUG
    instanceCounter.increment();
#endif
}

SVGElementInstance::~SVGElementInstance()
{
#ifndef NDEBUG
    instanceCounter.decrement();
#endif

    // Deregister as instance for passed element.
    m_element->removeInstanceMapping(this);

    removeAllChildrenInContainer<SVGElementInstance, SVGElementInstance>(this);
}

PassRefPtr<SVGElementInstanceList> SVGElementInstance::childNodes()
{
    return SVGElementInstanceList::create(this);
}

void SVGElementInstance::setShadowTreeElement(SVGElement* element)
{
    ASSERT(element);
    m_shadowTreeElement = element;
}

void SVGElementInstance::appendChild(PassRefPtr<SVGElementInstance> child)
{
    appendChildToContainer<SVGElementInstance, SVGElementInstance>(child.get(), this);
}

void SVGElementInstance::invalidateAllInstancesOfElement(SVGElement* element)
{
    if (!element || !element->isStyled())
        return;

    if (static_cast<SVGStyledElement*>(element)->instanceUpdatesBlocked())
        return;

    HashSet<SVGElementInstance*> set = element->instancesForElement();
    if (set.isEmpty())
        return;

    // Mark all use elements referencing 'element' for rebuilding
    HashSet<SVGElementInstance*>::const_iterator it = set.begin();
    const HashSet<SVGElementInstance*>::const_iterator end = set.end();

    for (; it != end; ++it) {
        ASSERT((*it)->correspondingElement() == element);
        (*it)->correspondingUseElement()->invalidateShadowTree();
    }
}

ScriptExecutionContext* SVGElementInstance::scriptExecutionContext() const
{
    if (SVGElement* element = correspondingElement())
        return element->document();
    return 0;
}

bool SVGElementInstance::addEventListener(const AtomicString& eventType, PassRefPtr<EventListener> listener, bool useCapture)
{
    if (!correspondingElement())
        return false;
    return correspondingElement()->addEventListener(eventType, listener, useCapture);
}

bool SVGElementInstance::removeEventListener(const AtomicString& eventType, EventListener* listener, bool useCapture)
{
    if (!correspondingElement())
        return false;
    return correspondingElement()->removeEventListener(eventType, listener, useCapture);
}

void SVGElementInstance::removeAllEventListeners()
{
    if (!correspondingElement())
        return;
    correspondingElement()->removeAllEventListeners();
}

bool SVGElementInstance::dispatchEvent(PassRefPtr<Event> prpEvent)
{
    RefPtr<EventTarget> protect = this;
    RefPtr<Event> event = prpEvent;

    event->setTarget(this);

    SVGElement* element = shadowTreeElement();
    if (!element)
        return false;

    RefPtr<FrameView> view = element->document()->view();
    return element->dispatchGenericEvent(event.release());
}

EventTargetData* SVGElementInstance::eventTargetData()
{
    return correspondingElement() ? correspondingElement()->eventTargetData() : 0;
}

EventTargetData* SVGElementInstance::ensureEventTargetData()
{
    return &dummyEventTargetData(); // return something, so we don't crash
}

} // namespace WebCore

#endif // ENABLE(SVG)
