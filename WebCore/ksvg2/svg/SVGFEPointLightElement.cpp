/*
     Copyright (C) 2005 Oliver Hunt <ojh16@student.canterbury.ac.nz>
     
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
#include "SVGFEPointLightElement.h"
#include "SVGPointLightSource.h"

namespace WebCore {

SVGFEPointLightElement::SVGFEPointLightElement(const QualifiedName& tagName, Document* doc)
    : SVGFELightElement(tagName, doc)
{
}

SVGFEPointLightElement::~SVGFEPointLightElement()
{
}

SVGLightSource* SVGFEPointLightElement::lightSource() const
{
    FloatPoint3D pos(x(), y(), z());
    return new SVGPointLightSource(pos);
}

}

#endif // SVG_SUPPORT

// vim:ts=4:noet
