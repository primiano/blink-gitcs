/*
    Copyright (C) 2006 Apple Computer, Inc.

    This file is part of the WebKit project

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

#ifndef KSVG_SVGForeignObjectElementImpl_H
#define KSVG_SVGForeignObjectElementImpl_H
#ifdef SVG_SUPPORT

#include "SVGTests.h"
#include "SVGLangSpace.h"
#include "SVGURIReference.h"
#include "SVGStyledTransformableElement.h"
#include "SVGExternalResourcesRequired.h"

namespace WebCore
{
    class SVGLength;
    class SVGDocument;

    class SVGForeignObjectElement : public SVGStyledTransformableElement,
                                public SVGTests,
                                public SVGLangSpace,
                                public SVGExternalResourcesRequired,
                                public SVGURIReference
    {
    public:
        SVGForeignObjectElement(const QualifiedName&, Document*);
        virtual ~SVGForeignObjectElement();
        
        virtual bool isValid() const { return SVGTests::isValid(); }
        virtual void parseMappedAttribute(MappedAttribute *attr);

        virtual bool rendererIsNeeded(RenderStyle *style) { return StyledElement::rendererIsNeeded(style); }
        bool childShouldCreateRenderer(Node *child) const;
        virtual RenderObject *createRenderer(RenderArena *arena, RenderStyle *style);

    protected:
        virtual const SVGElement* contextElement() const { return this; }

    private:
        ANIMATED_PROPERTY_FORWARD_DECLARATIONS(SVGExternalResourcesRequired, bool, ExternalResourcesRequired, externalResourcesRequired) 
        ANIMATED_PROPERTY_FORWARD_DECLARATIONS(SVGURIReference, String, Href, href)

        ANIMATED_PROPERTY_DECLARATIONS(SVGForeignObjectElement, SVGLength*, RefPtr<SVGLength>, X, x)
        ANIMATED_PROPERTY_DECLARATIONS(SVGForeignObjectElement, SVGLength*, RefPtr<SVGLength>, Y, y)
        ANIMATED_PROPERTY_DECLARATIONS(SVGForeignObjectElement, SVGLength*, RefPtr<SVGLength>, Width, width)
        ANIMATED_PROPERTY_DECLARATIONS(SVGForeignObjectElement, SVGLength*, RefPtr<SVGLength>, Height, height)
    };

} // namespace WebCore

#endif // SVG_SUPPORT
#endif

// vim:ts=4:noet
