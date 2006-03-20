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

#include "config.h"
#include "ReplaceSelectionCommand.h"

#include "ApplyStyleCommand.h"
#include "BeforeTextInsertedEvent.h"
#include "DocumentFragment.h"
#include "Document.h"
#include "EditingText.h"
#include "Frame.h"
#include "HTMLElement.h"
#include "VisiblePosition.h"
#include "CSSComputedStyleDeclaration.h"
#include "css_valueimpl.h"
#include "CSSPropertyNames.h"
#include "dom2_eventsimpl.h"
#include "Range.h"
#include "Position.h"
#include "HTMLInterchange.h"
#include "htmlediting.h"
#include "HTMLNames.h"
#include "RenderObject.h"
#include "SelectionController.h"
#include "visible_units.h"
#include "TextIterator.h"
#include <kxmlcore/Assertions.h>

namespace WebCore {

using namespace HTMLNames;

ReplacementFragment::ReplacementFragment(Document *document, DocumentFragment *fragment, bool matchStyle)
    : m_document(document), 
      m_fragment(fragment), 
      m_matchStyle(matchStyle), 
      m_hasInterchangeNewlineAtStart(false), 
      m_hasInterchangeNewlineAtEnd(false), 
      m_hasMoreThanOneBlock(false)
{
    if (!m_document)
        return;

    if (!m_fragment) {
        m_type = EmptyFragment;
        return;
    }
    
    Node *firstChild = m_fragment->firstChild();
    Node *lastChild = m_fragment->lastChild();

    if (!firstChild) {
        m_type = EmptyFragment;
        return;
    }

    if (firstChild == lastChild && firstChild->isTextNode()) {
        m_type = SingleTextNodeFragment;
        return;
    }
    
    m_type = TreeFragment;

    Node *node = m_fragment->firstChild();
    Node *newlineAtStartNode = 0;
    Node *newlineAtEndNode = 0;
    while (node) {
        Node *next = node->traverseNextNode();
        if (isInterchangeNewlineNode(node)) {
            if (next || node == m_fragment->firstChild()) {
                m_hasInterchangeNewlineAtStart = true;
                newlineAtStartNode = node;
            }
            else {
                m_hasInterchangeNewlineAtEnd = true;
                newlineAtEndNode = node;
            }
        }
        else if (isInterchangeConvertedSpaceSpan(node)) {
            RefPtr<Node> n = 0;
            while ((n = node->firstChild())) {
                removeNode(n);
                insertNodeBefore(n.get(), node);
            }
            removeNode(node);
            if (n)
                next = n->traverseNextNode();
        }
        node = next;
    }

    if (newlineAtStartNode)
        removeNode(newlineAtStartNode);
    if (newlineAtEndNode)
        removeNode(newlineAtEndNode);
    
    RefPtr<Node> holder = insertFragmentForTestRendering();
    if (!m_matchStyle)
        computeStylesUsingTestRendering(holder.get());
    removeUnrenderedNodesUsingTestRendering(holder.get());
    m_hasMoreThanOneBlock = countRenderedBlocks(holder.get()) > 1;
    
    // Send khtmlBeforeTextInsertedEvent.  The event handler will update text if necessary.
    if (m_document->frame()) {
        Node* selectionStartNode = m_document->frame()->selection().start().node();
        if (selectionStartNode && selectionStartNode->rootEditableElement()) {
            RefPtr<Range> range = new Range(holder->getDocument());
            ExceptionCode ec = 0;
            range->selectNodeContents(holder.get(), ec);
            String text = plainText(range.get());
            String newText = text.copy();
            RefPtr<Event> evt = new BeforeTextInsertedEvent(newText);
            selectionStartNode->rootEditableElement()->dispatchEvent(evt, ec, true);
            if (text != newText) {
                // If the event handler has changed the text, create a new holder node for test rendering
                m_fragment->removeChildren();
                m_fragment->appendChild(new Text(m_document.get(), newText), ec);
                removeNode(holder);
                holder = insertFragmentForTestRendering();
                if (!m_matchStyle)
                    computeStylesUsingTestRendering(holder.get());
                removeUnrenderedNodesUsingTestRendering(holder.get());
                m_hasMoreThanOneBlock = countRenderedBlocks(holder.get()) > 1;
            }
        }
    }
    
    restoreTestRenderingNodesToFragment(holder.get());
    removeNode(holder);
    removeStyleNodes();
}

ReplacementFragment::~ReplacementFragment()
{
}

Node *ReplacementFragment::firstChild() const 
{ 
    return m_fragment->firstChild(); 
}

Node *ReplacementFragment::lastChild() const 
{ 
    return  m_fragment->lastChild(); 
}

static bool isProbablyBlock(const Node *node)
{
    if (!node)
        return false;
    
    // FIXME: This function seems really broken to me.  It isn't even including all the block-level elements.
    return (node->hasTagName(blockquoteTag) || node->hasTagName(ddTag) || node->hasTagName(divTag) ||
            node->hasTagName(dlTag) || node->hasTagName(dtTag) || node->hasTagName(h1Tag) ||
            node->hasTagName(h2Tag) || node->hasTagName(h3Tag) || node->hasTagName(h4Tag) ||
            node->hasTagName(h5Tag) || node->hasTagName(h6Tag) || node->hasTagName(hrTag) ||
            node->hasTagName(liTag) || node->hasTagName(olTag) || node->hasTagName(pTag) ||
            node->hasTagName(preTag) || node->hasTagName(tdTag) || node->hasTagName(thTag) ||
            node->hasTagName(ulTag));
}

static bool isMailPasteAsQuotationNode(const Node *node)
{
    return node && static_cast<const Element *>(node)->getAttribute("class") == ApplePasteAsQuotation;
}

Node *ReplacementFragment::mergeStartNode() const
{
    Node *node = m_fragment->firstChild();
    while (node && isProbablyBlock(node) && !isMailPasteAsQuotationNode(node))
        node = node->traverseNextNode();
    return node;
}

bool ReplacementFragment::isInterchangeNewlineNode(const Node *node)
{
    static String interchangeNewlineClassString(AppleInterchangeNewline);
    return node && node->hasTagName(brTag) && 
           static_cast<const Element *>(node)->getAttribute(classAttr) == interchangeNewlineClassString;
}

bool ReplacementFragment::isInterchangeConvertedSpaceSpan(const Node *node)
{
    static String convertedSpaceSpanClassString(AppleConvertedSpace);
    return node->isHTMLElement() && 
           static_cast<const HTMLElement *>(node)->getAttribute(classAttr) == convertedSpaceSpanClassString;
}

Node *ReplacementFragment::enclosingBlock(Node *node) const
{
    while (node && !isProbablyBlock(node))
        node = node->parentNode();    
    return node ? node : m_fragment.get();
}

void ReplacementFragment::removeNodePreservingChildren(Node *node)
{
    if (!node)
        return;

    while (RefPtr<Node> n = node->firstChild()) {
        removeNode(n);
        insertNodeBefore(n.get(), node);
    }
    removeNode(node);
}

void ReplacementFragment::removeNode(PassRefPtr<Node> node)
{
    if (!node)
        return;
    
    Node *parent = node->parentNode();
    if (!parent)
        return;
    
    ExceptionCode ec = 0;
    parent->removeChild(node.get(), ec);
    ASSERT(ec == 0);
}

void ReplacementFragment::insertNodeBefore(Node *node, Node *refNode)
{
    if (!node || !refNode)
        return;
        
    Node *parent = refNode->parentNode();
    if (!parent)
        return;
        
    ExceptionCode ec = 0;
    parent->insertBefore(node, refNode, ec);
    ASSERT(ec == 0);
}

PassRefPtr<Node> ReplacementFragment::insertFragmentForTestRendering()
{
    Node *body = m_document->body();
    if (!body)
        return 0;

    RefPtr<Element> holder = createDefaultParagraphElement(m_document.get());
    
    ExceptionCode ec = 0;
    holder->appendChild(m_fragment.get(), ec);
    ASSERT(ec == 0);
    
    body->appendChild(holder.get(), ec);
    ASSERT(ec == 0);
    
    m_document->updateLayoutIgnorePendingStylesheets();
    
    return holder.release();
}

void ReplacementFragment::restoreTestRenderingNodesToFragment(Node *holder)
{
    if (!holder)
        return;

    ExceptionCode ec = 0;
    while (RefPtr<Node> node = holder->firstChild()) {
        holder->removeChild(node.get(), ec);
        ASSERT(ec == 0);
        m_fragment->appendChild(node.get(), ec);
        ASSERT(ec == 0);
    }
}

static String &matchNearestBlockquoteColorString()
{
    static String matchNearestBlockquoteColorString = "match";
    return matchNearestBlockquoteColorString;
}

// FIXME: Move this somewhere so that the other editing operations can use it to clean up after themselves.
void ReplaceSelectionCommand::removeNodeAndPruneAncestors(Node* node)
{
    Node* parent = node->parentNode();
    removeNode(node);
    while (parent) {
        Node* nextParent = parent->parentNode();
        // If you change this rule you may have to add an updateLayout() here.
        if (parent->renderer() && parent->renderer()->firstChild())
            return;
        removeNode(parent);
        parent = nextParent;
    }
}

void ReplaceSelectionCommand::fixupNodeStyles(const DeprecatedValueList<NodeDesiredStyle> &list)
{
    // This function uses the mapped "desired style" to apply the additional style needed, if any,
    // to make the node have the desired style.

    updateLayout();

    DeprecatedValueListConstIterator<NodeDesiredStyle> it;
    for (it = list.begin(); it != list.end(); ++it) {
        Node *node = (*it).node();
        CSSMutableStyleDeclaration *desiredStyle = (*it).style();
        ASSERT(desiredStyle);

        if (!node->inDocument())
            continue;

        // The desiredStyle declaration tells what style this node wants to be.
        // Compare that to the style that it is right now in the document.
        Position pos(node, 0);
        RefPtr<CSSComputedStyleDeclaration> currentStyle = pos.computedStyle();

        // Check for the special "match nearest blockquote color" property and resolve to the correct
        // color if necessary.
        String matchColorCheck = desiredStyle->getPropertyValue(CSS_PROP__KHTML_MATCH_NEAREST_MAIL_BLOCKQUOTE_COLOR);
        if (matchColorCheck == matchNearestBlockquoteColorString()) {
            Node *blockquote = nearestMailBlockquote(node);
            Position pos(blockquote ? blockquote : node->getDocument()->documentElement(), 0);
            RefPtr<CSSComputedStyleDeclaration> style = pos.computedStyle();
            String desiredColor = desiredStyle->getPropertyValue(CSS_PROP_COLOR);
            String nearestColor = style->getPropertyValue(CSS_PROP_COLOR);
            if (desiredColor != nearestColor)
                desiredStyle->setProperty(CSS_PROP_COLOR, nearestColor);
        }
        desiredStyle->removeProperty(CSS_PROP__KHTML_MATCH_NEAREST_MAIL_BLOCKQUOTE_COLOR);

        currentStyle->diff(desiredStyle);
        
        // Only add in block properties if the node is at the start of a 
        // paragraph. This matches AppKit.
        if (!isStartOfParagraph(VisiblePosition(pos, DOWNSTREAM)))
            desiredStyle->removeBlockProperties();
        
        // If the desiredStyle is non-zero length, that means the current style differs
        // from the desired by the styles remaining in the desiredStyle declaration.
        if (desiredStyle->length() > 0)
            applyStyle(desiredStyle, Position(node, 0), Position(node, maxDeepOffset(node)));
    }
}

static void computeAndStoreNodeDesiredStyle(WebCore::Node *node, DeprecatedValueList<NodeDesiredStyle> &list)
{
    if (!node || !node->inDocument())
        return;
        
    RefPtr<CSSComputedStyleDeclaration> computedStyle = Position(node, 0).computedStyle();
    RefPtr<CSSMutableStyleDeclaration> style = computedStyle->copyInheritableProperties();
    list.append(NodeDesiredStyle(node, style));

    // In either of the color-matching tests below, set the color to a pseudo-color that will
    // make the content take on the color of the nearest-enclosing blockquote (if any) after
    // being pasted in.
    if (Node *blockquote = nearestMailBlockquote(node)) {
        RefPtr<CSSComputedStyleDeclaration> blockquoteStyle = Position(blockquote, 0).computedStyle();
        bool match = (blockquoteStyle->getPropertyValue(CSS_PROP_COLOR) == style->getPropertyValue(CSS_PROP_COLOR));
        if (match) {
            style->setProperty(CSS_PROP__KHTML_MATCH_NEAREST_MAIL_BLOCKQUOTE_COLOR, matchNearestBlockquoteColorString());
            return;
        }
    }
    Node *documentElement = node->getDocument()->documentElement();
    RefPtr<CSSComputedStyleDeclaration> documentStyle = Position(documentElement, 0).computedStyle();
    bool match = (documentStyle->getPropertyValue(CSS_PROP_COLOR) == style->getPropertyValue(CSS_PROP_COLOR));
    if (match)
        style->setProperty(CSS_PROP__KHTML_MATCH_NEAREST_MAIL_BLOCKQUOTE_COLOR, matchNearestBlockquoteColorString());
}

void ReplacementFragment::computeStylesUsingTestRendering(Node *holder)
{
    if (!holder)
        return;

    m_document->updateLayoutIgnorePendingStylesheets();

    for (Node *node = holder->firstChild(); node; node = node->traverseNextNode(holder))
        computeAndStoreNodeDesiredStyle(node, m_styles);
}

void ReplacementFragment::removeUnrenderedNodesUsingTestRendering(Node *holder)
{
    if (!holder)
        return;

    DeprecatedPtrList<Node> unrendered;

    for (Node *node = holder->firstChild(); node; node = node->traverseNextNode(holder)) {
        if (!isNodeRendered(node) && !isTableStructureNode(node))
            unrendered.append(node);
    }

    for (DeprecatedPtrListIterator<Node> it(unrendered); it.current(); ++it)
        removeNode(it.current());
}

int ReplacementFragment::countRenderedBlocks(Node *holder)
{
    if (!holder)
        return 0;
    
    int count = 0;
    Node *prev = 0;
    for (Node *node = holder->firstChild(); node; node = node->traverseNextNode(holder)) {
        if (node->isBlockFlow()) {
            if (!prev) {
                count++;
                prev = node;
            }
        } else {
            Node *block = node->enclosingBlockFlowElement();
            if (block != prev) {
                count++;
                prev = block;
            }
        }
    }
    
    return count;
}

void ReplacementFragment::removeStyleNodes()
{
    // Since style information has been computed and cached away in
    // computeStylesUsingTestRendering(), these style nodes can be removed, since
    // the correct styles will be added back in fixupNodeStyles().
    Node *node = m_fragment->firstChild();
    while (node) {
        Node *next = node->traverseNextNode();
        // This list of tags change the appearance of content
        // in ways we can add back on later with CSS, if necessary.
        //  FIXME: This list is incomplete
        if (node->hasTagName(bTag) || 
            node->hasTagName(bigTag) || 
            node->hasTagName(centerTag) || 
            node->hasTagName(fontTag) || 
            node->hasTagName(iTag) || 
            node->hasTagName(sTag) || 
            node->hasTagName(smallTag) || 
            node->hasTagName(strikeTag) || 
            node->hasTagName(subTag) || 
            node->hasTagName(supTag) || 
            node->hasTagName(ttTag) || 
            node->hasTagName(uTag) || 
            isStyleSpan(node)) {
            removeNodePreservingChildren(node);
        }
        // need to skip tab span because fixupNodeStyles() is not called
        // when replace is matching style
        else if (node->isHTMLElement() && !isTabSpanNode(node)) {
            HTMLElement *elem = static_cast<HTMLElement *>(node);
            CSSMutableStyleDeclaration *inlineStyleDecl = elem->inlineStyleDecl();
            if (inlineStyleDecl) {
                inlineStyleDecl->removeBlockProperties();
                inlineStyleDecl->removeInheritableProperties();
            }
        }
        node = next;
    }
}

NodeDesiredStyle::NodeDesiredStyle(PassRefPtr<Node> node, PassRefPtr<CSSMutableStyleDeclaration> style) 
    : m_node(node), m_style(style)
{
}

ReplaceSelectionCommand::ReplaceSelectionCommand(Document *document, DocumentFragment *fragment, bool selectReplacement, bool smartReplace, bool matchStyle) 
    : CompositeEditCommand(document), 
      m_fragment(document, fragment, matchStyle),
      m_selectReplacement(selectReplacement), 
      m_smartReplace(smartReplace),
      m_matchStyle(matchStyle)
{
}

ReplaceSelectionCommand::~ReplaceSelectionCommand()
{
}

void ReplaceSelectionCommand::doApply()
{
    // collect information about the current selection, prior to deleting the selection
    Selection selection = endingSelection();
    ASSERT(selection.isCaretOrRange());
    
    if (m_matchStyle)
        m_insertionStyle = styleAtPosition(selection.start());
    
    VisiblePosition visibleStart(selection.start(), selection.affinity());
    VisiblePosition visibleEnd(selection.end(), selection.affinity());
    bool startAtStartOfBlock = isStartOfBlock(visibleStart);
    bool startAtEndOfBlock = isEndOfBlock(visibleStart);
    bool startAtBlockBoundary = startAtStartOfBlock || startAtEndOfBlock;
    Node *startBlock = selection.start().node()->enclosingBlockFlowElement();
    Node *endBlock = selection.end().node()->enclosingBlockFlowElement();

    // decide whether to later merge content into the startBlock
    bool mergeStart = false;
    if (startBlock == startBlock->rootEditableElement() && startAtStartOfBlock && startAtEndOfBlock) {
        // empty editable subtree, need to mergeStart so that fragment ends up
        // inside the editable subtree rather than just before it
        // FIXME: Reconcile comment versus mergeStart = false
        mergeStart = false;
    } else {
        // merge if current selection starts inside a paragraph, or there is only one block and no interchange newline to add
        mergeStart = !m_fragment.hasInterchangeNewlineAtStart() && 
            (!isStartOfParagraph(visibleStart) || (!m_fragment.hasInterchangeNewlineAtEnd() && !m_fragment.hasMoreThanOneBlock()));
        
        // This is a workaround for this bug:
        // <rdar://problem/4013642> Copied quoted word does not paste as a quote if pasted at the start of a line
        // We need more powerful logic in this whole mergeStart code for this case to come out right without
        // breaking other cases.
        if (isStartOfParagraph(visibleStart) && isMailBlockquote(m_fragment.firstChild()))
            mergeStart = false;
        
        // prevent first list item from getting merged into target, thereby pulled out of list
        // NOTE: ideally, we'd check for "first visible position in list" here,
        // but we cannot.  Fragments do not have any visible positions.  Instead, we
        // assume that the mergeStartNode() contains the first visible content to paste.
        // Any better ideas?
        if (mergeStart) {
            for (Node *n = m_fragment.mergeStartNode(); n; n = n->parentNode()) {
                if (isListElement(n)) {
                    mergeStart = false;
                    break;
                }
            }
        }
    }
    
    // decide whether to later append nodes to the end
    Node *beyondEndNode = 0;
    if (!isEndOfParagraph(visibleEnd) && !m_fragment.hasInterchangeNewlineAtEnd() &&
       (startBlock != endBlock || m_fragment.hasMoreThanOneBlock()))
        beyondEndNode = selection.end().downstream().node();

    Position startPos = selection.start();
    
    // delete the current range selection, or insert paragraph for caret selection, as needed
    if (selection.isRange()) {
        bool mergeBlocksAfterDelete = !(m_fragment.hasInterchangeNewlineAtStart() || m_fragment.hasInterchangeNewlineAtEnd() || m_fragment.hasMoreThanOneBlock());
        deleteSelection(false, mergeBlocksAfterDelete);
        updateLayout();
        visibleStart = VisiblePosition(endingSelection().start(), VP_DEFAULT_AFFINITY);
        if (m_fragment.hasInterchangeNewlineAtStart()) {
            if (isEndOfParagraph(visibleStart) && !isStartOfParagraph(visibleStart)) {
                if (!isEndOfDocument(visibleStart))
                    setEndingSelection(visibleStart.next());
            } else {
                insertParagraphSeparator();
                setEndingSelection(VisiblePosition(endingSelection().start(), VP_DEFAULT_AFFINITY));
            }
        }
        startPos = endingSelection().start();
    } 
    else {
        ASSERT(selection.isCaret());
        if (m_fragment.hasInterchangeNewlineAtStart()) {
            if (isEndOfParagraph(visibleStart) && !isStartOfParagraph(visibleStart)) {
                if (!isEndOfDocument(visibleStart))
                    setEndingSelection(visibleStart.next());
            } else {
                insertParagraphSeparator();
                setEndingSelection(VisiblePosition(endingSelection().start(), VP_DEFAULT_AFFINITY));
            }
        }
        if (!m_fragment.hasInterchangeNewlineAtEnd() && m_fragment.hasMoreThanOneBlock() && 
            !startAtBlockBoundary && !isEndOfParagraph(visibleEnd)) {
            // The start and the end need to wind up in separate blocks.
            // Insert a paragraph separator to make that happen.
            insertParagraphSeparator();
            setEndingSelection(VisiblePosition(endingSelection().start(), VP_DEFAULT_AFFINITY).previous());
        }
        startPos = endingSelection().start();
    }

    if (startAtStartOfBlock && startBlock->inDocument())
        startPos = Position(startBlock, 0);

    // paste into run of tabs splits the tab span
    startPos = positionOutsideTabSpan(startPos);
    
    // paste at start or end of link goes outside of link
    startPos = positionAvoidingSpecialElementBoundary(startPos);

    Frame *frame = document()->frame();
    
    // FIXME: Improve typing style.
    // See this bug: <rdar://problem/3769899> Implementation of typing style needs improvement
    frame->clearTypingStyle();
    setTypingStyle(0);    
    
    // done if there is nothing to add
    if (!m_fragment.firstChild())
        return;
    
    // check for a line placeholder, and store it away for possible removal later.
    Node *block = startPos.node()->enclosingBlockFlowElement();
    Node *linePlaceholder = findBlockPlaceholder(block);
    if (!linePlaceholder) {
        Position downstream = startPos.downstream();
        // NOTE: the check for brTag offset 0 could be false negative after
        // positionAvoidingSpecialElementBoundary() because "downstream" is
        // now a "second deepest position"
        downstream = positionAvoidingSpecialElementBoundary(downstream);
        if (downstream.node()->hasTagName(brTag) && downstream.offset() == 0 && 
            m_fragment.hasInterchangeNewlineAtEnd() &&
            isStartOfLine(VisiblePosition(downstream, VP_DEFAULT_AFFINITY)))
            linePlaceholder = downstream.node();
    }
    
    // check whether to "smart replace" needs to add leading and/or trailing space
    bool addLeadingSpace = false;
    bool addTrailingSpace = false;
    // FIXME: We need the affinity for startPos and endPos, but Position::downstream
    // and Position::upstream do not give it
    if (m_smartReplace) {
        VisiblePosition visiblePos = VisiblePosition(startPos, VP_DEFAULT_AFFINITY);
        assert(visiblePos.isNotNull());
        addLeadingSpace = startPos.leadingWhitespacePosition(VP_DEFAULT_AFFINITY, true).isNull() && !isStartOfLine(visiblePos);
        if (addLeadingSpace) {
            QChar previousChar = visiblePos.previous().character();
            if (!previousChar.isNull()) {
                addLeadingSpace = !frame->isCharacterSmartReplaceExempt(previousChar, true);
            }
        }
        addTrailingSpace = startPos.trailingWhitespacePosition(VP_DEFAULT_AFFINITY, true).isNull() && !isEndOfLine(visiblePos);
        if (addTrailingSpace) {
            QChar thisChar = visiblePos.character();
            if (!thisChar.isNull()) {
                addTrailingSpace = !frame->isCharacterSmartReplaceExempt(thisChar, false);
            }
        }
    }
    
    // There are five steps to adding the content: merge blocks at start, add remaining blocks,
    // add "smart replace" space, handle trailing newline, clean up.
    
    // initially, we say the insertion point is the start of selection
    updateLayout();
    Position insertionPos = startPos;

    // step 1: merge content into the start block
    if (mergeStart) {
        Node *refNode = m_fragment.mergeStartNode();
        if (refNode) {
            Node *parent = refNode->parentNode();
            Node *node = refNode->nextSibling();
            insertNodeAtAndUpdateNodesInserted(refNode, startPos.node(), startPos.offset());
            while (node && !isProbablyBlock(node)) {
                Node *next = node->nextSibling();
                insertNodeAfterAndUpdateNodesInserted(node, refNode);
                refNode = node;
                node = next;
            }

            // remove any ancestors we emptied, except the root itself which cannot be removed
            while (parent && parent->parentNode() && parent->childNodeCount() == 0) {
                Node *nextParent = parent->parentNode();
                removeNode(parent);
                parent = nextParent;
            }
        }
        
        // update insertion point to be at the end of the last block inserted
        if (m_lastNodeInserted) {
            updateLayout();
            insertionPos = Position(m_lastNodeInserted.get(), m_lastNodeInserted->caretMaxOffset());
        }
    }
    
    // step 2 : merge everything remaining in the fragment
    if (m_fragment.firstChild()) {
        Node *refNode = m_fragment.firstChild();
        Node *node = refNode ? refNode->nextSibling() : 0;
        Node *insertionBlock = insertionPos.node()->enclosingBlockFlowElement();
        bool insertionBlockIsRoot = insertionBlock == insertionBlock->rootEditableElement();
        VisiblePosition visiblePos(insertionPos, DOWNSTREAM);
        if (!insertionBlockIsRoot && isProbablyBlock(refNode) && isStartOfBlock(visiblePos))
            insertNodeBeforeAndUpdateNodesInserted(refNode, insertionBlock);
        else if (!insertionBlockIsRoot && isProbablyBlock(refNode) && isEndOfBlock(visiblePos)) {
            insertNodeAfterAndUpdateNodesInserted(refNode, insertionBlock);
        // Insert the rest of the fragment at the NEXT visible position ONLY IF part of the fragment was already merged AND !isProbablyBlock
        } else if (m_lastNodeInserted && !isProbablyBlock(refNode)) {
            Position pos = visiblePos.next().deepEquivalent().downstream();
            insertNodeAtAndUpdateNodesInserted(refNode, pos.node(), pos.offset());
        } else {
            insertNodeAtAndUpdateNodesInserted(refNode, insertionPos.node(), insertionPos.offset());
        }
        
        while (node) {
            Node *next = node->nextSibling();
            insertNodeAfterAndUpdateNodesInserted(node, refNode);
            refNode = node;
            node = next;
        }
        updateLayout();
        insertionPos = Position(m_lastNodeInserted.get(), m_lastNodeInserted->caretMaxOffset());
    }

    // step 3 : handle "smart replace" whitespace
    if (addTrailingSpace && m_lastNodeInserted) {
        updateLayout();
        Position pos(m_lastNodeInserted.get(), m_lastNodeInserted->caretMaxOffset());
        bool needsTrailingSpace = pos.trailingWhitespacePosition(VP_DEFAULT_AFFINITY, true).isNull();
        if (needsTrailingSpace) {
            if (m_lastNodeInserted->isTextNode()) {
                Text *text = static_cast<Text *>(m_lastNodeInserted.get());
                insertTextIntoNode(text, text->length(), nonBreakingSpaceString());
                insertionPos = Position(text, text->length());
            }
            else {
                RefPtr<Node> node = document()->createEditingTextNode(nonBreakingSpaceString());
                insertNodeAfterAndUpdateNodesInserted(node.get(), m_lastNodeInserted.get());
                insertionPos = Position(node.get(), 1);
            }
        }
    }

    if (addLeadingSpace && m_firstNodeInserted) {
        updateLayout();
        Position pos(m_firstNodeInserted.get(), 0);
        bool needsLeadingSpace = pos.leadingWhitespacePosition(VP_DEFAULT_AFFINITY, true).isNull();
        if (needsLeadingSpace) {
            if (m_firstNodeInserted->isTextNode()) {
                Text *text = static_cast<Text *>(m_firstNodeInserted.get());
                insertTextIntoNode(text, 0, nonBreakingSpaceString());
            } else {
                RefPtr<Node> node = document()->createEditingTextNode(nonBreakingSpaceString());
                insertNodeBeforeAndUpdateNodesInserted(node.get(), m_firstNodeInserted.get());
            }
        }
    }
    
    Position lastPositionToSelect;

    // step 4 : handle trailing newline
    if (m_fragment.hasInterchangeNewlineAtEnd()) {
        removeLinePlaceholderIfNeeded(linePlaceholder);

        if (!m_lastNodeInserted) {
            lastPositionToSelect = endingSelection().end().downstream();
        }
        else {
            bool insertParagraph = false;
            VisiblePosition pos(insertionPos, VP_DEFAULT_AFFINITY);

            if (startBlock == endBlock && !isProbablyBlock(m_lastTopNodeInserted.get())) {
                insertParagraph = true;
            } else {
                // Handle end-of-document case.
                updateLayout();
                if (isEndOfDocument(pos))
                    insertParagraph = true;
            }
            if (insertParagraph) {
                setEndingSelection(insertionPos, DOWNSTREAM);
                insertParagraphSeparator();
                VisiblePosition next = pos.next();

                // Select up to the paragraph separator that was added.
                lastPositionToSelect = next.deepEquivalent().downstream();
                updateNodesInserted(lastPositionToSelect.node());
            } else {
                // Select up to the preexising paragraph separator.
                VisiblePosition next = pos.next();
                lastPositionToSelect = next.deepEquivalent().downstream();
            }
        }
    } else {
        if (m_lastNodeInserted && m_lastNodeInserted->hasTagName(brTag) && !document()->inStrictMode()) {
            updateLayout();
            VisiblePosition pos(Position(m_lastNodeInserted.get(), 1), DOWNSTREAM);
            if (isEndOfBlock(pos)) {
                Node *next = m_lastNodeInserted->traverseNextNode();
                bool hasTrailingBR = next && next->hasTagName(brTag) && m_lastNodeInserted->enclosingBlockFlowElement() == next->enclosingBlockFlowElement();
                if (!hasTrailingBR) {
                    // Insert an "extra" BR at the end of the block. 
                    insertNodeBefore(createBreakElement(document()).get(), m_lastNodeInserted.get());
                }
            }
        }

        if (beyondEndNode) {
            updateLayout();
            DeprecatedValueList<NodeDesiredStyle> styles;
            Node* node = beyondEndNode->enclosingInlineElement();
            Node* refNode = m_lastNodeInserted.get();
            
            while (node) {
                if (node->isBlockFlowOrBlockTable())
                    break;
                    
                Node *next = node->nextSibling();
                computeAndStoreNodeDesiredStyle(node, styles);
                removeNodeAndPruneAncestors(node);
                // No need to update inserted node variables.
                insertNodeAfter(node, refNode);
                refNode = node;
                // We want to move the first BR we see, so check for that here.
                if (node->hasTagName(brTag))
                    break;
                node = next;
            }

            fixupNodeStyles(styles);
        }
    }
    
    if (!m_matchStyle)
        fixupNodeStyles(m_fragment.desiredStyles());
    completeHTMLReplacement(lastPositionToSelect);
    
    // step 5 : mop up
    removeLinePlaceholderIfNeeded(linePlaceholder);
}

void ReplaceSelectionCommand::removeLinePlaceholderIfNeeded(Node *linePlaceholder)
{
    if (!linePlaceholder)
        return;
        
    updateLayout();
    if (linePlaceholder->inDocument()) {
        VisiblePosition placeholderPos(linePlaceholder, linePlaceholder->renderer()->caretMinOffset(), DOWNSTREAM);
        if (placeholderPos.next().isNull() ||
            !(isStartOfLine(placeholderPos) && isEndOfLine(placeholderPos))) {
            
            removeNodeAndPruneAncestors(linePlaceholder);
        }
    }
}

void ReplaceSelectionCommand::completeHTMLReplacement(const Position &lastPositionToSelect)
{
    Position start;
    Position end;

    if (m_firstNodeInserted && m_firstNodeInserted->inDocument() && m_lastNodeInserted && m_lastNodeInserted->inDocument()) {
        // Find the last leaf.
        Node *lastLeaf = m_lastNodeInserted.get();
        while (1) {
            Node *nextChild = lastLeaf->lastChild();
            if (!nextChild)
                break;
            lastLeaf = nextChild;
        }
    
        // Find the first leaf.
        Node *firstLeaf = m_firstNodeInserted.get();
        while (1) {
            Node *nextChild = firstLeaf->firstChild();
            if (!nextChild)
                break;
            firstLeaf = nextChild;
        }
        
        // Call updateLayout so caretMinOffset and caretMaxOffset return correct values.
        updateLayout();
        start = Position(firstLeaf, firstLeaf->caretMinOffset());
        end = Position(lastLeaf, lastLeaf->caretMaxOffset());

        if (m_matchStyle) {
            assert(m_insertionStyle);
            applyStyle(m_insertionStyle.get(), start, end);
        }    
        
        if (lastPositionToSelect.isNotNull())
            end = lastPositionToSelect;
    } else if (lastPositionToSelect.isNotNull())
        start = end = lastPositionToSelect;
    else
        return;
    
    if (m_selectReplacement)
        setEndingSelection(Selection(start, end, SEL_DEFAULT_AFFINITY));
    else
        setEndingSelection(end, SEL_DEFAULT_AFFINITY);
    
    rebalanceWhitespace();
}

EditAction ReplaceSelectionCommand::editingAction() const
{
    return EditActionPaste;
}

void ReplaceSelectionCommand::insertNodeAfterAndUpdateNodesInserted(Node *insertChild, Node *refChild)
{
    insertNodeAfter(insertChild, refChild);
    updateNodesInserted(insertChild);
}

void ReplaceSelectionCommand::insertNodeAtAndUpdateNodesInserted(Node *insertChild, Node *refChild, int offset)
{
    insertNodeAt(insertChild, refChild, offset);
    updateNodesInserted(insertChild);
}

void ReplaceSelectionCommand::insertNodeBeforeAndUpdateNodesInserted(Node *insertChild, Node *refChild)
{
    insertNodeBefore(insertChild, refChild);
    updateNodesInserted(insertChild);
}

void ReplaceSelectionCommand::updateNodesInserted(Node *node)
{
    if (!node)
        return;

    m_lastTopNodeInserted = node;
    if (!m_firstNodeInserted)
        m_firstNodeInserted = node;
    
    if (node == m_lastNodeInserted)
        return;
    
    m_lastNodeInserted = node->lastDescendant();
}

} // namespace WebCore
