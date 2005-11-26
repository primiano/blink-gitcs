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

#ifndef KSVG_SVGTextPositioningElementImpl_H
#define KSVG_SVGTextPositioningElementImpl_H

#include "SVGTextContentElementImpl.h"

namespace KSVG
{
    class SVGAnimatedLengthListImpl;
    class SVGAnimatedNumberListImpl;

    class SVGTextPositioningElementImpl : public SVGTextContentElementImpl
    {
    public:
        SVGTextPositioningElementImpl(const KDOM::QualifiedName& tagName, KDOM::DocumentImpl *doc);
        virtual ~SVGTextPositioningElementImpl();

        // 'SVGTextPositioningElement' functions
        SVGAnimatedLengthListImpl *x() const;
        SVGAnimatedLengthListImpl *y() const;
        SVGAnimatedLengthListImpl *dx() const;
        SVGAnimatedLengthListImpl *dy() const;
        SVGAnimatedNumberListImpl *rotate() const;

        virtual void parseMappedAttribute(KDOM::MappedAttributeImpl *attr);

    private:
        mutable SharedPtr<SVGAnimatedLengthListImpl> m_x;
        mutable SharedPtr<SVGAnimatedLengthListImpl> m_y;
        mutable SharedPtr<SVGAnimatedLengthListImpl> m_dx;
        mutable SharedPtr<SVGAnimatedLengthListImpl> m_dy;
        mutable SharedPtr<SVGAnimatedNumberListImpl> m_rotate;
    };
};

#endif

// vim:ts=4:noet
