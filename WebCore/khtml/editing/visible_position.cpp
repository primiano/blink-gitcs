/*
 * Copyright (C) 2004 Apple Computer, Inc.  All rights reserved.
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

#include "visible_position.h"

#include "misc/htmltags.h"
#include "rendering/render_text.h"
#include "xml/dom2_rangeimpl.h"

#if APPLE_CHANGES
#include "KWQAssertions.h"
#include "KWQLogging.h"
#else
#define ASSERT(assertion) assert(assertion)
#define LOG(channel, formatAndArgs...) ((void)0)
#endif

using khtml::InlineTextBox;
using khtml::RenderObject;
using khtml::RenderText;
using khtml::VISIBLE;

namespace DOM {

CaretPosition::CaretPosition(NodeImpl *node, long offset)
{
    init(Position(node, offset));
}

CaretPosition::CaretPosition(const Position &pos)
{
    init(pos);
}

void CaretPosition::init(const Position &pos)
{
    Position deepPos = deepEquivalent(pos);
    
    if (isCandidate(deepPos)) {
        m_deepPosition = deepPos;
        Position previous = previousCaretPosition(deepPos);
        if (previous.isNotNull()) {
            Position next = nextCaretPosition(previous);
            if (next.isNotNull())
                m_deepPosition = next;
        }
    }
    else {
        Position next = nextCaretPosition(deepPos);
        if (next.isNotNull()) {
            m_deepPosition = next;
        }
        else {
            Position previous = previousCaretPosition(deepPos);
            if (previous.isNotNull())
                m_deepPosition = previous;
        }
    }
}

bool CaretPosition::isLastInBlock() const
{
    if (isNull())
        return false;
        
    CaretPosition n = next();
    return n.isNull() || (n.deepEquivalent().node()->enclosingBlockFlowElement() != m_deepPosition.node()->enclosingBlockFlowElement());
}

CaretPosition CaretPosition::next() const
{
    CaretPosition result;
    result.m_deepPosition = nextCaretPosition(m_deepPosition);
    return result;
}

CaretPosition CaretPosition::previous() const
{
    CaretPosition result;
    result.m_deepPosition = previousCaretPosition(m_deepPosition);
    return result;
}

Position CaretPosition::previousCaretPosition(const Position &pos)
{
    if (pos.isNull() || atStart(pos))
        return Position();

    Position test = deepEquivalent(pos);
    bool acceptAnyCaretPosition = !isCandidate(test) || pos.isFirstRenderedPositionOnLine();

    Position current = test;
    while (!atStart(current)) {
        current = previousPosition(current);
        if (isCandidate(current) && (acceptAnyCaretPosition || current.rendersInDifferentPosition(test))) {
            return current;
        }
    }
    
    return Position();
}

Position CaretPosition::nextCaretPosition(const Position &pos)
{
    if (pos.isNull() || atEnd(pos))
        return Position();

    Position test = deepEquivalent(pos);
    bool acceptAnyCaretPosition = !isCandidate(test) || pos.isLastRenderedPositionOnLine();

    Position current = test;
    while (!atEnd(current)) {
        current = nextPosition(current);
        if (isCandidate(current) && (acceptAnyCaretPosition || current.rendersInDifferentPosition(test))) {
            return current;
        }
    }
    
    return Position();
}

Position CaretPosition::previousPosition(const Position &pos)
{
    if (pos.isNull())
        return pos;
    
    Position result;

    if (pos.offset() <= 0) {
        NodeImpl *prevNode = pos.node()->previousLeafNode();
        if (prevNode)
            result = Position(prevNode, prevNode->maxOffset());
    }
    else {
        result = Position(pos.node(), pos.offset() - 1);
    }
    
    return result;
}

Position CaretPosition::nextPosition(const Position &pos)
{
    if (pos.isNull())
        return pos;
    
    Position result;

    if (pos.offset() >= pos.node()->maxOffset()) {
        NodeImpl *nextNode = pos.node()->nextLeafNode();
        if (nextNode)
            result = Position(nextNode, 0);
    }
    else {
        result = Position(pos.node(), pos.offset() + 1);
    }
    
    return result;
}

bool CaretPosition::atStart(const Position &pos)
{
    if (pos.isNull())
        return true;

    return pos.offset() <= 0 && pos.node()->previousLeafNode() == 0;
}

bool CaretPosition::atEnd(const Position &pos)
{
    if (pos.isNull())
        return true;

    return pos.offset() >= pos.node()->maxOffset() && pos.node()->nextLeafNode() == 0;
}

bool CaretPosition::isCandidate(const Position &pos)
{
    if (pos.isNull())
        return false;
        
    RenderObject *renderer = pos.node()->renderer();
    if (!renderer)
        return false;
    
    if (renderer->style()->visibility() != VISIBLE)
        return false;

    if (renderer->isReplaced())
        // return true for replaced elements
        return pos.offset() == 0 || pos.offset() == 1;

    if (renderer->isBR() && static_cast<RenderText *>(renderer)->firstTextBox())
        // return true for offset 0 into BR element on a line by itself
        return pos.offset() == 0;
    
    if (renderer->isText()) {
        RenderText *textRenderer = static_cast<RenderText *>(renderer);
        for (InlineTextBox *box = textRenderer->firstTextBox(); box; box = box->nextTextBox()) {
            if (pos.offset() >= box->m_start && pos.offset() <= box->m_start + box->m_len) {
                // return true if in a text node
                return true;
            }
            else if (pos.offset() < box->m_start) {
                // The offset we're looking for is before this node
                // this means the offset must be in content that is
                // not rendered. Return false.
                return false;
            }
        }
    }
    
    if (renderer->isBlockFlow() && !renderer->firstChild() && renderer->height())
        // return true for offset 0 into rendered blocks that are empty of rendered kids, but have a height
        return pos.offset() == 0;
    
    return false;
}

Position CaretPosition::deepEquivalent(const Position &pos)
{
    NodeImpl *node = pos.node();
    long offset = pos.offset();

    if (!node)
        return Position();
    
    if (isAtomicNode(node)) {
        // This is part of the strategy to wean the code off Positions with BRs and replaced elements
        // as the nodes and offsets > 0.
        if (offset > 0 && (node->id() == ID_BR || node->renderer() && node->renderer()->isReplaced())) {
            NodeImpl *next = node->traverseNextNode();
            if (next && node->enclosingBlockFlowElement() == next->enclosingBlockFlowElement())
                return deepEquivalent(Position(next, 0));
        }
        return pos;
    }

    if (offset >= (long)node->childNodeCount()) {
        do {
            NodeImpl *child = node->lastChild();
            if (!child)
                break;
            node = child;
        } while (!isAtomicNode(node));
        return Position(node, node->caretMaxOffset());
    }
    
    node = node->childNode(offset);
    ASSERT(node);
    while (!isAtomicNode(node)) {
        NodeImpl *child = node->firstChild();
        if (!child)
            break;
        node = child;
    }
    return Position(node, 0);
}

Position CaretPosition::rangeCompliantEquivalent(const Position &pos)
{
    NodeImpl *node = pos.node();
    if (!node)
        return Position();
    
    // FIXME: This clamps out-of-range values.
    // Instead we should probably assert, and not use such values.

    long offset = pos.offset();
    if (!offsetInCharacters(node->nodeType()) && isAtomicNode(node) && offset > 0)
        return Position(node->parentNode(), node->nodeIndex() + 1);

    return Position(node, kMax(0L, kMin(offset, maxOffset(node))));
}

long CaretPosition::maxOffset(const NodeImpl *node)
{
    return offsetInCharacters(node->nodeType()) ? (long)static_cast<const CharacterDataImpl *>(node)->length() : (long)node->childNodeCount();
}

bool CaretPosition::isAtomicNode(const NodeImpl *node)
{
    return node && (!node->hasChildNodes() || (node->id() == ID_OBJECT && node->renderer() && node->renderer()->isReplaced()));
}

void CaretPosition::debugPosition(const char *msg) const
{
    if (isNull())
        fprintf(stderr, "Position [%s]: null\n", msg);
    else
        fprintf(stderr, "Position [%s]: %s [%p] at %d\n", msg, getTagName(m_deepPosition.node()->id()).string().latin1(), m_deepPosition.node(), m_deepPosition.offset());
}

#ifndef NDEBUG
void CaretPosition::formatForDebugger(char *buffer, unsigned length) const
{
    m_deepPosition.formatForDebugger(buffer, length);
}
#endif

Range makeRange(const CaretPosition &start, const CaretPosition &end)
{
    Position s = start.position();
    Position e = end.position();
    return Range(s.node(), s.offset(), e.node(), e.offset());
}

CaretPosition start(const Range &r)
{
    return CaretPosition(r.startContainer().handle(), r.startOffset());
}

CaretPosition end(const Range &r)
{
    return CaretPosition(r.endContainer().handle(), r.endOffset());
}

bool setStart(Range &r, const CaretPosition &c)
{
    RangeImpl *ri = r.handle();
    if (!ri)
        return false;
    Position p = c.position();
    int code = 0;
    ri->setStart(p.node(), p.offset(), code);
    return code == 0;
}

bool setEnd(Range &r, const CaretPosition &c)
{
    RangeImpl *ri = r.handle();
    if (!ri)
        return false;
    Position p = c.position();
    int code = 0;
    ri->setEnd(p.node(), p.offset(), code);
    return code == 0;
}

}  // namespace DOM
