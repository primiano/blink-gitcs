// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"
#include "core/animation/Interpolation.h"

#include "core/css/CSSCalculationValue.h"
#include "core/css/resolver/AnimatedStyleBuilder.h"
#include "core/css/resolver/StyleBuilder.h"
#include "core/css/resolver/StyleResolverState.h"

namespace WebCore {

DEFINE_EMPTY_DESTRUCTOR_WILL_BE_REMOVED(Interpolation);

namespace {

bool typesMatch(const InterpolableValue* start, const InterpolableValue* end)
{
    if (start->isNumber())
        return end->isNumber();
    if (start->isBool())
        return end->isBool();
    if (start->isAnimatableValue())
        return end->isAnimatableValue();
    if (!(start->isList() && end->isList()))
        return false;
    const InterpolableList* startList = toInterpolableList(start);
    const InterpolableList* endList = toInterpolableList(end);
    if (startList->length() != endList->length())
        return false;
    for (size_t i = 0; i < startList->length(); ++i) {
        if (!typesMatch(startList->get(i), endList->get(i)))
            return false;
    }
    return true;
}

}

Interpolation::Interpolation(PassOwnPtrWillBeRawPtr<InterpolableValue> start, PassOwnPtrWillBeRawPtr<InterpolableValue> end)
    : m_start(start)
    , m_end(end)
    , m_cachedFraction(0)
    , m_cachedIteration(0)
    , m_cachedValue(m_start->clone())
{
    RELEASE_ASSERT(typesMatch(m_start.get(), m_end.get()));
}

void Interpolation::interpolate(int iteration, double fraction) const
{
    if (m_cachedFraction != fraction || m_cachedIteration != iteration) {
        m_cachedValue = m_start->interpolate(*m_end, fraction);
        m_cachedIteration = iteration;
        m_cachedFraction = fraction;
    }
}

void Interpolation::trace(Visitor* visitor)
{
    visitor->trace(m_start);
    visitor->trace(m_end);
    visitor->trace(m_cachedValue);
}

void StyleInterpolation::trace(Visitor* visitor)
{
    Interpolation::trace(visitor);
}

void LegacyStyleInterpolation::apply(StyleResolverState& state) const
{
    AnimatedStyleBuilder::applyProperty(m_id, state, currentValue().get());
}

void LegacyStyleInterpolation::trace(Visitor* visitor)
{
    StyleInterpolation::trace(visitor);
}

bool LengthStyleInterpolation::canCreateFrom(const CSSValue& value)
{
    if (value.isPrimitiveValue()) {
        const CSSPrimitiveValue& primitiveValue = WebCore::toCSSPrimitiveValue(value);
        if (primitiveValue.cssCalcValue())
            return true;

        CSSPrimitiveValue::LengthUnitType type;
        // Only returns true if the type is a primitive length unit.
        return CSSPrimitiveValue::unitTypeToLengthUnitType(primitiveValue.primitiveType(), type);
    }
    return value.isCalcValue();
}

PassOwnPtrWillBeRawPtr<InterpolableValue> LengthStyleInterpolation::lengthToInterpolableValue(CSSValue* value)
{
    OwnPtrWillBeRawPtr<InterpolableList> result = InterpolableList::create(CSSPrimitiveValue::LengthUnitTypeCount);
    CSSPrimitiveValue* primitive = toCSSPrimitiveValue(value);

    CSSLengthArray array;
    for (size_t i = 0; i < CSSPrimitiveValue::LengthUnitTypeCount; i++)
        array.append(0);
    primitive->accumulateLengthArray(array);

    for (size_t i = 0; i < CSSPrimitiveValue::LengthUnitTypeCount; i++)
        result->set(i, InterpolableNumber::create(array.at(i)));

    return result.release();
}

namespace {

static CSSPrimitiveValue::UnitTypes toUnitType(int lengthUnitType)
{
    return static_cast<CSSPrimitiveValue::UnitTypes>(CSSPrimitiveValue::lengthUnitTypeToUnitType(static_cast<CSSPrimitiveValue::LengthUnitType>(lengthUnitType)));
}

static PassRefPtrWillBeRawPtr<CSSCalcExpressionNode> constructCalcExpression(PassRefPtrWillBeRawPtr<CSSCalcExpressionNode> previous, InterpolableList* list, size_t position)
{
    while (position != CSSPrimitiveValue::LengthUnitTypeCount) {
        const InterpolableNumber *subValue = toInterpolableNumber(list->get(position));
        if (subValue->value()) {
            RefPtrWillBeRawPtr<CSSCalcExpressionNode> next;
            if (previous)
                next = CSSCalcValue::createExpressionNode(previous, CSSCalcValue::createExpressionNode(CSSPrimitiveValue::create(subValue->value(), toUnitType(position))), CalcAdd);
            else
                next = CSSCalcValue::createExpressionNode(CSSPrimitiveValue::create(subValue->value(), toUnitType(position)));
            return constructCalcExpression(next, list, position + 1);
        }
        position++;
    }
    return previous;
}

}

PassRefPtrWillBeRawPtr<CSSValue> LengthStyleInterpolation::interpolableValueToLength(InterpolableValue* value)
{
    InterpolableList* listValue = toInterpolableList(value);
    unsigned unitCount = 0;
    for (size_t i = 0; i < CSSPrimitiveValue::LengthUnitTypeCount; i++) {
        const InterpolableNumber* subValue = toInterpolableNumber(listValue->get(i));
        if (subValue->value()) {
            unitCount++;
        }
    }

    switch (unitCount) {
    case 0:
        return CSSPrimitiveValue::create(0, CSSPrimitiveValue::CSS_PX);
    case 1:
        for (size_t i = 0; i < CSSPrimitiveValue::LengthUnitTypeCount; i++) {
            const InterpolableNumber* subValue = toInterpolableNumber(listValue->get(i));
            if (subValue->value()) {
                return CSSPrimitiveValue::create(subValue->value(), toUnitType(i));
            }
        }
    default:
        return CSSPrimitiveValue::create(CSSCalcValue::create(constructCalcExpression(nullptr, listValue, 0)));
    }
}

void LengthStyleInterpolation::apply(StyleResolverState& state) const
{
    StyleBuilder::applyProperty(m_id, state, interpolableValueToLength(m_cachedValue.get()).get());
}

void LengthStyleInterpolation::trace(Visitor* visitor)
{
    StyleInterpolation::trace(visitor);
}

void DefaultStyleInterpolation::apply(StyleResolverState& state) const
{
    StyleBuilder::applyProperty(m_id, state, toInterpolableBool(m_cachedValue.get())->value() ? m_endCSSValue.get() : m_startCSSValue.get());
}

void DefaultStyleInterpolation::trace(Visitor* visitor)
{
    StyleInterpolation::trace(visitor);
    visitor->trace(m_startCSSValue);
    visitor->trace(m_endCSSValue);
}

}
