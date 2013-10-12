/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef AnimatableLength_h
#define AnimatableLength_h

#include "core/animation/AnimatableValue.h"
#include "core/css/CSSCalculationValue.h"
#include "core/css/CSSPrimitiveValue.h"
#include "core/platform/Length.h"

namespace WebCore {

enum NumberRange {
    AllValues,
    NonNegativeValues,
};

// Handles animation of CSS length and percentage values including CSS calc.
// See primitiveUnitToNumberType() for the list of supported units (with the exception of calc).
// If created from a CSSPrimitiveValue this class will cache it to be returned in toCSSValue().
class AnimatableLength : public AnimatableValue {
public:
    enum NumberUnitType {
        UnitTypePixels,
        UnitTypePercentage,
        UnitTypeFontSize,
        UnitTypeFontXSize,
        UnitTypeRootFontSize,
        UnitTypeViewportWidth,
        UnitTypeViewportHeight,
        UnitTypeViewportMin,
        UnitTypeViewportMax,
        UnitTypeInvalid,
    };

    virtual ~AnimatableLength() { }
    static bool canCreateFrom(const CSSValue*);
    static PassRefPtr<AnimatableLength> create(CSSValue*);
    static PassRefPtr<AnimatableLength> create(double number, NumberUnitType unitType, CSSPrimitiveValue* cssPrimitiveValue = 0)
    {
        return adoptRef(new AnimatableLength(number, unitType, cssPrimitiveValue));
    }
    static PassRefPtr<AnimatableLength> create(PassRefPtr<CSSCalcExpressionNode> calcExpression, CSSPrimitiveValue* cssPrimitiveValue = 0)
    {
        return adoptRef(new AnimatableLength(calcExpression, cssPrimitiveValue));
    }
    PassRefPtr<CSSValue> toCSSValue(NumberRange = AllValues) const;
    Length toLength(const RenderStyle* currStyle, const RenderStyle* rootStyle, double zoom, NumberRange = AllValues) const;

protected:
    virtual PassRefPtr<AnimatableValue> interpolateTo(const AnimatableValue*, double fraction) const OVERRIDE;
    virtual PassRefPtr<AnimatableValue> addWith(const AnimatableValue*) const OVERRIDE;

private:
    AnimatableLength(double number, NumberUnitType unitType, CSSPrimitiveValue* cssPrimitiveValue)
        : m_isCalc(false)
        , m_number(number)
        , m_unitType(unitType)
        , m_cachedCSSPrimitiveValue(cssPrimitiveValue)
    {
        ASSERT(m_unitType != UnitTypeInvalid);
    }
    AnimatableLength(PassRefPtr<CSSCalcExpressionNode> calcExpression, CSSPrimitiveValue* cssPrimitiveValue)
        : m_isCalc(true)
        , m_calcExpression(calcExpression)
        , m_cachedCSSPrimitiveValue(cssPrimitiveValue)
    {
        ASSERT(m_calcExpression);
    }
    virtual AnimatableType type() const OVERRIDE { return TypeLength; }
    virtual bool equalTo(const AnimatableValue*) const OVERRIDE;

    static PassRefPtr<AnimatableLength> create(const AnimatableLength* leftAddend, const AnimatableLength* rightAddend)
    {
        ASSERT(leftAddend && rightAddend);
        return create(CSSCalcValue::createExpressionNode(leftAddend->toCSSCalcExpressionNode(), rightAddend->toCSSCalcExpressionNode(), CalcAdd));
    }

    PassRefPtr<CSSPrimitiveValue> toCSSPrimitiveValue(NumberRange) const;
    PassRefPtr<CSSCalcExpressionNode> toCSSCalcExpressionNode() const;

    PassRefPtr<AnimatableLength> scale(double) const;
    double clampedNumber(NumberRange range) const
    {
        ASSERT(!m_isCalc);
        return (range == NonNegativeValues && m_number <= 0) ? 0 : m_number;
    }
    static NumberUnitType primitiveUnitToNumberType(unsigned short primitiveUnit);
    static unsigned short numberTypeToPrimitiveUnit(NumberUnitType numberType);
    bool hasSameUnitType(const AnimatableLength& length) const
    {
        return !m_isCalc && !length.m_isCalc && m_unitType == length.m_unitType;
    }

    bool m_isCalc;

    double m_number;
    NumberUnitType m_unitType;

    RefPtr<CSSCalcExpressionNode> m_calcExpression;

    mutable RefPtr<CSSPrimitiveValue> m_cachedCSSPrimitiveValue;
};

inline const AnimatableLength* toAnimatableLength(const AnimatableValue* value)
{
    ASSERT_WITH_SECURITY_IMPLICATION(value && value->isLength());
    return static_cast<const AnimatableLength*>(value);
}

} // namespace WebCore

#endif // AnimatableLength_h
