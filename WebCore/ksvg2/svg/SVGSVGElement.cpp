/*
    Copyright (C) 2004, 2005, 2006 Nikolas Zimmermann <zimmermann@kde.org>
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
#include "SVGSVGElement.h"

#include "AffineTransform.h"
#include "CSSPropertyNames.h"
#include "Document.h"
#include "EventListener.h"
#include "EventNames.h"
#include "Frame.h"
#include "HTMLNames.h"
#include "RenderSVGContainer.h"
#include "SVGAngle.h"
#include "SVGLength.h"
#include "SVGNames.h"
#include "SVGPreserveAspectRatio.h"
#include "SVGTransform.h"
#include "SVGZoomEvent.h"
#include "SelectionController.h"
#include "TextStream.h"
#include "TimeScheduler.h"

namespace WebCore {

using namespace HTMLNames;
using namespace EventNames;
using namespace SVGNames;

SVGSVGElement::SVGSVGElement(const QualifiedName& tagName, Document* doc)
    : SVGStyledLocatableElement(tagName, doc)
    , SVGTests()
    , SVGLangSpace()
    , SVGExternalResourcesRequired()
    , SVGFitToViewBox()
    , SVGZoomAndPan()
    , m_x(this, LengthModeWidth)
    , m_y(this, LengthModeHeight)
    , m_width(this, LengthModeWidth)
    , m_height(this, LengthModeHeight)
    , m_useCurrentView(false)
    , m_timeScheduler(new TimeScheduler(doc))
{
    setWidthBaseValue(SVGLength(this, LengthModeWidth, "100%"));
    setHeightBaseValue(SVGLength(this, LengthModeHeight, "100%"));
}

SVGSVGElement::~SVGSVGElement()
{
    delete m_timeScheduler;
}

ANIMATED_PROPERTY_DEFINITIONS(SVGSVGElement, SVGLength, Length, length, X, x, SVGNames::xAttr.localName(), m_x)
ANIMATED_PROPERTY_DEFINITIONS(SVGSVGElement, SVGLength, Length, length, Y, y, SVGNames::yAttr.localName(), m_y)
ANIMATED_PROPERTY_DEFINITIONS(SVGSVGElement, SVGLength, Length, length, Width, width, SVGNames::widthAttr.localName(), m_width)
ANIMATED_PROPERTY_DEFINITIONS(SVGSVGElement, SVGLength, Length, length, Height, height, SVGNames::heightAttr.localName(), m_height)

const AtomicString& SVGSVGElement::contentScriptType() const
{
    static const AtomicString defaultValue("text/ecmascript");
    const AtomicString& n = getAttribute(contentScriptTypeAttr);
    return n.isNull() ? defaultValue : n;
}

void SVGSVGElement::setContentScriptType(const AtomicString& type)
{
    setAttribute(SVGNames::contentScriptTypeAttr, type);
}

const AtomicString& SVGSVGElement::contentStyleType() const
{
    static const AtomicString defaultValue("text/css");
    const AtomicString& n = getAttribute(contentStyleTypeAttr);
    return n.isNull() ? defaultValue : n;
}

void SVGSVGElement::setContentStyleType(const AtomicString& type)
{
    setAttribute(SVGNames::contentStyleTypeAttr, type);
}

FloatRect SVGSVGElement::viewport() const
{
    double _x = x().value();
    double _y = y().value();
    double w = width().value();
    double h = height().value();
    AffineTransform viewBox = viewBoxToViewTransform(w, h);
    viewBox.map(_x, _y, &_x, &_y);
    viewBox.map(w, h, &w, &h);
    return FloatRect(_x, _y, w, h);
}

float SVGSVGElement::pixelUnitToMillimeterX() const
{
    // FIXME: Implement me (see bug 11273)
    return .28;
}

float SVGSVGElement::pixelUnitToMillimeterY() const
{
    // FIXME: Implement me (see bug 11273)
    return .28;
}

float SVGSVGElement::screenPixelToMillimeterX() const
{
    return pixelUnitToMillimeterX();
}

float SVGSVGElement::screenPixelToMillimeterY() const
{
    return pixelUnitToMillimeterY();
}

bool SVGSVGElement::useCurrentView() const
{
    return m_useCurrentView;
}

void SVGSVGElement::setUseCurrentView(bool currentView)
{
    m_useCurrentView = currentView;
}

float SVGSVGElement::currentScale() const
{
    // FIXME: Return the current zoom/scale value (see bug 11272)
    return 1.;
}

void SVGSVGElement::setCurrentScale(float scale)
{
    // FIXME: Set the current zoom/scale value (see bug 11272)
}

FloatPoint SVGSVGElement::currentTranslate() const
{
    // FIXME: Return the current pan/translate value (see bug 11272)
    return FloatPoint();
}

void SVGSVGElement::addSVGWindowEventListener(const AtomicString& eventType, const Attribute* attr)
{
    // FIXME: None of these should be window events long term.
    // Once we propertly support SVGLoad, etc.
    RefPtr<EventListener> listener = document()->accessSVGExtensions()->
        createSVGEventListener(attr->localName().domString(), attr->value(), this);
    document()->setHTMLWindowEventListener(eventType, listener.release());
}

void SVGSVGElement::parseMappedAttribute(MappedAttribute* attr)
{
    const AtomicString& value = attr->value();
    if (!nearestViewportElement()) {
        // Only handle events if we're the outermost <svg> element
        if (attr->name() == onunloadAttr)
            addSVGWindowEventListener(unloadEvent, attr);
        else if (attr->name() == onabortAttr)
            addSVGWindowEventListener(abortEvent, attr);
        else if (attr->name() == onerrorAttr)
            addSVGWindowEventListener(errorEvent, attr);
        else if (attr->name() == onresizeAttr)
            addSVGWindowEventListener(resizeEvent, attr);
        else if (attr->name() == onscrollAttr)
            addSVGWindowEventListener(scrollEvent, attr);
        else if (attr->name() == SVGNames::onzoomAttr)
            addSVGWindowEventListener(zoomEvent, attr);
    }
    if (attr->name() == SVGNames::xAttr)
        setXBaseValue(SVGLength(this, LengthModeWidth, value));
    else if (attr->name() == SVGNames::yAttr)
        setYBaseValue(SVGLength(this, LengthModeHeight, value));
    else if (attr->name() == SVGNames::widthAttr) {
        setWidthBaseValue(SVGLength(this, LengthModeWidth, value));
        addCSSProperty(attr, CSS_PROP_WIDTH, value);
        if (width().value() < 0.0)
            document()->accessSVGExtensions()->reportError("A negative value for svg attribute <width> is not allowed");
    } else if (attr->name() == SVGNames::heightAttr) {
        setHeightBaseValue(SVGLength(this, LengthModeHeight, value));
        addCSSProperty(attr, CSS_PROP_HEIGHT, value);
        if (height().value() < 0.0)
            document()->accessSVGExtensions()->reportError("A negative value for svg attribute <height> is not allowed");
    } else {
        if (SVGTests::parseMappedAttribute(attr))
            return;
        if (SVGLangSpace::parseMappedAttribute(attr))
            return;
        if (SVGExternalResourcesRequired::parseMappedAttribute(attr))
            return;
        if (SVGFitToViewBox::parseMappedAttribute(attr) && renderer()) {
            static_cast<RenderSVGContainer*>(renderer())->setViewBox(viewBox());
            return;
        }
        if (SVGZoomAndPan::parseMappedAttribute(attr))
            return;

        SVGStyledLocatableElement::parseMappedAttribute(attr);
    }
}

unsigned long SVGSVGElement::suspendRedraw(unsigned long /* max_wait_milliseconds */)
{
    // FIXME: Implement me (see bug 11275)
    return 0;
}

