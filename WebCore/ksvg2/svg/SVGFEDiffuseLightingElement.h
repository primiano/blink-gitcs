/*
    Copyright (C) 2004, 2005 Nikolas Zimmermann <wildfox@kde.org>
                  2004, 2005 Rob Buis <buis@kde.org>
                  2005 Oliver Hunt <ojh16@student.canterbury.ac.nz>

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

#ifndef KSVG_SVGFEDiffuseLightingElementImpl_H
#define KSVG_SVGFEDiffuseLightingElementImpl_H
#ifdef SVG_SUPPORT

#include "SVGFilterPrimitiveStandardAttributes.h"

namespace WebCore {
    class KCanvasFEDiffuseLighting;
    class SVGColor;
    
    class SVGFEDiffuseLightingElement : public SVGFilterPrimitiveStandardAttributes
    {
    public:
        SVGFEDiffuseLightingElement(const QualifiedName&, Document*);
        virtual ~SVGFEDiffuseLightingElement();

        // 'SVGFEDiffuseLightingElement' functions
        // Derived from: 'Element'
        virtual void parseMappedAttribute(MappedAttribute *attr);

        virtual KCanvasFEDiffuseLighting *filterEffect() const;

    protected:
        virtual const SVGElement* contextElement() const { return this; }

    private:
        ANIMATED_PROPERTY_DECLARATIONS(String, String, In, in)
        ANIMATED_PROPERTY_DECLARATIONS(double, double, DiffuseConstant, diffuseConstant)
        ANIMATED_PROPERTY_DECLARATIONS(double, double, SurfaceScale, surfaceScale)
        ANIMATED_PROPERTY_DECLARATIONS(SVGColor*, RefPtr<SVGColor>, LightingColor, lightingColor)
        ANIMATED_PROPERTY_DECLARATIONS(double, double, KernelUnitLengthX, kernelUnitLengthX)
        ANIMATED_PROPERTY_DECLARATIONS(double, double, KernelUnitLengthY, kernelUnitLengthY)
        //need other properties here...
        mutable KCanvasFEDiffuseLighting *m_filterEffect;
        
        //light management
        void updateLights() const;
    };

} // namespace WebCore

#endif // SVG_SUPPORT
#endif
