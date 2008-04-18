/*
    Copyright (C) 2007 Eric Seidel <eric@webkit.org>
              (C) 2007 Rob Buis <buis@kde.org>
    Copyright (C) 2008 Apple Inc. All Rights Reserved.

    This file is part of the WebKit project

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
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#if ENABLE(SVG) && ENABLE(SVG_ANIMATION)
#include "SVGAnimateMotionElement.h"

#include "RenderObject.h"
#include "SVGMPathElement.h"
#include "SVGParserUtilities.h"
#include "SVGPathElement.h"
#include "SVGTransformList.h"

namespace WebCore {
    
using namespace SVGNames;

SVGAnimateMotionElement::SVGAnimateMotionElement(const QualifiedName& tagName, Document* doc)
    : SVGAnimationElement(tagName, doc)
    , m_baseIndexInTransformList(0)
    , m_rotateMode(AngleMode)
    , m_angle(0)
{
}

SVGAnimateMotionElement::~SVGAnimateMotionElement()
{
}

bool SVGAnimateMotionElement::hasValidTarget() const
{
    if (!SVGAnimationElement::hasValidTarget())
        return false;
    if (!targetElement()->isStyledTransformable())
        return false;
    // Spec: SVG 1.1 section 19.2.15
    if (targetElement()->hasTagName(gTag)
        || targetElement()->hasTagName(defsTag)
        || targetElement()->hasTagName(useTag)
        || targetElement()->hasTagName(imageTag)
        || targetElement()->hasTagName(switchTag)
        || targetElement()->hasTagName(pathTag)
        || targetElement()->hasTagName(rectTag)
        || targetElement()->hasTagName(circleTag)
        || targetElement()->hasTagName(ellipseTag)
        || targetElement()->hasTagName(lineTag)
        || targetElement()->hasTagName(polylineTag)
        || targetElement()->hasTagName(polygonTag)
        || targetElement()->hasTagName(textTag)
        || targetElement()->hasTagName(clipPathTag)
        || targetElement()->hasTagName(maskTag)
        || targetElement()->hasTagName(aTag)
#if ENABLE(SVG_FOREIGN_OBJECT)
        || targetElement()->hasTagName(foreignObjectTag)
#endif
        )
        return true;
    return false;
}

void SVGAnimateMotionElement::parseMappedAttribute(MappedAttribute* attr)
{
    if (attr->name() == SVGNames::rotateAttr) {
        if (attr->value() == "auto")
            m_rotateMode = AutoMode;
        else if (attr->value() == "auto-reverse")
            m_rotateMode = AutoReverseMode;
        else {
            m_rotateMode = AngleMode;
            m_angle = attr->value().toFloat();
        }
    } else if (attr->name() == SVGNames::keyPointsAttr) {
        // FIXME: Implement key points.
    } else if (attr->name() == SVGNames::dAttr) {
        m_path = Path();
        pathFromSVGData(m_path, attr->value());
    } else
        SVGAnimationElement::parseMappedAttribute(attr);
}

Path SVGAnimateMotionElement::animationPath()
{
    for (Node* child = firstChild(); child; child->nextSibling()) {
        if (child->hasTagName(SVGNames::mpathTag)) {
            SVGMPathElement* mPath = static_cast<SVGMPathElement*>(child);
            SVGPathElement* pathElement = mPath->pathElement();
            if (pathElement)
                return pathElement->toPathData();
            // The spec would probably have us throw up an error here, but instead we try to fall back to the d value
        }
    }
    if (hasAttribute(SVGNames::dAttr))
        return m_path;
    return Path();
}

static bool parsePoint(const String& s, FloatPoint& point)
{
    if (s.isEmpty())
        return false;
    const UChar* cur = s.characters();
    const UChar* end = cur + s.length();
    
    if (!skipOptionalSpaces(cur, end))
        return false;
    
    float x = 0.0f;
    if (!parseNumber(cur, end, x))
        return false;
    
    float y = 0.0f;
    if (!parseNumber(cur, end, y))
        return false;
    
    point = FloatPoint(x, y);
    
    // disallow anything except spaces at the end
    return !skipOptionalSpaces(cur, end);
}
    
void SVGAnimateMotionElement::resetToBaseValue(const String&)
{
    if (!hasValidTarget())
        return;
    SVGStyledTransformableElement* transformableElement = static_cast<SVGStyledTransformableElement*>(targetElement());
    // FIXME: This should modify supplemental transform, not the transform attribute!
    ExceptionCode ec;
    transformableElement->transform()->clear(ec);
}

bool SVGAnimateMotionElement::calculateFromAndToValues(const String& fromString, const String& toString)
{
    parsePoint(fromString, m_fromPoint);
    parsePoint(toString, m_toPoint);
    return true;
}
    
bool SVGAnimateMotionElement::calculateFromAndByValues(const String& fromString, const String& byString)
{
    parsePoint(fromString, m_fromPoint);
    FloatPoint byPoint;
    parsePoint(byString, byPoint);
    m_toPoint = FloatPoint(m_fromPoint.x() + byPoint.x(), m_fromPoint.y() + byPoint.y());
    return true;
}

void SVGAnimateMotionElement::calculateAnimatedValue(float percentage, unsigned repeat, SVGSMILElement* resultElement)
{
    if (!resultElement->targetElement()->isStyledTransformable())
        return;
    
    // FIXME: This should modify supplemental transform, not the transform attribute!
    SVGStyledTransformableElement* transformableElement = static_cast<SVGStyledTransformableElement*>(resultElement->targetElement());
    RefPtr<SVGTransformList> transformList = transformableElement->transform();
    if (!transformList)
        return;
    
    ExceptionCode ec;
    if (!isAdditive()) {
        ASSERT(this == resultElement);
        transformList->clear(ec);
    }
    
    FloatSize diff = m_toPoint - m_fromPoint;
    AffineTransform transform;
    // FIXME: Animate angles
    transform.translate(diff.width() * percentage + m_fromPoint.x(), diff.height() * percentage + m_fromPoint.y());
    
    // FIXME: Accumulate.

    if (!transform.isIdentity())
        transformList->appendItem(SVGTransform(transform), ec);

    if (transformableElement->renderer())
        transformableElement->renderer()->setNeedsLayout(true); // should be part of setTransform
}
    
void SVGAnimateMotionElement::applyResultsToTarget()
{
    
}

}

#endif // ENABLE(SVG)

// vim:ts=4:noet
