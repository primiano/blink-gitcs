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

#include "config.h"
#ifdef SVG_SUPPORT
#include "SVGPolylineElement.h"

#include "SVGPointList.h"

namespace WebCore {

SVGPolylineElement::SVGPolylineElement(const QualifiedName& tagName, Document* doc)
    : SVGPolyElement(tagName, doc)
{
}

SVGPolylineElement::~SVGPolylineElement()
{
}

Path SVGPolylineElement::toPathData() const
{
    Path polyData;
    int len = points()->numberOfItems();
    if (len < 1)
        return polyData;

    ExceptionCode ec = 0;
    polyData.moveTo(points()->getItem(0, ec));
    for (int i = 1; i < len; ++i)
        polyData.addLineTo(points()->getItem(i, ec));
    
    return polyData;
}

}

// vim:ts=4:noet
#endif // SVG_SUPPORT

