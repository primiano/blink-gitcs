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
#include <kdom/core/AttrImpl.h>

#include <kcanvas/KCanvas.h>
#include <kcanvas/KCanvasMatrix.h>
#include <kcanvas/KCanvasContainer.h>
#include <kcanvas/KCanvasCreator.h>
#include <kcanvas/KCanvasImage.h>
#include <kcanvas/device/KRenderingDevice.h>
#include <kcanvas/device/KRenderingPaintServerPattern.h>

#include "ksvg.h"
#include "SVGNames.h"
#include "SVGHelper.h"
#include "SVGMatrixImpl.h"
#include "SVGSVGElementImpl.h"
#include "SVGTransformableImpl.h"
#include "SVGTransformListImpl.h"
#include "KCanvasRenderingStyle.h"
#include "SVGPatternElementImpl.h"
#include "SVGAnimatedStringImpl.h"
#include "SVGAnimatedLengthImpl.h"
#include "SVGDOMImplementationImpl.h"
#include "SVGAnimatedEnumerationImpl.h"
#include "SVGAnimatedTransformListImpl.h"

using namespace KSVG;

SVGPatternElementImpl::SVGPatternElementImpl(const KDOM::QualifiedName& tagName, KDOM::DocumentImpl *doc) : SVGStyledLocatableElementImpl(tagName, doc), SVGURIReferenceImpl(), SVGTestsImpl(), SVGLangSpaceImpl(), SVGExternalResourcesRequiredImpl(), SVGFitToViewBoxImpl(), KCanvasResourceListener()
{
    m_patternUnits = 0;
    m_patternTransform = 0;
    m_patternContentUnits = 0;
    m_x = m_y = m_width = m_height = 0;

    m_tile = 0;
    m_paintServer = 0;
    m_ignoreAttributeChanges = false;
}

SVGPatternElementImpl::~SVGPatternElementImpl()
{
    if(m_x)
        m_x->deref();
    if(m_y)
        m_y->deref();
    if(m_width)
        m_width->deref();
    if(m_height)
        m_height->deref();
    if(m_patternUnits)
        m_patternUnits->deref();
    if(m_patternContentUnits)
        m_patternContentUnits->deref();
    if(m_patternTransform)
        m_patternTransform->deref();
}

SVGAnimatedEnumerationImpl *SVGPatternElementImpl::patternUnits() const
{
    if(!m_patternUnits)
    {
        lazy_create<SVGAnimatedEnumerationImpl>(m_patternUnits, this);
        m_patternUnits->setBaseVal(SVG_UNIT_TYPE_OBJECTBOUNDINGBOX);
    }

    return m_patternUnits;
}

SVGAnimatedEnumerationImpl *SVGPatternElementImpl::patternContentUnits() const
{
    if(!m_patternContentUnits)
    {
        lazy_create<SVGAnimatedEnumerationImpl>(m_patternContentUnits, this);
        m_patternContentUnits->setBaseVal(SVG_UNIT_TYPE_USERSPACEONUSE);
    }

    return m_patternContentUnits;
}

SVGAnimatedLengthImpl *SVGPatternElementImpl::x() const
{
    return lazy_create<SVGAnimatedLengthImpl>(m_x, this, LM_WIDTH, viewportElement());
}

SVGAnimatedLengthImpl *SVGPatternElementImpl::y() const
{
    return lazy_create<SVGAnimatedLengthImpl>(m_y, this, LM_HEIGHT, viewportElement());
}

SVGAnimatedLengthImpl *SVGPatternElementImpl::width() const
{
    return lazy_create<SVGAnimatedLengthImpl>(m_width, this, LM_WIDTH, viewportElement());
}

SVGAnimatedLengthImpl *SVGPatternElementImpl::height() const
{
    return lazy_create<SVGAnimatedLengthImpl>(m_height, this, LM_HEIGHT, viewportElement());
}

