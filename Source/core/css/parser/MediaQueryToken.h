// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MediaQueryToken_h
#define MediaQueryToken_h

#include "core/css/CSSPrimitiveValue.h"
#include "wtf/text/WTFString.h"

namespace WebCore {

enum MediaQueryTokenType {
    IdentToken = 0,
    FunctionToken = 1,
    DelimiterToken = 2,
    NumberToken = 3,
    PercentageToken = 4,
    DimensionToken = 5,
    WhitespaceToken = 6,
    ColonToken = 7,
    SemicolonToken = 8,
    CommaToken = 9,
    LeftParenthesisToken = 10,
    RightParenthesisToken = 11,
    EOFToken = 12,
};

enum NumericValueType {
    IntegerValueType,
    NumberValueType,
};

class MediaQueryToken {
public:
    MediaQueryToken(MediaQueryTokenType);
    MediaQueryToken(MediaQueryTokenType, String);

    MediaQueryToken(MediaQueryTokenType, UChar); // for DelimiterToken
    MediaQueryToken(MediaQueryTokenType, double, NumericValueType); // for NumberToken

    // Converts NumberToken to DimensionToken.
    void convertToDimensionWithUnit(String);

    // Converts NumberToken to PercentageToken.
    void convertToPercentage();

    MediaQueryTokenType type() const { return m_type; }
    String value() const { return m_value; }

    UChar delimiter() const { return m_delimiter; }
    NumericValueType numericValueType() const { return m_numericValueType; }
    double numericValue() const { return m_numericValue; }
    CSSPrimitiveValue::UnitTypes unitType() const { return m_unit; }

private:
    MediaQueryTokenType m_type;
    String m_value;

    UChar m_delimiter; // Could be rolled into m_value?

    NumericValueType m_numericValueType;
    double m_numericValue;
    CSSPrimitiveValue::UnitTypes m_unit;
};

}

#endif // MediaQueryToken_h
