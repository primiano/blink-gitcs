/**
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2004, 2005, 2006 Apple Computer, Inc.
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
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"
#include "HTMLElementImpl.h"

#include "EventNames.h"
#include "Frame.h"
#include "css_ruleimpl.h"
#include "css_stylesheetimpl.h"
#include "css_valueimpl.h"
#include "cssproperties.h"
#include "cssvalues.h"
#include "dom2_events.h"
#include "dom2_eventsimpl.h"
#include "dom_exception.h"
#include "dom_textimpl.h"
#include "html_documentimpl.h"
#include "htmlfactory.h"
#include "htmlnames.h"
#include "htmltokenizer.h"
#include "markup.h"
#include "render_replaced.h"
#include "visible_text.h"
#include <kxmlcore/HashSet.h>

namespace WebCore {

using namespace EventNames;
using namespace HTMLNames;

HTMLElementImpl::HTMLElementImpl(const QualifiedName& tagName, DocumentImpl *doc)
    : StyledElementImpl(tagName, doc)
{
}

HTMLElementImpl::~HTMLElementImpl()
{
}

DOMString HTMLElementImpl::nodeName() const
{
    // FIXME: Would be nice to have an atomicstring lookup based off uppercase chars that does not have to copy
    // the string on a hit in the hash.
    if (getDocument()->isHTMLDocument())
        return m_tagName.localName().impl()->upper();
    return ElementImpl::nodeName();
}
    
HTMLTagStatus HTMLElementImpl::endTagRequirement() const
{
    if (hasLocalName(dtTag) || hasLocalName(ddTag))
        return TagStatusOptional;

    // Same values as <span>.  This way custom tag name elements will behave like inline spans.
    return TagStatusRequired;
}

int HTMLElementImpl::tagPriority() const
{
    if (hasLocalName(addressTag) || hasLocalName(ddTag) || hasLocalName(dtTag) || hasLocalName(noscriptTag))
        return 3;
    if (hasLocalName(centerTag) || hasLocalName(nobrTag))
        return 5;
    if (hasLocalName(noembedTag) || hasLocalName(noframesTag))
        return 10;

    // Same values as <span>.  This way custom tag name elements will behave like inline spans.
    return 1;
}

PassRefPtr<NodeImpl> HTMLElementImpl::cloneNode(bool deep)
{
    PassRefPtr<HTMLElementImpl> clone = HTMLElementFactory::createHTMLElement(m_tagName.localName(), getDocument(), 0, false);
    if (!clone)
        return 0;

    if (namedAttrMap)
        *clone->attributes() = *namedAttrMap;

    if (m_inlineStyleDecl)
        *clone->getInlineStyleDecl() = *m_inlineStyleDecl;

    clone->copyNonAttributeProperties(this);

    if (deep)
        cloneChildNodes(clone.get());

    return clone;
}

bool HTMLElementImpl::mapToEntry(const QualifiedName& attrName, MappedAttributeEntry& result) const
{
    if (attrName == alignAttr ||
        attrName == contenteditableAttr) {
        result = eUniversal;
        return false;
    }
    if (attrName == dirAttr) {
        result = hasTagName(bdoTag) ? eBDO : eUniversal;
        return false;
    }

    return StyledElementImpl::mapToEntry(attrName, result);
}
    
void HTMLElementImpl::parseMappedAttribute(MappedAttributeImpl *attr)
{
    if (attr->name() == idAttr || attr->name() == classAttr || attr->name() == styleAttr)
        return StyledElementImpl::parseMappedAttribute(attr);

    DOMString indexstring;
    if (attr->name() == alignAttr) {
        if (equalIgnoringCase(attr->value(), "middle"))
            addCSSProperty(attr, CSS_PROP_TEXT_ALIGN, "center");
        else
            addCSSProperty(attr, CSS_PROP_TEXT_ALIGN, attr->value());
    } else if (attr->name() == contenteditableAttr) {
        setContentEditable(attr);
    } else if (attr->name() == tabindexAttr) {
        indexstring = getAttribute(tabindexAttr);
        if (indexstring.length())
            setTabIndex(indexstring.toInt());
    } else if (attr->name() == langAttr) {
        // FIXME: Implement
    } else if (attr->name() == dirAttr) {
        addCSSProperty(attr, CSS_PROP_DIRECTION, attr->value());
        addCSSProperty(attr, CSS_PROP_UNICODE_BIDI, hasTagName(bdoTag) ? CSS_VAL_BIDI_OVERRIDE : CSS_VAL_EMBED);
    }
// standard events
    else if (attr->name() == onclickAttr) {
        setHTMLEventListener(clickEvent, attr);
    } else if (attr->name() == oncontextmenuAttr) {
        setHTMLEventListener(contextmenuEvent, attr);
    } else if (attr->name() == ondblclickAttr) {
        setHTMLEventListener(khtmlDblclickEvent, attr);
    } else if (attr->name() == onmousedownAttr) {
        setHTMLEventListener(mousedownEvent, attr);
    } else if (attr->name() == onmousemoveAttr) {
        setHTMLEventListener(mousemoveEvent, attr);
    } else if (attr->name() == onmouseoutAttr) {
        setHTMLEventListener(mouseoutEvent, attr);
    } else if (attr->name() == onmouseoverAttr) {
        setHTMLEventListener(mouseoverEvent, attr);
    } else if (attr->name() == onmouseupAttr) {
        setHTMLEventListener(mouseupEvent, attr);
    } else if (attr->name() == onmousewheelAttr) {
        setHTMLEventListener(mousewheelEvent, attr);
    } else if (attr->name() == onfocusAttr) {
        setHTMLEventListener(DOMFocusInEvent, attr);
    } else if (attr->name() == onblurAttr) {
        setHTMLEventListener(DOMFocusOutEvent, attr);
    } else if (attr->name() == onkeydownAttr) {
        setHTMLEventListener(keydownEvent, attr);
    } else if (attr->name() == onkeypressAttr) {
        setHTMLEventListener(keypressEvent, attr);
    } else if (attr->name() == onkeyupAttr) {
        setHTMLEventListener(keyupEvent, attr);
    } else if (attr->name() == onscrollAttr) {
        setHTMLEventListener(scrollEvent, attr);
    } else if (attr->name() == onbeforecutAttr) {
        setHTMLEventListener(beforecutEvent, attr);
    } else if (attr->name() == oncutAttr) {
        setHTMLEventListener(cutEvent, attr);
    } else if (attr->name() == onbeforecopyAttr) {
        setHTMLEventListener(beforecopyEvent, attr);
    } else if (attr->name() == oncopyAttr) {
        setHTMLEventListener(copyEvent, attr);
    } else if (attr->name() == onbeforepasteAttr) {
        setHTMLEventListener(beforepasteEvent, attr);
    } else if (attr->name() == onpasteAttr) {
        setHTMLEventListener(pasteEvent, attr);
    } else if (attr->name() == ondragenterAttr) {
        setHTMLEventListener(dragenterEvent, attr);
    } else if (attr->name() == ondragoverAttr) {
        setHTMLEventListener(dragoverEvent, attr);
    } else if (attr->name() == ondragleaveAttr) {
        setHTMLEventListener(dragleaveEvent, attr);
    } else if (attr->name() == ondropAttr) {
        setHTMLEventListener(dropEvent, attr);
    } else if (attr->name() == ondragstartAttr) {
        setHTMLEventListener(dragstartEvent, attr);
    } else if (attr->name() == ondragAttr) {
        setHTMLEventListener(dragEvent, attr);
    } else if (attr->name() == ondragendAttr) {
        setHTMLEventListener(dragendEvent, attr);
    } else if (attr->name() == onselectstartAttr) {
        setHTMLEventListener(selectstartEvent, attr);
    } 
}

DOMString HTMLElementImpl::innerHTML() const
{
    return createMarkup(this, ChildrenOnly);
}

DOMString HTMLElementImpl::outerHTML() const
{
    return createMarkup(this);
}

DOMString HTMLElementImpl::innerText() const
{
    // We need to update layout, since plainText uses line boxes in the render tree.
    getDocument()->updateLayoutIgnorePendingStylesheets();
    return plainText(rangeOfContents(const_cast<HTMLElementImpl *>(this)).get());
}

DOMString HTMLElementImpl::outerText() const
{
    // Getting outerText is the same as getting innerText, only
    // setting is different. You would think this should get the plain
    // text for the outer range, but this is wrong, <br> for instance
    // would return different values for inner and outer text by such
    // a rule, but it doesn't in WinIE, and we want to match that.
    return innerText();
}

DocumentFragmentImpl *HTMLElementImpl::createContextualFragment(const DOMString &html)
{
    // the following is in accordance with the definition as used by IE
    if (endTagRequirement() == TagStatusForbidden)
        return 0;

    if (hasLocalName(colTag) || hasLocalName(colgroupTag) || hasLocalName(framesetTag) ||
        hasLocalName(headTag) || hasLocalName(styleTag) || hasLocalName(titleTag))
        return 0;

    DocumentFragmentImpl *fragment = new DocumentFragmentImpl(getDocument());
    fragment->ref();
    
    if (getDocument()->isHTMLDocument())
         parseHTMLDocumentFragment(html, fragment);
    else {
        if (!parseXMLDocumentFragment(html, fragment, this)) {
            // FIXME: We should propagate a syntax error exception out here.
            fragment->deref();
            return 0;
        }
    }

    // Exceptions are ignored because none ought to happen here.
    int ignoredExceptionCode;

    // we need to pop <html> and <body> elements and remove <head> to
    // accommodate folks passing complete HTML documents to make the
    // child of an element.

    NodeImpl *nextNode;
    for (NodeImpl *node = fragment->firstChild(); node != NULL; node = nextNode) {
        nextNode = node->nextSibling();
        node->ref();
        if (node->hasTagName(htmlTag) || node->hasTagName(bodyTag)) {
            NodeImpl *firstChild = node->firstChild();
            if (firstChild != NULL) {
                nextNode = firstChild;
            }
            NodeImpl *nextChild;
            for (NodeImpl *child = firstChild; child != NULL; child = nextChild) {
                nextChild = child->nextSibling();
                child->ref();
                node->removeChild(child, ignoredExceptionCode);
                fragment->insertBefore(child, node, ignoredExceptionCode);
                child->deref();
            }
            fragment->removeChild(node, ignoredExceptionCode);
        } else if (node->hasTagName(headTag))
            fragment->removeChild(node, ignoredExceptionCode);

        // Important to do this deref after removeChild, because if the only thing
        // keeping a node around is a parent that is non-0, removeChild will not
        // delete the node. This works fine in JavaScript because there's always
        // a ref of the node, but here in C++ we need to do it explicitly.
        node->deref();
    }

    // Trick to get the fragment back to the floating state, with 0
    // refs but not destroyed.
    fragment->setParent(this);
    fragment->deref();
    fragment->setParent(0);

    return fragment;
}

void HTMLElementImpl::setInnerHTML(const DOMString &html, int &exception)
{
    DocumentFragmentImpl *fragment = createContextualFragment(html);
    if (fragment == NULL) {
        exception = DOMException::NO_MODIFICATION_ALLOWED_ERR;
        return;
    }

    removeChildren();
    appendChild(fragment, exception);
}

void HTMLElementImpl::setOuterHTML(const DOMString &html, int &exception)
{
    NodeImpl *p = parent();
    if (!p || !p->isHTMLElement()) {
        exception = DOMException::NO_MODIFICATION_ALLOWED_ERR;
        return;
    }
    HTMLElementImpl *parent = static_cast<HTMLElementImpl *>(p);
    DocumentFragmentImpl *fragment = parent->createContextualFragment(html);

    if (!fragment) {
        exception = DOMException::NO_MODIFICATION_ALLOWED_ERR;
        return;
    }
    
    ref();
    parent->replaceChild(fragment, this, exception);
    deref();
}


void HTMLElementImpl::setInnerText(const DOMString &text, int &exception)
{
    // following the IE specs.
    if (endTagRequirement() == TagStatusForbidden) {
        exception = DOMException::NO_MODIFICATION_ALLOWED_ERR;
        return;
    }

    if (hasLocalName(colTag) || hasLocalName(colgroupTag) || hasLocalName(framesetTag) ||
        hasLocalName(headTag) || hasLocalName(htmlTag) || hasLocalName(tableTag) || 
        hasLocalName(tbodyTag) || hasLocalName(tfootTag) || hasLocalName(theadTag) ||
        hasLocalName(trTag)) {
        exception = DOMException::NO_MODIFICATION_ALLOWED_ERR;
        return;
    }

    removeChildren();
    appendChild(new TextImpl(getDocument(), text), exception);
}

void HTMLElementImpl::setOuterText(const DOMString &text, int &exception)
{
    // following the IE specs.
    if (endTagRequirement() == TagStatusForbidden) {
        exception = DOMException::NO_MODIFICATION_ALLOWED_ERR;
        return;
    }

    if (hasLocalName(colTag) || hasLocalName(colgroupTag) || hasLocalName(framesetTag) ||
        hasLocalName(headTag) || hasLocalName(htmlTag) || hasLocalName(tableTag) || 
        hasLocalName(tbodyTag) || hasLocalName(tfootTag) || hasLocalName(theadTag) ||
        hasLocalName(trTag)) {
        exception = DOMException::NO_MODIFICATION_ALLOWED_ERR;
        return;
    }

    NodeImpl *parent = parentNode();

    if (!parent) {
        exception = DOMException::NO_MODIFICATION_ALLOWED_ERR;
        return;
    }

    TextImpl *t = new TextImpl(getDocument(), text);
    ref();
    parent->replaceChild(t, this, exception);
    deref();
    if (exception)
        return;

    // is previous node a text node? if so, merge into it
    NodeImpl *prev = t->previousSibling();
    if (prev && prev->isTextNode()) {
        TextImpl *textPrev = static_cast<TextImpl *>(prev);
        textPrev->appendData(t->data(), exception);
        if (exception)
            return;
        t->remove(exception);
        if (exception)
            return;
        t = textPrev;
    }

    // is next node a text node? if so, merge it in
    NodeImpl *next = t->nextSibling();
    if (next && next->isTextNode()) {
        TextImpl *textNext = static_cast<TextImpl *>(next);
        t->appendData(textNext->data(), exception);
        if (exception)
            return;
        textNext->remove(exception);
        if (exception)
            return;
    }
}

void HTMLElementImpl::addHTMLAlignment(MappedAttributeImpl* attr)
{
    // vertical alignment with respect to the current baseline of the text
    // right or left means floating images
    int propfloat = -1;
    int propvalign = -1;
    const AtomicString& alignment = attr->value();
    if (equalIgnoringCase(alignment, "absmiddle")) {
        propvalign = CSS_VAL_MIDDLE;
    } else if (equalIgnoringCase(alignment, "absbottom")) {
        propvalign = CSS_VAL_BOTTOM;
    } else if (equalIgnoringCase(alignment, "left")) {
        propfloat = CSS_VAL_LEFT;
        propvalign = CSS_VAL_TOP;
    } else if (equalIgnoringCase(alignment, "right")) {
        propfloat = CSS_VAL_RIGHT;
        propvalign = CSS_VAL_TOP;
    } else if (equalIgnoringCase(alignment, "top")) {
        propvalign = CSS_VAL_TOP;
    } else if (equalIgnoringCase(alignment, "middle")) {
        propvalign = CSS_VAL__KHTML_BASELINE_MIDDLE;
    } else if (equalIgnoringCase(alignment, "center")) {
        propvalign = CSS_VAL_MIDDLE;
    } else if (equalIgnoringCase(alignment, "bottom")) {
        propvalign = CSS_VAL_BASELINE;
    } else if (equalIgnoringCase(alignment, "texttop")) {
        propvalign = CSS_VAL_TEXT_TOP;
    }
    
    if ( propfloat != -1 )
        addCSSProperty( attr, CSS_PROP_FLOAT, propfloat );
    if ( propvalign != -1 )
        addCSSProperty( attr, CSS_PROP_VERTICAL_ALIGN, propvalign );
}

bool HTMLElementImpl::isFocusable() const
{
    // Added the check for !parent() so shadow elements, like those in engine-based textfields, can get focus.
    // We may reevaluate this when we resolve outstanding focus issues with these shadow elements.
    return isContentEditable() && ((parent() && !parent()->isContentEditable()) || !parent());
}

bool HTMLElementImpl::isContentEditable() const 
{
    if (getDocument()->frame() && getDocument()->frame()->isContentEditable())
        return true;

    getDocument()->updateRendering();

    if (!renderer()) {
        if (parentNode())
            return parentNode()->isContentEditable();
        else
            return false;
    }
    
    return renderer()->style()->userModify() == READ_WRITE;
}

DOMString HTMLElementImpl::contentEditable() const 
{
    getDocument()->updateRendering();

    if (!renderer())
        return "false";
    
    switch (renderer()->style()->userModify()) {
        case READ_WRITE:
            return "true";
        case READ_ONLY:
            return "false";
        default:
            return "inherit";
    }
    return "inherit";
}

void HTMLElementImpl::setContentEditable(MappedAttributeImpl* attr) 
{
    Frame *frame = getDocument()->frame();
    const AtomicString& enabled = attr->value();
    if (enabled.isEmpty() || equalIgnoringCase(enabled, "true")) {
        addCSSProperty(attr, CSS_PROP__KHTML_USER_MODIFY, CSS_VAL_READ_WRITE);
        if (frame)
            frame->applyEditingStyleToElement(this);    
    } else if (equalIgnoringCase(enabled, "false")) {
        addCSSProperty(attr, CSS_PROP__KHTML_USER_MODIFY, CSS_VAL_READ_ONLY);
        if (frame)
            frame->removeEditingStyleFromElement(this);    
    } else if (equalIgnoringCase(enabled, "inherit")) {
        addCSSProperty(attr, CSS_PROP__KHTML_USER_MODIFY, CSS_VAL_INHERIT);
        if (frame)
            frame->removeEditingStyleFromElement(this);    
    }
}

void HTMLElementImpl::setContentEditable(const DOMString &enabled)
{
    if (enabled == "inherit") {
        int exceptionCode;
        removeAttribute(contenteditableAttr, exceptionCode);
    }
    else
        setAttribute(contenteditableAttr, enabled.isEmpty() ? "true" : enabled);
}

void HTMLElementImpl::click(bool sendMouseEvents, bool showPressedLook)
{
    // send mousedown and mouseup before the click, if requested
    if (sendMouseEvents)
        dispatchSimulatedMouseEvent(mousedownEvent);
    setActive(true, showPressedLook);
    if (sendMouseEvents)
        dispatchSimulatedMouseEvent(mouseupEvent);
    setActive(false);

    // always send click
    dispatchSimulatedMouseEvent(clickEvent);
}

// accessKeyAction is used by the accessibility support code
// to send events to elements that our JavaScript caller does
// does not.  The elements JS is interested in have subclasses
// that override this method to direct the click appropriately.
// Here in the base class, then, we only send the click if
// the caller wants it to go to any HTMLElementImpl, and we say
// to send the mouse events in addition to the click.
void HTMLElementImpl::accessKeyAction(bool sendToAnyElement)
{
    if (sendToAnyElement)
        click(true);
}

DOMString HTMLElementImpl::toString() const
{
    if (!hasChildNodes() && getDocument()->isHTMLDocument()) {
        DOMString result = openTagStartToString();
        result += ">";

        if (endTagRequirement() == TagStatusRequired) {
            result += "</";
            result += nodeName();
            result += ">";
        }

        return result;
    }

    return ElementImpl::toString();
}

DOMString HTMLElementImpl::id() const
{
    return getAttribute(idAttr);
}

void HTMLElementImpl::setId(const DOMString &value)
{
    setAttribute(idAttr, value);
}

DOMString HTMLElementImpl::title() const
{
    return getAttribute(titleAttr);
}

void HTMLElementImpl::setTitle(const DOMString &value)
{
    setAttribute(titleAttr, value);
}

DOMString HTMLElementImpl::lang() const
{
    return getAttribute(langAttr);
}

void HTMLElementImpl::setLang(const DOMString &value)
{
    setAttribute(langAttr, value);
}

DOMString HTMLElementImpl::dir() const
{
    return getAttribute(dirAttr);
}

void HTMLElementImpl::setDir(const DOMString &value)
{
    setAttribute(dirAttr, value);
}

DOMString HTMLElementImpl::className() const
{
    return getAttribute(classAttr);
}

void HTMLElementImpl::setClassName(const DOMString &value)
{
    setAttribute(classAttr, value);
}

PassRefPtr<HTMLCollectionImpl> HTMLElementImpl::children()
{
    return new HTMLCollectionImpl(this, HTMLCollectionImpl::NODE_CHILDREN);
}

// DOM Section 1.1.1
bool HTMLElementImpl::childAllowed(NodeImpl *newChild)
{
    if (!ElementImpl::childAllowed(newChild))
        return false;

    // For XML documents, we are non-validating and do not check against a DTD, even for HTML elements.
    if (!getDocument()->isHTMLDocument())
        return true;

    // Future-proof for XML content inside HTML documents (we may allow this some day).
    if (newChild->isElementNode() && !newChild->isHTMLElement())
        return true;

    // Elements with forbidden tag status can never have children
    if (endTagRequirement() == TagStatusForbidden)
        return false;

    // Comment nodes are always allowed.
    if (newChild->isCommentNode())
        return true;

    // Now call checkDTD.
    return checkDTD(newChild);
}

// DTD Stuff
// This unfortunate function is only needed when checking against the DTD.  Other languages (like SVG) won't need this.
bool HTMLElementImpl::isRecognizedTagName(const QualifiedName& tagName)
{
    static HashSet<DOMStringImpl*, PointerHash<DOMStringImpl*> > tagList;
    if (tagList.isEmpty()) {
        #define ADD_TAG(name) tagList.add(name##Tag.localName().impl());
        DOM_HTMLNAMES_FOR_EACH_TAG(ADD_TAG)
    }
    return tagList.contains(tagName.localName().impl());
}

// The terms inline and block are used here loosely.  Don't make the mistake of assuming all inlines or all blocks
// need to be in these two lists.
HashSet<DOMStringImpl*, PointerHash<DOMStringImpl*> >* inlineTagList()
{
    static HashSet<DOMStringImpl*, PointerHash<DOMStringImpl*> > tagList;
    if (tagList.isEmpty()) {
        tagList.add(ttTag.localName().impl());
        tagList.add(iTag.localName().impl());
        tagList.add(bTag.localName().impl());
        tagList.add(uTag.localName().impl());
        tagList.add(sTag.localName().impl());
        tagList.add(strikeTag.localName().impl());
        tagList.add(bigTag.localName().impl());
        tagList.add(smallTag.localName().impl());
        tagList.add(emTag.localName().impl());
        tagList.add(strongTag.localName().impl());
        tagList.add(dfnTag.localName().impl());
        tagList.add(codeTag.localName().impl());
        tagList.add(sampTag.localName().impl());
        tagList.add(kbdTag.localName().impl());
        tagList.add(varTag.localName().impl());
        tagList.add(citeTag.localName().impl());
        tagList.add(abbrTag.localName().impl());
        tagList.add(acronymTag.localName().impl());
        tagList.add(aTag.localName().impl());
        tagList.add(canvasTag.localName().impl());
        tagList.add(imgTag.localName().impl());
        tagList.add(appletTag.localName().impl());
        tagList.add(objectTag.localName().impl());
        tagList.add(embedTag.localName().impl());
        tagList.add(fontTag.localName().impl());
        tagList.add(basefontTag.localName().impl());
        tagList.add(brTag.localName().impl());
        tagList.add(scriptTag.localName().impl());
        tagList.add(mapTag.localName().impl());
        tagList.add(qTag.localName().impl());
        tagList.add(subTag.localName().impl());
        tagList.add(supTag.localName().impl());
        tagList.add(spanTag.localName().impl());
        tagList.add(bdoTag.localName().impl());
        tagList.add(iframeTag.localName().impl());
        tagList.add(inputTag.localName().impl());
        tagList.add(keygenTag.localName().impl());
        tagList.add(selectTag.localName().impl());
        tagList.add(textareaTag.localName().impl());
        tagList.add(labelTag.localName().impl());
        tagList.add(buttonTag.localName().impl());
        tagList.add(insTag.localName().impl());
        tagList.add(delTag.localName().impl());
        tagList.add(nobrTag.localName().impl());
        tagList.add(wbrTag.localName().impl());
    }
    return &tagList;
}

HashSet<DOMStringImpl*, PointerHash<DOMStringImpl*> >* blockTagList()
{
    static HashSet<DOMStringImpl*, PointerHash<DOMStringImpl*> > tagList;
    if (tagList.isEmpty()) {
        tagList.add(pTag.localName().impl());
        tagList.add(h1Tag.localName().impl());
        tagList.add(h2Tag.localName().impl());
        tagList.add(h3Tag.localName().impl());
        tagList.add(h4Tag.localName().impl());
        tagList.add(h5Tag.localName().impl());
        tagList.add(h6Tag.localName().impl());
        tagList.add(ulTag.localName().impl());
        tagList.add(olTag.localName().impl());
        tagList.add(dirTag.localName().impl());
        tagList.add(menuTag.localName().impl());
        tagList.add(preTag.localName().impl());
        tagList.add(plaintextTag.localName().impl());
        tagList.add(xmpTag.localName().impl());
        tagList.add(dlTag.localName().impl());
        tagList.add(divTag.localName().impl());
        tagList.add(layerTag.localName().impl());
        tagList.add(centerTag.localName().impl());
        tagList.add(noscriptTag.localName().impl());
        tagList.add(noframesTag.localName().impl());
        tagList.add(noembedTag.localName().impl());
        tagList.add(nolayerTag.localName().impl());
        tagList.add(blockquoteTag.localName().impl());
        tagList.add(formTag.localName().impl());
        tagList.add(isindexTag.localName().impl());
        tagList.add(hrTag.localName().impl());
        tagList.add(tableTag.localName().impl());
        tagList.add(fieldsetTag.localName().impl());
        tagList.add(addressTag.localName().impl());
        tagList.add(liTag.localName().impl());
        tagList.add(ddTag.localName().impl());
        tagList.add(dtTag.localName().impl());
        tagList.add(marqueeTag.localName().impl());
    }
    return &tagList;
}

bool HTMLElementImpl::inEitherTagList(const NodeImpl* newChild)
{
    if (newChild->isTextNode())
        return true;
        
    if (newChild->isHTMLElement()) {
        const HTMLElementImpl* child = static_cast<const HTMLElementImpl*>(newChild);
        if (inlineTagList()->contains(child->tagName().localName().impl()))
            return true;
        if (blockTagList()->contains(child->tagName().localName().impl()))
            return true;
        return !isRecognizedTagName(child->tagName()); // Accept custom html tags
    }

    return false;
}

bool HTMLElementImpl::inInlineTagList(const NodeImpl* newChild)
{
    if (newChild->isTextNode())
        return true;

    if (newChild->isHTMLElement()) {
        const HTMLElementImpl* child = static_cast<const HTMLElementImpl*>(newChild);
        if (inlineTagList()->contains(child->tagName().localName().impl()))
            return true;
        return !isRecognizedTagName(child->tagName()); // Accept custom html tags
    }

    return false;
}

bool HTMLElementImpl::inBlockTagList(const NodeImpl* newChild)
{
    if (newChild->isTextNode())
        return true;
            
    if (newChild->isHTMLElement()) {
        const HTMLElementImpl* child = static_cast<const HTMLElementImpl*>(newChild);
        return (blockTagList()->contains(child->tagName().localName().impl()));
    }

    return false;
}

bool HTMLElementImpl::checkDTD(const NodeImpl* newChild)
{
    if (hasTagName(addressTag) && newChild->hasTagName(pTag))
        return true;
    return inEitherTagList(newChild);
}

void HTMLElementImpl::setHTMLEventListener(const AtomicString& eventType, AttributeImpl* attr)
{
    ElementImpl::setHTMLEventListener(eventType, getDocument()->createHTMLEventListener(attr->value(), this));
}

}
