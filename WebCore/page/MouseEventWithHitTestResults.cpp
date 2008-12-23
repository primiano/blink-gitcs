/*
   Copyright (C) 2006 Apple Computer, Inc.

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
#include "MouseEventWithHitTestResults.h"

#include "Element.h"
#include "Node.h"

// Would TargetedMouseEvent be a better name?

namespace WebCore {

MouseEventWithHitTestResults::MouseEventWithHitTestResults(const PlatformMouseEvent& event, const HitTestResult& hitTestResult)
    : m_event(event)
    , m_hitTestResult(hitTestResult)
{
}
        
Node* MouseEventWithHitTestResults::targetNode() const
{
    Node* node = m_hitTestResult.innerNode();
    if (!node)
        return 0;
    if (node->inDocument())
        return node;

    Element* element = node->parentElement();
    if (element && element->inDocument())
        return element;

    return node;
}

const IntPoint MouseEventWithHitTestResults::localPoint() const
{
    return m_hitTestResult.localPoint();
}

Scrollbar* MouseEventWithHitTestResults::scrollbar() const
{
    return m_hitTestResult.scrollbar();
}

bool MouseEventWithHitTestResults::isOverLink() const
{
    return m_hitTestResult.URLElement() && m_hitTestResult.URLElement()->isLink();
}

}
