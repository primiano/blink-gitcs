/*
    Copyright (C) 2007 Nikolas Zimmermann <zimmermann@kde.org>

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

#ifndef SVGTextPathElement_h
#define SVGTextPathElement_h

#if ENABLE(SVG)
#include "SVGTextContentElement.h"

#include "SVGURIReference.h"

namespace WebCore {
    enum SVGTextPathMethodType {
        SVG_TEXTPATH_METHODTYPE_UNKNOWN = 0,
        SVG_TEXTPATH_METHODTYPE_ALIGN = 1,
        SVG_TEXTPATH_METHODTYPE_STRETCH = 2
    };

    enum SVGTextPathSpacingType {
        SVG_TEXTPATH_SPACINGTYPE_UNKNOWN = 0,
        SVG_TEXTPATH_SPACINGTYPE_AUTO = 1,
        SVG_TEXTPATH_SPACINGTYPE_EXACT = 2
    };

    class SVGTextPathElement : public SVGTextContentElement,
                               public SVGURIReference {
    public:
        // Forward declare these enums in the w3c naming scheme, for IDL generation
        enum {
            TEXTPATH_METHODTYPE_UNKNOWN = SVG_TEXTPATH_METHODTYPE_UNKNOWN,
            TEXTPATH_METHODTYPE_ALIGN = SVG_TEXTPATH_METHODTYPE_ALIGN,
            TEXTPATH_METHODTYPE_STRETCH = SVG_TEXTPATH_METHODTYPE_STRETCH,
            TEXTPATH_SPACINGTYPE_UNKNOWN = SVG_TEXTPATH_SPACINGTYPE_UNKNOWN,
            TEXTPATH_SPACINGTYPE_AUTO = SVG_TEXTPATH_SPACINGTYPE_AUTO,
            TEXTPATH_SPACINGTYPE_EXACT = SVG_TEXTPATH_SPACINGTYPE_EXACT
        };

        SVGTextPathElement(const QualifiedName&, Document*);
        virtual ~SVGTextPathElement();
 
        virtual void insertedIntoDocument();

        virtual void parseMappedAttribute(Attribute*);
        virtual void synchronizeProperty(const QualifiedName&);
        virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);

        bool childShouldCreateRenderer(Node*) const;

    private:
        DECLARE_ANIMATED_PROPERTY(SVGTextPathElement, SVGNames::startOffsetAttr, SVGLength, StartOffset, startOffset)
        DECLARE_ANIMATED_PROPERTY(SVGTextPathElement, SVGNames::methodAttr, int, Method, method)
        DECLARE_ANIMATED_PROPERTY(SVGTextPathElement, SVGNames::spacingAttr, int, Spacing, spacing)
    
        // SVGURIReference
        DECLARE_ANIMATED_PROPERTY(SVGTextPathElement, XLinkNames::hrefAttr, String, Href, href)
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
