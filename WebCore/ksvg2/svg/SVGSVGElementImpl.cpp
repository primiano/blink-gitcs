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
#include <kdom/core/AttrImpl.h>
#include <kdom/core/DOMStringImpl.h>
#include <kdom/core/NamedAttrMapImpl.h>
#include "DocumentImpl.h"

#include <kcanvas/KCanvas.h>
#include <kcanvas/RenderPath.h>
#include <kcanvas/KCanvasMatrix.h>
#include <kcanvas/KCanvasCreator.h>
#include <kcanvas/KCanvasContainer.h>
#include <kcanvas/device/KRenderingDevice.h>

#include "ksvg.h"
#include "SVGNames.h"
#include "SVGHelper.h"
#include "SVGRectImpl.h"
#include "SVGAngleImpl.h"
#include "SVGPointImpl.h"
#include "SVGMatrixImpl.h"
#include "SVGNumberImpl.h"
#include "SVGLengthImpl.h"
#include "SVGRenderStyle.h"
#include "SVGZoomEventImpl.h"
#include "SVGTransformImpl.h"
#include "SVGSVGElementImpl.h"
#include "KSVGTimeScheduler.h"
#include "SVGZoomAndPanImpl.h"
#include "SVGFitToViewBoxImpl.h"
#include "SVGAnimatedRectImpl.h"
#include "SVGAnimatedLengthImpl.h"
#include "KCanvasRenderingStyle.h"
#include "SVGPreserveAspectRatioImpl.h"
#include "SVGAnimatedPreserveAspectRatioImpl.h"
#include "SVGDocumentExtensions.h"

#include "cssproperties.h"

#include <q3paintdevicemetrics.h>
#include <qtextstream.h>

#include "htmlnames.h"
#include "EventNames.h"

