// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"
#include "core/css/parser/CSSParser.h"

#include "core/css/CSSKeyframeRule.h"
#include "core/css/StyleColor.h"
#include "core/css/StyleRule.h"

namespace blink {

CSSParser::CSSParser(const CSSParserContext& context)
    : m_bisonParser(context)
{
}

bool CSSParser::parseDeclaration(MutableStylePropertySet* propertySet, const String& declaration, CSSParserObserver* observer, StyleSheetContents* styleSheet)
{
    return m_bisonParser.parseDeclaration(propertySet, declaration, observer, styleSheet);
}

void CSSParser::parseSelector(const String& selector, CSSSelectorList& selectorList)
{
    m_bisonParser.parseSelector(selector, selectorList);
}

PassRefPtrWillBeRawPtr<StyleRuleBase> CSSParser::parseRule(const CSSParserContext& context, StyleSheetContents* styleSheet, const String& rule)
{
    return BisonCSSParser(context).parseRule(styleSheet, rule);
}

void CSSParser::parseSheet(const CSSParserContext& context, StyleSheetContents* styleSheet, const String& text, const TextPosition& startPosition, CSSParserObserver* observer, bool logErrors)
{
    BisonCSSParser(context).parseSheet(styleSheet, text, startPosition, observer, logErrors);
}

bool CSSParser::parseValue(MutableStylePropertySet* declaration, CSSPropertyID propertyID, const String& string, bool important, CSSParserMode parserMode, StyleSheetContents* styleSheet)
{
    return BisonCSSParser::parseValue(declaration, propertyID, string, important, parserMode, styleSheet);
}

bool CSSParser::parseValue(MutableStylePropertySet* declaration, CSSPropertyID propertyID, const String& string, bool important, const Document& document)
{
    return BisonCSSParser::parseValue(declaration, propertyID, string, important, document);
}

PassRefPtrWillBeRawPtr<ImmutableStylePropertySet> CSSParser::parseInlineStyleDeclaration(const String& styleString, Element* element)
{
    return BisonCSSParser::parseInlineStyleDeclaration(styleString, element);
}

PassOwnPtr<Vector<double> > CSSParser::parseKeyframeKeyList(const String& keyList)
{
    return BisonCSSParser(strictCSSParserContext()).parseKeyframeKeyList(keyList);
}

PassRefPtrWillBeRawPtr<StyleKeyframe> CSSParser::parseKeyframeRule(const CSSParserContext& context, StyleSheetContents* styleSheet, const String& rule)
{
    return BisonCSSParser(context).parseKeyframeRule(styleSheet, rule);
}

bool CSSParser::parseSupportsCondition(const String& condition)
{
    return BisonCSSParser(CSSParserContext(HTMLStandardMode, 0)).parseSupportsCondition(condition);
}

PassRefPtrWillBeRawPtr<CSSValueList> CSSParser::parseFontFaceValue(const AtomicString& fontFace)
{
    return BisonCSSParser::parseFontFaceValue(fontFace);
}

PassRefPtrWillBeRawPtr<CSSValue> CSSParser::parseAnimationTimingFunctionValue(const String& timingFunction)
{
    return BisonCSSParser::parseAnimationTimingFunctionValue(timingFunction);
}

bool CSSParser::parseColor(RGBA32& color, const String& string, bool strict)
{
    return BisonCSSParser::parseColor(color, string, strict);
}

StyleColor CSSParser::colorFromRGBColorString(const String& string)
{
    return BisonCSSParser::colorFromRGBColorString(string);
}

bool CSSParser::parseSystemColor(RGBA32& color, const String& colorString)
{
    return BisonCSSParser::parseSystemColor(color, colorString);
}

} // namespace blink
