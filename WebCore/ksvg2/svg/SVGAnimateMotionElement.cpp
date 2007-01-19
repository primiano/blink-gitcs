/*
    Copyright (C) 2007 Eric Seidel <eric@webkit.org>

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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include "config.h"
#ifdef SVG_SUPPORT
#include "SVGAnimateMotionElement.h"

#include "SVGMPathElement.h"
#include "SVGPathElement.h"
#include "SVGTransformList.h"

namespace WebCore {

SVGAnimateMotionElement::SVGAnimateMotionElement(const QualifiedName& tagName, Document* doc)
    : SVGAnimationElement(tagName, doc)
    , m_rotateMode(AngleMode)
    , m_angle(0)
{
}

SVGAnimateMotionElement::~SVGAnimateMotionElement()
{
}

bool SVGAnimateMotionElement::hasValidTarget() const
{
    return (SVGAnimationElement::hasValidTarget() && targetElement()->isStyledTransformable());
}

void SVGAnimateMotionElement::parseMappedAttribute(MappedAttribute* attr)
{
    const QualifiedName& name = attr->name();
    const String& value = attr->value();
    if (name == SVGNames::rotateAttr) {
        if (value == "auto")
            m_rotateMode = AutoMode;
        else if (value == "auto-reverse")
            m_rotateMode = AutoReverseMode;
        else {
            m_rotateMode = AngleMode;
            m_angle = value.toDouble();
        }
    } else if (name == SVGNames::keyPointsAttr) {
        m_keyPoints.clear();
        parseKeyNumbers(m_keyPoints, value);
    } else if (name == SVGNames::dAttr) {
        // FIXME: This dummy object is necessary until path parsing is untangled from SVGPathElement, see bug 12122
        RefPtr<SVGPathElement> dummyPath = new SVGPathElement(SVGNames::pathTag, document());
        if (dummyPath->parseSVG(attr->value(), true))
            m_path = dummyPath->toPathData();
        else
            m_path = Path();
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

bool SVGAnimateMotionElement::updateCurrentValue(double timePercentage)
{
    return true;
}

bool SVGAnimateMotionElement::handleStartCondition()
{
    return true;
}

void SVGAnimateMotionElement::applyAnimationToValue(SVGTransformList* targetTransforms)
{
    ExceptionCode ec;
    if (!isAdditive())
        targetTransforms->clear(ec);
}

}

#endif // SVG_SUPPORT

// vim:ts=4:noet
