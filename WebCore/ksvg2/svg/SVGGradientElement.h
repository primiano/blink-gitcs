/*
    Copyright (C) 2004, 2005 Nikolas Zimmermann <wildfox@kde.org>
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

#ifndef KSVG_SVGGradientElementImpl_H
#define KSVG_SVGGradientElementImpl_H
#ifdef SVG_SUPPORT

#include "KRenderingPaintServerGradient.h"
#include "SVGExternalResourcesRequired.h"
#include "SVGStyledElement.h"
#include "SVGURIReference.h"

namespace WebCore {
    class SVGGradientElement;
    class SVGTransformList;
    class SVGGradientElement : public SVGStyledElement,
                               public SVGURIReference,
                               public SVGExternalResourcesRequired,
                               public KCanvasResourceListener
    {
    public:
        enum SVGGradientType {
            SVG_SPREADMETHOD_UNKNOWN = 0,
            SVG_SPREADMETHOD_PAD     = 1,
            SVG_SPREADMETHOD_REFLECT = 2,
            SVG_SPREADMETHOD_REPEAT  = 3
        };

        SVGGradientElement(const QualifiedName&, Document*);
        virtual ~SVGGradientElement();

        // 'SVGGradientElement' functions
        virtual void parseMappedAttribute(MappedAttribute*);
        virtual void notifyAttributeChange() const;
        
        virtual KRenderingPaintServerGradient* canvasResource();
        virtual void resourceNotification() const;

    protected:
        virtual void buildGradient(KRenderingPaintServerGradient*) const = 0;
        virtual KCPaintServerType gradientType() const = 0;
        void rebuildStops() const;

    protected:
        ANIMATED_PROPERTY_FORWARD_DECLARATIONS(SVGURIReference, String, Href, href)
        ANIMATED_PROPERTY_FORWARD_DECLARATIONS(SVGExternalResourcesRequired, bool, ExternalResourcesRequired, externalResourcesRequired)
 
        ANIMATED_PROPERTY_DECLARATIONS(SVGGradientElement, int, int, SpreadMethod, spreadMethod)
        ANIMATED_PROPERTY_DECLARATIONS(SVGGradientElement, int, int, GradientUnits, gradientUnits)
        ANIMATED_PROPERTY_DECLARATIONS(SVGGradientElement, SVGTransformList*, RefPtr<SVGTransformList>, GradientTransform, gradientTransform)

        mutable KRenderingPaintServerGradient* m_resource;
    };

} // namespace WebCore

#endif // SVG_SUPPORT
#endif

// vim:ts=4:noet
