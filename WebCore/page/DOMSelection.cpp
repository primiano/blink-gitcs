/*
 * Copyright (C) 2007, 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "config.h"
#include "DOMSelection.h"

#include "ExceptionCode.h"
#include "Frame.h"
#include "Node.h"
#include "PlatformString.h"
#include "Range.h"
#include "SelectionController.h"
#include "TextIterator.h"
#include "htmlediting.h"

namespace WebCore {

static Node* selectionShadowAncestor(Frame* frame)
{
    Node* node = frame->selection()->selection().base().anchorNode();
    if (!node)
        return 0;
    Node* shadowAncestor = node->shadowAncestorNode();
    if (shadowAncestor == node)
        return 0;
    return shadowAncestor;
}

DOMSelection::DOMSelection(Frame* frame)
    : m_frame(frame)
{
}

Frame* DOMSelection::frame() const
{
    return m_frame;
}

void DOMSelection::disconnectFrame()
{
    m_frame = 0;
}

const VisibleSelection& DOMSelection::visibleSelection() const
{
    ASSERT(m_frame);
    return m_frame->selection()->selection();
}

static Position anchorPosition(const VisibleSelection& selection)
{
    Position anchor = selection.isBaseFirst() ? selection.start() : selection.end();
    return rangeCompliantEquivalent(anchor);
}

static Position focusPosition(const VisibleSelection& selection)
{
    Position focus = selection.isBaseFirst() ? selection.end() : selection.start();
    return rangeCompliantEquivalent(focus);
}

static Position basePosition(const VisibleSelection& selection)
{
    return rangeCompliantEquivalent(selection.base());
}

static Position extentPosition(const VisibleSelection& selection)
{
    return rangeCompliantEquivalent(selection.extent());
}

Node* DOMSelection::anchorNode() const
{
    if (!m_frame)
        return 0;
    if (Node* shadowAncestor = selectionShadowAncestor(m_frame))
        return shadowAncestor->parentNode();
    return anchorPosition(visibleSelection()).node();
}

int DOMSelection::anchorOffset() const
{
    if (!m_frame)
        return 0;
    if (Node* shadowAncestor = selectionShadowAncestor(m_frame))
        return shadowAncestor->nodeIndex();
    return anchorPosition(visibleSelection()).deprecatedEditingOffset();
}

Node* DOMSelection::focusNode() const
{
    if (!m_frame)
        return 0;
    if (Node* shadowAncestor = selectionShadowAncestor(m_frame))
        return shadowAncestor->parentNode();
    return focusPosition(visibleSelection()).node();
}

int DOMSelection::focusOffset() const
{
    if (!m_frame)
        return 0;
    if (Node* shadowAncestor = selectionShadowAncestor(m_frame))
        return shadowAncestor->nodeIndex();
    return focusPosition(visibleSelection()).deprecatedEditingOffset();
}

Node* DOMSelection::baseNode() const
{
    if (!m_frame)
        return 0;
    if (Node* shadowAncestor = selectionShadowAncestor(m_frame))
        return shadowAncestor->parentNode();
    return basePosition(visibleSelection()).node();
}

int DOMSelection::baseOffset() const
{
    if (!m_frame)
        return 0;
    if (Node* shadowAncestor = selectionShadowAncestor(m_frame))
        return shadowAncestor->nodeIndex();
    return basePosition(visibleSelection()).deprecatedEditingOffset();
}

Node* DOMSelection::extentNode() const
{
    if (!m_frame)
        return 0;
    if (Node* shadowAncestor = selectionShadowAncestor(m_frame))
        return shadowAncestor->parentNode();
    return extentPosition(visibleSelection()).node();
}

int DOMSelection::extentOffset() const
{
    if (!m_frame)
        return 0;
    if (Node* shadowAncestor = selectionShadowAncestor(m_frame))
        return shadowAncestor->nodeIndex();
    return extentPosition(visibleSelection()).deprecatedEditingOffset();
}

bool DOMSelection::isCollapsed() const
{
    if (!m_frame || selectionShadowAncestor(m_frame))
        return true;
    return !m_frame->selection()->isRange();
}

String DOMSelection::type() const
{
    if (!m_frame)
        return String();

    SelectionController* selection = m_frame->selection();

    // This is a WebKit DOM extension, incompatible with an IE extension
    // IE has this same attribute, but returns "none", "text" and "control"
    // http://msdn.microsoft.com/en-us/library/ms534692(VS.85).aspx
    if (selection->isNone())
        return "None";
    if (selection->isCaret())
        return "Caret";
    return "Range";
}

int DOMSelection::rangeCount() const
{
    if (!m_frame)
        return 0;
    return m_frame->selection()->isNone() ? 0 : 1;
}

void DOMSelection::collapse(Node* node, int offset, ExceptionCode& ec)
{
    if (!m_frame)
        return;

    if (offset < 0) {
        ec = INDEX_SIZE_ERR;
        return;
    }

    if (!isValidForPosition(node))
        return;

    m_frame->selection()->moveTo(VisiblePosition(node, offset, DOWNSTREAM));
}

void DOMSelection::collapseToEnd()
{
    if (!m_frame)
        return;

    const VisibleSelection& selection = m_frame->selection()->selection();
    m_frame->selection()->moveTo(VisiblePosition(selection.end(), DOWNSTREAM));
}

void DOMSelection::collapseToStart()
{
    if (!m_frame)
        return;

    const VisibleSelection& selection = m_frame->selection()->selection();
    m_frame->selection()->moveTo(VisiblePosition(selection.start(), DOWNSTREAM));
}

void DOMSelection::empty()
{
    if (!m_frame)
        return;
    m_frame->selection()->clear();
}

void DOMSelection::setBaseAndExtent(Node* baseNode, int baseOffset, Node* extentNode, int extentOffset, ExceptionCode& ec)
{
    if (!m_frame)
        return;

    if (baseOffset < 0 || extentOffset < 0) {
        ec = INDEX_SIZE_ERR;
        return;
    }

    if (!isValidForPosition(baseNode) || !isValidForPosition(extentNode))
        return;

    VisiblePosition visibleBase = VisiblePosition(baseNode, baseOffset, DOWNSTREAM);
    VisiblePosition visibleExtent = VisiblePosition(extentNode, extentOffset, DOWNSTREAM);

    m_frame->selection()->moveTo(visibleBase, visibleExtent);
}

void DOMSelection::setPosition(Node* node, int offset, ExceptionCode& ec)
{
    if (!m_frame)
        return;
    if (offset < 0) {
        ec = INDEX_SIZE_ERR;
        return;
    }

    if (!isValidForPosition(node))
        return;

    m_frame->selection()->moveTo(VisiblePosition(node, offset, DOWNSTREAM));
}

void DOMSelection::modify(const String& alterString, const String& directionString, const String& granularityString)
{
    if (!m_frame)
        return;

    SelectionController::EAlteration alter;
    if (equalIgnoringCase(alterString, "extend"))
        alter = SelectionController::EXTEND;
    else if (equalIgnoringCase(alterString, "move"))
        alter = SelectionController::MOVE;
    else
        return;

    SelectionController::EDirection direction;
    if (equalIgnoringCase(directionString, "forward"))
        direction = SelectionController::FORWARD;
    else if (equalIgnoringCase(directionString, "backward"))
        direction = SelectionController::BACKWARD;
    else if (equalIgnoringCase(directionString, "left"))
        direction = SelectionController::LEFT;
    else if (equalIgnoringCase(directionString, "right"))
        direction = SelectionController::RIGHT;
    else
        return;

    TextGranularity granularity;
    if (equalIgnoringCase(granularityString, "character"))
        granularity = CharacterGranularity;
    else if (equalIgnoringCase(granularityString, "word"))
        granularity = WordGranularity;
    else if (equalIgnoringCase(granularityString, "sentence"))
        granularity = SentenceGranularity;
    else if (equalIgnoringCase(granularityString, "line"))
        granularity = LineGranularity;
    else if (equalIgnoringCase(granularityString, "paragraph"))
        granularity = ParagraphGranularity;
    else if (equalIgnoringCase(granularityString, "lineboundary"))
        granularity = LineBoundary;
    else if (equalIgnoringCase(granularityString, "sentenceboundary"))
        granularity = SentenceBoundary;
    else if (equalIgnoringCase(granularityString, "paragraphboundary"))
        granularity = ParagraphBoundary;
    else if (equalIgnoringCase(granularityString, "documentboundary"))
        granularity = DocumentBoundary;
    else
        return;

    m_frame->selection()->modify(alter, direction, granularity, false);
}

void DOMSelection::extend(Node* node, int offset, ExceptionCode& ec)
{
    if (!m_frame)
        return;

    if (!node) {
        ec = TYPE_MISMATCH_ERR;
        return;
    }

    if (offset < 0 || offset > (node->offsetInCharacters() ? caretMaxOffset(node) : (int)node->childNodeCount())) {
        ec = INDEX_SIZE_ERR;
        return;
    }

    if (!isValidForPosition(node))
        return;

    m_frame->selection()->setExtent(VisiblePosition(node, offset, DOWNSTREAM));
}

PassRefPtr<Range> DOMSelection::getRangeAt(int index, ExceptionCode& ec)
{
    if (!m_frame)
        return 0;

    if (index < 0 || index >= rangeCount()) {
        ec = INDEX_SIZE_ERR;
        return 0;
    }

    // If you're hitting this, you've added broken multi-range selection support
    ASSERT(rangeCount() == 1);

    if (Node* shadowAncestor = selectionShadowAncestor(m_frame)) {
        Node* container = shadowAncestor->parentNode();
        int offset = shadowAncestor->nodeIndex();
        return Range::create(shadowAncestor->document(), container, offset, container, offset);
    }

    const VisibleSelection& selection = m_frame->selection()->selection();
    return selection.firstRange();
}

void DOMSelection::removeAllRanges()
{
    if (!m_frame)
        return;
    m_frame->selection()->clear();
}

void DOMSelection::addRange(Range* r)
{
    if (!m_frame)
        return;
    if (!r)
        return;

    SelectionController* selection = m_frame->selection();

    if (selection->isNone()) {
        selection->setSelection(VisibleSelection(r));
        return;
    }

    RefPtr<Range> range = selection->selection().toNormalizedRange();
    ExceptionCode ec = 0;
    if (r->compareBoundaryPoints(Range::START_TO_START, range.get(), ec) == -1) {
        // We don't support discontiguous selection. We don't do anything if r and range don't intersect.
        if (r->compareBoundaryPoints(Range::START_TO_END, range.get(), ec) > -1) {
            if (r->compareBoundaryPoints(Range::END_TO_END, range.get(), ec) == -1)
                // The original range and r intersect.
                selection->setSelection(VisibleSelection(r->startPosition(), range->endPosition(), DOWNSTREAM));
            else
                // r contains the original range.
                selection->setSelection(VisibleSelection(r));
        }
    } else {
        // We don't support discontiguous selection. We don't do anything if r and range don't intersect.
        if (r->compareBoundaryPoints(Range::END_TO_START, range.get(), ec) < 1) {
            if (r->compareBoundaryPoints(Range::END_TO_END, range.get(), ec) == -1)
                // The original range contains r.
                selection->setSelection(VisibleSelection(range.get()));
            else
                // The original range and r intersect.
                selection->setSelection(VisibleSelection(range->startPosition(), r->endPosition(), DOWNSTREAM));
        }
    }
}

void DOMSelection::deleteFromDocument()
{
    if (!m_frame)
        return;

    SelectionController* selection = m_frame->selection();

    if (selection->isNone())
        return;

    if (isCollapsed())
        selection->modify(SelectionController::EXTEND, SelectionController::BACKWARD, CharacterGranularity);

    RefPtr<Range> selectedRange = selection->selection().toNormalizedRange();

    ExceptionCode ec = 0;
    selectedRange->deleteContents(ec);
    ASSERT(!ec);

    setBaseAndExtent(selectedRange->startContainer(ec), selectedRange->startOffset(ec), selectedRange->startContainer(ec), selectedRange->startOffset(ec), ec);
    ASSERT(!ec);
}

bool DOMSelection::containsNode(const Node* n, bool allowPartial) const
{
    if (!m_frame)
        return false;

    SelectionController* selection = m_frame->selection();

    if (!n || m_frame->document() != n->document() || selection->isNone())
        return false;

    Node* parentNode = n->parentNode();
    unsigned nodeIndex = n->nodeIndex();
    RefPtr<Range> selectedRange = selection->selection().toNormalizedRange();

    if (!parentNode)
        return false;

    ExceptionCode ec = 0;
    bool nodeFullySelected = Range::compareBoundaryPoints(parentNode, nodeIndex, selectedRange->startContainer(ec), selectedRange->startOffset(ec)) >= 0
        && Range::compareBoundaryPoints(parentNode, nodeIndex + 1, selectedRange->endContainer(ec), selectedRange->endOffset(ec)) <= 0;
    ASSERT(!ec);
    if (nodeFullySelected)
        return true;

    bool nodeFullyUnselected = Range::compareBoundaryPoints(parentNode, nodeIndex, selectedRange->endContainer(ec), selectedRange->endOffset(ec)) > 0
        || Range::compareBoundaryPoints(parentNode, nodeIndex + 1, selectedRange->startContainer(ec), selectedRange->startOffset(ec)) < 0;
    ASSERT(!ec);
    if (nodeFullyUnselected)
        return false;

    return allowPartial || n->isTextNode();
}

void DOMSelection::selectAllChildren(Node* n, ExceptionCode& ec)
{
    if (!n)
        return;

    // This doesn't (and shouldn't) select text node characters.
    setBaseAndExtent(n, 0, n, n->childNodeCount(), ec);
}

String DOMSelection::toString()
{
    if (!m_frame)
        return String();

    return plainText(m_frame->selection()->selection().toNormalizedRange().get());
}

bool DOMSelection::isValidForPosition(Node* node) const
{
    ASSERT(m_frame);
    if (!node)
        return true;
    return node->document() == m_frame->document();
}

} // namespace WebCore
