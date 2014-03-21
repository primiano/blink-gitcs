// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MediaQueryParser_h
#define MediaQueryParser_h

#include "core/css/CSSParserValues.h"
#include "core/css/MediaList.h"
#include "core/css/MediaQuery.h"
#include "core/css/MediaQueryExp.h"
#include "core/css/parser/MediaQueryToken.h"
#include "wtf/Vector.h"
#include "wtf/text/WTFString.h"

namespace WebCore {

class MediaQuerySet;

class MediaQueryData {
    STACK_ALLOCATED();
private:
    MediaQuery::Restrictor m_restrictor;
    String m_mediaType;
    OwnPtrWillBeMember<ExpressionHeapVector> m_expressions;
    String m_mediaFeature;
    CSSParserValueList m_valueList;
    bool m_mediaTypeSet;

public:
    MediaQueryData();
    void clear();
    bool addExpression();
    void addParserValue(MediaQueryTokenType, MediaQueryToken&);
    void setMediaType(const String&);
    PassOwnPtrWillBeRawPtr<MediaQuery> takeMediaQuery();

    inline bool currentMediaQueryChanged() const
    {
        return (m_restrictor != MediaQuery::None || m_mediaTypeSet || m_expressions->size() > 0);
    }

    inline void setRestrictor(MediaQuery::Restrictor restrictor) { m_restrictor = restrictor; }

    inline void setMediaFeature(const String& str) { m_mediaFeature = str; }
};

class MediaQueryParser {
    STACK_ALLOCATED();
public:
    static PassRefPtrWillBeRawPtr<MediaQuerySet> parse(const String&);

private:
    MediaQueryParser(const String&);
    virtual ~MediaQueryParser() { };

    PassRefPtrWillBeRawPtr<MediaQuerySet> parseImpl();

    typedef Vector<MediaQueryToken>::iterator TokenIterator;

    void processToken(TokenIterator&);

    void readRestrictor(MediaQueryTokenType, TokenIterator&);
    void readMediaType(MediaQueryTokenType, TokenIterator&);
    void readAnd(MediaQueryTokenType, TokenIterator&);
    void readFeatureStart(MediaQueryTokenType, TokenIterator&);
    void readFeature(MediaQueryTokenType, TokenIterator&);
    void readFeatureColon(MediaQueryTokenType, TokenIterator&);
    void readFeatureValue(MediaQueryTokenType, TokenIterator&);
    void readFeatureEnd(MediaQueryTokenType, TokenIterator&);
    void skipUntilComma(MediaQueryTokenType, TokenIterator&);
    void skipUntilParenthesis(MediaQueryTokenType, TokenIterator&);
    void done(MediaQueryTokenType, TokenIterator&);

    typedef void (MediaQueryParser::*State)(MediaQueryTokenType, TokenIterator&);

    void setStateAndRestrict(State, MediaQuery::Restrictor);

    State m_state;
    Vector<MediaQueryToken> m_tokens;
    MediaQueryData m_mediaQueryData;
    RefPtrWillBeMember<MediaQuerySet> m_querySet;

    const static State ReadRestrictor;
    const static State ReadMediaType;
    const static State ReadAnd;
    const static State ReadFeatureStart;
    const static State ReadFeature;
    const static State ReadFeatureColon;
    const static State ReadFeatureValue;
    const static State ReadFeatureEnd;
    const static State SkipUntilComma;
    const static State SkipUntilParenthesis;
    const static State Done;

};

} // namespace WebCore

#endif // MediaQueryParser_h
