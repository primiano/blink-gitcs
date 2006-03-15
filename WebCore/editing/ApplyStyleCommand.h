/*
 * Copyright (C) 2005, 2006 Apple Computer, Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef apply_style_command_h__
#define apply_style_command_h__

#include "CompositeEditCommand.h"
#include <kxmlcore/PassRefPtr.h>

namespace WebCore {

class HTMLElementImpl;

class ApplyStyleCommand : public CompositeEditCommand
{
public:
    enum EPropertyLevel { PropertyDefault, ForceBlockProperties };

    ApplyStyleCommand(DocumentImpl*, CSSStyleDeclarationImpl*, ElementImpl*, EditAction = EditActionChangeAttributes, EPropertyLevel = PropertyDefault);
    ApplyStyleCommand(DocumentImpl*, CSSStyleDeclarationImpl*, ElementImpl*, const Position& start, const Position& end, EditAction = EditActionChangeAttributes, EPropertyLevel = PropertyDefault);

    virtual void doApply();
    virtual EditAction editingAction() const;

    CSSMutableStyleDeclarationImpl* style() const { return m_style.get(); }

private:
    // style-removal helpers
    bool isHTMLStyleNode(CSSMutableStyleDeclarationImpl*, HTMLElementImpl*);
    void removeHTMLStyleNode(HTMLElementImpl *);
    void removeHTMLFontStyle(CSSMutableStyleDeclarationImpl*, HTMLElementImpl*);
    void removeCSSStyle(CSSMutableStyleDeclarationImpl*, HTMLElementImpl*);
    void removeBlockStyle(CSSMutableStyleDeclarationImpl*, const Position& start, const Position& end);
    void removeInlineStyle(PassRefPtr<CSSMutableStyleDeclarationImpl>, const Position& start, const Position& end);
    bool nodeFullySelected(NodeImpl*, const Position& start, const Position& end) const;
    bool nodeFullyUnselected(NodeImpl*, const Position& start, const Position& end) const;
    PassRefPtr<CSSMutableStyleDeclarationImpl> extractTextDecorationStyle(NodeImpl*);
    PassRefPtr<CSSMutableStyleDeclarationImpl> extractAndNegateTextDecorationStyle(NodeImpl*);
    void applyTextDecorationStyle(NodeImpl*, CSSMutableStyleDeclarationImpl *style);
    void pushDownTextDecorationStyleAroundNode(NodeImpl*, const Position& start, const Position& end, bool force);
    void pushDownTextDecorationStyleAtBoundaries(const Position& start, const Position& end);
    
    // style-application helpers
    void applyBlockStyle(CSSMutableStyleDeclarationImpl*);
    void applyRelativeFontStyleChange(CSSMutableStyleDeclarationImpl*);
    void applyInlineStyle(CSSMutableStyleDeclarationImpl*);
    void addBlockStyleIfNeeded(CSSMutableStyleDeclarationImpl*, NodeImpl*);
    void addInlineStyleIfNeeded(CSSMutableStyleDeclarationImpl*, NodeImpl* start, NodeImpl* end);
    bool splitTextAtStartIfNeeded(const Position& start, const Position& end);
    bool splitTextAtEndIfNeeded(const Position& start, const Position& end);
    bool splitTextElementAtStartIfNeeded(const Position& start, const Position& end);
    bool splitTextElementAtEndIfNeeded(const Position& start, const Position& end);
    bool mergeStartWithPreviousIfIdentical(const Position& start, const Position& end);
    bool mergeEndWithNextIfIdentical(const Position& start, const Position& end);
    void cleanUpEmptyStyleSpans(const Position& start, const Position& end);

    void surroundNodeRangeWithElement(NodeImpl* start, NodeImpl* end, ElementImpl* element);
    float computedFontSize(const NodeImpl*);
    void joinChildTextNodes(NodeImpl*, const Position& start, const Position& end);

    void updateStartEnd(const Position& newStart, const Position& newEnd);
    Position startPosition();
    Position endPosition();

    RefPtr<CSSMutableStyleDeclarationImpl> m_style;
    EditAction m_editingAction;
    EPropertyLevel m_propertyLevel;
    Position m_start;
    Position m_end;
    bool m_useEndingSelection;
    RefPtr<ElementImpl> m_styledInlineElement;
};

bool isStyleSpan(const NodeImpl*);
PassRefPtr<HTMLElementImpl> createStyleSpanElement(DocumentImpl*);

} // namespace WebCore

#endif