SVGAnimatedTransformListImpl *SVGPatternElementImpl::patternTransform() const
{
    return lazy_create<SVGAnimatedTransformListImpl>(m_patternTransform, this);
}

void SVGPatternElementImpl::parseMappedAttribute(KDOM::MappedAttributeImpl *attr)
{
    KDOM::DOMString value(attr->value());
    if (attr->name() == SVGNames::patternUnitsAttr)
    {
        if(value == "userSpaceOnUse")
            patternUnits()->setBaseVal(SVG_UNIT_TYPE_USERSPACEONUSE);
        else if(value == "objectBoundingBox")
            patternUnits()->setBaseVal(SVG_UNIT_TYPE_OBJECTBOUNDINGBOX);
    }
    else if (attr->name() == SVGNames::patternContentUnitsAttr)
    {
        if(value == "userSpaceOnUse")
            patternContentUnits()->setBaseVal(SVG_UNIT_TYPE_USERSPACEONUSE);
        else if(value == "objectBoundingBox")
            patternContentUnits()->setBaseVal(SVG_UNIT_TYPE_OBJECTBOUNDINGBOX);
    }
    else if (attr->name() == SVGNames::patternTransformAttr)
    {
        SVGTransformListImpl *patternTransforms = patternTransform()->baseVal();
        SVGTransformableImpl::parseTransformAttribute(patternTransforms, value.impl());
    }
    else if (attr->name() == SVGNames::xAttr)
        x()->baseVal()->setValueAsString(value.impl());
    else if (attr->name() == SVGNames::yAttr)
        y()->baseVal()->setValueAsString(value.impl());
    else if (attr->name() == SVGNames::widthAttr)
        width()->baseVal()->setValueAsString(value.impl());
    else if (attr->name() == SVGNames::heightAttr)
        height()->baseVal()->setValueAsString(value.impl());
    else
    {
        if(SVGURIReferenceImpl::parseMappedAttribute(attr)) return;
        if(SVGTestsImpl::parseMappedAttribute(attr)) return;
        if(SVGLangSpaceImpl::parseMappedAttribute(attr)) return;
        if(SVGExternalResourcesRequiredImpl::parseMappedAttribute(attr)) return;
        if(SVGFitToViewBoxImpl::parseMappedAttribute(attr)) return;

        SVGStyledElementImpl::parseMappedAttribute(attr);
    }
}

const SVGStyledElementImpl *SVGPatternElementImpl::pushAttributeContext(const SVGStyledElementImpl *context)
{
    // All attribute's contexts are equal (so just take the one from 'x').
    const SVGStyledElementImpl *restore = x()->baseVal()->context();

    x()->baseVal()->setContext(context);
    y()->baseVal()->setContext(context);
    width()->baseVal()->setContext(context);
    height()->baseVal()->setContext(context);

    return restore;
}

void SVGPatternElementImpl::resourceNotification() const
{
    // We're referenced by a "client", calculate the tile now...
    notifyAttributeChange();
}