namespace WebCore {

using namespace HTMLNames;
using namespace EventNames;

SVGSVGElementImpl::SVGSVGElementImpl(const KDOM::QualifiedName& tagName, KDOM::DocumentImpl *doc)
: SVGStyledLocatableElementImpl(tagName, doc), SVGTestsImpl(), SVGLangSpaceImpl(),
  SVGExternalResourcesRequiredImpl(), SVGFitToViewBoxImpl(),
  SVGZoomAndPanImpl()
{
    m_useCurrentView = false;
}

SVGSVGElementImpl::~SVGSVGElementImpl()
{
}

SVGAnimatedLengthImpl *SVGSVGElementImpl::x() const
{
    const SVGElementImpl *viewport = ownerDocument()->documentElement() == this ? this : viewportElement();
    return lazy_create<SVGAnimatedLengthImpl>(m_x, (SVGStyledElementImpl *)0, LM_WIDTH, viewport);
}

SVGAnimatedLengthImpl *SVGSVGElementImpl::y() const
{
    const SVGElementImpl *viewport = ownerDocument()->documentElement() == this ? this : viewportElement();
    return lazy_create<SVGAnimatedLengthImpl>(m_y, (SVGStyledElementImpl *)0, LM_HEIGHT, viewport);
}

SVGAnimatedLengthImpl *SVGSVGElementImpl::width() const
{
    if (!m_width) {
        KDOM::DOMString temp("100%");
        const SVGElementImpl *viewport = ownerDocument()->documentElement() == this ? this : viewportElement();
        lazy_create<SVGAnimatedLengthImpl>(m_width, (SVGStyledElementImpl *)0, LM_WIDTH, viewport);
        m_width->baseVal()->setValueAsString(temp.impl());
    }

    return m_width.get();
}

SVGAnimatedLengthImpl *SVGSVGElementImpl::height() const
{
    if (!m_height) {
        KDOM::DOMString temp("100%");
        const SVGElementImpl *viewport = ownerDocument()->documentElement() == this ? this : viewportElement();
        lazy_create<SVGAnimatedLengthImpl>(m_height, (SVGStyledElementImpl *)0, LM_HEIGHT, viewport);
        m_height->baseVal()->setValueAsString(temp.impl());
    }

    return m_height.get();
}

KDOM::AtomicString SVGSVGElementImpl::contentScriptType() const
{
    return tryGetAttribute("contentScriptType", "text/ecmascript");
}

void SVGSVGElementImpl::setContentScriptType(const KDOM::AtomicString& type)
{
    setAttribute(SVGNames::contentScriptTypeAttr, type);
}

KDOM::AtomicString SVGSVGElementImpl::contentStyleType() const
{
    return tryGetAttribute("contentStyleType", "text/css");
}

void SVGSVGElementImpl::setContentStyleType(const KDOM::AtomicString& type)
{
    setAttribute(SVGNames::contentStyleTypeAttr, type);
}

SVGRectImpl *SVGSVGElementImpl::viewport() const
{
    SVGRectImpl *ret = createSVGRect();
    double _x = x()->baseVal()->value();
    double _y = y()->baseVal()->value();
    double w = width()->baseVal()->value();
    double h = height()->baseVal()->value();
    RefPtr<SVGMatrixImpl> viewBox = viewBoxToViewTransform(w, h);
    viewBox->qmatrix().map(_x, _y, &_x, &_y);
    viewBox->qmatrix().map(w, h, &w, &h);
    ret->setX(_x);
    ret->setY(_y);
    ret->setWidth(w);
    ret->setHeight(h);
    return ret;
}

float SVGSVGElementImpl::pixelUnitToMillimeterX() const
{
#if 0
    if(ownerDocument() && ownerDocument()->paintDeviceMetrics())
    {
        Q3PaintDeviceMetrics *metrics = ownerDocument()->paintDeviceMetrics();
        return float(metrics->widthMM()) / float(metrics->width());
    }
#endif

    return .28;
}

float SVGSVGElementImpl::pixelUnitToMillimeterY() const
{
#if 0
    if(ownerDocument() && ownerDocument()->paintDeviceMetrics())
    {
        Q3PaintDeviceMetrics *metrics = ownerDocument()->paintDeviceMetrics();
        return float(metrics->heightMM()) / float(metrics->height());
    }
#endif

    return .28;
}

float SVGSVGElementImpl::screenPixelToMillimeterX() const
{
    return pixelUnitToMillimeterX();
}

float SVGSVGElementImpl::screenPixelToMillimeterY() const
{
    return pixelUnitToMillimeterY();
}

bool SVGSVGElementImpl::useCurrentView() const
{
    return m_useCurrentView;
}

void SVGSVGElementImpl::setUseCurrentView(bool currentView)
{
    m_useCurrentView = currentView;
}

float SVGSVGElementImpl::currentScale() const
{
    //if(!canvasView())
        return 1.;

    //return canvasView()->zoom();
}

void SVGSVGElementImpl::setCurrentScale(float scale)
{
//    if(canvasView())
//    {
//        float oldzoom = canvasView()->zoom();
//        canvasView()->setZoom(scale);
//        getDocument()->dispatchZoomEvent(oldzoom, scale);
//    }
}

SVGPointImpl *SVGSVGElementImpl::currentTranslate() const
{
    //if(!canvas())
        return 0;

    //return createSVGPoint(canvasView()->pan());
}

void SVGSVGElementImpl::addSVGWindowEventListner(const AtomicString& eventType, const AttributeImpl* attr)
{
    // FIXME: None of these should be window events long term.
    // Once we propertly support SVGLoad, etc.
    EventListener *listener = getDocument()->accessSVGExtensions()->createSVGEventListener(attr->value(), this);
    getDocument()->setHTMLWindowEventListener(eventType, listener);
}

void SVGSVGElementImpl::parseMappedAttribute(KDOM::MappedAttributeImpl *attr)
{
    const KDOM::AtomicString& value = attr->value();
    if (!nearestViewportElement()) {
        // Only handle events if we're the outermost <svg> element
        if (attr->name() == onloadAttr)
            addSVGWindowEventListner(loadEvent, attr);
        else if (attr->name() == onunloadAttr)
            addSVGWindowEventListner(unloadEvent, attr);
        else if (attr->name() == onabortAttr)
            addSVGWindowEventListner(abortEvent, attr);
        else if (attr->name() == onerrorAttr)
            addSVGWindowEventListner(errorEvent, attr);
        else if (attr->name() == onresizeAttr)
            addSVGWindowEventListner(resizeEvent, attr);
        else if (attr->name() == onscrollAttr)
            addSVGWindowEventListner(scrollEvent, attr);
        else if (attr->name() == SVGNames::onzoomAttr)
            addSVGWindowEventListner(zoomEvent, attr);
    }
    if (attr->name() == SVGNames::xAttr) {
        x()->baseVal()->setValueAsString(value.impl());
    } else if (attr->name() == SVGNames::yAttr) {
        y()->baseVal()->setValueAsString(value.impl());
    } else if (attr->name() == SVGNames::widthAttr) {
        width()->baseVal()->setValueAsString(value.impl());
        addCSSProperty(attr, CSS_PROP_WIDTH, value);
    } else if (attr->name() == SVGNames::heightAttr) {
        height()->baseVal()->setValueAsString(value.impl());
        addCSSProperty(attr, CSS_PROP_HEIGHT, value);
    } else
    {
        if(SVGTestsImpl::parseMappedAttribute(attr)) return;
        if(SVGLangSpaceImpl::parseMappedAttribute(attr)) return;
        if(SVGExternalResourcesRequiredImpl::parseMappedAttribute(attr)) return;
        if(SVGFitToViewBoxImpl::parseMappedAttribute(attr)) return;
        if(SVGZoomAndPanImpl::parseMappedAttribute(attr)) return;

        SVGStyledLocatableElementImpl::parseMappedAttribute(attr);
    }
}

unsigned long SVGSVGElementImpl::suspendRedraw(unsigned long /* max_wait_milliseconds */)
{
    // TODO
    return 0;
}

void SVGSVGElementImpl::unsuspendRedraw(unsigned long /* suspend_handle_id */)
{
    // TODO
}

void SVGSVGElementImpl::unsuspendRedrawAll()
{
    // TODO
}

void SVGSVGElementImpl::forceRedraw()
{
    // TODO
}

KDOM::NodeListImpl *SVGSVGElementImpl::getIntersectionList(SVGRectImpl *rect, SVGElementImpl *)
{
    //KDOM::NodeListImpl *list;
    // TODO
    return 0;
}

KDOM::NodeListImpl *SVGSVGElementImpl::getEnclosureList(SVGRectImpl *rect, SVGElementImpl *)
{
    // TODO
    return 0;
}

bool SVGSVGElementImpl::checkIntersection(SVGElementImpl *element, SVGRectImpl *rect)
{
    // TODO : take into account pointer-events?
    RefPtr<SVGRectImpl> bbox = getBBox();
    
    FloatRect r(rect->x(), rect->y(), rect->width(), rect->height());
    FloatRect r2(bbox->x(), bbox->y(), bbox->width(), bbox->height());
    
    return r.intersects(r2);
}

bool SVGSVGElementImpl::checkEnclosure(SVGElementImpl *element, SVGRectImpl *rect)
{
    // TODO : take into account pointer-events?
    RefPtr<SVGRectImpl> bbox = getBBox();
    
    FloatRect r(rect->x(), rect->y(), rect->width(), rect->height());
    FloatRect r2(bbox->x(), bbox->y(), bbox->width(), bbox->height());
    
    return r.contains(r2);
}

void SVGSVGElementImpl::deselectAll()
{
    // TODO
}

SVGNumberImpl *SVGSVGElementImpl::createSVGNumber()
{
    return new SVGNumberImpl(0);
}

SVGLengthImpl *SVGSVGElementImpl::createSVGLength()
{
    return new SVGLengthImpl(0);
}

SVGAngleImpl *SVGSVGElementImpl::createSVGAngle()
{
    return new SVGAngleImpl(0);
}

SVGPointImpl *SVGSVGElementImpl::createSVGPoint(const IntPoint &p)
{
    if(p.isNull())
        return new SVGPointImpl();
    else
        return new SVGPointImpl(p);
}

SVGMatrixImpl *SVGSVGElementImpl::createSVGMatrix()
{
    return new SVGMatrixImpl();
}

SVGRectImpl *SVGSVGElementImpl::createSVGRect()
{
    return new SVGRectImpl(0);
}

SVGTransformImpl *SVGSVGElementImpl::createSVGTransform()
{
    return new SVGTransformImpl();
}

SVGTransformImpl *SVGSVGElementImpl::createSVGTransformFromMatrix(SVGMatrixImpl *matrix)
{    
    SVGTransformImpl *obj = SVGSVGElementImpl::createSVGTransform();
    obj->setMatrix(matrix);
    return obj;
}

SVGMatrixImpl *SVGSVGElementImpl::getCTM() const
{
    SVGMatrixImpl *mat = createSVGMatrix();
    if(mat)
    {
        mat->translate(x()->baseVal()->value(), y()->baseVal()->value());

        if(attributes()->getNamedItem(SVGNames::viewBoxAttr))
        {
            RefPtr<SVGMatrixImpl> viewBox = viewBoxToViewTransform(width()->baseVal()->value(), height()->baseVal()->value());
            mat->multiply(viewBox.get());
        }
    }

    return mat;
}

SVGMatrixImpl *SVGSVGElementImpl::getScreenCTM() const
{
    SVGMatrixImpl *mat = SVGStyledLocatableElementImpl::getScreenCTM();
    if(mat)
    {
        mat->translate(x()->baseVal()->value(), y()->baseVal()->value());

        if(attributes()->getNamedItem(SVGNames::viewBoxAttr))
        {
            RefPtr<SVGMatrixImpl> viewBox = viewBoxToViewTransform(width()->baseVal()->value(), height()->baseVal()->value());
            mat->multiply(viewBox.get());
        }
    }

    return mat;
}

khtml::RenderObject *SVGSVGElementImpl::createRenderer(RenderArena *arena, khtml::RenderStyle *style)
{
    KCanvasContainer *rootContainer = QPainter::renderingDevice()->createContainer(arena, style, this);

    // FIXME: all this setup should be done after attributesChanged, not here.
    float _x = x()->baseVal()->value();
    float _y = y()->baseVal()->value();
    float _width = width()->baseVal()->value();
    float _height = height()->baseVal()->value();

    rootContainer->setViewport(FloatRect(_x, _y, _width, _height));
    rootContainer->setViewBox(FloatRect(viewBox()->baseVal()->x(), viewBox()->baseVal()->y(), viewBox()->baseVal()->width(), viewBox()->baseVal()->height()));
    rootContainer->setAlign(KCAlign(preserveAspectRatio()->baseVal()->align() - 1));
    rootContainer->setSlice(preserveAspectRatio()->baseVal()->meetOrSlice() == SVG_MEETORSLICE_SLICE);
    
    return rootContainer;
}

void SVGSVGElementImpl::setZoomAndPan(unsigned short zoomAndPan)
{
    SVGZoomAndPanImpl::setZoomAndPan(zoomAndPan);
    //canvasView()->enableZoomAndPan(zoomAndPan == SVG_ZOOMANDPAN_MAGNIFY);
}

}

// vim:ts=4:noet
#endif // SVG_SUPPORT

