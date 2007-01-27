/*
    Copyright (C) 2004, 2005 Nikolas Zimmermann <wildfox@kde.org>
                  2004, 2005, 2006 Rob Buis <buis@kde.org>
    Copyright (C) 2007 Eric Seidel <eric@webkit.org>

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

#ifndef SVGAnimateColorElement_h
#define SVGAnimateColorElement_h
#ifdef SVG_SUPPORT

#include "SVGAnimationElement.h"
#include "ColorDistance.h"
#include <wtf/RefPtr.h>

namespace WebCore {

    class SVGColor;

    class SVGAnimateColorElement : public SVGAnimationElement {
    public:
        SVGAnimateColorElement(const QualifiedName&, Document*);
        virtual ~SVGAnimateColorElement();
        
        virtual bool updateAnimationBaseValueFromElement();
        virtual void applyAnimatedValueToElement();

    protected:
        virtual const SVGElement* contextElement() const { return this; }
        
        virtual bool updateAnimatedValue(EAnimationMode, float timePercentage, unsigned valueIndex, float percentagePast);
        virtual bool calculateFromAndToValues(EAnimationMode, unsigned valueIndex);

    private:
        Color m_baseColor;
        Color m_animatedColor;

        Color m_toColor;
        Color m_fromColor;

        ColorDistance m_colorDistance;
    };

} // namespace WebCore

#endif // SVG_SUPPORT
#endif // KSVG_SVGAnimateColorElementImpl_H

// vim:ts=4:noet
