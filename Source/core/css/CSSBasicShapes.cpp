/*
 * Copyright (C) 2011 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER “AS IS” AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config.h"
#include "core/css/CSSBasicShapes.h"

#include "wtf/text/StringBuilder.h"

using namespace WTF;

namespace WebCore {

static String buildRectangleString(const String& x, const String& y, const String& width, const String& height, const String& radiusX, const String& radiusY, const String& layoutBox)
{
    const char opening[] = "rectangle(";
    const char separator[] = ", ";
    StringBuilder result;
    // Compute the required capacity in advance to reduce allocations.
    result.reserveCapacity((sizeof(opening) - 1) + (5 * (sizeof(separator) - 1)) + 1 + x.length() + y.length() + width.length() + height.length() + radiusX.length() + radiusY.length() + (layoutBox.isEmpty() ? 0 : layoutBox.length() + 1));
    result.appendLiteral(opening);
    result.append(x);
    result.appendLiteral(separator);
    result.append(y);
    result.appendLiteral(separator);
    result.append(width);
    result.appendLiteral(separator);
    result.append(height);
    if (!radiusX.isNull()) {
        result.appendLiteral(separator);
        result.append(radiusX);
        if (!radiusY.isNull()) {
            result.appendLiteral(separator);
            result.append(radiusY);
        }
    }
    if (!layoutBox.isEmpty()) {
        result.append(' ');
        result.append(layoutBox);
    }
    result.append(')');
    return result.toString();
}

String CSSBasicShapeRectangle::cssText() const
{
    return buildRectangleString(m_x->cssText(),
        m_y->cssText(),
        m_width->cssText(),
        m_height->cssText(),
        m_radiusX.get() ? m_radiusX->cssText() : String(),
        m_radiusY.get() ? m_radiusY->cssText() : String(),
        m_layoutBox ? m_layoutBox->cssText() : String());
}

bool CSSBasicShapeRectangle::equals(const CSSBasicShape& shape) const
{
    if (shape.type() != CSSBasicShapeRectangleType)
        return false;

    const CSSBasicShapeRectangle& other = static_cast<const CSSBasicShapeRectangle&>(shape);
    return compareCSSValuePtr(m_x, other.m_x)
        && compareCSSValuePtr(m_y, other.m_y)
        && compareCSSValuePtr(m_width, other.m_width)
        && compareCSSValuePtr(m_height, other.m_height)
        && compareCSSValuePtr(m_radiusX, other.m_radiusX)
        && compareCSSValuePtr(m_radiusY, other.m_radiusY)
        && compareCSSValuePtr(m_layoutBox, other.m_layoutBox);
}

String CSSBasicShapeRectangle::serializeResolvingVariables(const HashMap<AtomicString, String>& variables) const
{
    return buildRectangleString(m_x->serializeResolvingVariables(variables),
        m_y->serializeResolvingVariables(variables),
        m_width->serializeResolvingVariables(variables),
        m_height->serializeResolvingVariables(variables),
        m_radiusX.get() ? m_radiusX->serializeResolvingVariables(variables) : String(),
        m_radiusY.get() ? m_radiusY->serializeResolvingVariables(variables) : String(),
        m_layoutBox ? m_layoutBox->serializeResolvingVariables(variables) : String());
}

bool CSSBasicShapeRectangle::hasVariableReference() const
{
    return m_x->hasVariableReference()
        || m_y->hasVariableReference()
        || m_width->hasVariableReference()
        || m_height->hasVariableReference()
        || (m_radiusX.get() && m_radiusX->hasVariableReference())
        || (m_radiusY.get() && m_radiusY->hasVariableReference())
        || (m_layoutBox && m_layoutBox->hasVariableReference());
}

static String buildCircleString(const String& radius, const String& centerX, const String& centerY, const String& layoutBox)
{
    char at[] = "at";
    char separator[] = " ";
    StringBuilder result;
    result.appendLiteral("circle(");
    if (!radius.isNull())
        result.append(radius);

    if (!centerX.isNull() || !centerY.isNull()) {
        if (!radius.isNull())
            result.appendLiteral(separator);
        result.append(at);
        result.appendLiteral(separator);
        result.append(centerX);
        result.appendLiteral(separator);
        result.append(centerY);
    }
    result.append(")");
    if (layoutBox.length()) {
        result.appendLiteral(separator);
        result.append(layoutBox);
    }
    return result.toString();
}

String CSSBasicShapeCircle::cssText() const
{
    return buildCircleString(m_radius ? m_radius->cssText() : String(),
        m_centerX ? m_centerX->cssText() : String(),
        m_centerY ? m_centerY->cssText() : String(),
        m_layoutBox ? m_layoutBox->cssText() : String());
}

bool CSSBasicShapeCircle::equals(const CSSBasicShape& shape) const
{
    if (shape.type() != CSSBasicShapeCircleType)
        return false;

    const CSSBasicShapeCircle& other = static_cast<const CSSBasicShapeCircle&>(shape);
    return compareCSSValuePtr(m_centerX, other.m_centerX)
        && compareCSSValuePtr(m_centerY, other.m_centerY)
        && compareCSSValuePtr(m_radius, other.m_radius)
        && compareCSSValuePtr(m_layoutBox, other.m_layoutBox);
}

String CSSBasicShapeCircle::serializeResolvingVariables(const HashMap<AtomicString, String>& variables) const
{
    return buildCircleString(m_radius.get() ? m_radius->serializeResolvingVariables(variables) : String(),
        m_centerX.get() ? m_centerX->serializeResolvingVariables(variables) : String(),
        m_centerY.get() ? m_centerY->serializeResolvingVariables(variables) : String(),
        m_layoutBox.get() ? m_layoutBox->serializeResolvingVariables(variables) : String());
}

bool CSSBasicShapeCircle::hasVariableReference() const
{
    return (m_centerX && m_centerX->hasVariableReference())
        || (m_centerY && m_centerY->hasVariableReference())
        || (m_radius && m_radius->hasVariableReference())
        || (m_layoutBox && m_layoutBox->hasVariableReference());
}

static String buildDeprecatedCircleString(const String& x, const String& y, const String& radius)
{
    return "circle(" + x + ", " + y + ", " + radius + ')';
}

String CSSDeprecatedBasicShapeCircle::cssText() const
{
    return buildDeprecatedCircleString(m_centerX->cssText(), m_centerY->cssText(), m_radius->cssText());
}

bool CSSDeprecatedBasicShapeCircle::equals(const CSSBasicShape& shape) const
{
    if (shape.type() != CSSDeprecatedBasicShapeCircleType)
        return false;

    const CSSDeprecatedBasicShapeCircle& other = static_cast<const CSSDeprecatedBasicShapeCircle&>(shape);
    return compareCSSValuePtr(m_centerX, other.m_centerX)
        && compareCSSValuePtr(m_centerY, other.m_centerY)
        && compareCSSValuePtr(m_radius, other.m_radius);
}

String CSSDeprecatedBasicShapeCircle::serializeResolvingVariables(const HashMap<AtomicString, String>& variables) const
{
    return buildDeprecatedCircleString(m_centerX->serializeResolvingVariables(variables),
        m_centerY->serializeResolvingVariables(variables),
        m_radius->serializeResolvingVariables(variables));
}

bool CSSDeprecatedBasicShapeCircle::hasVariableReference() const
{
    return m_centerX->hasVariableReference()
        || m_centerY->hasVariableReference()
        || m_radius->hasVariableReference()
        || (m_layoutBox && m_layoutBox->hasVariableReference());
}

static String buildEllipseString(const String& radiusX, const String& radiusY, const String& centerX, const String& centerY, const String& box)
{
    char at[] = "at";
    char separator[] = " ";
    StringBuilder result;
    result.appendLiteral("ellipse(");
    bool needsSeparator = false;
    if (!radiusX.isNull()) {
        result.append(radiusX);
        needsSeparator = true;
    }
    if (!radiusY.isNull()) {
        if (needsSeparator)
            result.appendLiteral(separator);
        result.append(radiusY);
        needsSeparator = true;
    }

    if (!centerX.isNull() || !centerY.isNull()) {
        if (needsSeparator)
            result.appendLiteral(separator);
        result.appendLiteral(at);
        result.appendLiteral(separator);
        result.append(centerX);
        result.appendLiteral(separator);
        result.append(centerY);
    }
    result.append(")");
    if (box.length()) {
        result.appendLiteral(separator);
        result.append(box);
    }
    return result.toString();
}

String CSSBasicShapeEllipse::cssText() const
{
    return buildEllipseString(m_radiusX ? m_radiusX->cssText() : String(),
        m_radiusY ? m_radiusY->cssText() : String(),
        m_centerX ? m_centerX->cssText() : String(),
        m_centerY ? m_centerY->cssText() : String(),
        m_layoutBox ? m_layoutBox->cssText() : String());
}

bool CSSBasicShapeEllipse::equals(const CSSBasicShape& shape) const
{
    if (shape.type() != CSSBasicShapeEllipseType)
        return false;

    const CSSBasicShapeEllipse& other = static_cast<const CSSBasicShapeEllipse&>(shape);
    return compareCSSValuePtr(m_centerX, other.m_centerX)
        && compareCSSValuePtr(m_centerY, other.m_centerY)
        && compareCSSValuePtr(m_radiusX, other.m_radiusX)
        && compareCSSValuePtr(m_radiusY, other.m_radiusY)
        && compareCSSValuePtr(m_layoutBox, other.m_layoutBox);
}

String CSSBasicShapeEllipse::serializeResolvingVariables(const HashMap<AtomicString, String>& variables) const
{
    return buildEllipseString(m_radiusX.get() ? m_radiusX->serializeResolvingVariables(variables) : String(),
        m_radiusY.get() ? m_radiusY->serializeResolvingVariables(variables) : String(),
        m_centerX.get() ? m_centerX->serializeResolvingVariables(variables) : String(),
        m_centerY.get() ? m_centerY->serializeResolvingVariables(variables) : String(),
        m_layoutBox.get() ? m_layoutBox->serializeResolvingVariables(variables) : String());
}

bool CSSBasicShapeEllipse::hasVariableReference() const
{
    return (m_centerX && m_centerX->hasVariableReference())
        || (m_centerY && m_centerY->hasVariableReference())
        || (m_radiusX && m_radiusX->hasVariableReference())
        || (m_radiusY && m_radiusY->hasVariableReference())
        || (m_layoutBox && m_layoutBox->hasVariableReference());
}

static String buildDeprecatedEllipseString(const String& x, const String& y, const String& radiusX, const String& radiusY)
{
    return "ellipse(" + x + ", " + y + ", " + radiusX + ", " + radiusY + ')';
}

String CSSDeprecatedBasicShapeEllipse::cssText() const
{
    return buildDeprecatedEllipseString(m_centerX->cssText(), m_centerY->cssText(), m_radiusX->cssText(), m_radiusY->cssText());
}

bool CSSDeprecatedBasicShapeEllipse::equals(const CSSBasicShape& shape) const
{
    if (shape.type() != CSSDeprecatedBasicShapeEllipseType)
        return false;

    const CSSDeprecatedBasicShapeEllipse& other = static_cast<const CSSDeprecatedBasicShapeEllipse&>(shape);
    return compareCSSValuePtr(m_centerX, other.m_centerX)
        && compareCSSValuePtr(m_centerY, other.m_centerY)
        && compareCSSValuePtr(m_radiusX, other.m_radiusX)
        && compareCSSValuePtr(m_radiusY, other.m_radiusY);
}

String CSSDeprecatedBasicShapeEllipse::serializeResolvingVariables(const HashMap<AtomicString, String>& variables) const
{
    return buildDeprecatedEllipseString(m_centerX->serializeResolvingVariables(variables),
        m_centerY->serializeResolvingVariables(variables),
        m_radiusX->serializeResolvingVariables(variables),
        m_radiusY->serializeResolvingVariables(variables));
}

bool CSSDeprecatedBasicShapeEllipse::hasVariableReference() const
{
    return m_centerX->hasVariableReference()
        || m_centerY->hasVariableReference()
        || m_radiusX->hasVariableReference()
        || m_radiusY->hasVariableReference()
        || (m_layoutBox && m_layoutBox->hasVariableReference());
}

static String buildPolygonString(const WindRule& windRule, const Vector<String>& points, const String& layoutBox)
{
    ASSERT(!(points.size() % 2));

    StringBuilder result;
    const char evenOddOpening[] = "polygon(evenodd, ";
    const char nonZeroOpening[] = "polygon(nonzero, ";
    const char commaSeparator[] = ", ";
    COMPILE_ASSERT(sizeof(evenOddOpening) == sizeof(nonZeroOpening), polygon_string_openings_have_same_length);

    // Compute the required capacity in advance to reduce allocations.
    size_t length = sizeof(evenOddOpening) - 1;
    for (size_t i = 0; i < points.size(); i += 2) {
        if (i)
            length += (sizeof(commaSeparator) - 1);
        // add length of two strings, plus one for the space separator.
        length += points[i].length() + 1 + points[i + 1].length();
    }
    if (!layoutBox.isEmpty())
        length += layoutBox.length() + 1;
    result.reserveCapacity(length);

    if (windRule == RULE_EVENODD)
        result.appendLiteral(evenOddOpening);
    else
        result.appendLiteral(nonZeroOpening);

    for (size_t i = 0; i < points.size(); i += 2) {
        if (i)
            result.appendLiteral(commaSeparator);
        result.append(points[i]);
        result.append(' ');
        result.append(points[i + 1]);
    }

    result.append(')');

    if (!layoutBox.isEmpty()) {
        result.append(' ');
        result.append(layoutBox);
    }

    return result.toString();
}

String CSSBasicShapePolygon::cssText() const
{
    Vector<String> points;
    points.reserveInitialCapacity(m_values.size());

    for (size_t i = 0; i < m_values.size(); ++i)
        points.append(m_values.at(i)->cssText());

    return buildPolygonString(m_windRule, points, m_layoutBox ? m_layoutBox->cssText() : String());
}

bool CSSBasicShapePolygon::equals(const CSSBasicShape& shape) const
{
    if (shape.type() != CSSBasicShapePolygonType)
        return false;

    const CSSBasicShapePolygon& rhs = static_cast<const CSSBasicShapePolygon&>(shape);

    if (!compareCSSValuePtr(m_layoutBox, rhs.m_layoutBox))
        return false;

    return compareCSSValueVector<CSSPrimitiveValue>(m_values, rhs.m_values);
}

String CSSBasicShapePolygon::serializeResolvingVariables(const HashMap<AtomicString, String>& variables) const
{
    Vector<String> points;
    points.reserveInitialCapacity(m_values.size());

    for (size_t i = 0; i < m_values.size(); ++i)
        points.append(m_values.at(i)->serializeResolvingVariables(variables));

    return buildPolygonString(m_windRule, points, m_layoutBox ? m_layoutBox->serializeResolvingVariables(variables) : String());
}

bool CSSBasicShapePolygon::hasVariableReference() const
{
    if (m_layoutBox && m_layoutBox->hasVariableReference())
        return true;

    for (size_t i = 0; i < m_values.size(); ++i) {
        if (m_values.at(i)->hasVariableReference())
            return true;
    }
    return false;
}

static String buildInsetRectangleString(const String& top, const String& right, const String& bottom, const String& left, const String& radiusX, const String& radiusY, const String& layoutBox)
{
    const char opening[] = "inset-rectangle(";
    const char separator[] = ", ";
    StringBuilder result;
    // Compute the required capacity in advance to reduce allocations.
    result.reserveCapacity((sizeof(opening) - 1) + (5 * (sizeof(separator) - 1)) + 1 + top.length() + right.length() + bottom.length() + left.length() + radiusX.length() + radiusY.length() + (layoutBox.isEmpty() ? 0 : layoutBox.length() + 1));
    result.appendLiteral(opening);
    result.append(top);
    result.appendLiteral(separator);
    result.append(right);
    result.appendLiteral(separator);
    result.append(bottom);
    result.appendLiteral(separator);
    result.append(left);
    if (!radiusX.isNull()) {
        result.appendLiteral(separator);
        result.append(radiusX);
        if (!radiusY.isNull()) {
            result.appendLiteral(separator);
            result.append(radiusY);
        }
    }
    result.append(')');

    if (!layoutBox.isEmpty()) {
        result.append(' ');
        result.append(layoutBox);
    }

    return result.toString();
}

String CSSBasicShapeInsetRectangle::cssText() const
{
    return buildInsetRectangleString(m_top->cssText(),
        m_right->cssText(),
        m_bottom->cssText(),
        m_left->cssText(),
        m_radiusX.get() ? m_radiusX->cssText() : String(),
        m_radiusY.get() ? m_radiusY->cssText() : String(),
        m_layoutBox ? m_layoutBox->cssText() : String());
}

bool CSSBasicShapeInsetRectangle::equals(const CSSBasicShape& shape) const
{
    if (shape.type() != CSSBasicShapeInsetRectangleType)
        return false;

    const CSSBasicShapeInsetRectangle& other = static_cast<const CSSBasicShapeInsetRectangle&>(shape);
    return compareCSSValuePtr(m_top, other.m_top)
        && compareCSSValuePtr(m_right, other.m_right)
        && compareCSSValuePtr(m_bottom, other.m_bottom)
        && compareCSSValuePtr(m_left, other.m_left)
        && compareCSSValuePtr(m_radiusX, other.m_radiusX)
        && compareCSSValuePtr(m_radiusY, other.m_radiusY)
        && compareCSSValuePtr(m_layoutBox, other.m_layoutBox);
}

String CSSBasicShapeInsetRectangle::serializeResolvingVariables(const HashMap<AtomicString, String>& variables) const
{
    return buildInsetRectangleString(m_top->serializeResolvingVariables(variables),
        m_right->serializeResolvingVariables(variables),
        m_bottom->serializeResolvingVariables(variables),
        m_left->serializeResolvingVariables(variables),
        m_radiusX.get() ? m_radiusX->serializeResolvingVariables(variables) : String(),
        m_radiusY.get() ? m_radiusY->serializeResolvingVariables(variables) : String(),
        m_layoutBox ? m_layoutBox->serializeResolvingVariables(variables) : String());
}

bool CSSBasicShapeInsetRectangle::hasVariableReference() const
{
    return m_top->hasVariableReference()
        || m_right->hasVariableReference()
        || m_bottom->hasVariableReference()
        || m_left->hasVariableReference()
        || (m_radiusX.get() && m_radiusX->hasVariableReference())
        || (m_radiusY.get() && m_radiusY->hasVariableReference())
        || (m_layoutBox && m_layoutBox->hasVariableReference());
}

} // namespace WebCore

