/*
   Copyright (C) Research In Motion Limited 2010. All rights reserved.

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

#ifndef SVGVKernElement_h
#define SVGVKernElement_h

#if ENABLE(SVG_FONTS)
#include "SVGFontElement.h"
#include "SVGParserUtilities.h"
#include "SVGStyledElement.h"

#include <limits>

namespace WebCore {

class AtomicString;
class SVGFontData;

class SVGVKernElement : public SVGElement {
public:
    SVGVKernElement(const QualifiedName&, Document*);
    virtual ~SVGVKernElement();

    virtual void insertedIntoDocument();
    virtual void removedFromDocument();

    virtual bool rendererIsNeeded(RenderStyle*) { return false; }

    void buildVerticalKerningPair(KerningPairVector&);
};

} // namespace WebCore

#endif // ENABLE(SVG_FONTS)
#endif
