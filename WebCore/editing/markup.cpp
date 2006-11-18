/*
 * Copyright (C) 2004, 2005, 2006 Apple Computer, Inc.  All rights reserved.
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
#include "markup.h"

#include "CSSComputedStyleDeclaration.h"
#include "CSSPropertyNames.h"
#include "CSSRule.h"
#include "CSSRuleList.h"
#include "CSSStyleRule.h"
#include "cssstyleselector.h"
#include "Comment.h"
#include "DeleteButtonController.h"
#include "DeprecatedStringList.h"
#include "Document.h"
#include "DocumentFragment.h"
#include "DocumentType.h"
#include "Editor.h"
#include "Frame.h"
#include "HTMLElement.h"
#include "HTMLNames.h"
#include "InlineTextBox.h"
#include "KURL.h"
#include "Logging.h"
#include "ProcessingInstruction.h"
#include "Range.h"
#include "Selection.h"
#include "htmlediting.h"
#include "visible_units.h"
#include "TextIterator.h"

using namespace std;

namespace WebCore {

using namespace HTMLNames;

static inline bool shouldSelfClose(const Node *node);

static DeprecatedString escapeTextForMarkup(const DeprecatedString &in)
{
    DeprecatedString s = "";

    unsigned len = in.length();
    for (unsigned i = 0; i < len; ++i) {
        switch (in[i].unicode()) {
            case '&':
                s += "&amp;";
                break;
            case '<':
                s += "&lt;";
                break;
            case '>':
                s += "&gt;";
                break;
            default:
                s += in[i];
        }
    }

    return s;
}

static String stringValueForRange(const Node *node, const Range *range)
{
    String str = node->nodeValue().copy();
    if (range) {
        ExceptionCode ec;
        if (node == range->endContainer(ec))
            str.truncate(range->endOffset(ec));
        if (node == range->startContainer(ec))
            str.remove(0, range->startOffset(ec));
    }
    return str;
}

static DeprecatedString renderedText(const Node *node, const Range *range)
{
    if (!node->isTextNode())
        return DeprecatedString();

    ExceptionCode ec;
    const Text* textNode = static_cast<const Text*>(node);
    unsigned startOffset = 0;
    unsigned endOffset = textNode->length();

    if (range && node == range->startContainer(ec))
        startOffset = range->startOffset(ec);
    if (range && node == range->endContainer(ec))
        endOffset = range->endOffset(ec);
    
    Position start(const_cast<Node*>(node), startOffset);
    Position end(const_cast<Node*>(node), endOffset);
    Range r(node->document(), start, end);
    return plainText(&r);
}

static PassRefPtr<CSSMutableStyleDeclaration> styleFromMatchedRulesForElement(Element* element, bool authorOnly = true)
{
    RefPtr<CSSMutableStyleDeclaration> style = new CSSMutableStyleDeclaration();
    RefPtr<CSSRuleList> matchedRules = element->document()->styleSelector()->styleRulesForElement(element, authorOnly);
    if (matchedRules) {
        for (unsigned i = 0; i < matchedRules->length(); i++) {
            if (matchedRules->item(i)->type() == CSSRule::STYLE_RULE) {
                RefPtr<CSSMutableStyleDeclaration> s = static_cast<CSSStyleRule*>(matchedRules->item(i))->style();
                style->merge(s.get(), true);
            }
        }
    }
    
    return style.release();
}

static void removeEnclosingMailBlockquoteStyle(CSSMutableStyleDeclaration* style, Node* node)
{
    Node* blockquote = nearestMailBlockquote(node);
    if (!blockquote || !blockquote->parentNode())
        return;
            
    RefPtr<CSSMutableStyleDeclaration> parentStyle = Position(blockquote->parentNode(), 0).computedStyle()->copyInheritableProperties();
    RefPtr<CSSMutableStyleDeclaration> blockquoteStyle = Position(blockquote, 0).computedStyle()->copyInheritableProperties();
    parentStyle->diff(blockquoteStyle.get());
    blockquoteStyle->diff(style);
}

static DeprecatedString startMarkup(const Node *node, const Range *range, EAnnotateForInterchange annotate, CSSMutableStyleDeclaration *defaultStyle)
{
    bool documentIsHTML = node->document()->isHTMLDocument();
    switch (node->nodeType()) {
        case Node::TEXT_NODE: {
            if (Node* parent = node->parentNode()) {
                if (parent->hasTagName(listingTag)
                        || parent->hasTagName(preTag)
                        || parent->hasTagName(scriptTag)
                        || parent->hasTagName(styleTag)
                        || parent->hasTagName(textareaTag)
                        || parent->hasTagName(xmpTag))
                    return stringValueForRange(node, range).deprecatedString();
            }
            bool useRenderedText = annotate && !enclosingNodeWithTag(const_cast<Node*>(node), selectTag);
            
            DeprecatedString markup = useRenderedText ? escapeTextForMarkup(renderedText(node, range)) : escapeTextForMarkup(stringValueForRange(node, range).deprecatedString());            
            if (defaultStyle) {
                Node *element = node->parentNode();
                if (element) {
                    RefPtr<CSSComputedStyleDeclaration> computedStyle = Position(element, 0).computedStyle();
                    RefPtr<CSSMutableStyleDeclaration> style = computedStyle->copyInheritableProperties();
                    // Styles that Mail blockquotes contribute should only be placed on the Mail blockquote, to help
                    // us differentiate those styles from ones that the user has applied.  This helps us
                    // get the color of content pasted into blockquotes right.
                    removeEnclosingMailBlockquoteStyle(style.get(), const_cast<Node*>(node));
                    
                    defaultStyle->diff(style.get());
                    if (style->length() > 0) {
                        // FIXME: Handle case where style->cssText() has illegal characters in it, like "
                        DeprecatedString openTag = DeprecatedString("<span class=\"") + AppleStyleSpanClass + "\" style=\"" + style->cssText().deprecatedString() + "\">";
                        markup = openTag + markup + "</span>";
                    }
                }            
            }
            return annotate ? convertHTMLTextToInterchangeFormat(markup) : markup;
        }
        case Node::COMMENT_NODE:
            return static_cast<const Comment*>(node)->toString().deprecatedString();
        case Node::DOCUMENT_NODE: {
            // Documents do not normally contain a docType as a child node, force it to print here instead.
            const DocumentType* docType = static_cast<const Document*>(node)->doctype();
            if (docType)
                return docType->toString().deprecatedString();
            return "";
        }
        case Node::DOCUMENT_FRAGMENT_NODE:
            return "";
        case Node::DOCUMENT_TYPE_NODE:
            return static_cast<const DocumentType*>(node)->toString().deprecatedString();
        case Node::PROCESSING_INSTRUCTION_NODE:
            return static_cast<const ProcessingInstruction*>(node)->toString().deprecatedString();
        case Node::ELEMENT_NODE: {
            DeprecatedString markup = DeprecatedChar('<');
            const Element* el = static_cast<const Element*>(node);
            markup += el->nodeNamePreservingCase().deprecatedString();
            String additionalStyle;
            if (defaultStyle && el->isHTMLElement() && !isMailBlockquote(node)) {
                RefPtr<CSSComputedStyleDeclaration> computedStyle = Position(const_cast<Element*>(el), 0).computedStyle();
                RefPtr<CSSMutableStyleDeclaration> style = computedStyle->copyInheritableProperties();
                style->merge(styleFromMatchedRulesForElement(const_cast<Element*>(el)).get());
                
                // Styles that Mail blockquotes contribute should only be placed on the Mail blockquote, to help
                // us differentiate those styles from ones that the user has applied.  This helps us
                // get the color of content pasted into blockquotes right.
                removeEnclosingMailBlockquoteStyle(style.get(), const_cast<Node*>(node));
                
                defaultStyle->diff(style.get());
                if (style->length() > 0) {
                    CSSMutableStyleDeclaration *inlineStyleDecl = static_cast<const HTMLElement*>(el)->inlineStyleDecl();
                    if (inlineStyleDecl)
                        inlineStyleDecl->diff(style.get());
                    additionalStyle = style->cssText();
                }
            }
            NamedAttrMap *attrs = el->attributes();
            unsigned length = attrs->length();

            for (unsigned int i = 0; i < length; i++) {
                Attribute *attr = attrs->attributeItem(i);
                String value = attr->value();
                if (attr->name() == styleAttr && additionalStyle.length() > 0) {
                    value += "; " + additionalStyle;
                    additionalStyle = "";
                }
                // FIXME: Handle case where value has illegal characters in it, like "
                if (documentIsHTML)
                    markup += " " + attr->name().localName().deprecatedString();
                else
                    markup += " " + attr->name().toString().deprecatedString();
                markup += "=\"" + escapeTextForMarkup(value.deprecatedString()) + "\"";
            }
            
            if (additionalStyle.length() > 0)
                // FIXME: Handle case where additionalStyle has illegal characters in it, like "
                markup += " " +  styleAttr.localName().deprecatedString() + "=\"" + additionalStyle.deprecatedString() + "\"";
            
            if (shouldSelfClose(el)) {
                if (el->isHTMLElement())
                    markup += " "; // XHTML 1.0 <-> HTML compatibility.
                markup += "/>";
            } else
                markup += ">";
            
            return markup;
        }
        case Node::ATTRIBUTE_NODE:
        case Node::CDATA_SECTION_NODE:
        case Node::ENTITY_NODE:
        case Node::ENTITY_REFERENCE_NODE:
        case Node::NOTATION_NODE:
        case Node::XPATH_NAMESPACE_NODE:
            break;
    }
    return "";
}

static inline bool doesHTMLForbidEndTag(const Node *node)
{
    if (node->isHTMLElement()) {
        const HTMLElement* htmlElt = static_cast<const HTMLElement*>(node);
        return (htmlElt->endTagRequirement() == TagStatusForbidden);
    }
    return false;
}

// Rules of self-closure
// 1. No elements in HTML documents use the self-closing syntax.
// 2. Elements w/ children never self-close because they use a separate end tag.
// 3. HTML elements which do not have a "forbidden" end tag will close with a separate end tag.
// 4. Other elements self-close.
static inline bool shouldSelfClose(const Node *node)
{
    if (node->document()->isHTMLDocument())
        return false;
    if (node->hasChildNodes())
        return false;
    if (node->isHTMLElement() && !doesHTMLForbidEndTag(node))
        return false;
    return true;
}

static DeprecatedString endMarkup(const Node *node)
{
    if (node->isElementNode() && !shouldSelfClose(node) && (node->hasChildNodes() || !doesHTMLForbidEndTag(node)))
        return "</" + static_cast<const Element*>(node)->nodeNamePreservingCase().deprecatedString() + ">";
    return "";
}

static DeprecatedString markup(Node* startNode, bool onlyIncludeChildren, bool includeSiblings, Vector<Node*> *nodes)
{
    // Doesn't make sense to only include children and include siblings.
    ASSERT(!(onlyIncludeChildren && includeSiblings));
    DeprecatedString me = "";
    for (Node* current = startNode; current != NULL; current = includeSiblings ? current->nextSibling() : NULL) {
        if (!onlyIncludeChildren) {
            if (nodes)
                nodes->append(current);
            me += startMarkup(current, 0, DoNotAnnotateForInterchange, 0);
        }
        // print children
        if (Node *n = current->firstChild())
            if (!(n->document()->isHTMLDocument() && doesHTMLForbidEndTag(current)))
                me += markup(n, false, true, nodes);
        
        // Print my ending tag
        if (!onlyIncludeChildren)
            me += endMarkup(current);
    }
    return me;
}

static void completeURLs(Node *node, const DeprecatedString &baseURL)
{
    Node *end = node->traverseNextSibling();
    for (Node *n = node; n != end; n = n->traverseNextNode()) {
        if (n->isElementNode()) {
            Element *e = static_cast<Element*>(n);
            NamedAttrMap *attrs = e->attributes();
            unsigned length = attrs->length();
            for (unsigned i = 0; i < length; i++) {
                Attribute *attr = attrs->attributeItem(i);
                if (e->isURLAttribute(attr))
                    e->setAttribute(attr->name(), KURL(baseURL, attr->value().deprecatedString()).url());
            }
        }
    }
}

static bool needInterchangeNewlineAfter(const VisiblePosition& v)
{
    VisiblePosition next = v.next();
    return isEndOfParagraph(v) && isStartOfParagraph(next) && !next.deepEquivalent().upstream().node()->hasTagName(brTag);
}

// FIXME: Shouldn't we omit style info when annotate == DoNotAnnotateForInterchange? 
// FIXME: At least, annotation and style info should probably not be included in range.markupString()
DeprecatedString createMarkup(const Range *range, Vector<Node*>* nodes, EAnnotateForInterchange annotate)
{
    if (!range || range->isDetached())
        return DeprecatedString();

    static const DeprecatedString interchangeNewlineString = DeprecatedString("<br class=\"") + AppleInterchangeNewline + "\">";

    ExceptionCode ec = 0;
    if (range->collapsed(ec))
        return "";
    ASSERT(ec == 0);
    Node *commonAncestor = range->commonAncestorContainer(ec);
    ASSERT(ec == 0);

    Document *doc = commonAncestor->document();
    // disable the delete button so it's elements are not serialized into the markup
    doc->frame()->editor()->deleteButtonController()->disable();
    doc->updateLayoutIgnorePendingStylesheets();

    Node *commonAncestorBlock = 0;
    if (commonAncestor) {
        commonAncestorBlock = enclosingBlock(commonAncestor);
        if (commonAncestorBlock->hasTagName(tbodyTag)) {
            Node* table = commonAncestorBlock->parentNode();
            while (table && !table->hasTagName(tableTag))
                table = table->parentNode();
            if (table)
                commonAncestorBlock = table;
        }
    }

    if (!commonAncestorBlock) {
        doc->frame()->editor()->deleteButtonController()->enable();
        return "";
    }

    DeprecatedStringList markups;
    Node *pastEnd = range->pastEndNode();
    Node *lastClosed = 0;
    Vector<Node*> ancestorsToClose;

    // Calculate the "default style" for this markup.
    Position pos(doc->documentElement(), 0);
    RefPtr<CSSComputedStyleDeclaration> computedStyle = pos.computedStyle();
    RefPtr<CSSMutableStyleDeclaration> defaultStyle = computedStyle->copyInheritableProperties();
    
    Node* startNode = range->startNode();
    VisiblePosition visibleStart(range->startPosition(), VP_DEFAULT_AFFINITY);
    VisiblePosition visibleEnd(range->endPosition(), VP_DEFAULT_AFFINITY);
    if (annotate && needInterchangeNewlineAfter(visibleStart)) {
        if (visibleStart == visibleEnd.previous()) {
            doc->frame()->editor()->deleteButtonController()->enable();
            return interchangeNewlineString;
        }

        markups.append(interchangeNewlineString);
        startNode = visibleStart.next().deepEquivalent().node();
    }

    Node *next;
    for (Node *n = startNode; n != pastEnd; n = next) {
        next = n->traverseNextNode();
        bool skipDescendants = false;
        bool addMarkupForNode = true;
        
        if (!n->renderer() && !enclosingNodeWithTag(n, selectTag)) {
            skipDescendants = true;
            addMarkupForNode = false;
            next = n->traverseNextSibling();
            // Don't skip over pastEnd.
            if (pastEnd && pastEnd->isDescendantOf(n))
                next = pastEnd;
        }

        if (isBlock(n) && canHaveChildrenForEditing(n) && next == pastEnd)
            // Don't write out empty block containers that aren't fully selected.
            continue;
        
        // Add the node to the markup.
        if (addMarkupForNode) {
            markups.append(startMarkup(n, range, annotate, defaultStyle.get()));
            if (nodes)
                nodes->append(n);
        }
        
        if (n->firstChild() == 0 || skipDescendants) {
            // Node has no children, or we are skipping it's descendants, add its close tag now.
            if (addMarkupForNode) {
                markups.append(endMarkup(n));
                lastClosed = n;
            }
            
            // Check if the node is the last leaf of a tree.
            if (!n->nextSibling() || next == pastEnd) {
                if (!ancestorsToClose.isEmpty()) {
                    // Close up the ancestors.
                    do {
                        Node *ancestor = ancestorsToClose.last();
                        if (next != pastEnd && next->isDescendantOf(ancestor))
                            break;
                        // Not at the end of the range, close ancestors up to sibling of next node.
                        markups.append(endMarkup(ancestor));
                        lastClosed = ancestor;
                        ancestorsToClose.removeLast();
                    } while (!ancestorsToClose.isEmpty());
                }
                
                // Surround the currently accumulated markup with markup for ancestors we never opened as we leave the subtree(s) rooted at those ancestors.
                Node* nextParent = next ? next->parentNode() : 0;
                if (next != pastEnd && n != nextParent) {
                    Node* lastAncestorClosedOrSelf = n->isDescendantOf(lastClosed) ? lastClosed : n;
                    for (Node *parent = lastAncestorClosedOrSelf->parent(); parent != 0 && parent != nextParent; parent = parent->parentNode()) {
                        // All ancestors that aren't in the ancestorsToClose list should either be a) unrendered:
                        if (!parent->renderer())
                            continue;
                        // or b) ancestors that we never encountered during a pre-order traversal starting at startNode:
                        ASSERT(startNode->isDescendantOf(parent));
                        markups.prepend(startMarkup(parent, range, annotate, defaultStyle.get()));
                        markups.append(endMarkup(parent));
                        if (nodes)
                            nodes->append(parent);
                        lastClosed = parent;
                    }
                }
            }
        } else if (addMarkupForNode && !skipDescendants)
            // We added markup for this node, and we're descending into it.  Set it to close eventually.
            ancestorsToClose.append(n);
    }
    
    Node *rangeStartNode = range->startNode();
    int rangeStartOffset = range->startOffset(ec);
    ASSERT(ec == 0);
    
    // Add ancestors up to the common ancestor block so inline ancestors such as FONT and B are part of the markup.
    if (lastClosed) {
        for (Node *ancestor = lastClosed->parentNode(); ancestor; ancestor = ancestor->parentNode()) {
            if (Range::compareBoundaryPoints(ancestor, 0, rangeStartNode, rangeStartOffset) >= 0) {
                // we have already added markup for this node
                continue;
            }
            bool breakAtEnd = false;
            if (commonAncestorBlock == ancestor) {
                // Include ancestors that are required to retain the appearance of the copied markup.
                if (annotate &&
                    (ancestor->hasTagName(listingTag)
                        || ancestor->hasTagName(olTag)
                        || ancestor->hasTagName(preTag)
                        || ancestor->hasTagName(tableTag)
                        || ancestor->hasTagName(ulTag)
                        || ancestor->hasTagName(xmpTag))) {
                    breakAtEnd = true;
                } else
                    break;
            }
            markups.prepend(startMarkup(ancestor, range, annotate, defaultStyle.get()));
            markups.append(endMarkup(ancestor));
            if (nodes) {
                nodes->append(ancestor);
            }        
            if (breakAtEnd)
                break;
        }
    }

    if (annotate && needInterchangeNewlineAfter(visibleEnd.previous()))
        markups.append(interchangeNewlineString);

    // Retain the Mail quote level by including all ancestor mail block quotes.
    if (annotate) {
        for (Node *ancestor = commonAncestorBlock; ancestor; ancestor = ancestor->parentNode()) {
            if (isMailBlockquote(ancestor)) {
                markups.prepend(startMarkup(ancestor, range, annotate, defaultStyle.get()));
                markups.append(endMarkup(ancestor));
            }
        }
    }
    
    // FIXME: Do this for all fully selected blocks, not just a body.
    Node* root = range->startPosition().node();
    while (root && !root->hasTagName(bodyTag))
        root = root->parentNode();
    if (root && *Selection::selectionFromContentsOfNode(root).toRange() == *range) {
        CSSMutableStyleDeclaration* inlineStyleDecl = static_cast<HTMLElement*>(root)->inlineStyleDecl();
        RefPtr<CSSMutableStyleDeclaration> style = inlineStyleDecl ? inlineStyleDecl->copy() : new CSSMutableStyleDeclaration();
        style->merge(styleFromMatchedRulesForElement(static_cast<Element*>(root)).get());
        
        defaultStyle->diff(style.get());
        
        // Bring the background attribute over, but not as an attribute because a background attribute on a div
        // appears to have no effect.
        if (!style->getPropertyCSSValue(CSS_PROP_BACKGROUND_IMAGE) && static_cast<Element*>(root)->hasAttribute(backgroundAttr))
            style->setProperty(CSS_PROP_BACKGROUND_IMAGE, "url('" + static_cast<Element*>(root)->getAttribute(backgroundAttr) + "')");
        
        markups.prepend("<div style='" + style->cssText().deprecatedString() + "'>");
        markups.append("</div>");
    }
    
    // add in the "default style" for this markup
    // FIXME: Handle case where value has illegal characters in it, like "
    DeprecatedString openTag = DeprecatedString("<span class=\"") + AppleStyleSpanClass + "\" style=\"" + defaultStyle->cssText().deprecatedString() + "\">";
    markups.prepend(openTag);
    markups.append("</span>");

    doc->frame()->editor()->deleteButtonController()->enable();
    return markups.join("");
}

PassRefPtr<DocumentFragment> createFragmentFromMarkup(Document* document, const String& markup, const String& baseURL)
{
    ASSERT(document->documentElement()->isHTMLElement());
    // FIXME: What if the document element is not an HTML element?
    HTMLElement *element = static_cast<HTMLElement*>(document->documentElement());

    RefPtr<DocumentFragment> fragment = element->createContextualFragment(markup);
    ASSERT(fragment);

    if (!baseURL.isEmpty() && baseURL != document->baseURL())
        completeURLs(fragment.get(), baseURL.deprecatedString());

    return fragment.release();
}

DeprecatedString createMarkup(const Node* node, EChildrenOnly includeChildren,
    Vector<Node*>* nodes, EAnnotateForInterchange annotate)
{
    ASSERT(annotate == DoNotAnnotateForInterchange); // annotation not yet implemented for this code path
    // disable the delete button so it's elements are not serialized into the markup
    if (node->document()->frame())
        node->document()->frame()->editor()->deleteButtonController()->disable();
    node->document()->updateLayoutIgnorePendingStylesheets();
    DeprecatedString result(markup(const_cast<Node*>(node), includeChildren, false, nodes));
    if (node->document()->frame())
        node->document()->frame()->editor()->deleteButtonController()->enable();
    return result;
}

static void fillContainerFromString(ContainerNode* paragraph, const DeprecatedString& string)
{
    Document* document = paragraph->document();

    ExceptionCode ec = 0;
    if (string.isEmpty()) {
        paragraph->appendChild(createBlockPlaceholderElement(document), ec);
        ASSERT(ec == 0);
        return;
    }

    assert(string.find('\n') == -1);

    DeprecatedStringList tabList = DeprecatedStringList::split('\t', string, true);
    DeprecatedString tabText = "";
    while (!tabList.isEmpty()) {
        DeprecatedString s = tabList.first();
        tabList.pop_front();

        // append the non-tab textual part
        if (!s.isEmpty()) {
            if (!tabText.isEmpty()) {
                paragraph->appendChild(createTabSpanElement(document, tabText), ec);
                ASSERT(ec == 0);
                tabText = "";
            }
            RefPtr<Node> textNode = document->createTextNode(s);
            rebalanceWhitespaceInTextNode(textNode.get(), 0, s.length());
            paragraph->appendChild(textNode.release(), ec);
            ASSERT(ec == 0);
        }

        // there is a tab after every entry, except the last entry
        // (if the last character is a tab, the list gets an extra empty entry)
        if (!tabList.isEmpty())
            tabText += '\t';
        else if (!tabText.isEmpty()) {
            paragraph->appendChild(createTabSpanElement(document, tabText), ec);
            ASSERT(ec == 0);
        }
    }
}

PassRefPtr<DocumentFragment> createFragmentFromText(Range* context, const String& text)
{
    if (!context)
        return 0;

    Node* styleNode = context->startNode();
    if (!styleNode) {
        styleNode = context->startPosition().node();
        if (!styleNode)
            return 0;
    }

    Document* document = styleNode->document();
    RefPtr<DocumentFragment> fragment = document->createDocumentFragment();
    
    if (text.isEmpty())
        return fragment.release();

    DeprecatedString string = text.deprecatedString();
    string.replace("\r\n", "\n");
    string.replace('\r', '\n');

    ExceptionCode ec = 0;
    RenderObject* renderer = styleNode->renderer();
    if (renderer && renderer->style()->preserveNewline()) {
        fragment->appendChild(document->createTextNode(string), ec);
        ASSERT(ec == 0);
        if (string.endsWith("\n")) {
            RefPtr<Element> element;
            element = document->createElementNS(xhtmlNamespaceURI, "br", ec);
            ASSERT(ec == 0);
            element->setAttribute(classAttr, AppleInterchangeNewline);            
            fragment->appendChild(element.release(), ec);
            ASSERT(ec == 0);
        }
        return fragment.release();
    }

    // A string with no newlines gets added inline, rather than being put into a paragraph.
    if (string.find('\n') == -1) {
        fillContainerFromString(fragment.get(), string);
        return fragment.release();
    }

    // Break string into paragraphs. Extra line breaks turn into empty paragraphs.
    DeprecatedStringList list = DeprecatedStringList::split('\n', string, true); // true gets us empty strings in the list
    while (!list.isEmpty()) {
        DeprecatedString s = list.first();
        list.pop_front();

        RefPtr<Element> element;
        if (s.isEmpty() && list.isEmpty()) {
            // For last line, use the "magic BR" rather than a P.
            element = document->createElementNS(xhtmlNamespaceURI, "br", ec);
            ASSERT(ec == 0);
            element->setAttribute(classAttr, AppleInterchangeNewline);            
        } else {
            element = createDefaultParagraphElement(document);
            fillContainerFromString(element.get(), s);
        }
        fragment->appendChild(element.release(), ec);
        ASSERT(ec == 0);
    }
    return fragment.release();
}

PassRefPtr<DocumentFragment> createFragmentFromNodes(Document *document, const Vector<Node*>& nodes)
{
    if (!document)
        return 0;
    
    RefPtr<DocumentFragment> fragment = document->createDocumentFragment();

    ExceptionCode ec = 0;
    size_t size = nodes.size();
    for (size_t i = 0; i < size; ++i) {
        RefPtr<Element> element = createDefaultParagraphElement(document);
        element->appendChild(nodes[i], ec);
        ASSERT(ec == 0);
        fragment->appendChild(element.release(), ec);
        ASSERT(ec == 0);
    }

    return fragment.release();
}

}
