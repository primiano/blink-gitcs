/*
    Copyright (C) 2004, 2005, 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
                  2004, 2005, 2006 Rob Buis <buis@kde.org>

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

#ifndef SVGClipPathElement_h
#define SVGClipPathElement_h

#if ENABLE(SVG)
#include "RenderObject.h"
#include "SVGExternalResourcesRequired.h"
#include "SVGLangSpace.h"
#include "SVGStyledTransformableElement.h"
#include "SVGTests.h"

namespace WebCore {
class SVGClipPathElement : public SVGStyledTransformableElement,
                           public SVGTests,
                           public SVGLangSpace,
                           public SVGExternalResourcesRequired {
public:
    SVGClipPathElement(const QualifiedName&, Document*);
    virtual ~SVGClipPathElement();

    virtual bool isValid() const { return SVGTests::isValid(); }

    virtual void parseMappedAttribute(MappedAttribute*);
    virtual void svgAttributeChanged(const QualifiedName&);
    virtual void synchronizeProperty(const QualifiedName&);
    virtual void childrenChanged(bool changedByParser = false, Node* beforeChange = 0, Node* afterChange = 0, int childCountDelta = 0);

    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);

private:
    DECLARE_ANIMATED_PROPERTY(SVGClipPathElement, SVGNames::clipPathUnitsAttr, int, ClipPathUnits, clipPathUnits)

    // SVGExternalResourcesRequired
    DECLARE_ANIMATED_PROPERTY(SVGClipPathElement, SVGNames::externalResourcesRequiredAttr, bool, ExternalResourcesRequired, externalResourcesRequired)
};

}

#endif
#endif
