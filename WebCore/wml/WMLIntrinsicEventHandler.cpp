/**
 * Copyright (C) 2008 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
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

#if ENABLE(WML)
#include "WMLIntrinsicEventHandler.h"

namespace WebCore {

WMLIntrinsicEventHandler::WMLIntrinsicEventHandler()
{
}

bool WMLIntrinsicEventHandler::registerIntrinsicEvent(WMLIntrinsicEventType type, PassRefPtr<WMLIntrinsicEvent> event)
{
    if (m_events.contains(type))
        return false;

    m_events.set(type, event);
    return true;
}

void WMLIntrinsicEventHandler::deregisterIntrinsicEvent(WMLIntrinsicEventType type)
{
    if (m_events.contains(type))
        m_events.remove(type);
}

void WMLIntrinsicEventHandler::triggerIntrinsicEvent(WMLIntrinsicEventType type) const
{
    RefPtr<WMLIntrinsicEvent> event = m_events.get(type);
    ASSERT(event->taskElement());
    event->taskElement()->executeTask(0);
}

bool WMLIntrinsicEventHandler::hasIntrinsicEvent(WMLIntrinsicEventType type) const
{
    return m_events.contains(type);
}

}

#endif
