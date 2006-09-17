/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006 Apple Computer, Inc.
 *           (C) 2006 Alexey Proskuryakov (ap@nypop.com)
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
#include "HTMLSelectElement.h"

#include "CSSPropertyNames.h"
#include "Document.h"
#include "DeprecatedRenderSelect.h"
#include "Event.h"
#include "EventNames.h"
#include "FormDataList.h"
#include "HTMLFormElement.h"
#include "HTMLNames.h"
#include "HTMLOptionElement.h"
#include "HTMLOptionsCollection.h"
#include "KeyboardEvent.h"
#include "RenderMenuList.h"
#include "RenderPopupMenu.h"
#include "cssstyleselector.h"
#include <wtf/Vector.h>

#if PLATFORM(MAC)
#define ARROW_KEYS_POP_MENU 1
#else
#define ARROW_KEYS_POP_MENU 0
#endif

namespace WebCore {

using namespace EventNames;
using namespace HTMLNames;

HTMLSelectElement::HTMLSelectElement(Document* doc, HTMLFormElement* f)
    : HTMLGenericFormElement(selectTag, doc, f)
    , m_minwidth(0)
    , m_size(0)
    , m_multiple(false)
    , m_recalcListItems(false)
{
    document()->registerFormElementWithState(this);
}

HTMLSelectElement::HTMLSelectElement(const QualifiedName& tagName, Document* doc, HTMLFormElement* f)
    : HTMLGenericFormElement(tagName, doc, f), m_minwidth(0), m_size(0), m_multiple(false), m_recalcListItems(false)
{
    document()->registerFormElementWithState(this);
}

HTMLSelectElement::~HTMLSelectElement()
{
    document()->deregisterFormElementWithState(this);
}

bool HTMLSelectElement::checkDTD(const Node* newChild)
{
    return newChild->isTextNode() || newChild->hasTagName(optionTag) || newChild->hasTagName(optgroupTag) || newChild->hasTagName(hrTag) ||
           newChild->hasTagName(scriptTag);
}

void HTMLSelectElement::recalcStyle( StyleChange ch )
{
    if (hasChangedChild() && renderer()) {
        if (shouldUseMenuList())
            static_cast<RenderMenuList*>(renderer())->setOptionsChanged(true);
        else
            static_cast<DeprecatedRenderSelect*>(renderer())->setOptionsChanged(true);
    }

    HTMLGenericFormElement::recalcStyle( ch );
}


const AtomicString& HTMLSelectElement::type() const
{
    static const AtomicString selectMultiple("select-multiple");
    static const AtomicString selectOne("select-one");
    return m_multiple ? selectMultiple : selectOne;
}

int HTMLSelectElement::selectedIndex() const
{
    // return the number of the first option selected
    unsigned o = 0;
    const Vector<HTMLElement*>& items = listItems();
    for (unsigned int i = 0; i < items.size(); i++) {
        if (items[i]->hasLocalName(optionTag)) {
            if (static_cast<HTMLOptionElement*>(items[i])->selected())
                return o;
            o++;
        }
    }
    return -1;
}

void HTMLSelectElement::setSelectedIndex(int index, bool deselect)
{
    // deselect all other options and select only the new one
    const Vector<HTMLElement*>& items = listItems();
    int listIndex;
    if (deselect) {
        for (listIndex = 0; listIndex < int(items.size()); listIndex++) {
            if (items[listIndex]->hasLocalName(optionTag))
                static_cast<HTMLOptionElement*>(items[listIndex])->setSelected(false);
        }
    }
    listIndex = optionToListIndex(index);
    if (listIndex >= 0)
        static_cast<HTMLOptionElement*>(items[listIndex])->setSelected(true);

    setChanged(true);
}

int HTMLSelectElement::length() const
{
    int len = 0;
    unsigned i;
    const Vector<HTMLElement*>& items = listItems();
    for (i = 0; i < items.size(); i++) {
        if (items[i]->hasLocalName(optionTag))
            len++;
    }
    return len;
}

void HTMLSelectElement::add( HTMLElement *element, HTMLElement *before, ExceptionCode& ec)
{
    RefPtr<HTMLElement> protectNewChild(element); // make sure the element is ref'd and deref'd so we don't leak it

    if (!element || !(element->hasLocalName(optionTag) || element->hasLocalName(hrTag)))
        return;

    insertBefore(element, before, ec);
    if (!ec)
        setRecalcListItems();
}

void HTMLSelectElement::remove(int index)
{
    ExceptionCode ec = 0;
    int listIndex = optionToListIndex(index);

    const Vector<HTMLElement*>& items = listItems();
    if (listIndex < 0 || index >= int(items.size()))
        return; // ### what should we do ? remove the last item?

    removeChild(items[listIndex], ec);
    if (!ec)
        setRecalcListItems();
}

String HTMLSelectElement::value()
{
    unsigned i;
    const Vector<HTMLElement*>& items = listItems();
    for (i = 0; i < items.size(); i++) {
        if (items[i]->hasLocalName(optionTag) && static_cast<HTMLOptionElement*>(items[i])->selected())
            return static_cast<HTMLOptionElement*>(items[i])->value();
    }
    return String("");
}

void HTMLSelectElement::setValue(const String &value)
{
    if (value.isNull())
        return;
    // find the option with value() matching the given parameter
    // and make it the current selection.
    const Vector<HTMLElement*>& items = listItems();
    for (unsigned i = 0; i < items.size(); i++)
        if (items[i]->hasLocalName(optionTag) && static_cast<HTMLOptionElement*>(items[i])->value() == value) {
            static_cast<HTMLOptionElement*>(items[i])->setSelected(true);
            return;
        }
}

String HTMLSelectElement::stateValue() const
{
    const Vector<HTMLElement*>& items = listItems();
    int l = items.size();
    Vector<char, 1024> characters(l);
    for (int i = 0; i < l; ++i) {
        HTMLElement* e = items[i];
        bool selected = e->hasLocalName(optionTag) && static_cast<HTMLOptionElement*>(e)->selected();
        characters[i] = selected ? 'X' : '.';
    }
    return String(characters, l);
}

void HTMLSelectElement::restoreState(const String& state)
{
    recalcListItems();

    const Vector<HTMLElement*>& items = listItems();
    int l = items.size();
    for (int i = 0; i < l; i++)
        if (items[i]->hasLocalName(optionTag))
            static_cast<HTMLOptionElement*>(items[i])->setSelected(state[i] == 'X');
    setChanged(true);
}

bool HTMLSelectElement::insertBefore(PassRefPtr<Node> newChild, Node* refChild, ExceptionCode& ec)
{
    bool result = HTMLGenericFormElement::insertBefore(newChild, refChild, ec);
    if (result)
        setRecalcListItems();
    return result;
}

bool HTMLSelectElement::replaceChild(PassRefPtr<Node> newChild, Node *oldChild, ExceptionCode& ec)
{
    bool result = HTMLGenericFormElement::replaceChild(newChild, oldChild, ec);
    if (result)
        setRecalcListItems();
    return result;
}

bool HTMLSelectElement::removeChild(Node* oldChild, ExceptionCode& ec)
{
    bool result = HTMLGenericFormElement::removeChild(oldChild, ec);
    if (result)
        setRecalcListItems();
    return result;
}

bool HTMLSelectElement::appendChild(PassRefPtr<Node> newChild, ExceptionCode& ec)
{
    bool result = HTMLGenericFormElement::appendChild(newChild, ec);
    if (result)
        setRecalcListItems();
    return result;
}

ContainerNode* HTMLSelectElement::addChild(PassRefPtr<Node> newChild)
{
    ContainerNode* result = HTMLGenericFormElement::addChild(newChild);
    if (result)
        setRecalcListItems();
    return result;
}

void HTMLSelectElement::parseMappedAttribute(MappedAttribute *attr)
{
    bool oldShouldUseMenuList = shouldUseMenuList();
    if (attr->name() == sizeAttr) {
        m_size = max(attr->value().toInt(), 1);
        if (oldShouldUseMenuList != shouldUseMenuList() && attached()) {
            detach();
            attach();
        }
    } else if (attr->name() == widthAttr) {
        m_minwidth = max(attr->value().toInt(), 0);
    } else if (attr->name() == multipleAttr) {
        m_multiple = (!attr->isNull());
        if (oldShouldUseMenuList != shouldUseMenuList() && attached()) {
            detach();
            attach();
        }
    } else if (attr->name() == accesskeyAttr) {
        // FIXME: ignore for the moment
    } else if (attr->name() == onfocusAttr) {
        setHTMLEventListener(focusEvent, attr);
    } else if (attr->name() == onblurAttr) {
        setHTMLEventListener(blurEvent, attr);
    } else if (attr->name() == onchangeAttr) {
        setHTMLEventListener(changeEvent, attr);
    } else
        HTMLGenericFormElement::parseMappedAttribute(attr);
}

bool HTMLSelectElement::isKeyboardFocusable() const
{
    if (renderer() && shouldUseMenuList())
        return isFocusable();
    return HTMLGenericFormElement::isKeyboardFocusable();
}

bool HTMLSelectElement::isMouseFocusable() const
{
    if (renderer() && shouldUseMenuList())
        return isFocusable();
    return HTMLGenericFormElement::isMouseFocusable();
}

RenderObject *HTMLSelectElement::createRenderer(RenderArena *arena, RenderStyle *style)
{
    if (shouldUseMenuList())
        return new (arena) RenderMenuList(this);
    return new (arena) DeprecatedRenderSelect(this);
}

bool HTMLSelectElement::appendFormData(FormDataList& list, bool)
{
    bool successful = false;
    const Vector<HTMLElement*>& items = listItems();

    unsigned i;
    for (i = 0; i < items.size(); i++) {
        if (items[i]->hasLocalName(optionTag)) {
            HTMLOptionElement *option = static_cast<HTMLOptionElement*>(items[i]);
            if (option->selected()) {
                list.appendData(name(), option->value());
                successful = true;
            }
        }
    }

    // ### this case should not happen. make sure that we select the first option
    // in any case. otherwise we have no consistency with the DOM interface. FIXME!
    // we return the first one if it was a combobox select
    if (!successful && !m_multiple && m_size <= 1 && items.size() &&
        (items[0]->hasLocalName(optionTag))) {
        HTMLOptionElement *option = static_cast<HTMLOptionElement*>(items[0]);
        if (option->value().isNull())
            list.appendData(name(), option->text().stripWhiteSpace());
        else
            list.appendData(name(), option->value());
        successful = true;
    }

    return successful;
}

int HTMLSelectElement::optionToListIndex(int optionIndex) const
{
    const Vector<HTMLElement*>& items = listItems();
    if (optionIndex < 0 || optionIndex >= int(items.size()))
        return -1;

    int listIndex = 0;
    int optionIndex2 = 0;
    for (;
         optionIndex2 < int(items.size()) && optionIndex2 <= optionIndex;
         listIndex++) { // not a typo!
        if (items[listIndex]->hasLocalName(optionTag))
            optionIndex2++;
    }
    listIndex--;
    return listIndex;
}

int HTMLSelectElement::listToOptionIndex(int listIndex) const
{
    const Vector<HTMLElement*>& items = listItems();
    if (listIndex < 0 || listIndex >= int(items.size()) ||
        !items[listIndex]->hasLocalName(optionTag))
        return -1;

    int optionIndex = 0; // actual index of option not counting OPTGROUP entries that may be in list
    for (int i = 0; i < listIndex; i++)
        if (items[i]->hasLocalName(optionTag))
            optionIndex++;
    return optionIndex;
}

PassRefPtr<HTMLOptionsCollection> HTMLSelectElement::options()
{
    return new HTMLOptionsCollection(this);
}

void HTMLSelectElement::recalcListItems() const
{
    Node* current = firstChild();
    m_listItems.clear();
    HTMLOptionElement* foundSelected = 0;
    while (current) {
        if (current->hasTagName(optgroupTag) && current->firstChild()) {
            // ### what if optgroup contains just comments? don't want one of no options in it...
            m_listItems.append(static_cast<HTMLElement*>(current));
            current = current->firstChild();
        }
        if (current->hasTagName(optionTag)) {
            m_listItems.append(static_cast<HTMLElement*>(current));
            if (!foundSelected && !m_multiple && m_size <= 1) {
                foundSelected = static_cast<HTMLOptionElement*>(current);
                foundSelected->m_selected = true;
            } else if (foundSelected && !m_multiple && static_cast<HTMLOptionElement*>(current)->selected()) {
                foundSelected->m_selected = false;
                foundSelected = static_cast<HTMLOptionElement*>(current);
            }
        }
        if (current->hasTagName(hrTag))
            m_listItems.append(static_cast<HTMLElement*>(current));

        Node* parent = current->parentNode();
        current = current->nextSibling();
        if (!current) {
            if (parent != this)
                current = parent->nextSibling();
        }
    }
    m_recalcListItems = false;
}

void HTMLSelectElement::childrenChanged()
{
    setRecalcListItems();

    HTMLGenericFormElement::childrenChanged();
}

void HTMLSelectElement::setRecalcListItems()
{
    m_recalcListItems = true;
    if (renderer()) {
        if (shouldUseMenuList())
            static_cast<RenderMenuList*>(renderer())->setOptionsChanged(true);
        else
            static_cast<DeprecatedRenderSelect*>(renderer())->setOptionsChanged(true);
    }
    setChanged();
}

void HTMLSelectElement::reset()
{
    bool optionSelected = false;
    HTMLOptionElement* firstOption = 0;
    const Vector<HTMLElement*>& items = listItems();
    unsigned i;
    for (i = 0; i < items.size(); i++) {
        if (items[i]->hasLocalName(optionTag)) {
            HTMLOptionElement *option = static_cast<HTMLOptionElement*>(items[i]);
            if (!option->getAttribute(selectedAttr).isNull()) {
                option->setSelected(true);
                optionSelected = true;
            } else
                option->setSelected(false);
            if (!firstOption)
                firstOption = option;
        }
    }
    if (!optionSelected && firstOption)
        firstOption->setSelected(true);
    if (renderer() && !shouldUseMenuList())
        static_cast<DeprecatedRenderSelect*>(renderer())->setSelectionChanged(true);
    setChanged(true);
}

void HTMLSelectElement::notifyOptionSelected(HTMLOptionElement *selectedOption, bool selected)
{
    if (selected && !m_multiple) {
        // deselect all other options
        const Vector<HTMLElement*>& items = listItems();
        unsigned i;
        for (i = 0; i < items.size(); i++) {
            if (items[i]->hasLocalName(optionTag))
                static_cast<HTMLOptionElement*>(items[i])->m_selected = (items[i] == selectedOption);
        }
    }
    if (renderer() && !shouldUseMenuList())
        static_cast<DeprecatedRenderSelect*>(renderer())->setSelectionChanged(true);

    setChanged(true);
}

void HTMLSelectElement::defaultEventHandler(Event *evt)
{
    RenderMenuList* menuList = static_cast<RenderMenuList*>(renderer());

    // Use key press event here since sending simulated mouse events
    // on key down blocks the proper sending of the key press event.
    if (evt->type() == keypressEvent) {
        if (!renderer() || !evt->isKeyboardEvent())
            return;
        String keyIdentifier = static_cast<KeyboardEvent*>(evt)->keyIdentifier();
        bool handled = false;
#if ARROW_KEYS_POP_MENU
        if (form() && keyIdentifier == "Enter") {
            blur();
            // Make sure the form hasn't been destroyed during the blur.
            if (form())
                form()->submitClick();
            handled = true;
        }
        if ((keyIdentifier == "Down" || keyIdentifier == "Up" || keyIdentifier == "U+000020") && renderer() && shouldUseMenuList()) {
            focus();
            menuList->showPopup();
            handled = true;
        }
#else
        int index = optionToListIndex(selectedIndex());
        if (keyIdentifier == "Down" || keyIdentifier == "Right") {
            if (index < listItems().size() - 1)
                setSelectedIndex(++index);
            handled = true;
        } else if (keyIdentifier == "Up" || keyIdentifier == "Left") {
            if (index > 0)
                setSelectedIndex(--index);
            handled = true;
        }
#endif
        if (handled)
            evt->setDefaultHandled();

    }
    if (evt->type() == mousedownEvent && renderer() && shouldUseMenuList()) {
        focus();
        if (menuList->popupIsVisible())
            menuList->hidePopup();
        else
            menuList->showPopup();
        evt->setDefaultHandled();
    }

    HTMLGenericFormElement::defaultEventHandler(evt);
}

void HTMLSelectElement::accessKeyAction(bool sendToAnyElement)
{
    // send the mouse button events iff the
    // caller specified sendToAnyElement
    click(sendToAnyElement);
}

void HTMLSelectElement::setMultiple(bool multiple)
{
    setAttribute(multipleAttr, multiple ? "" : 0);
}

void HTMLSelectElement::setSize(int size)
{
    setAttribute(sizeAttr, String::number(size));
}

Node* HTMLSelectElement::namedItem(const String &name, bool caseSensitive)
{
    return (options()->namedItem(name, caseSensitive));
}

void HTMLSelectElement::setOption(unsigned index, HTMLOptionElement* option, ExceptionCode& ec)
{
    ec = 0;
    if (index > INT_MAX)
        index = INT_MAX;
    int diff = index  - length();
    HTMLElement* before = 0;
    // out of array bounds ? first insert empty dummies
    if (diff > 0) {
        setLength(index, ec);
        // replace an existing entry ?
    } else if (diff < 0) {
        before = static_cast<HTMLElement*>(options()->item(index+1));
        remove(index);
    }
    // finally add the new element
    if (ec == 0) {
        add(option, before, ec);
        if (diff >= 0 && option->selected())
            setSelectedIndex(index, !m_multiple);
    }
}

void HTMLSelectElement::setLength(unsigned newLen, ExceptionCode& ec)
{
    ec = 0;
    if (newLen > INT_MAX)
        newLen = INT_MAX;
    int diff = length() - newLen;

    if (diff < 0) { // add dummy elements
        do {
            RefPtr<Element> option = ownerDocument()->createElement("option", ec);
            if (!option)
                break;
            add(static_cast<HTMLElement*>(option.get()), 0, ec);
            if (ec)
                break;
        } while (++diff);
    }
    else // remove elements
        while (diff-- > 0)
            remove(newLen);
}

} // namespace
