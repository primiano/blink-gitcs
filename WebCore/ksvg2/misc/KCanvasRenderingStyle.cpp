/*
    Copyright (C) 2004, 2005 Nikolas Zimmermann <wildfox@kde.org>
                  2004, 2005 Rob Buis <buis@kde.org>
                        2006 Alexander Kellett <lypanov@kde.org>

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
    aint with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include "config.h"
#ifdef SVG_SUPPORT

#include "CSSValueList.h"
#include "Document.h"
#include "KCanvasRenderingStyle.h"
#include "KRenderingDevice.h"
#include "KRenderingPaintServerGradient.h"
#include "KRenderingPaintServerSolid.h"
#include "RenderObject.h"
#include "SVGLength.h"
#include "SVGRenderStyle.h"
#include "SVGStyledElement.h"
#include "ksvg.h"

namespace WebCore {

static KRenderingPaintServerSolid* sharedSolidPaintServer()
{
    static KRenderingPaintServerSolid* _sharedSolidPaintServer = 0;
    if (!_sharedSolidPaintServer)
        _sharedSolidPaintServer = static_cast<KRenderingPaintServerSolid*>(renderingDevice()->createPaintServer(PS_SOLID));
    return _sharedSolidPaintServer;
}

KRenderingPaintServer* KSVGPainterFactory::fillPaintServer(const RenderStyle* style, const RenderObject* item)
{
    if (!style->svgStyle()->hasFill())
        return 0;

    SVGPaint* fill = style->svgStyle()->fillPaint();

    KRenderingPaintServer* fillPaintServer = 0;
    if (fill->paintType() == SVGPaint::SVG_PAINTTYPE_URI) {
        fillPaintServer = getPaintServerById(item->document(), AtomicString(fill->uri().substring(1)));
        if (fillPaintServer && item->isRenderPath())
            fillPaintServer->addClient(static_cast<const RenderPath*>(item));
        if (!fillPaintServer) {
            // default value (black), see bug 11017
            fillPaintServer = sharedSolidPaintServer();
            static_cast<KRenderingPaintServerSolid*>(fillPaintServer)->setColor(Color::black);
        }
    } else {
        fillPaintServer = sharedSolidPaintServer();
        KRenderingPaintServerSolid* fillPaintServerSolid = static_cast<KRenderingPaintServerSolid*>(fillPaintServer);
        if (fill->paintType() == SVGPaint::SVG_PAINTTYPE_CURRENTCOLOR)
            fillPaintServerSolid->setColor(style->color());
        else
            fillPaintServerSolid->setColor(fill->color());
    }
    return fillPaintServer;
}

KRenderingPaintServer* KSVGPainterFactory::strokePaintServer(const RenderStyle* style, const RenderObject* item)
{
    if (!style->svgStyle()->hasStroke())
        return 0;

    SVGPaint* stroke = style->svgStyle()->strokePaint();

    KRenderingPaintServer* strokePaintServer = 0;
    if (stroke->paintType() == SVGPaint::SVG_PAINTTYPE_URI) {
        strokePaintServer = getPaintServerById(item->document(), AtomicString(stroke->uri().substring(1)));
        if (item && strokePaintServer && item->isRenderPath())
            strokePaintServer->addClient(static_cast<const RenderPath*>(item));
    } else {
        strokePaintServer = sharedSolidPaintServer();
        KRenderingPaintServerSolid* strokePaintServerSolid = static_cast<KRenderingPaintServerSolid*>(strokePaintServer);
        if (stroke->paintType() == SVGPaint::SVG_PAINTTYPE_CURRENTCOLOR)
            strokePaintServerSolid->setColor(style->color());
        else
            strokePaintServerSolid->setColor(stroke->color());
    }

    return strokePaintServer;
}

double KSVGPainterFactory::cssPrimitiveToLength(const RenderObject* item, CSSValue *value, double defaultValue)
{
    CSSPrimitiveValue* primitive = static_cast<CSSPrimitiveValue*>(value);

    unsigned short cssType = (primitive ? primitive->primitiveType() : (unsigned short) CSSPrimitiveValue::CSS_UNKNOWN);
    if (!(cssType > CSSPrimitiveValue::CSS_UNKNOWN && cssType <= CSSPrimitiveValue::CSS_PC))
        return defaultValue;

    if (cssType == CSSPrimitiveValue::CSS_PERCENTAGE) {
        SVGElement* element = static_cast<SVGElement*>(item->element());
        SVGElement* viewportElement = (element ? element->viewportElement() : 0);
        if (viewportElement) {
            double result = primitive->getFloatValue() / 100.0;
            return SVGHelper::PercentageOfViewport(result, viewportElement, LM_OTHER);
        }
    }

    return primitive->computeLengthFloat(const_cast<RenderStyle*>(item->style()));
}

KCDashArray KSVGPainterFactory::dashArrayFromRenderingStyle(const RenderStyle* style)
{
    KCDashArray array;
    
    CSSValueList* dashes = style->svgStyle()->strokeDashArray();
    if (dashes) {
        CSSPrimitiveValue* dash = 0;
        unsigned long len = dashes->length();
        for (unsigned long i = 0; i < len; i++) {
            dash = static_cast<CSSPrimitiveValue*>(dashes->item(i));
            if (!dash)
                continue;

            array.append((float) dash->computeLengthFloat(const_cast<RenderStyle*>(style)));
        }
    }

    return array;
}

}

// vim:ts=4:noet
#endif // SVG_SUPPORT

