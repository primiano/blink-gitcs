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
#include "SVGStyledElementImpl.h"

#include <kdom/kdom.h>
#include <kdom/core/AttrImpl.h>
#include <kdom/core/domattrs.h>
#include "PlatformString.h"
#include "DocumentImpl.h"

#include <kxmlcore/Assertions.h>

#include <kcanvas/KCanvas.h>
#include <kcanvas/RenderPath.h>
#include <kcanvas/KCanvasCreator.h>
#include <kcanvas/device/KRenderingDevice.h>

#include "ksvg.h"
#include "SVGMatrixImpl.h"
#include "SVGRenderStyle.h"
#include "SVGElementImpl.h"
#include "SVGSVGElementImpl.h"
#include "SVGAnimatedStringImpl.h"
#include "KCanvasRenderingStyle.h"
#include "SVGDOMImplementationImpl.h"
#include "ksvgcssproperties.h"
#include "css_base.h"
#include "SVGHelper.h"

#include "SVGNames.h"

namespace WebCore {

using namespace SVGNames;

SVGStyledElementImpl::SVGStyledElementImpl(const QualifiedName& tagName, DocumentImpl *doc)
: SVGElementImpl(tagName, doc)
{
    m_updateVectorial = false;
}

SVGStyledElementImpl::~SVGStyledElementImpl()
{
}

SVGAnimatedStringImpl *SVGStyledElementImpl::className() const
{
    return lazy_create(m_className, (SVGStyledElementImpl *)0); // TODO: use notification context?
}

RenderObject *SVGStyledElementImpl::createRenderer(RenderArena *arena, RenderStyle *style)
{
    RefPtr<KCanvasPath> pathData = toPathData();
    if (!pathData)
        return 0;
    return renderingDevice()->createItem(arena, style, this, pathData.get());
}

void SVGStyledElementImpl::parseMappedAttribute(MappedAttributeImpl *attr)
{
    DOMString value(attr->value());
    // id and class are handled by StyledElementImpl
    QString qProp = attr->name().localName().qstring();
    int propId = DOM::getPropertyID(qProp.ascii(), qProp.length());
    if (propId == 0)
        propId = getSVGCSSPropertyID(qProp.ascii(), qProp.length());
    if(propId > 0) {
        addCSSProperty(attr, propId, value);
        setChanged();
        return;
    }
    
    SVGElementImpl::parseMappedAttribute(attr);
}

void SVGStyledElementImpl::notifyAttributeChange() const
{
    // For most cases we need to handle vectorial data changes (ie. rect x changed)
    if(!ownerDocument()->parsing())
    {
        // TODO: Use a more optimized way of updating, means not calling updateCanvasItem() here!
        const_cast<SVGStyledElementImpl *>(this)->m_updateVectorial = true;
        const_cast<SVGStyledElementImpl *>(this)->updateCanvasItem();
    }
}

void SVGStyledElementImpl::attributeChanged(AttributeImpl *attr, bool preserveDecls)
{
    // FIXME: Eventually subclasses from SVGElementImpl should implement
    // attributeChanged() instead of notifyAttributeChange()
    // This is a quick fix to allow dynamic updates of SVG elements
    // but will result in slower dynamic-update performance than necessary.
    SVGElementImpl::attributeChanged(attr, preserveDecls);
    notifyAttributeChange();
}

RenderCanvas *SVGStyledElementImpl::canvas() const
{
    return static_cast<RenderCanvas *>(getDocument()->renderer());
}

void SVGStyledElementImpl::updateCanvasItem()
{
    if(!m_updateVectorial || !renderer() || !renderer()->isRenderPath())
        return;
    
    RenderPath *renderPath = static_cast<RenderPath *>(renderer());
    bool renderSection = false;
    
    SVGElementImpl *parentElement = svg_dynamic_cast(parentNode());
    if(parentElement && parentElement->renderer() && parentElement->isStyled()
        && parentElement->childShouldCreateRenderer(this))
        renderSection = true;

    renderPath->setPath(toPathData());

    if(renderSection)
        renderPath->setNeedsLayout(true);

    m_updateVectorial = false;
}

const SVGStyledElementImpl *SVGStyledElementImpl::pushAttributeContext(const SVGStyledElementImpl *)
{
    if(canvas())
        static_cast<RenderPath *>(renderer())->setPath(toPathData());

    return 0;
}

}

// vim:ts=4:noet
#endif // SVG_SUPPORT

