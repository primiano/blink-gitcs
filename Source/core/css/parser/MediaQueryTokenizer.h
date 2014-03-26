// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MediaQueryTokenizer_h
#define MediaQueryTokenizer_h

#include "core/css/parser/MediaQueryToken.h"
#include "core/html/parser/InputStreamPreprocessor.h"
#include "wtf/text/WTFString.h"

#include <climits>

namespace WebCore {

class MediaQueryInputStream;

class MediaQueryTokenizer {
    WTF_MAKE_NONCOPYABLE(MediaQueryTokenizer);
    WTF_MAKE_FAST_ALLOCATED;
public:
    static void tokenize(String, Vector<MediaQueryToken>&);
private:
    MediaQueryTokenizer(MediaQueryInputStream&);

    MediaQueryToken nextToken();

    UChar consume();
    void consume(unsigned);
    void reconsume(UChar);

    MediaQueryToken consumeNumericToken();
    MediaQueryToken consumeIdentLikeToken();
    MediaQueryToken consumeNumber();

    void consumeUntilNonWhitespace();

    bool consumeIfNext(UChar);
    String consumeName();
    UChar consumeEscape();

    bool nextTwoCharsAreValidEscape();
    bool nextCharsAreNumber();
    bool nextCharsAreIdentifier();

    typedef MediaQueryToken (MediaQueryTokenizer::*CodePoint)(UChar);

    static const CodePoint codePoints[];

    MediaQueryToken whiteSpace(UChar);
    MediaQueryToken leftParenthesis(UChar);
    MediaQueryToken rightParenthesis(UChar);
    MediaQueryToken plusOrFullStop(UChar);
    MediaQueryToken comma(UChar);
    MediaQueryToken hyphenMinus(UChar);
    MediaQueryToken solidus(UChar);
    MediaQueryToken colon(UChar);
    MediaQueryToken semiColon(UChar);
    MediaQueryToken reverseSolidus(UChar);
    MediaQueryToken asciiDigit(UChar);
    MediaQueryToken nameStart(UChar);
    MediaQueryToken endOfFile(UChar);

    MediaQueryInputStream& m_input;
};



} // namespace WebCore

#endif // MediaQueryTokenizer_h
