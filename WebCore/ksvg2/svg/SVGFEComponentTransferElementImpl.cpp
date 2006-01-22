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
#include <qstringlist.h>

#include <kdom/core/AttrImpl.h>

#include <kcanvas/KCanvas.h>
#include <kcanvas/KCanvasResources.h>
#include <kcanvas/KCanvasFilters.h>
#include <kcanvas/device/KRenderingDevice.h>

#include "SVGNames.h"
#include "SVGHelper.h"
#include "SVGRenderStyle.h"
#include "SVGFEComponentTransferElementImpl.h"
#include "SVGFEFuncRElementImpl.h"
#include "SVGFEFuncGElementImpl.h"
#include "SVGFEFuncBElementImpl.h"
#include "SVGFEFuncAElementImpl.h"
#include "SVGAnimatedStringImpl.h"
#include "SVGAnimatedNumberImpl.h"
#include "SVGAnimatedEnumerationImpl.h"
#include "SVGDOMImplementationImpl.h"

using namespace KSVG;

SVGFEComponentTransferElementImpl::SVGFEComponentTransferElementImpl(const KDOM::QualifiedName& tagName, KDOM::DocumentImpl *doc) : 
SVGFilterPrimitiveStandardAttributesImpl(tagName, doc)
{
    m_filterEffect = 0;
}

SVGFEComponentTransferElementImpl::~SVGFEComponentTransferElementImpl()
{
    delete m_filterEffect;
}

SVGAnimatedStringImpl *SVGFEComponentTransferElementImpl::in1() const
{
    SVGStyledElementImpl *dummy = 0;
    return lazy_create<SVGAnimatedStringImpl>(m_in1, dummy);
}

void SVGFEComponentTransferElementImpl::parseMappedAttribute(KDOM::MappedAttributeImpl *attr)
{
    KDOM::DOMString value(attr->value());
    if (attr->name() == SVGNames::inAttr)
        in1()->setBaseVal(value.impl());
    else
        SVGFilterPrimitiveStandardAttributesImpl::parseMappedAttribute(attr);
}

KCanvasFEComponentTransfer *SVGFEComponentTransferElementImpl::filterEffect() const
{
    if (!m_filterEffect)
        m_filterEffect = static_cast<KCanvasFEComponentTransfer *>(QPainter::renderingDevice()->createFilterEffect(FE_COMPONENT_TRANSFER));
    if (!m_filterEffect)
        return 0;
    
    m_filterEffect->setIn(KDOM::DOMString(in1()->baseVal()).qstring());
    setStandardAttributes(m_filterEffect);
    
    for (KDOM::NodeImpl *n = firstChild(); n != 0; n = n->nextSibling()) {
        if (n->hasTagName(SVGNames::feFuncRTag))
            m_filterEffect->setRedFunction(static_cast<SVGFEFuncRElementImpl *>(n)->transferFunction());
        else if (n->hasTagName(SVGNames::feFuncGTag))
            m_filterEffect->setGreenFunction(static_cast<SVGFEFuncGElementImpl *>(n)->transferFunction());
        else if (n->hasTagName(SVGNames::feFuncBTag))
            m_filterEffect->setBlueFunction(static_cast<SVGFEFuncBElementImpl *>(n)->transferFunction());
        else if (n->hasTagName(SVGNames::feFuncATag))
            m_filterEffect->setAlphaFunction(static_cast<SVGFEFuncAElementImpl *>(n)->transferFunction());
    }

    return m_filterEffect;
}

// vim:ts=4:noet
#endif // SVG_SUPPORT

