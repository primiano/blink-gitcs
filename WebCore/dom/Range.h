/*
 * (C) 1999 Lars Knoll (knoll@kde.org)
 * (C) 2000 Gunnstein Lye (gunnstein@netcom.no)
 * (C) 2000 Frederik Holljen (frederik.holljen@hig.no)
 * (C) 2001 Peter Kelly (pmk@post.com)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef Range_h
#define Range_h

#include "FloatQuad.h"
#include "RangeBoundaryPoint.h"
#include <wtf/Forward.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class ClientRect;
class ClientRectList;
class DocumentFragment;
class NodeWithIndex;
class Text;

class Range : public RefCounted<Range> {
public:
    static PassRefPtr<Range> create(PassRefPtr<Document>);
    static PassRefPtr<Range> create(PassRefPtr<Document>, PassRefPtr<Node> startContainer, int startOffset, PassRefPtr<Node> endContainer, int endOffset);
    static PassRefPtr<Range> create(PassRefPtr<Document>, const Position&, const Position&);
    ~Range();

    Document* ownerDocument() const { return m_ownerDocument.get(); }
    Node* startContainer() const { return m_start.container(); }
    int startOffset() const { return m_start.offset(); }
    Node* endContainer() const { return m_end.container(); }
    int endOffset() const { return m_end.offset(); }

    Node* startContainer(ExceptionCode&) const;
    int startOffset(ExceptionCode&) const;
    Node* endContainer(ExceptionCode&) const;
    int endOffset(ExceptionCode&) const;
    bool collapsed(ExceptionCode&) const;

    Node* commonAncestorContainer(ExceptionCode&) const;
    static Node* commonAncestorContainer(Node* containerA, Node* containerB);
    void setStart(PassRefPtr<Node> container, int offset, ExceptionCode&);
    void setEnd(PassRefPtr<Node> container, int offset, ExceptionCode&);
    void collapse(bool toStart, ExceptionCode&);
    bool isPointInRange(Node* refNode, int offset, ExceptionCode&);
    short comparePoint(Node* refNode, int offset, ExceptionCode&) const;
    enum CompareResults { NODE_BEFORE, NODE_AFTER, NODE_BEFORE_AND_AFTER, NODE_INSIDE };
    CompareResults compareNode(Node* refNode, ExceptionCode&) const;
    enum CompareHow { START_TO_START, START_TO_END, END_TO_END, END_TO_START };
    short compareBoundaryPoints(CompareHow, const Range* sourceRange, ExceptionCode&) const;
    static short compareBoundaryPoints(Node* containerA, int offsetA, Node* containerB, int offsetB);
    static short compareBoundaryPoints(const RangeBoundaryPoint& boundaryA, const RangeBoundaryPoint& boundaryB);
    bool boundaryPointsValid() const;
    bool intersectsNode(Node* refNode, ExceptionCode&);
    void deleteContents(ExceptionCode&);
    PassRefPtr<DocumentFragment> extractContents(ExceptionCode&);
    PassRefPtr<DocumentFragment> cloneContents(ExceptionCode&);
    void insertNode(PassRefPtr<Node>, ExceptionCode&);
    String toString(ExceptionCode&) const;

    String toHTML() const;
    String text() const;

    PassRefPtr<DocumentFragment> createContextualFragment(const String& html, ExceptionCode&) const;

    void detach(ExceptionCode&);
    PassRefPtr<Range> cloneRange(ExceptionCode&) const;

    void setStartAfter(Node*, ExceptionCode&);
    void setEndBefore(Node*, ExceptionCode&);
    void setEndAfter(Node*, ExceptionCode&);
    void selectNode(Node*, ExceptionCode&);
    void selectNodeContents(Node*, ExceptionCode&);
    void surroundContents(PassRefPtr<Node>, ExceptionCode&);
    void setStartBefore(Node*, ExceptionCode&);

    const Position startPosition() const { return m_start.toPosition(); }
    const Position endPosition() const { return m_end.toPosition(); }

    Node* firstNode() const;
    Node* pastLastNode() const;

    Position editingStartPosition() const;

    Node* shadowTreeRootNode() const;

    IntRect boundingBox();
    // Not transform-friendly
    void textRects(Vector<IntRect>&, bool useSelectionHeight = false);
    // Transform-friendly
    void textQuads(Vector<FloatQuad>&, bool useSelectionHeight = false);

    void nodeChildrenChanged(ContainerNode*);
    void nodeChildrenWillBeRemoved(ContainerNode*);
    void nodeWillBeRemoved(Node*);

    void textInserted(Node*, unsigned offset, unsigned length);
    void textRemoved(Node*, unsigned offset, unsigned length);
    void textNodesMerged(NodeWithIndex& oldNode, unsigned offset);
    void textNodeSplit(Text* oldNode);

    // Expand range to a unit (word or sentence or block or document) boundary.
    // Please refer to https://bugs.webkit.org/show_bug.cgi?id=27632 comment #5 
    // for details.
    void expand(const String&, ExceptionCode&);

    PassRefPtr<ClientRectList> getClientRects() const;
    PassRefPtr<ClientRect> getBoundingClientRect() const;

#ifndef NDEBUG
    void formatForDebugger(char* buffer, unsigned length) const;
#endif

private:
    Range(PassRefPtr<Document>);
    Range(PassRefPtr<Document>, PassRefPtr<Node> startContainer, int startOffset, PassRefPtr<Node> endContainer, int endOffset);

    Node* checkNodeWOffset(Node*, int offset, ExceptionCode&) const;
    void checkNodeBA(Node*, ExceptionCode&) const;
    void checkDeleteExtract(ExceptionCode&);
    bool containedByReadOnly() const;
    int maxStartOffset() const;
    int maxEndOffset() const;

    enum ActionType { DELETE_CONTENTS, EXTRACT_CONTENTS, CLONE_CONTENTS };
    PassRefPtr<DocumentFragment> processContents(ActionType, ExceptionCode&);

    void getBorderAndTextQuads(Vector<FloatQuad>&) const;

    RefPtr<Document> m_ownerDocument;
    RangeBoundaryPoint m_start;
    RangeBoundaryPoint m_end;
};

PassRefPtr<Range> rangeOfContents(Node*);

bool areRangesEqual(const Range*, const Range*);

} // namespace

#ifndef NDEBUG
// Outside the WebCore namespace for ease of invocation from gdb.
void showTree(const WebCore::Range*);
#endif

#endif
