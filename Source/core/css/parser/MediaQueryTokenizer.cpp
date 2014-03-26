// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"
#include "core/css/parser/MediaQueryTokenizer.h"

namespace WebCore {
#include "MediaQueryTokenizerCodepoints.cpp"
}

#include "core/css/parser/MediaQueryInputStream.h"
#include "core/html/parser/HTMLParserIdioms.h"
#include "wtf/unicode/CharacterNames.h"

namespace WebCore {

// http://dev.w3.org/csswg/css-syntax/#name-start-code-point
static bool isNameStart(UChar c)
{
    if (isASCIIAlpha(c))
        return true;
    if (c == '_')
        return true;
    return !isASCII(c);
}

// http://www.w3.org/TR/css-syntax-3/#name-code-point
static bool isNameChar(UChar c)
{
    return isNameStart(c) || isASCIIDigit(c) || c == '-';
}

// http://www.w3.org/TR/css-syntax-3/#check-if-two-code-points-are-a-valid-escape
static bool twoCharsAreValidEscape(UChar first, UChar second)
{
    return ((first == '\\') && (second != '\n') && (second != kEndOfFileMarker));
}

MediaQueryTokenizer::MediaQueryTokenizer(MediaQueryInputStream& inputStream)
    : m_input(inputStream)
{
}

void MediaQueryTokenizer::reconsume(UChar c)
{
    m_input.pushBack(c);
}

UChar MediaQueryTokenizer::consume()
{
    UChar current = m_input.currentInputChar();
    m_input.advance();
    return current;
}

void MediaQueryTokenizer::consume(unsigned offset)
{
    m_input.advance(offset);
}

MediaQueryToken MediaQueryTokenizer::whiteSpace(UChar cc)
{
    // CSS Tokenization is currently lossy, but we could record
    // the exact whitespace instead of discarding it here.
    consumeUntilNonWhitespace();
    return MediaQueryToken(WhitespaceToken);
}

MediaQueryToken MediaQueryTokenizer::leftParenthesis(UChar cc)
{
    return MediaQueryToken(LeftParenthesisToken);
}

MediaQueryToken MediaQueryTokenizer::rightParenthesis(UChar cc)
{
    return MediaQueryToken(RightParenthesisToken);
}

MediaQueryToken MediaQueryTokenizer::plusOrFullStop(UChar cc)
{
    if (nextCharsAreNumber()) {
        reconsume(cc);
        return consumeNumericToken();
    }
    return MediaQueryToken(DelimiterToken, cc);
}

MediaQueryToken MediaQueryTokenizer::comma(UChar cc)
{
    return MediaQueryToken(CommaToken);
}

MediaQueryToken MediaQueryTokenizer::hyphenMinus(UChar cc)
{
    if (nextCharsAreNumber()) {
        reconsume(cc);
        return consumeNumericToken();
    }
    if (nextCharsAreIdentifier()) {
        reconsume(cc);
        return consumeIdentLikeToken();
    }
    return MediaQueryToken(DelimiterToken, cc);
}

MediaQueryToken MediaQueryTokenizer::solidus(UChar cc)
{
    return MediaQueryToken(DelimiterToken, cc);
}

MediaQueryToken MediaQueryTokenizer::colon(UChar cc)
{
    return MediaQueryToken(ColonToken);
}

MediaQueryToken MediaQueryTokenizer::semiColon(UChar cc)
{
    return MediaQueryToken(SemicolonToken);
}

MediaQueryToken MediaQueryTokenizer::reverseSolidus(UChar cc)
{
    if (twoCharsAreValidEscape(cc, m_input.currentInputChar())) {
        reconsume(cc);
        return consumeIdentLikeToken();
    }
    return MediaQueryToken(DelimiterToken, cc);
}

MediaQueryToken MediaQueryTokenizer::asciiDigit(UChar cc)
{
    reconsume(cc);
    return consumeNumericToken();
}

MediaQueryToken MediaQueryTokenizer::nameStart(UChar cc)
{
    reconsume(cc);
    return consumeIdentLikeToken();
}

MediaQueryToken MediaQueryTokenizer::endOfFile(UChar cc)
{
    return MediaQueryToken(EOFToken);
}

void MediaQueryTokenizer::tokenize(String string, Vector<MediaQueryToken>& outTokens)
{
    // According to the spec, we should perform preprocessing here.
    // See: http://www.w3.org/TR/css-syntax-3/#input-preprocessing
    //
    // However, we can skip this step since:
    // * We're using HTML spaces (which accept \r and \f as a valid white space)
    // * Do not count white spaces
    // * consumeEscape replaces NULLs for replacement characters

    MediaQueryInputStream input(string);
    MediaQueryTokenizer tokenizer(input);
    while (true) {
        outTokens.append(tokenizer.nextToken());
        if (outTokens.last().type() == EOFToken)
            return;
    }
}

MediaQueryToken MediaQueryTokenizer::nextToken()
{
    // Unlike the HTMLTokenizer, the CSS Syntax spec is written
    // as a stateless, (fixed-size) look-ahead tokenizer.
    // We could move to the stateful model and instead create
    // states for all the "next 3 codepoints are X" cases.
    // State-machine tokenizers are easier to write to handle
    // incremental tokenization of partial sources.
    // However, for now we follow the spec exactly.
    UChar cc = consume();
    CodePoint codePointFunc = 0;

    if (isASCII(cc)) {
        ASSERT_WITH_SECURITY_IMPLICATION(cc < codePointsNumber);
        codePointFunc = codePoints[cc];
    } else {
        codePointFunc = &MediaQueryTokenizer::nameStart;
    }

    if (codePointFunc)
        return ((this)->*(codePointFunc))(cc);

    return MediaQueryToken(DelimiterToken, cc);
}

static int getSign(MediaQueryInputStream& input, unsigned& offset)
{
    int sign = 1;
    if (input.currentInputChar() == '+') {
        ++offset;
    } else if (input.peek(offset) == '-') {
        sign = -1;
        ++offset;
    }
    return sign;
}

static unsigned long long getInteger(MediaQueryInputStream& input, unsigned& offset)
{
    unsigned intStartPos = offset;
    offset = input.skipWhilePredicate<isASCIIDigit>(offset);
    unsigned intEndPos = offset;
    return input.getUInt(intStartPos, intEndPos);
}

static double getFraction(MediaQueryInputStream& input, unsigned& offset, unsigned& digitsNumber)
{
    unsigned fractionStartPos = 0;
    unsigned fractionEndPos = 0;
    if (input.peek(offset) == '.' && isASCIIDigit(input.peek(++offset))) {
        fractionStartPos = offset - 1;
        offset = input.skipWhilePredicate<isASCIIDigit>(offset);
        fractionEndPos = offset;
    }
    digitsNumber = fractionEndPos- fractionStartPos;
    return input.getDouble(fractionStartPos, fractionEndPos);
}

static unsigned long long getExponent(MediaQueryInputStream& input, unsigned& offset, int sign)
{
    unsigned exponentStartPos = 0;
    unsigned exponentEndPos = 0;
    if ((input.peek(offset) == 'E' || input.peek(offset) == 'e')) {
        int offsetBeforeExponent = offset;
        ++offset;
        if (input.peek(offset) == '+') {
            ++offset;
        } else if (input.peek(offset) =='-') {
            sign = -1;
            ++offset;
        }
        exponentStartPos = offset;
        offset = input.skipWhilePredicate<isASCIIDigit>(offset);
        exponentEndPos = offset;
        if (exponentEndPos == exponentStartPos)
            offset = offsetBeforeExponent;
    }
    return input.getUInt(exponentStartPos, exponentEndPos);
}

// This method merges the following spec sections for efficiency
// http://www.w3.org/TR/css3-syntax/#consume-a-number
// http://www.w3.org/TR/css3-syntax/#convert-a-string-to-a-number
MediaQueryToken MediaQueryTokenizer::consumeNumber()
{
    ASSERT(nextCharsAreNumber());
    NumericValueType type = IntegerValueType;
    double value = 0;
    unsigned offset = 0;
    int exponentSign = 1;
    unsigned fractionDigits;
    int sign = getSign(m_input, offset);
    unsigned long long integerPart = getInteger(m_input, offset);
    double fractionPart = getFraction(m_input, offset, fractionDigits);
    unsigned long long exponentPart = getExponent(m_input, offset, exponentSign);
    double exponent = pow(10, (float)exponentSign * (double)exponentPart);
    value = (double)sign * ((double)integerPart + fractionPart) * exponent;

    m_input.advance(offset);
    if (fractionDigits > 0)
        type = NumberValueType;

    return MediaQueryToken(NumberToken, value, type);
}

// http://www.w3.org/TR/css3-syntax/#consume-a-numeric-token
MediaQueryToken MediaQueryTokenizer::consumeNumericToken()
{
    MediaQueryToken token = consumeNumber();
    if (nextCharsAreIdentifier())
        token.convertToDimensionWithUnit(consumeName());
    else if (consumeIfNext('%'))
        token.convertToPercentage();
    return token;
}

// http://www.w3.org/TR/css3-syntax/#consume-an-ident-like-token
MediaQueryToken MediaQueryTokenizer::consumeIdentLikeToken()
{
    String name = consumeName();
    if (consumeIfNext('('))
        return MediaQueryToken(FunctionToken, name);
    return MediaQueryToken(IdentToken, name);
}

void MediaQueryTokenizer::consumeUntilNonWhitespace()
{
    // Using HTML space here rather than CSS space since we don't do preprocessing
    while (isHTMLSpace<UChar>(m_input.currentInputChar()))
        consume();
}

bool MediaQueryTokenizer::consumeIfNext(UChar character)
{
    if (m_input.currentInputChar() == character) {
        consume();
        return true;
    }
    return false;
}

// http://www.w3.org/TR/css3-syntax/#consume-a-name
String MediaQueryTokenizer::consumeName()
{
    // FIXME: Is this as efficient as it can be?
    // The possibility of escape chars mandates a copy AFAICT.
    Vector<UChar> result;
    while (true) {
        if (isNameChar(m_input.currentInputChar())) {
            result.append(consume());
            continue;
        }
        if (nextTwoCharsAreValidEscape()) {
            // "consume()" fixes a spec bug.
            // The first code point should be consumed before consuming the escaped code point.
            consume();
            result.append(consumeEscape());
            continue;
        }
        return String(result);
    }
}

// http://www.w3.org/TR/css-syntax-3/#consume-an-escaped-code-point
UChar MediaQueryTokenizer::consumeEscape()
{
    UChar cc = consume();
    ASSERT(cc != '\n');
    if (isASCIIHexDigit(cc)) {
        unsigned consumedHexDigits = 1;
        String hexChars;
        do {
            hexChars.append(cc);
            cc = consume();
            consumedHexDigits++;
        } while (consumedHexDigits < 6 && isASCIIHexDigit(cc));
        bool ok = false;
        UChar codePoint = hexChars.toUIntStrict(&ok, 16);
        if (!ok)
            return WTF::Unicode::replacementCharacter;
        return codePoint;
    }

    // Replaces NULLs with replacement characters, since we do not perform preprocessing
    if (cc == kEndOfFileMarker)
        return WTF::Unicode::replacementCharacter;
    return cc;
}

bool MediaQueryTokenizer::nextTwoCharsAreValidEscape()
{
    if (m_input.leftChars() < 2)
        return false;
    return twoCharsAreValidEscape(m_input.peek(1), m_input.peek(2));
}

// http://www.w3.org/TR/css3-syntax/#starts-with-a-number
bool MediaQueryTokenizer::nextCharsAreNumber()
{
    UChar first = m_input.currentInputChar();
    UChar second = m_input.peek(1);
    if (isASCIIDigit(first))
        return true;
    if (first == '+' || first == '-')
        return ((isASCIIDigit(second)) || (second == '.' && isASCIIDigit(m_input.peek(2))));
    if (first =='.')
        return (isASCIIDigit(second));
    return false;
}

// http://www.w3.org/TR/css3-syntax/#would-start-an-identifier
bool MediaQueryTokenizer::nextCharsAreIdentifier()
{
    UChar firstChar = m_input.currentInputChar();
    if (isNameStart(firstChar) || nextTwoCharsAreValidEscape())
        return true;

    if (firstChar == '-') {
        if (isNameStart(m_input.peek(1)))
            return true;
        return nextTwoCharsAreValidEscape();
    }

    return false;
}

} // namespace WebCore
