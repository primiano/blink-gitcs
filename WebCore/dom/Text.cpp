/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
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
 */

#include "config.h"
#include "Text.h"

#include "ExceptionCode.h"
#include "RenderText.h"
#include "TextBreakIterator.h"
#include <wtf/text/CString.h>

#if ENABLE(SVG)
#include "RenderSVGInlineText.h"
#include "SVGNames.h"
#endif

#if ENABLE(WML)
#include "WMLDocument.h"
#include "WMLVariables.h"
#endif

using namespace std;

namespace WebCore {

Text::Text(Document* document, const String& data)
    : CharacterData(document, data, CreateText)
{
}

PassRefPtr<Text> Text::create(Document* document, const String& data)
{
    return adoptRef(new Text(document, data));
}

PassRefPtr<Text> Text::splitText(unsigned offset, ExceptionCode& ec)
{
    ec = 0;

    // INDEX_SIZE_ERR: Raised if the specified offset is negative or greater than
    // the number of 16-bit units in data.
    if (offset > length()) {
        ec = INDEX_SIZE_ERR;
        return 0;
    }

    RefPtr<StringImpl> oldStr = dataImpl();
    RefPtr<Text> newText = virtualCreate(oldStr->substring(offset));
    setDataImpl(oldStr->substring(0, offset));

    dispatchModifiedEvent(oldStr.get());

    if (parentNode())
        parentNode()->insertBefore(newText.get(), nextSibling(), ec);
    if (ec)
        return 0;

    if (parentNode())
        document()->textNodeSplit(this);

    if (renderer())
        toRenderText(renderer())->setText(dataImpl());

    return newText.release();
}

static const Text* earliestLogicallyAdjacentTextNode(const Text* t)
{
    const Node* n = t;
    while ((n = n->previousSibling())) {
        Node::NodeType type = n->nodeType();
        if (type == Node::TEXT_NODE || type == Node::CDATA_SECTION_NODE) {
            t = static_cast<const Text*>(n);
            continue;
        }

        // We would need to visit EntityReference child text nodes if they existed
        ASSERT(type != Node::ENTITY_REFERENCE_NODE || !n->hasChildNodes());
        break;
    }
    return t;
}

static const Text* latestLogicallyAdjacentTextNode(const Text* t)
{
    const Node* n = t;
    while ((n = n->nextSibling())) {
        Node::NodeType type = n->nodeType();
        if (type == Node::TEXT_NODE || type == Node::CDATA_SECTION_NODE) {
            t = static_cast<const Text*>(n);
            continue;
        }

        // We would need to visit EntityReference child text nodes if they existed
        ASSERT(type != Node::ENTITY_REFERENCE_NODE || !n->hasChildNodes());
        break;
    }
    return t;
}

String Text::wholeText() const
{
    const Text* startText = earliestLogicallyAdjacentTextNode(this);
    const Text* endText = latestLogicallyAdjacentTextNode(this);

    Node* onePastEndText = endText->nextSibling();
    unsigned resultLength = 0;
    for (const Node* n = startText; n != onePastEndText; n = n->nextSibling()) {
        if (!n->isTextNode())
            continue;
        const Text* t = static_cast<const Text*>(n);
        const String& data = t->data();
        resultLength += data.length();
    }
    UChar* resultData;
    String result = String::createUninitialized(resultLength, resultData);
    UChar* p = resultData;
    for (const Node* n = startText; n != onePastEndText; n = n->nextSibling()) {
        if (!n->isTextNode())
            continue;
        const Text* t = static_cast<const Text*>(n);
        const String& data = t->data();
        unsigned dataLength = data.length();
        memcpy(p, data.characters(), dataLength * sizeof(UChar));
        p += dataLength;
    }
    ASSERT(p == resultData + resultLength);

    return result;
}

PassRefPtr<Text> Text::replaceWholeText(const String& newText, ExceptionCode&)
{
    // Remove all adjacent text nodes, and replace the contents of this one.

    // Protect startText and endText against mutation event handlers removing the last ref
    RefPtr<Text> startText = const_cast<Text*>(earliestLogicallyAdjacentTextNode(this));
    RefPtr<Text> endText = const_cast<Text*>(latestLogicallyAdjacentTextNode(this));

    RefPtr<Text> protectedThis(this); // Mutation event handlers could cause our last ref to go away
    Node* parent = parentNode(); // Protect against mutation handlers moving this node during traversal
    ExceptionCode ignored = 0;
    for (RefPtr<Node> n = startText; n && n != this && n->isTextNode() && n->parentNode() == parent;) {
        RefPtr<Node> nodeToRemove(n.release());
        n = nodeToRemove->nextSibling();
        parent->removeChild(nodeToRemove.get(), ignored);
    }

    if (this != endText) {
        Node* onePastEndText = endText->nextSibling();
        for (RefPtr<Node> n = nextSibling(); n && n != onePastEndText && n->isTextNode() && n->parentNode() == parent;) {
            RefPtr<Node> nodeToRemove(n.release());
            n = nodeToRemove->nextSibling();
            parent->removeChild(nodeToRemove.get(), ignored);
        }
    }

    if (newText.isEmpty()) {
        if (parent && parentNode() == parent)
            parent->removeChild(this, ignored);
        return 0;
    }

    setData(newText, ignored);
    return protectedThis.release();
}

String Text::nodeName() const
{
    return textAtom.string();
}

Node::NodeType Text::nodeType() const
{
    return TEXT_NODE;
}

PassRefPtr<Node> Text::cloneNode(bool /*deep*/)
{
    return create(document(), data());
}

bool Text::rendererIsNeeded(RenderStyle *style)
{
    if (!CharacterData::rendererIsNeeded(style))
        return false;

    bool onlyWS = containsOnlyWhitespace();
    if (!onlyWS)
        return true;

    RenderObject *par = parentNode()->renderer();
    
    if (par->isTable() || par->isTableRow() || par->isTableSection() || par->isTableCol() || par->isFrameSet())
        return false;
    
    if (style->preserveNewline()) // pre/pre-wrap/pre-line always make renderers.
        return true;
    
    RenderObject *prev = previousRenderer();
    if (prev && prev->isBR()) // <span><br/> <br/></span>
        return false;
        
    if (par->isRenderInline()) {
        // <span><div/> <div/></span>
        if (prev && !prev->isInline())
            return false;
    } else {
        if (par->isRenderBlock() && !par->childrenInline() && (!prev || !prev->isInline()))
            return false;
        
        RenderObject *first = par->firstChild();
        while (first && first->isFloatingOrPositioned())
            first = first->nextSibling();
        RenderObject *next = nextRenderer();
        if (!first || next == first)
            // Whitespace at the start of a block just goes away.  Don't even
            // make a render object for this text.
            return false;
    }
    
    return true;
}

RenderObject* Text::createRenderer(RenderArena* arena, RenderStyle*)
{
#if ENABLE(SVG)
    if (parentNode()->isSVGElement()
#if ENABLE(SVG_FOREIGN_OBJECT)
        && !parentNode()->hasTagName(SVGNames::foreignObjectTag)
#endif
    )
        return new (arena) RenderSVGInlineText(this, dataImpl());
#endif
    
    return new (arena) RenderText(this, dataImpl());
}

void Text::attach()
{
#if ENABLE(WML)
    if (document()->isWMLDocument() && !containsOnlyWhitespace()) {
        String text = data();
        ASSERT(!text.isEmpty());

        text = substituteVariableReferences(text, document());

        ExceptionCode code = 0;
        setData(text, code);
        ASSERT(!code);
    }
#endif

    createRendererIfNeeded();
    CharacterData::attach();
}

void Text::recalcStyle(StyleChange change)
{
    if (change != NoChange && parentNode()) {
        if (renderer())
            renderer()->setStyle(parentNode()->renderer()->style());
    }
    if (needsStyleRecalc()) {
        if (renderer()) {
            if (renderer()->isText())
                toRenderText(renderer())->setText(dataImpl());
        } else {
            if (attached())
                detach();
            attach();
        }
    }
    setNeedsStyleRecalc(NoStyleChange);
}

bool Text::childTypeAllowed(NodeType)
{
    return false;
}

PassRefPtr<Text> Text::virtualCreate(const String& data)
{
    return create(document(), data);
}

PassRefPtr<Text> Text::createWithLengthLimit(Document* document, const String& data, unsigned& charsLeft, unsigned maxChars)
{
    unsigned dataLength = data.length();

    if (charsLeft == dataLength && charsLeft <= maxChars) {
        charsLeft = 0;
        return create(document, data);
    }

    unsigned start = dataLength - charsLeft;
    unsigned end = start + min(charsLeft, maxChars);
    
    // Check we are not on an unbreakable boundary.
    // Some text break iterator implementations work best if the passed buffer is as small as possible, 
    // see <https://bugs.webkit.org/show_bug.cgi?id=29092>. 
    // We need at least two characters look-ahead to account for UTF-16 surrogates.
    if (end < dataLength) {
        TextBreakIterator* it = characterBreakIterator(data.characters() + start, (end + 2 > dataLength) ? dataLength - start : end - start + 2);
        if (!isTextBreak(it, end - start))
            end = textBreakPreceding(it, end - start) + start;
    }
    
    // If we have maxChars of unbreakable characters the above could lead to
    // an infinite loop.
    // FIXME: It would be better to just have the old value of end before calling
    // textBreakPreceding rather than this, because this exceeds the length limit.
    if (end <= start)
        end = dataLength;
    
    charsLeft = dataLength - end;
    return create(document, data.substring(start, end - start));
}

#ifndef NDEBUG
void Text::formatForDebugger(char *buffer, unsigned length) const
{
    String result;
    String s;
    
    s = nodeName();
    if (s.length() > 0) {
        result += s;
    }
          
    s = data();
    if (s.length() > 0) {
        if (result.length() > 0)
            result += "; ";
        result += "value=";
        result += s;
    }
          
    strncpy(buffer, result.utf8().data(), length - 1);
}
#endif

} // namespace WebCore
