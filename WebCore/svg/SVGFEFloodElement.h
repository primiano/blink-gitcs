/*
    Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
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
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef SVGFEFloodElement_h
#define SVGFEFloodElement_h

#if ENABLE(SVG) && ENABLE(SVG_FILTERS)
#include "SVGFilterPrimitiveStandardAttributes.h"
#include "SVGFEFlood.h"

namespace WebCore
{
    class SVGFEFloodElement : public SVGFilterPrimitiveStandardAttributes
    {
    public:
        SVGFEFloodElement(const QualifiedName&, Document*);
        virtual ~SVGFEFloodElement();

        virtual void parseMappedAttribute(MappedAttribute*);
        virtual SVGFEFlood* filterEffect(SVGResourceFilter*) const;

    protected:
        virtual SVGElement* contextElement() { return this; }

    private:
        ANIMATED_PROPERTY_DECLARATIONS(SVGFEFloodElement, String, String, In1, in1)

        mutable SVGFEFlood *m_filterEffect;
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
