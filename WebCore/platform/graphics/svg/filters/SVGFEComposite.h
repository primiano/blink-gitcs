/*
    Copyright (C) 2004, 2005, 2006 Nikolas Zimmermann <wildfox@kde.org>
                  2004, 2005 Rob Buis <buis@kde.org>
                  2005 Eric Seidel <eric.seidel@kdemail.net>

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
    aint with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#ifndef SVGFEComposite_h
#define SVGFEComposite_h

#ifdef SVG_SUPPORT
#include "SVGFilterEffect.h"

namespace WebCore {

enum SVGCompositeOperationType {
    SVG_FECOMPOSITE_OPERATOR_UNKNOWN    = 0, 
    SVG_FECOMPOSITE_OPERATOR_OVER       = 1,
    SVG_FECOMPOSITE_OPERATOR_IN         = 2,
    SVG_FECOMPOSITE_OPERATOR_OUT        = 3,
    SVG_FECOMPOSITE_OPERATOR_ATOP       = 4,
    SVG_FECOMPOSITE_OPERATOR_XOR        = 5,
    SVG_FECOMPOSITE_OPERATOR_ARITHMETIC = 6
};

class SVGFEComposite : public SVGFilterEffect {
public:
    String in2() const;
    void setIn2(const String&);

    SVGCompositeOperationType operation() const;
    void setOperation(SVGCompositeOperationType);

    float k1() const;
    void setK1(float);

    float k2() const;
    void setK2(float);

    float k3() const;
    void setK3(float);

    float k4() const;
    void setK4(float);

    virtual TextStream& externalRepresentation(TextStream&) const;

#if PLATFORM(CI)
    virtual CIFilter* getCIFilter(SVGResourceFilter*) const;
#endif

private:
    String m_in2;
    SVGCompositeOperationType m_operation;
    float m_k1;
    float m_k2;
    float m_k3;
    float m_k4;
};

} // namespace WebCore

#endif // SVG_SUPPORT

#endif // SVGFEComposite_h