void SVGSVGElement::unsuspendRedraw(unsigned long /* suspend_handle_id */, ExceptionCode& ec)
{
    // if suspend_handle_id is not found, throw exception
    // FIXME: Implement me (see bug 11275)
}

void SVGSVGElement::unsuspendRedrawAll()
{
    // FIXME: Implement me (see bug 11275)
}

void SVGSVGElement::forceRedraw()
{
    // FIXME: Implement me (see bug 11275)
}

NodeList* SVGSVGElement::getIntersectionList(const FloatRect& rect, SVGElement*)
{
    // FIXME: Implement me (see bug 11274)
    return 0;
}

NodeList* SVGSVGElement::getEnclosureList(const FloatRect& rect, SVGElement*)
{
    // FIXME: Implement me (see bug 11274)
    return 0;
}

bool SVGSVGElement::checkIntersection(SVGElement* element, const FloatRect& rect)
{
    // TODO : take into account pointer-events?
    // FIXME: Why is element ignored??
    // FIXME: Implement me (see bug 11274)
    return rect.intersects(getBBox());
}

bool SVGSVGElement::checkEnclosure(SVGElement* element, const FloatRect& rect)
{
    // TODO : take into account pointer-events?
    // FIXME: Why is element ignored??
    // FIXME: Implement me (see bug 11274)
    return rect.contains(getBBox());
}

