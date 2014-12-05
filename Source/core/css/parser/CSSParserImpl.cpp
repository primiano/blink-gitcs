// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"
#include "core/css/parser/CSSParserImpl.h"

#include "core/css/CSSStyleSheet.h"
#include "core/css/StylePropertySet.h"
#include "core/css/StyleSheetContents.h"
#include "core/css/parser/CSSParserValues.h"
#include "core/css/parser/CSSPropertyParser.h"
#include "core/css/parser/CSSTokenizer.h"
#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/frame/UseCounter.h"
#include "wtf/BitArray.h"

namespace blink {

CSSParserImpl::CSSParserImpl(const CSSParserContext& context, const String& s)
: m_context(context)
{
    CSSTokenizer::tokenize(s, m_tokens);
}

bool CSSParserImpl::parseValue(MutableStylePropertySet* declaration, CSSPropertyID propertyID, const String& string, bool important, const CSSParserContext& context)
{
    CSSParserImpl parser(context, string);
    CSSRuleSourceData::Type ruleType = CSSRuleSourceData::STYLE_RULE;
    if (declaration->cssParserMode() == CSSViewportRuleMode)
        ruleType = CSSRuleSourceData::VIEWPORT_RULE;
    parser.consumeDeclarationValue(CSSParserTokenRange(parser.m_tokens), propertyID, important, ruleType);
    if (parser.m_parsedProperties.isEmpty())
        return false;
    declaration->addParsedProperties(parser.m_parsedProperties);
    return true;
}

static inline void filterProperties(bool important, const WillBeHeapVector<CSSProperty, 256>& input, WillBeHeapVector<CSSProperty, 256>& output, size_t& unusedEntries, BitArray<numCSSProperties>& seenProperties)
{
    // Add properties in reverse order so that highest priority definitions are reached first. Duplicate definitions can then be ignored when found.
    for (size_t i = input.size(); i--; ) {
        const CSSProperty& property = input[i];
        if (property.isImportant() != important)
            continue;
        const unsigned propertyIDIndex = property.id() - firstCSSProperty;
        if (seenProperties.get(propertyIDIndex))
            continue;
        seenProperties.set(propertyIDIndex);
        output[--unusedEntries] = property;
    }
}

static PassRefPtrWillBeRawPtr<ImmutableStylePropertySet> createStylePropertySet(const WillBeHeapVector<CSSProperty, 256>& parsedProperties, CSSParserMode mode)
{
    BitArray<numCSSProperties> seenProperties;
    size_t unusedEntries = parsedProperties.size();
    WillBeHeapVector<CSSProperty, 256> results(unusedEntries);

    filterProperties(true, parsedProperties, results, unusedEntries, seenProperties);
    filterProperties(false, parsedProperties, results, unusedEntries, seenProperties);

    return ImmutableStylePropertySet::create(results.data() + unusedEntries, results.size() - unusedEntries, mode);
}

PassRefPtrWillBeRawPtr<ImmutableStylePropertySet> CSSParserImpl::parseInlineStyleDeclaration(const String& string, Element* element)
{
    Document& document = element->document();
    CSSParserContext context = CSSParserContext(document.elementSheet().contents()->parserContext(), UseCounter::getFrom(&document));
    CSSParserMode mode = element->isHTMLElement() && !document.inQuirksMode() ? HTMLStandardMode : HTMLQuirksMode;
    context.setMode(mode);
    CSSParserImpl parser(context, string);
    parser.consumeDeclarationList(CSSParserTokenRange(parser.m_tokens), CSSRuleSourceData::STYLE_RULE);
    return createStylePropertySet(parser.m_parsedProperties, mode);
}

bool CSSParserImpl::parseDeclaration(MutableStylePropertySet* declaration, const String& string, const CSSParserContext& context)
{
    CSSParserImpl parser(context, string);
    CSSRuleSourceData::Type ruleType = CSSRuleSourceData::STYLE_RULE;
    if (declaration->cssParserMode() == CSSViewportRuleMode)
        ruleType = CSSRuleSourceData::VIEWPORT_RULE;
    parser.consumeDeclarationList(CSSParserTokenRange(parser.m_tokens), ruleType);
    if (parser.m_parsedProperties.isEmpty())
        return false;
    declaration->addParsedProperties(parser.m_parsedProperties);
    return true;
}

void CSSParserImpl::consumeDeclarationList(CSSParserTokenRange range, CSSRuleSourceData::Type ruleType)
{
    while (!range.atEnd()) {
        switch (range.peek().type()) {
        case CommentToken:
        case WhitespaceToken:
        case SemicolonToken:
            range.consume();
            break;
        case IdentToken: {
            const CSSParserToken* declarationStart = &range.peek();
            while (!range.atEnd() && range.peek().type() != SemicolonToken)
                range.consume();
            consumeDeclaration(range.makeSubRange(declarationStart, &range.peek()), ruleType);
            break;
        }
        default: // Parser error
            // FIXME: The spec allows at-rules in a declaration list
            while (!range.atEnd() && range.peek().type() != SemicolonToken)
                range.consume();
            break;
        }
    }
}

void CSSParserImpl::consumeDeclaration(CSSParserTokenRange range, CSSRuleSourceData::Type ruleType)
{
    ASSERT(range.peek().type() == IdentToken);
    CSSPropertyID id = range.consume().parseAsCSSPropertyID();
    range.consumeWhitespaceAndComments();
    if (range.consume().type() != ColonToken)
        return; // Parser error

    const CSSParserToken* last = range.end() - 1;
    while (last->type() == WhitespaceToken || last->type() == CommentToken)
        --last;
    if (last->type() == IdentToken && equalIgnoringCase(last->value(), "important")) {
        --last;
        while (last->type() == WhitespaceToken || last->type() == CommentToken)
            --last;
        if (last->type() == DelimiterToken && last->delimiter() == '!') {
            consumeDeclarationValue(range.makeSubRange(&range.peek(), last), id, true, ruleType);
            return;
        }
    }
    consumeDeclarationValue(range.makeSubRange(&range.peek(), range.end()), id, false, ruleType);
}

void CSSParserImpl::consumeDeclarationValue(CSSParserTokenRange range, CSSPropertyID propertyID, bool important, CSSRuleSourceData::Type ruleType)
{
    CSSParserValueList valueList(range);
    if (!valueList.size())
        return; // Parser error
    bool inViewport = ruleType == CSSRuleSourceData::VIEWPORT_RULE;
    CSSPropertyParser::parseValue(propertyID, important, &valueList, m_context, inViewport, m_parsedProperties, ruleType);
}

} // namespace blink
