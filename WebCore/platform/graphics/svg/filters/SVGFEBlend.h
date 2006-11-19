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

#ifndef SVGFEBlend_H
#define SVGFEBlend_H

#ifdef SVG_SUPPORT
#include "SVGFilterEffect.h"

namespace WebCore {

enum SVGBlendModeType {
    SVG_FEBLEND_MODE_UNKNOWN  = 0,
    SVG_FEBLEND_MODE_NORMAL   = 1,
    SVG_FEBLEND_MODE_MULTIPLY = 2,
    SVG_FEBLEND_MODE_SCREEN   = 3,
    SVG_FEBLEND_MODE_DARKEN   = 4,
    SVG_FEBLEND_MODE_LIGHTEN  = 5
};

class SVGFEBlend : public SVGFilterEffect {
public:
    String in2() const;
    void setIn2(const String&);

    SVGBlendModeType blendMode() const;
    void setBlendMode(SVGBlendModeType);

    virtual TextStream& externalRepresentation(TextStream&) const;

#if PLATFORM(CI)
    virtual CIFilter* getCIFilter(SVGResourceFilter*) const;
#endif

private:
    SVGBlendModeType m_mode;
    String m_in2;
};

} // namespace WebCore

#endif // SVG_SUPPORT

#endif // SVGFEBlend_H
