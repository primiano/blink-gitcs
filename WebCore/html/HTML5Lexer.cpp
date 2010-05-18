/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 * Copyright (C) 2009 Torch Mobile, Inc. http://www.torchmobile.com/
 * Copyright (C) 2010 Google, Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "HTML5Lexer.h"

#include "AtomicString.h"
#include "HTML5Token.h"
#include "HTMLNames.h"
#include "NotImplemented.h"
#include <wtf/text/CString.h>
#include <wtf/CurrentTime.h>
#include <wtf/unicode/Unicode.h>

// Use __GNUC__ instead of PLATFORM(GCC) to stay consistent with the gperf generated c file
#ifdef __GNUC__
// The main tokenizer includes this too so we are getting two copies of the data. However, this way the code gets inlined.
#include "HTMLEntityNames.c"
#else
// Not inlined for non-GCC compilers
struct Entity {
    const char* name;
    int code;
};
const struct Entity* findEntity(register const char* str, register unsigned int len);
#endif

using namespace WTF;

namespace WebCore {

using namespace HTMLNames;

HTML5Lexer::HTML5Lexer()
    : m_outputToken(0)
{
}

HTML5Lexer::~HTML5Lexer()
{
}

void HTML5Lexer::begin() 
{ 
    reset(); 
}

void HTML5Lexer::end() 
{
}

void HTML5Lexer::reset()
{
    m_source.clear();

    m_state = DataState;
    m_escape = false;
    m_contentModel = PCDATA;
    m_commentPos = 0;

    m_closeTag = false;
    m_tagName.clear();
    m_attributeName.clear();
    m_attributeValue.clear();
    m_lastStartTag = AtomicString();

    m_lastCharacterIndex = 0;
    clearLastCharacters();
}

void HTML5Lexer::write(const SegmentedString& source, HTML5Token& outputToken)
{
    m_outputToken = &outputToken;
    tokenize(source);
    m_outputToken = 0;
}

static inline bool isWhitespace(UChar c)
{
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

inline void HTML5Lexer::clearLastCharacters()
{
    memset(m_lastCharacters, 0, lastCharactersBufferSize * sizeof(UChar));
}

inline void HTML5Lexer::rememberCharacter(UChar c)
{
    m_lastCharacterIndex = (m_lastCharacterIndex + 1) % lastCharactersBufferSize;
    m_lastCharacters[m_lastCharacterIndex] = c;
}

inline bool HTML5Lexer::lastCharactersMatch(const char* chars, unsigned count) const
{
    unsigned pos = m_lastCharacterIndex;
    while (count) {
        if (chars[count - 1] != m_lastCharacters[pos])
            return false;
        --count;
        if (!pos)
            pos = lastCharactersBufferSize;
        --pos;
    }
    return true;
}
    
static inline unsigned legalEntityFor(unsigned value)
{
    // FIXME There is a table for more exceptions in the HTML5 specification.
    if (value == 0 || value > 0x10FFFF || (value >= 0xD800 && value <= 0xDFFF))
        return 0xFFFD;
    return value;
}
    
unsigned HTML5Lexer::consumeEntity(SegmentedString& source, bool& notEnoughCharacters)
{
    enum EntityState {
        Initial,
        NumberType,
        MaybeHex,
        Hex,
        Decimal,
        Named
    };
    EntityState entityState = Initial;
    unsigned result = 0;
    Vector<UChar, 10> seenChars;
    Vector<char, 10> entityName;

    while (!source.isEmpty()) {
        UChar cc = *source;
        seenChars.append(cc);
        switch (entityState) {
        case Initial:
            if (isWhitespace(cc) || cc == '<' || cc == '&')
                return 0;
            else if (cc == '#') 
                entityState = NumberType;
            else if ((cc >= 'a' && cc <= 'z') || (cc >= 'A' && cc <= 'Z')) {
                entityName.append(cc);
                entityState = Named;
            } else
                return 0;
            break;
        case NumberType:
            if (cc == 'x' || cc == 'X')
                entityState = MaybeHex;
            else if (cc >= '0' && cc <= '9') {
                entityState = Decimal;
                result = cc - '0';
            } else {
                source.push('#');
                return 0;
            }
            break;
        case MaybeHex:
            if (cc >= '0' && cc <= '9')
                result = cc - '0';
            else if (cc >= 'a' && cc <= 'f')
                result = 10 + cc - 'a';
            else if (cc >= 'A' && cc <= 'F')
                result = 10 + cc - 'A';
            else {
                source.push('#');
                source.push(seenChars[1]);
                return 0;
            }
            entityState = Hex;
            break;
        case Hex:
            if (cc >= '0' && cc <= '9')
                result = result * 16 + cc - '0';
            else if (cc >= 'a' && cc <= 'f')
                result = result * 16 + 10 + cc - 'a';
            else if (cc >= 'A' && cc <= 'F')
                result = result * 16 + 10 + cc - 'A';
            else if (cc == ';') {
                source.advance();
                return legalEntityFor(result);
            } else 
                return legalEntityFor(result);
            break;
        case Decimal:
            if (cc >= '0' && cc <= '9')
                result = result * 10 + cc - '0';
            else if (cc == ';') {
                source.advance();
                return legalEntityFor(result);
            } else
                return legalEntityFor(result);
            break;               
        case Named:
            // This is the attribute only version, generic version matches somewhat differently
            while (entityName.size() <= 8) {
                if (cc == ';') {
                    const Entity* entity = findEntity(entityName.data(), entityName.size());
                    if (entity) {
                        source.advance();
                        return entity->code;
                    }
                    break;
                }
                if (!(cc >= 'a' && cc <= 'z') && !(cc >= 'A' && cc <= 'Z') && !(cc >= '0' && cc <= '9')) {
                    const Entity* entity = findEntity(entityName.data(), entityName.size());
                    if (entity)
                        return entity->code;
                    break;
                }
                entityName.append(cc);
                source.advance();
                if (source.isEmpty())
                    goto outOfCharacters;
                cc = *source;
                seenChars.append(cc);
            }
            if (seenChars.size() == 2)
                source.push(seenChars[0]);
            else if (seenChars.size() == 3) {
                source.push(seenChars[0]);
                source.push(seenChars[1]);
            } else
                source.prepend(SegmentedString(String(seenChars.data(), seenChars.size() - 1)));
            return 0;
        }
        source.advance();
    }
outOfCharacters:
    notEnoughCharacters = true;
    source.prepend(SegmentedString(String(seenChars.data(), seenChars.size())));
    return 0;
}

void HTML5Lexer::tokenize(const SegmentedString& source)
{
    m_source.append(source);

    // Source: http://www.whatwg.org/specs/web-apps/current-work/#tokenisation0
    while (!m_source.isEmpty()) {
        UChar cc = *m_source;
        switch (m_state) {
        case DataState: {
            if (cc == '&')
                m_state = CharacterReferenceInDataState;
            else if (cc == '<')
                m_state = TagOpenState;
            else
                emitCharacter(cc);
            break;
        }
        case CharacterReferenceInDataState: {
            notImplemented();
            break;
        }
        case RCDATAState: {
            if (cc == '&')
                m_state = CharacterReferenceInRCDATAState;
            else if (cc == '<')
                m_state = RCDATALessThanSignState;
            else
                emitCharacter(cc);
            break;
        }
        case CharacterReferenceInRCDATAState: {
            notImplemented();
            break;
        }
        case RAWTEXTState: {
            if (cc == '<')
                m_state = RAWTEXTLessThanSignState;
            else
                emitCharacter(cc);
            break;
        }
        case ScriptDataState: {
            if (cc == '<')
                m_state = ScriptDataLessThanSignState;
            else
                emitCharacter(cc);
            break;
        }
        case PLAINTEXTState: {
            emitCharacter(cc);
            break;
        }
        case TagOpenState: {
            if (cc == '!')
                m_state = MarkupDeclarationOpenState;
            else if (cc == '/')
                m_state = EndTagOpenState;
            else if (cc >= 'A' && cc <= 'Z') {
                notImplemented();
                m_state = TagNameState;
            } else if (cc >= 'a' && cc <= 'z') {
                notImplemented();
                m_state = TagNameState;
            } else if (cc == '?') {
                emitParseError();
                m_state = BogusCommentState;
            } else {
                emitParseError();
                m_state = DataState;
                emitCharacter('<');
                continue;
            }
            break;
        }
        case EndTagOpenState: {
            if (cc >= 'A' && cc <= 'Z') {
                notImplemented();
                m_state = TagNameState;
            } else if (cc >= 'a' && cc <= 'z') {
                notImplemented();
                m_state = TagNameState;
            } else if (cc == '>') {
                emitParseError();
                m_state = DataState;
            } else {
                emitParseError();
                m_state = DataState;
            }
            // FIXME: Handle EOF properly.
            break;
        }
        case TagNameState: {
            if (cc == '\x09' || cc == '\x0A' || cc == '\x0C' || cc == ' ')
                m_state = BeforeAttributeNameState;
            else if (cc == '/')
                m_state = SelfClosingStartTagState;
            else if (cc == '>')
                m_state = DataState;
            else if (cc >= 'A' && cc <= 'Z')
                notImplemented();
            else
                notImplemented();
            // FIXME: Handle EOF properly.
            break;
        }
        case RCDATALessThanSignState: {
            if (cc == '/') {
                m_temporaryBuffer.clear();
                m_state = RCDATAEndTagOpenState;
            } else {
                emitCharacter('<');
                m_state = RCDATAState;
                continue;
            }
            break;
        }
        case RCDATAEndTagOpenState: {
            if (cc >= 'A' && cc <= 'Z') {
                notImplemented();
                m_state = RCDATAEndTagNameState;
            } else if (cc >= 'a' && cc <= 'z') {
                notImplemented();
                m_state = RCDATAEndTagNameState;
                emitCharacter('<');
                emitCharacter('/');
                m_state = RCDATAState;
                continue;
            }
            break;
        }
        case RCDATAEndTagNameState: {
            if (cc == '\x09' || cc == '\x0A' || cc == '\x0C' || cc == ' ') {
                notImplemented();
                m_state = BeforeAttributeNameState;
            } else if (cc == '/') {
                notImplemented();
                m_state = SelfClosingStartTagState;
            } else if (cc == '>') {
                notImplemented();
                m_state = DataState;
            } else if (cc >= 'A' && cc <= 'Z')
                notImplemented();
            else if (cc >= 'a' && cc <= 'z')
                notImplemented();
            else {
                emitCharacter('<');
                emitCharacter('/');
                notImplemented();
                m_state = RCDATAState;
                continue;
            }
            break;
        }
        case RAWTEXTLessThanSignState: {
            if (cc == '/') {
                m_temporaryBuffer.clear();
                m_state = RAWTEXTEndTagOpenState;
            } else {
                emitCharacter('<');
                m_state = RAWTEXTState;
                continue;
            }
            break;
        }
        case RAWTEXTEndTagOpenState: {
            if (cc >= 'A' && cc <= 'Z') {
                notImplemented();
                m_state = RAWTEXTEndTagNameState;
            } else if (cc >= 'a' && cc <= 'z') {
                notImplemented();
                m_state = RAWTEXTEndTagNameState;
            } else {
                emitCharacter('<');
                emitCharacter('/');
                m_state = RAWTEXTState;
                continue;
            }
            break;
        }
        case RAWTEXTEndTagNameState: {
            if (cc == '\x09' || cc == '\x0A' || cc == '\x0C' || cc == ' ') {
                notImplemented();
                m_state = BeforeAttributeNameState;
            } else if (cc == '/') {
                notImplemented();
                m_state = SelfClosingStartTagState;
            } else if (cc == '>') {
                notImplemented();
                m_state = DataState;
            } else if (cc >= 'A' && cc <= 'Z')
                notImplemented();
            else if (cc >= 'a' && cc <= 'z')
                notImplemented();
            else {
                emitCharacter('<');
                emitCharacter('/');
                notImplemented();
                m_state = RAWTEXTState;
                continue;
            }
        }
        case ScriptDataLessThanSignState: {
            if (cc == '/') {
                m_temporaryBuffer.clear();
                m_state = ScriptDataEndTagOpenState;
            } else if (cc == '!') {
                emitCharacter('<');
                emitCharacter('!');
                m_state = ScriptDataEscapeStartState;
            } else {
                emitCharacter('<');
                m_state = ScriptDataState;
                continue;
            }
            break;
        }
        case ScriptDataEndTagOpenState: {
            if (cc >= 'A' && cc <= 'Z') {
                notImplemented();
                m_state = ScriptDataEndTagNameState;
            } else if (cc >= 'a' && cc <= 'z') {
                notImplemented();
                m_state = ScriptDataEndTagNameState;
            } else {
                emitCharacter('<');
                emitCharacter('/');
                m_state = ScriptDataState;
                continue;
            }
            break;
        }
        case ScriptDataEndTagNameState: {
            if (cc == '\x09' || cc == '\x0A' || cc == '\x0C' || cc == ' ') {
                notImplemented();
                m_state = BeforeAttributeNameState;
            } else if (cc == '/') {
                notImplemented();
                m_state = SelfClosingStartTagState;
            } else if (cc == '>') {
                notImplemented();
                m_state = DataState;
            } else if (cc >= 'A' && cc <= 'Z')
                notImplemented();
            else if (cc >= 'a' && cc <= 'z')
                notImplemented();
            else {
                emitCharacter('<');
                emitCharacter('/');
                notImplemented();
                m_state = ScriptDataState;
                continue;
            }
            break;
        }
        case ScriptDataEscapeStartState: {
            if (cc == '-') {
                emitCharacter(cc);
                m_state = ScriptDataEscapeStartDashState;
            } else {
                m_state = ScriptDataState;
                continue;
            }
            break;
        }
        case ScriptDataEscapeStartDashState: {
            if (cc == '-') {
                emitCharacter(cc);
                m_state = ScriptDataEscapedDashDashState;
            } else {
                m_state = ScriptDataState;
                continue;
            }
            break;
        }
        case ScriptDataEscapedState: {
            if (cc == '-') {
                emitCharacter(cc);
                m_state = ScriptDataEscapedDashState;
            } else if (cc == '<')
                m_state = ScriptDataEscapedLessThanSignState;
            else
                emitCharacter(cc);
            // FIXME: Handle EOF properly.
            break;
        }
        case ScriptDataEscapedDashState: {
            if (cc == '-') {
                emitCharacter(cc);
                m_state = ScriptDataEscapedDashDashState;
            } else if (cc == '<')
                m_state = ScriptDataEscapedLessThanSignState;
            else {
                emitCharacter(cc);
                m_state = ScriptDataEscapedState;
            }
            // FIXME: Handle EOF properly.
            break;
        }
        case ScriptDataEscapedDashDashState: {
            if (cc == '-')
                emitCharacter(cc);
            else if (cc == '<')
                m_state = ScriptDataEscapedLessThanSignState;
            else if (cc == '>') {
                emitCharacter(cc);
                m_state = ScriptDataState;
            } else {
                emitCharacter(cc);
                m_state = ScriptDataEscapedState;
            }
            // FIXME: Handle EOF properly.
            break;
        }
        case ScriptDataEscapedLessThanSignState: {
            if (cc == '/') {
                m_temporaryBuffer.clear();
                m_state = ScriptDataEscapedEndTagOpenState;
            } else if (cc >= 'A' && cc <= 'Z') {
                notImplemented();
                m_state = ScriptDataDoubleEscapeStartState;
            } else if (cc >= 'a' && cc <= 'z') {
                notImplemented();
                m_state = ScriptDataDoubleEscapeStartState;
            } else {
                emitCharacter('<');
                m_state = ScriptDataEscapedState;
                continue;
            }
            break;
        }
        case ScriptDataEscapedEndTagOpenState: {
            if (cc >= 'A' && cc <= 'Z') {
                notImplemented();
                m_state = ScriptDataEscapedEndTagNameState;
            } else if (cc >= 'a' && cc <= 'z') {
                notImplemented();
                m_state = ScriptDataEscapedEndTagNameState;
            } else {
                emitCharacter('<');
                emitCharacter('/');
                m_state = ScriptDataEscapedState;
                continue;
            }
            break;
        }
        case ScriptDataEscapedEndTagNameState: {
            if (cc == '\x09' || cc == '\x0A' || cc == '\x0C' || cc == ' ') {
                notImplemented();
                m_state = BeforeAttributeNameState;
            } else if (cc == '/') {
                notImplemented();
                m_state = SelfClosingStartTagState;
            } else if (cc == '>') {
                notImplemented();
                m_state = DataState;
            } else if (cc >= 'A' && cc <= 'Z')
                notImplemented();
            else if (cc >= 'a' && cc <= 'z')
                notImplemented();
            else {
                emitCharacter('<');
                emitCharacter('/');
                notImplemented();
                m_state = ScriptDataEscapedState;
                continue;
            }
            break;
        }
        case ScriptDataDoubleEscapeStartState: {
            if (cc == '\x09' || cc == '\x0A' || cc == '\x0C' || cc == ' ' || cc == '/' || cc == '>') {
                emitCharacter(cc);
                if (temporaryBufferIs("string"))
                    m_state = ScriptDataDoubleEscapedState;
                else
                    m_state = ScriptDataEscapedState;
            } else if (cc >= 'A' && cc <= 'Z')
                notImplemented();
            else if (cc >= 'a' && cc <= 'z')
                notImplemented();
            else {
                m_state = ScriptDataEscapedState;
                continue;
            }
            break;
        }
        case ScriptDataDoubleEscapedState: {
            if (cc == '-') {
                emitCharacter(cc);
                m_state = ScriptDataDoubleEscapedDashState;
            } else if (cc == '<') {
                emitCharacter(cc);
                m_state = ScriptDataDoubleEscapedLessThanSignState;
            } else
                emitCharacter(cc);
            // FIXME: Handle EOF properly.
            break;
        }
        case ScriptDataDoubleEscapedDashState: {
            if (cc == '-') {
                emitCharacter(cc);
                m_state = ScriptDataDoubleEscapedDashDashState;
            } else if (cc == '<') {
                emitCharacter(cc);
                m_state = ScriptDataDoubleEscapedLessThanSignState;
            } else {
                emitCharacter(cc);
                m_state = ScriptDataDoubleEscapedState;
            }
            // FIXME: Handle EOF properly.
            break;
        }
        case ScriptDataDoubleEscapedDashDashState: {
            if (cc == '-')
                emitCharacter(cc);
            else if (cc == '<') {
                emitCharacter(cc);
                m_state = ScriptDataDoubleEscapedLessThanSignState;
            } else if (cc == '>') {
                emitCharacter(cc);
                m_state = ScriptDataState;
            } else {
                emitCharacter(cc);
                m_state = ScriptDataDoubleEscapedState;
            }
            // FIXME: Handle EOF properly.
            break;
        }
        case ScriptDataDoubleEscapedLessThanSignState: {
            if (cc == '/') {
                emitCharacter(cc);
                m_temporaryBuffer.clear();
                m_state = ScriptDataDoubleEscapeEndState;
            } else {
                m_state = ScriptDataDoubleEscapedState;
                continue;
            }
            break;
        }
        case ScriptDataDoubleEscapeEndState: {
            if (cc == '\x09' || cc == '\x0A' || cc == '\x0C' || cc == ' ' || cc == '/' || cc == '>') {
                emitCharacter(cc);
                if (temporaryBufferIs("string"))
                    m_state = ScriptDataEscapedState;
                else
                    m_state = ScriptDataDoubleEscapedState;
            } else if (cc >= 'A' && cc <= 'Z')
                notImplemented();
            else if (cc >= 'a' && cc <= 'z')
                notImplemented();
            else {
                m_state = ScriptDataDoubleEscapedState;
                continue;
            }
            break;
        }
        case BeforeAttributeNameState:
        case AttributeNameState:
        case AfterAttributeNameState:
        case BeforeAttributeValueState:
        case AttributeValueDoubleQuotedState:
        case AttributeValueSingleQuotedState:
        case AttributeValueUnquotedState:
        case CharacterReferenceInAttributeValueState:
        case AfterAttributeValueQuotedState:
        case SelfClosingStartTagState:
        case BogusCommentState:
        case MarkupDeclarationOpenState:
        case CommentStartState:
        case CommentStartDashState:
        case CommentState:
        case CommentEndDashState:
        case CommentEndState:
        case CommentEndBangState:
        case CommentEndSpaceState:
        case DOCTYPEState:
        case BeforeDOCTYPENameState:
        case DOCTYPENameState:
        case AfterDOCTYPENameState:
        case AfterDOCTYPEPublicKeywordState:
        case BeforeDOCTYPEPublicIdentifierState:
        case DOCTYPEPublicIdentifierDoubleQuotedState:
        case DOCTYPEPublicIdentifierSingleQuotedState:
        case AfterDOCTYPEPublicIdentifierState:
        case BetweenDOCTYPEPublicAndSystemIdentifiersState:
        case AfterDOCTYPESystemKeywordState:
        case BeforeDOCTYPESystemIdentifierState:
        case DOCTYPESystemIdentifierDoubleQuotedState:
        case DOCTYPESystemIdentifierSingleQuotedState:
        case AfterDOCTYPESystemIdentifierState:
        case BogusDOCTYPEState:
        case CDATASectionState:
        case TokenizingCharacterReferencesState:
        default:
            notImplemented();
            break;
        }
        m_source.advance();
    }
}

void HTML5Lexer::processAttribute()
{
    AtomicString tag = AtomicString(m_tagName.data(), m_tagName.size());
    AtomicString attribute = AtomicString(m_attributeName.data(), m_attributeName.size());

    String value(m_attributeValue.data(), m_attributeValue.size());
}

inline bool HTML5Lexer::temporaryBufferIs(const char*)
{
    notImplemented();
    return true;
}

inline void HTML5Lexer::emitCharacter(UChar character)
{
    m_outputToken->setToCharacter(character);
}

inline void HTML5Lexer::emitParseError()
{
}

void HTML5Lexer::emitTag()
{
    if (m_closeTag) {
        m_contentModel = PCDATA;
        clearLastCharacters();
        return;
    }

    AtomicString tag(m_tagName.data(), m_tagName.size());
    m_lastStartTag = tag;

    if (tag == textareaTag || tag == titleTag)
        m_contentModel = RCDATA;
    else if (tag == styleTag || tag == xmpTag || tag == scriptTag || tag == iframeTag || tag == noembedTag || tag == noframesTag)
        m_contentModel = CDATA;
    else if (tag == noscriptTag)
        // we wouldn't be here if scripts were disabled
        m_contentModel = CDATA;
    else if (tag == plaintextTag)
        m_contentModel = PLAINTEXT;
    else
        m_contentModel = PCDATA;
}

}
