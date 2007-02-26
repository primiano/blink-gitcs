/*
    Copyright (C) 2004, 2005, 2006 Nikolas Zimmermann <zimmermann@kde.org>
                  2004, 2005, 2006 Rob Buis <buis@kde.org>

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

#ifndef SVGCircleElement_h
#define SVGCircleElement_h

#if ENABLE(SVG)

#include "SVGExternalResourcesRequired.h"
#include "SVGLangSpace.h"
#include "SVGStyledTransformableElement.h"
#include "SVGTests.h"

namespace WebCore
{
    class SVGCircleElement : public SVGStyledTransformableElement,
                             public SVGTests,
                             public SVGLangSpace,
                             public SVGExternalResourcesRequired
    {
    public:
        SVGCircleElement(const QualifiedName&, Document*);
        virtual ~SVGCircleElement();
        
        virtual bool isValid() const { return SVGTests::isValid(); }

        // 'SVGCircleElement' functions
        virtual void parseMappedAttribute(MappedAttribute* attr);
        virtual void notifyAttributeChange() const;

        virtual bool rendererIsNeeded(RenderStyle* style) { return StyledElement::rendererIsNeeded(style); }
        virtual Path toPathData() const;

    protected:
        virtual const SVGElement* contextElement() const { return this; }
        virtual bool hasRelativeValues() const;

    private:
        ANIMATED_PROPERTY_FORWARD_DECLARATIONS(SVGExternalResourcesRequired, bool, ExternalResourcesRequired, externalResourcesRequired)

        ANIMATED_PROPERTY_DECLARATIONS(SVGCircleElement, SVGLength, SVGLength, Cx, cx)
        ANIMATED_PROPERTY_DECLARATIONS(SVGCircleElement, SVGLength, SVGLength, Cy, cy)
        ANIMATED_PROPERTY_DECLARATIONS(SVGCircleElement, SVGLength, SVGLength, R, r)
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGCircleElement_h

// vim:ts=4:noet