void SVGSVGElement::deselectAll()
{
    document()->frame()->selectionController()->clear();
}

double SVGSVGElement::createSVGNumber()
{
    return 0.0;
}

SVGLength SVGSVGElement::createSVGLength()
{
    return SVGLength();
}

SVGAngle* SVGSVGElement::createSVGAngle()
{
    return new SVGAngle(0);
}

FloatPoint SVGSVGElement::createSVGPoint()
{
    return FloatPoint();
}

AffineTransform SVGSVGElement::createSVGMatrix()
{
    return AffineTransform();
}

FloatRect SVGSVGElement::createSVGRect()
{
    return FloatRect();
}

SVGTransform SVGSVGElement::createSVGTransform()
{
    return SVGTransform();
}

SVGTransform SVGSVGElement::createSVGTransformFromMatrix(const AffineTransform& matrix)
{
    return SVGTransform(matrix);
}

AffineTransform SVGSVGElement::getCTM() const
{
    AffineTransform mat;
    mat.translate(x().value(), y().value());

    if (attributes()->getNamedItem(SVGNames::viewBoxAttr)) {
        AffineTransform viewBox = viewBoxToViewTransform(width().value(), height().value());
        mat = viewBox * mat;
    }

    return mat;
}

AffineTransform SVGSVGElement::getScreenCTM() const
{
    AffineTransform mat = SVGStyledLocatableElement::getScreenCTM();
    mat.translate(x().value(), y().value());

    if (attributes()->getNamedItem(SVGNames::viewBoxAttr)) {
        AffineTransform viewBox = viewBoxToViewTransform(width().value(), height().value());
        mat = viewBox * mat;
    }

    return mat;
}

RenderObject* SVGSVGElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    RenderSVGContainer* rootContainer = new (arena) RenderSVGContainer(this);

    // FIXME: All this setup should be done after attributesChanged, not here.
    rootContainer->setViewBox(viewBox());
    rootContainer->setAlign(KCAlign(preserveAspectRatio()->align() - 1));
    rootContainer->setSlice(preserveAspectRatio()->meetOrSlice() == SVGPreserveAspectRatio::SVG_MEETORSLICE_SLICE);
    
    return rootContainer;
}

void SVGSVGElement::insertedIntoDocument()
{
    document()->accessSVGExtensions()->addTimeContainer(this);
    SVGStyledLocatableElement::insertedIntoDocument();
}

void SVGSVGElement::removedFromDocument()
{
    document()->accessSVGExtensions()->removeTimeContainer(this);
    SVGStyledLocatableElement::removedFromDocument();
}

void SVGSVGElement::setZoomAndPan(unsigned short zoomAndPan)
{
    SVGZoomAndPan::setZoomAndPan(zoomAndPan);
    //canvasView()->enableZoomAndPan(zoomAndPan == SVG_ZOOMANDPAN_MAGNIFY);
}

void SVGSVGElement::pauseAnimations()
{
    if (!m_timeScheduler->animationsPaused())
        m_timeScheduler->toggleAnimations();
}

void SVGSVGElement::unpauseAnimations()
{
    if (m_timeScheduler->animationsPaused())
        m_timeScheduler->toggleAnimations();
}

bool SVGSVGElement::animationsPaused() const
{
    return m_timeScheduler->animationsPaused();
}

float SVGSVGElement::getCurrentTime() const
{
    return m_timeScheduler->elapsed();
}

void SVGSVGElement::setCurrentTime(float /* seconds */)
{
    // FIXME: Implement me, bug 12073
}

bool SVGSVGElement::hasRelativeValues() const
{
    return (x().isRelative() || width().isRelative() ||
            y().isRelative() || height().isRelative());
}

void SVGSVGElement::attributeChanged(Attribute* attr, bool preserveDecls)
{
    if (attr->name() == SVGNames::xAttr ||
        attr->name() == SVGNames::yAttr ||
        attr->name() == SVGNames::widthAttr ||
        attr->name() == SVGNames::heightAttr)
        if (renderer())
            renderer()->setNeedsLayout(true);

    SVGStyledElement::attributeChanged(attr, preserveDecls);
}

}

#endif // SVG_SUPPORT

// vim:ts=4:noet