void SVGPatternElementImpl::notifyAttributeChange() const
{
    KRenderingDevice *device = canvas()->renderingDevice();

    if(!m_paintServer || !m_paintServer->activeClient())
        return;

    if(m_ignoreAttributeChanges)
        return;

    float w = width()->baseVal()->value();
    float h = height()->baseVal()->value();

    QSize newSize = QSize(qRound(w), qRound(h));
    if(m_tile && (m_tile->size() == newSize))
        return;

    m_ignoreAttributeChanges = true;

    // Find first pattern def that has children
    const KDOM::ElementImpl *target = this;

    const KDOM::NodeImpl *test = this;
    while(test && !test->hasChildNodes())
    {
        QString ref = KDOM::DOMString(href()->baseVal()).qstring();
        test = ownerDocument()->getElementById(KDOM::DOMString(ref.mid(1)).impl());
        if(test && test->hasTagName(SVGNames::patternTag))
            target = static_cast<const KDOM::ElementImpl *>(test);
    }

    unsigned short savedPatternUnits = patternUnits()->baseVal();
    unsigned short savedPatternContentUnits = patternContentUnits()->baseVal();

    QString ref = KDOM::DOMString(href()->baseVal()).qstring();
    KRenderingPaintServer *refServer = getPaintServerById(getDocument(), ref.mid(1));

    KCanvasMatrix patternTransformMatrix;
    if(patternTransform()->baseVal()->numberOfItems() > 0)
        patternTransformMatrix = KCanvasMatrix(patternTransform()->baseVal()->consolidate()->matrix()->qmatrix());

    if(refServer && refServer->type() == PS_PATTERN)
    {
        KRenderingPaintServerPattern *refPattern = static_cast<KRenderingPaintServerPattern *>(refServer);
        
        if(!hasAttribute(SVGNames::patternUnitsAttr))
        {
            const KDOM::AtomicString& value = target->getAttribute(SVGNames::patternUnitsAttr);
            if(value == "userSpaceOnUse")
                patternUnits()->setBaseVal(SVG_UNIT_TYPE_USERSPACEONUSE);
            else if(value == "objectBoundingBox")
                patternUnits()->setBaseVal(SVG_UNIT_TYPE_OBJECTBOUNDINGBOX);
        }
        
        if(!hasAttribute(SVGNames::patternContentUnitsAttr))
        {
            const KDOM::AtomicString& value = target->getAttribute(SVGNames::patternContentUnitsAttr);
            if(value == "userSpaceOnUse")
                patternContentUnits()->setBaseVal(SVG_UNIT_TYPE_USERSPACEONUSE);
            else if(value == "objectBoundingBox")
                patternContentUnits()->setBaseVal(SVG_UNIT_TYPE_OBJECTBOUNDINGBOX);
        }

        if(!hasAttribute(SVGNames::patternTransformAttr))
            patternTransformMatrix = refPattern->patternTransform();
    }

    SVGStyledElementImpl *activeElement = static_cast<SVGStyledElementImpl *>(m_paintServer->activeClient()->element());

    bool bbox = (patternUnits()->baseVal() == SVG_UNIT_TYPE_OBJECTBOUNDINGBOX);

    const SVGStyledElementImpl *savedContext = 0;
    if(bbox)
    {
        if(width()->baseVal()->unitType() != SVG_LENGTHTYPE_PERCENTAGE)
            width()->baseVal()->newValueSpecifiedUnits(SVG_LENGTHTYPE_PERCENTAGE, width()->baseVal()->value() * 100.);
        if(height()->baseVal()->unitType() != SVG_LENGTHTYPE_PERCENTAGE)
            height()->baseVal()->newValueSpecifiedUnits(SVG_LENGTHTYPE_PERCENTAGE, height()->baseVal()->value() * 100.);
        if(activeElement)
            savedContext = const_cast<SVGPatternElementImpl *>(this)->pushAttributeContext(activeElement);
    }    

    delete m_tile;
    m_tile = static_cast<KCanvasImage *>(canvas()->renderingDevice()->createResource(RS_IMAGE));
    m_tile->init(newSize);

    KRenderingDeviceContext *patternContext = device->contextForImage(m_tile);
    device->pushContext(patternContext);
    
    KRenderingPaintServerPattern *pattern = static_cast<KRenderingPaintServerPattern *>(m_paintServer);
    pattern->setX(x()->baseVal()->value());
    pattern->setY(y()->baseVal()->value());
    pattern->setWidth(width()->baseVal()->value());
    pattern->setHeight(height()->baseVal()->value());
    pattern->setPatternTransform(patternTransformMatrix);
    pattern->setTile(m_tile);

#if !APPLE_COMPILE_HACK
    for(KDOM::NodeImpl *n = target->firstChild(); n != 0; n = n->nextSibling())
    {
        SVGElementImpl *elem = svg_dynamic_cast(n);
        if (!elem || !elem->isStyled())
            return;
        SVGStyledElementImpl *e = static_cast<SVGStyledElementImpl *>(elem);
        khtml::RenderObject *item = e->renderer();
        if(item && item->canvasStyle())
        {
            KCanvasRenderingStyle *canvasStyle = item->canvasStyle();
            KCanvasMatrix savedMatrix = canvasStyle->objectMatrix();

            const SVGStyledElementImpl *savedContext = 0;
            if(patternContentUnits()->baseVal() == SVG_UNIT_TYPE_OBJECTBOUNDINGBOX)
            {
                if(activeElement)
                    savedContext = e->pushAttributeContext(activeElement);
            }

            // Take into account viewportElement's viewBox, if existant...
            if(viewportElement() && viewportElement()->hasTagName(SVGNames::svgTag))
            {
                SVGSVGElementImpl *svgElement = static_cast<SVGSVGElementImpl *>(viewportElement());

                SharedPtr<SVGMatrixImpl> svgCTM = svgElement->getCTM();
                SharedPtr<SVGMatrixImpl> ctm = SVGLocatableImpl::getCTM();

                KCanvasMatrix newMatrix(svgCTM->qmatrix());
                newMatrix.multiply(savedMatrix);
                newMatrix.scale(1.0 / ctm->a(), 1.0 / ctm->d());

                canvasStyle->setObjectMatrix(newMatrix);
            }

            item->draw(QRect());

            if(savedContext)
                e->pushAttributeContext(savedContext);

            canvasStyle->setObjectMatrix(savedMatrix);
        }
    }
#endif

    if(savedContext)
        const_cast<SVGPatternElementImpl *>(this)->pushAttributeContext(savedContext);

    device->popContext();
    delete patternContext;

    patternUnits()->setBaseVal(savedPatternUnits);
    patternContentUnits()->setBaseVal(savedPatternContentUnits);

    // Update all users of this resource...
    const KCanvasItemList &clients = pattern->clients();

    KCanvasItemList::ConstIterator it = clients.begin();
    KCanvasItemList::ConstIterator end = clients.end();

    for(; it != end; ++it)
    {
        const RenderPath *current = (*it);

        SVGStyledElementImpl *styled = (current ? static_cast<SVGStyledElementImpl *>(current->element()) : 0);
        if(styled)
        {
            styled->setChanged(true);

            if(styled->renderer())
                styled->renderer()->repaint();
        }
    }

    m_ignoreAttributeChanges = false;
}

khtml::RenderObject *SVGPatternElementImpl::createRenderer(RenderArena *arena, khtml::RenderStyle *style)
{
    KCanvasContainer *patternContainer = canvas()->renderingDevice()->createContainer(arena, style, this);
    patternContainer->setDrawContents(false);
    return patternContainer;
}

KRenderingPaintServerPattern *SVGPatternElementImpl::canvasResource()
{
    if (!m_paintServer) {
        KRenderingPaintServer *pserver = canvas()->renderingDevice()->createPaintServer(KCPaintServerType(PS_PATTERN));
        m_paintServer = static_cast<KRenderingPaintServerPattern *>(pserver);
        m_paintServer->setListener(const_cast<SVGPatternElementImpl *>(this));
    }
    return m_paintServer;
}

SVGMatrixImpl *SVGPatternElementImpl::getCTM() const
{
    SVGMatrixImpl *mat = SVGSVGElementImpl::createSVGMatrix();
    if(mat)
    {
        SVGMatrixImpl *viewBox = viewBoxToViewTransform(width()->baseVal()->value(),
                                                        height()->baseVal()->value());

        viewBox->ref();
        mat->multiply(viewBox);
        viewBox->deref();
    }

    return mat;
}

// vim:ts=4:noet
