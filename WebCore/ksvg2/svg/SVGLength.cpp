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
#include "SVGLength.h"

#include "DeprecatedString.h"
#include "FrameView.h"
#include "RenderObject.h"
#include "RenderView.h"
#include "SVGSVGElement.h"
#include "SVGStyledElement.h"
#include "svgpathparser.h"

#include <math.h>
#include <wtf/Assertions.h>

namespace WebCore {

// Helper functions
inline unsigned int storeUnit(SVGLengthMode mode, SVGLengthType type)
{
    return (mode << 4) | type;
}

inline SVGLengthMode extractMode(unsigned int unit)
{
    unsigned int mode = unit >> 4;    
    return static_cast<SVGLengthMode>(mode);
}

inline SVGLengthType extractType(unsigned int unit)
{
    unsigned int mode = unit >> 4;
    unsigned int type = unit ^ (mode << 4);
    return static_cast<SVGLengthType>(type);
}

inline String lengthTypeToString(SVGLengthType type)
{
    switch (type) {
    case LengthTypeUnknown:
    case LengthTypeNumber:
        return "";    
    case LengthTypePercentage:
        return "%";
    case LengthTypeEMS:
        return "em";
    case LengthTypeEXS:
        return "ex";
    case LengthTypePX:
        return "px";
    case LengthTypeCM:
        return "cm";
    case LengthTypeMM:
        return "mm";
    case LengthTypeIN:
        return "in";
    case LengthTypePT:
        return "pt";
    case LengthTypePC:
        return "pc";
    }

    return String();
}

inline SVGLengthType stringToLengthType(const String& string)
{
    if (string.endsWith("%"))
        return LengthTypePercentage;
    else if (string.endsWith("em"))
        return LengthTypeEMS;
    else if (string.endsWith("ex"))
        return LengthTypeEXS;
    else if (string.endsWith("px"))
        return LengthTypePX;
    else if (string.endsWith("cm"))
        return LengthTypeCM;
    else if (string.endsWith("mm"))
        return LengthTypeMM;
    else if (string.endsWith("in"))
        return LengthTypeIN;
    else if (string.endsWith("pt"))
        return LengthTypePT;
    else if (string.endsWith("pc"))
        return LengthTypePC;
    else if (!string.isEmpty())
        return LengthTypeNumber;

    return LengthTypeUnknown;
}

SVGLength::SVGLength(const SVGStyledElement* context, SVGLengthMode mode, const String& valueAsString)
    : m_valueInSpecifiedUnits(0.0)
    , m_unit(storeUnit(mode, LengthTypeNumber))
    , m_context(context)
{
    setValueAsString(valueAsString);
}

SVGLengthType SVGLength::unitType() const
{
    return extractType(m_unit);
}

float SVGLength::value() const
{
    SVGLengthType type = extractType(m_unit);
    if (type == LengthTypeUnknown)
        return 0.0;

    switch (type) {
    case LengthTypeNumber:
        return m_valueInSpecifiedUnits;
    case LengthTypePercentage:
        return SVGLength::PercentageOfViewport(m_valueInSpecifiedUnits / 100.0, m_context, extractMode(m_unit));
    case LengthTypeEMS:
    case LengthTypeEXS:
    {
        RenderStyle* style = 0;
        if (m_context && m_context->renderer())
            style = m_context->renderer()->style();
        if (style) {
            float useSize = style->fontSize();
            ASSERT(useSize > 0);
            if (type == LengthTypeEMS)
                return m_valueInSpecifiedUnits * useSize;
            else {
                float xHeight = style->font().xHeight();
                // Use of ceil allows a pixel match to the W3Cs expected output of coords-units-03-b.svg
                // if this causes problems in real world cases maybe it would be best to remove this
                return m_valueInSpecifiedUnits * ceil(xHeight);
            }
        }
        return 0.0;
    }
    case LengthTypePX:
        return m_valueInSpecifiedUnits;
    case LengthTypeCM:
        return m_valueInSpecifiedUnits / 2.54 * dpi();
    case LengthTypeMM:
        return m_valueInSpecifiedUnits / 25.4 * dpi();
    case LengthTypeIN:
        return m_valueInSpecifiedUnits * dpi();
    case LengthTypePT:
        return m_valueInSpecifiedUnits / 72.0 * dpi();
    case LengthTypePC:
        return m_valueInSpecifiedUnits / 6.0 * dpi();
    default:
        break;
    }

    ASSERT_NOT_REACHED();
    return 0.0;
}

void SVGLength::setValue(float value)
{
    SVGLengthType type = extractType(m_unit);
    ASSERT(type != LengthTypeUnknown);

    switch (type) {
    case LengthTypeNumber:
        m_valueInSpecifiedUnits = value;
        break;
    case LengthTypePercentage:
    case LengthTypeEMS:
    case LengthTypeEXS:
        ASSERT_NOT_REACHED();
        break;
    case LengthTypePX:
        m_valueInSpecifiedUnits = value;
        break;
    case LengthTypeCM:
        m_valueInSpecifiedUnits = value * 2.54 / dpi();
        break;
    case LengthTypeMM:
        m_valueInSpecifiedUnits = value * 25.4 / dpi();
        break;
    case LengthTypeIN:
        m_valueInSpecifiedUnits = value / dpi();
        break;
    case LengthTypePT:
        m_valueInSpecifiedUnits = value * 72.0 / dpi();
        break;
    case LengthTypePC:
        m_valueInSpecifiedUnits = value / 6.0 * dpi();
        break;
    default:
        break;
    }

    if (m_context && m_context->ownerDocument()->documentElement() != m_context)
        m_context->notifyAttributeChange();
}

void SVGLength::setValueInSpecifiedUnits(float value)
{
    m_valueInSpecifiedUnits = value;

    if (m_context && m_context->ownerDocument()->documentElement() != m_context)    
        m_context->notifyAttributeChange();
}

float SVGLength::valueInSpecifiedUnits() const
{
    return m_valueInSpecifiedUnits;
}

float SVGLength::valueAsPercentage() const
{
    // 100% = 100.0 instead of 1.0 for historical reasons, this could eventually be changed
    if (extractType(m_unit) == LengthTypePercentage)
        return valueInSpecifiedUnits();
    return valueInSpecifiedUnits() * 100.f;
}

void SVGLength::setValueAsString(const String& s)
{
    if (s.isEmpty())
        return;

    double convertedNumber = 0;
    DeprecatedString depString = s.deprecatedString();
    const char* start = depString.latin1();
    parseCoord(start, convertedNumber);
    
    m_unit = storeUnit(extractMode(m_unit), stringToLengthType(s));
    m_valueInSpecifiedUnits = convertedNumber;
}

String SVGLength::valueAsString() const
{
    return String::number(m_valueInSpecifiedUnits) + lengthTypeToString(extractType(m_unit));
}

void SVGLength::newValueSpecifiedUnits(unsigned short type, float value)
{
    ASSERT(type <= LengthTypePC);

    m_unit = storeUnit(extractMode(m_unit), (SVGLengthType) type);
    m_valueInSpecifiedUnits = value;
}

void SVGLength::convertToSpecifiedUnits(unsigned short type)
{
    ASSERT(type <= LengthTypePC);

    float valueInUserUnits = value();
    m_unit = storeUnit(extractMode(m_unit), (SVGLengthType) type);
    setValue(valueInUserUnits);
}

double SVGLength::dpi() const
{
    /* FIXME: DPI detection
    if (context && context->ownerDoc()) {
        if (mode == LengthModeWidth)
            return 25.4 * context->ownerDoc()->screenPixelsPerMillimeterX();
        else if (mode == LengthModeHeight)
            return 25.4 * context->ownerDoc()->screenPixelsPerMillimeterY();
        else if (mode == LengthModeOther)
            return 25.4 * context->ownerDoc()->screenPixelsPerMillimeterX();
    }
    */

    return 90.0;
}

float SVGLength::PercentageOfViewport(float value, const SVGStyledElement* context, SVGLengthMode mode)
{
    ASSERT(context);

    float width = 0, height = 0;
    SVGElement* viewportElement = context->viewportElement();

    Document* doc = context->document();
    if (doc->documentElement() == context) {
        // We have to ask the canvas for the full "canvas size"...
        RenderView* view = static_cast<RenderView*>(doc->renderer());
        if (view) {
            width = view->frameView()->visibleWidth(); // TODO: recheck!
            height = view->frameView()->visibleHeight(); // TODO: recheck!
         }
    } else if (viewportElement && viewportElement->isSVG()) {
        const SVGSVGElement* svg = static_cast<const SVGSVGElement*>(viewportElement);
        if (svg->hasAttribute(SVGNames::viewBoxAttr)) {
            width = svg->viewBox().width();
            height = svg->viewBox().height();
        } else {
            width = svg->width().value();
            height = svg->height().value();
        }
    }

    if (mode == LengthModeWidth)
        return value * width;
    else if (mode == LengthModeHeight)
        return value * height;
    else if (mode == LengthModeOther)
        return value * sqrt(pow(double(width), 2) + pow(double(height), 2)) / sqrt(2.0);

    return 0.0;
}

}

#endif // SVG_SUPPORT

// vim:ts=4:noet
