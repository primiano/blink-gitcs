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
#if SVG_SUPPORT
#include "PlatformString.h"

#include "SVGHelper.h"
#include "SVGSymbolElementImpl.h"
#include "SVGFitToViewBoxImpl.h"

using namespace WebCore;

SVGSymbolElementImpl::SVGSymbolElementImpl(const QualifiedName& tagName, DocumentImpl *doc)
: SVGStyledElementImpl(tagName, doc), SVGLangSpaceImpl(), SVGExternalResourcesRequiredImpl(), SVGFitToViewBoxImpl()
{
}

SVGSymbolElementImpl::~SVGSymbolElementImpl()
{
}

void SVGSymbolElementImpl::parseMappedAttribute(MappedAttributeImpl *attr)
{
    if(SVGLangSpaceImpl::parseMappedAttribute(attr)) return;
    if(SVGExternalResourcesRequiredImpl::parseMappedAttribute(attr)) return;
    if(SVGFitToViewBoxImpl::parseMappedAttribute(attr)) return;

    SVGStyledElementImpl::parseMappedAttribute(attr);
}

// vim:ts=4:noet
#endif // SVG_SUPPORT

