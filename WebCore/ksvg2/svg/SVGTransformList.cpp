/*
    Copyright (C) 2004, 2005 Nikolas Zimmermann <wildfox@kde.org>
                  2004, 2005 Rob Buis <buis@kde.org>

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

#include "config.h"

#ifdef SVG_SUPPORT

#include "AffineTransform.h"
#include "SVGTransform.h"
#include "SVGSVGElement.h"
#include "SVGTransformList.h"

using namespace WebCore;

SVGTransformList::SVGTransformList()
    : SVGPODList<SVGTransform>()
{
}

SVGTransformList::~SVGTransformList()
{
}

SVGTransform SVGTransformList::createSVGTransformFromMatrix(const AffineTransform& matrix) const
{
    return SVGSVGElement::createSVGTransformFromMatrix(matrix);
}

SVGTransform SVGTransformList::consolidate()
{
    ExceptionCode ec = 0;
    return initialize(concatenate(), ec);
}

SVGTransform SVGTransformList::concatenate() const
{
    unsigned int length = numberOfItems();
    if (!length)
        return SVGTransform();
        
    AffineTransform matrix;
    ExceptionCode ec = 0;
    for (unsigned int i = 0; i < length; i++)
        matrix = getItem(i, ec).matrix() * matrix;

    return SVGTransform(matrix);
}

#endif // SVG_SUPPORT

// vim:ts=4:noet
