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

#if ENABLE(SVG)
#include "SVGLinearGradientElement.h"

#include "LinearGradientAttributes.h"
#include "SVGPaintServerLinearGradient.h"
#include "SVGLength.h"
#include "SVGNames.h"
#include "SVGTransform.h"
#include "SVGTransformList.h"
#include "SVGUnitTypes.h"

namespace WebCore {

SVGLinearGradientElement::SVGLinearGradientElement(const QualifiedName& tagName, Document* doc)
    : SVGGradientElement(tagName, doc)
    , m_x1(this, LengthModeWidth)
    , m_y1(this, LengthModeHeight)
    , m_x2(this, LengthModeWidth)
    , m_y2(this, LengthModeHeight)
{
    // Spec: If the attribute is not specified, the effect is as if a value of "100%" were specified.
    setX2BaseValue(SVGLength(this, LengthModeWidth, "100%"));
}

SVGLinearGradientElement::~SVGLinearGradientElement()
{
}

ANIMATED_PROPERTY_DEFINITIONS(SVGLinearGradientElement, SVGLength, Length, length, X1, x1, SVGNames::x1Attr.localName(), m_x1)
ANIMATED_PROPERTY_DEFINITIONS(SVGLinearGradientElement, SVGLength, Length, length, Y1, y1, SVGNames::y1Attr.localName(), m_y1)
ANIMATED_PROPERTY_DEFINITIONS(SVGLinearGradientElement, SVGLength, Length, length, X2, x2, SVGNames::x2Attr.localName(), m_x2)
ANIMATED_PROPERTY_DEFINITIONS(SVGLinearGradientElement, SVGLength, Length, length, Y2, y2, SVGNames::y2Attr.localName(), m_y2)

void SVGLinearGradientElement::parseMappedAttribute(MappedAttribute* attr)
{
    const AtomicString& value = attr->value();
    if (attr->name() == SVGNames::x1Attr)
        setX1BaseValue(SVGLength(this, LengthModeWidth, value));
    else if (attr->name() == SVGNames::y1Attr)
        setY1BaseValue(SVGLength(this, LengthModeHeight, value));
    else if (attr->name() == SVGNames::x2Attr)
        setX2BaseValue(SVGLength(this, LengthModeWidth, value));
    else if (attr->name() == SVGNames::y2Attr)
        setY2BaseValue(SVGLength(this, LengthModeHeight, value));
    else
        SVGGradientElement::parseMappedAttribute(attr);
}

void SVGLinearGradientElement::buildGradient() const
{
    LinearGradientAttributes attributes = collectGradientProperties();

    // If we didn't find any gradient containing stop elements, ignore the request.
    if (attributes.stops().isEmpty())
        return;

    RefPtr<SVGPaintServerLinearGradient> linearGradient = WTF::static_pointer_cast<SVGPaintServerLinearGradient>(m_resource);

    linearGradient->setGradientStops(attributes.stops());
    linearGradient->setBoundingBoxMode(attributes.boundingBoxMode());
    linearGradient->setGradientSpreadMethod(attributes.spreadMethod());
    linearGradient->setGradientTransform(attributes.gradientTransform());
    linearGradient->setGradientStart(FloatPoint(attributes.x1(), attributes.y1()));
    linearGradient->setGradientEnd(FloatPoint(attributes.x2(), attributes.y2()));
}

LinearGradientAttributes SVGLinearGradientElement::collectGradientProperties() const
{
    LinearGradientAttributes attributes;
    HashSet<const SVGGradientElement*> processedGradients;

    bool isLinear = true;
    const SVGGradientElement* current = this;

    while (current) {
        if (!attributes.hasSpreadMethod() && current->hasAttribute(SVGNames::spreadMethodAttr))
            attributes.setSpreadMethod((SVGGradientSpreadMethod) current->spreadMethod());

        if (!attributes.hasBoundingBoxMode() && current->hasAttribute(SVGNames::gradientUnitsAttr))
            attributes.setBoundingBoxMode(current->getAttribute(SVGNames::gradientUnitsAttr) == "objectBoundingBox");

        if (!attributes.hasGradientTransform() && current->hasAttribute(SVGNames::gradientTransformAttr))
            attributes.setGradientTransform(current->gradientTransform()->consolidate().matrix());

        if (!attributes.hasStops()) {
            const Vector<SVGGradientStop>& stops(current->buildStops());
            if (!stops.isEmpty())
                attributes.setStops(stops);
        }

        if (isLinear) {
            const SVGLinearGradientElement* linear = static_cast<const SVGLinearGradientElement*>(current);

            if (!attributes.hasX1() && current->hasAttribute(SVGNames::x1Attr))
                attributes.setX1(linear->x1().valueAsPercentage());

            if (!attributes.hasY1() && current->hasAttribute(SVGNames::y1Attr))
                attributes.setY1(linear->y1().valueAsPercentage());

            if (!attributes.hasX2() && current->hasAttribute(SVGNames::x2Attr))
                attributes.setX2(linear->x2().valueAsPercentage());

            if (!attributes.hasY2() && current->hasAttribute(SVGNames::y2Attr))
                attributes.setY2(linear->y2().valueAsPercentage());
        }

        processedGradients.add(current);

        // Respect xlink:href, take attributes from referenced element
        Node* refNode = ownerDocument()->getElementById(SVGURIReference::getTarget(current->href()));
        if (refNode && (refNode->hasTagName(SVGNames::linearGradientTag) || refNode->hasTagName(SVGNames::radialGradientTag))) {
            current = static_cast<const SVGGradientElement*>(const_cast<const Node*>(refNode));

            // Cycle detection
            if (processedGradients.contains(current))
                return LinearGradientAttributes();

            isLinear = current->gradientType() == LinearGradientPaintServer;
        } else
            current = 0;
    }

    return attributes;
}

}

#endif // ENABLE(SVG)

// vim:ts=4:noet
