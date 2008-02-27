/*
   Copyright (C) 2007 Eric Seidel <eric@webkit.org>

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

#ifndef SVGFontFaceUriElement_h
#define SVGFontFaceUriElement_h

#if ENABLE(SVG_FONTS)
#include "SVGElement.h"

namespace WebCore {
    class CSSFontFaceSrcValue;
    class SVGFontFaceUriElement : public SVGElement {
    public:
        SVGFontFaceUriElement(const QualifiedName&, Document*);
        
        PassRefPtr<CSSFontFaceSrcValue> srcValue() const;
        
        virtual void childrenChanged(bool changedByParser = false, Node* beforeChange = 0, Node* afterChange = 0, int childCountDelta = 0);
    };

} // namespace WebCore

#endif // ENABLE(SVG_FONTS)
#endif

// vim:ts=4:noet
