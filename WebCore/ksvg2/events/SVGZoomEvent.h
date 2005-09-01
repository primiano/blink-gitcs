/*
    Copyright (C) 2004, 2005 Nikolas Zimmermann <wildfox@kde.org>
                  2004, 2005 Rob Buis <buis@kde.org>
                  
    This file is part of the KDE project

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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#ifndef KSVG_SVGZoomEvent_H
#define KSVG_SVGZoomEvent_H

#include <kdom/events/UIEvent.h>
#include <ksvg2/ecma/SVGLookup.h>
#include <ksvg2/events/ksvgevents.h>

namespace KSVG
{
    class SVGRect;
    class SVGPoint;
    class SVGZoomEventImpl;

    class SVGZoomEvent : public KDOM::UIEvent
    {
    public:
        SVGZoomEvent();
        explicit SVGZoomEvent(SVGZoomEventImpl *i);
        SVGZoomEvent(const SVGZoomEvent &other);
        SVGZoomEvent(const KDOM::Event &other);
        virtual ~SVGZoomEvent();

        // Operators
        SVGZoomEvent &operator=(const SVGZoomEvent &other);
        SVGZoomEvent &operator=(const KDOM::Event &other);

        // 'SVGZoomEvent' functions
        SVGRect zoomRectScreen() const;
        float previousScale() const;
        SVGPoint previousTranslate() const;
        float newScale() const;
        SVGPoint newTranslate() const;

        // Internal
        KSVG_INTERNAL(SVGZoomEvent)

    public: // EcmaScript section
        KDOM_GET
        KDOM_FORWARDPUT

        KJS::ValueImp *getValueProperty(KJS::ExecState *exec, int token) const;
    };
};

#endif

// vim:ts=4:noet
