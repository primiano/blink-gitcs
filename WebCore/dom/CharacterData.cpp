/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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
#include "CharacterData.h"

#include "CString.h"
#include "Document.h"
#include "EventNames.h"
#include "ExceptionCode.h"
#include "TextStream.h"
#include "MutationEvent.h"
#include "RenderText.h"

namespace WebCore {

using namespace EventNames;

CharacterData::CharacterData(Document *doc)
    : EventTargetNode(doc)
{
}

CharacterData::CharacterData(Document *doc, const String &_text)
    : EventTargetNode(doc)
{
    str = _text.impl() ? _text.impl() : StringImpl::empty();
}

CharacterData::~CharacterData()
{
}

void CharacterData::setData(const String& data, ExceptionCode& ec)
{
    // NO_MODIFICATION_ALLOWED_ERR: Raised when the node is readonly
    if (isReadOnlyNode()) {
        ec = NO_MODIFICATION_ALLOWED_ERR;
        return;
    }

    if (equal(str.get(), data.impl()))
        return;

    RefPtr<StringImpl> oldStr = str;
    str = data.impl();
    
    if ((!renderer() || !rendererIsNeeded(renderer()->style())) && attached()) {
        detach();
        attach();
    } else if (renderer())
        static_cast<RenderText*>(renderer())->setText(str);
    
    dispatchModifiedEvent(oldStr.get());
    
    document()->removeMarkers(this);
}

String CharacterData::substringData( const unsigned offset, const unsigned count, ExceptionCode& ec)
{
    ec = 0;
    checkCharDataOperation(offset, ec);
    if (ec)
        return String();

    return str->substring(offset, count);
}

void CharacterData::appendData( const String &arg, ExceptionCode& ec)
{
    ec = 0;

    // NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly
    if (isReadOnlyNode()) {
        ec = NO_MODIFICATION_ALLOWED_ERR;
        return;
    }

    String newStr = str;
    newStr.append(arg);

    RefPtr<StringImpl> oldStr = str;
    str = newStr.impl();

    if ((!renderer() || !rendererIsNeeded(renderer()->style())) && attached()) {
        detach();
        attach();
    } else if (renderer())
        static_cast<RenderText*>(renderer())->setTextWithOffset(str, oldStr->length(), 0);
    
    dispatchModifiedEvent(oldStr.get());
}

void CharacterData::insertData( const unsigned offset, const String &arg, ExceptionCode& ec)
{
    ec = 0;
    checkCharDataOperation(offset, ec);
    if (ec)
        return;

    String newStr = str;
    newStr.insert(arg, offset);

    RefPtr<StringImpl> oldStr = str;
    str = newStr.impl();

    if ((!renderer() || !rendererIsNeeded(renderer()->style())) && attached()) {
        detach();
        attach();
    } else if (renderer())
        static_cast<RenderText*>(renderer())->setTextWithOffset(str, offset, 0);
    
    dispatchModifiedEvent(oldStr.get());
    
    // update the markers for spell checking and grammar checking
    unsigned length = arg.length();
    document()->shiftMarkers(this, offset, length);
}

void CharacterData::deleteData( const unsigned offset, const unsigned count, ExceptionCode& ec)
{
    ec = 0;
    checkCharDataOperation(offset, ec);
    if (ec)
        return;

    String newStr = str;
    newStr.remove(offset, count);

    RefPtr<StringImpl> oldStr = str;
    str = newStr.impl();
    
    if ((!renderer() || !rendererIsNeeded(renderer()->style())) && attached()) {
        detach();
        attach();
    } else if (renderer())
        static_cast<RenderText*>(renderer())->setTextWithOffset(str, offset, count);

    dispatchModifiedEvent(oldStr.get());

    // update the markers for spell checking and grammar checking
    document()->removeMarkers(this, offset, count);
    document()->shiftMarkers(this, offset + count, -static_cast<int>(count));
}

void CharacterData::replaceData( const unsigned offset, const unsigned count, const String &arg, ExceptionCode& ec)
{
    ec = 0;
    checkCharDataOperation(offset, ec);
    if (ec)
        return;

    unsigned realCount;
    if (offset + count > str->length())
        realCount = str->length()-offset;
    else
        realCount = count;

    String newStr = str;
    newStr.remove(offset, realCount);
    newStr.insert(arg, offset);

    RefPtr<StringImpl> oldStr = str;
    str = newStr.impl();

    if ((!renderer() || !rendererIsNeeded(renderer()->style())) && attached()) {
        detach();
        attach();
    } else if (renderer())
        static_cast<RenderText*>(renderer())->setTextWithOffset(str, offset, count);
    
    dispatchModifiedEvent(oldStr.get());
    
    // update the markers for spell checking and grammar checking
    int diff = arg.length() - count;
    document()->removeMarkers(this, offset, count);
    document()->shiftMarkers(this, offset + count, diff);
}

String CharacterData::nodeValue() const
{
    return str;
}

bool CharacterData::containsOnlyWhitespace() const
{
    if (str)
        return str->containsOnlyWhitespace();
    return true;
}

void CharacterData::setNodeValue( const String &_nodeValue, ExceptionCode& ec)
{
    // NO_MODIFICATION_ALLOWED_ERR: taken care of by setData()
    setData(_nodeValue, ec);
}

void CharacterData::dispatchModifiedEvent(StringImpl *prevValue)
{
    if (parentNode())
        parentNode()->childrenChanged();
    if (document()->hasListenerType(Document::DOMCHARACTERDATAMODIFIED_LISTENER)) {
        ExceptionCode ec;
        dispatchEvent(new MutationEvent(DOMCharacterDataModifiedEvent, true, false, 0, prevValue, str, String(), 0), ec);
    }
    dispatchSubtreeModifiedEvent();
}

void CharacterData::checkCharDataOperation( const unsigned offset, ExceptionCode& ec)
{
    ec = 0;

    // INDEX_SIZE_ERR: Raised if the specified offset is negative or greater than the number of 16-bit
    // units in data.
    if (offset > str->length()) {
        ec = INDEX_SIZE_ERR;
        return;
    }

    // NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly
    if (isReadOnlyNode()) {
        ec = NO_MODIFICATION_ALLOWED_ERR;
        return;
    }
}

int CharacterData::maxCharacterOffset() const 
{
    return (int)length();
}

bool CharacterData::rendererIsNeeded(RenderStyle *style)
{
    if (!str || str->length() == 0)
        return false;
    return EventTargetNode::rendererIsNeeded(style);
}

bool CharacterData::offsetInCharacters() const
{
    return true;
}

#ifndef NDEBUG
void CharacterData::dump(TextStream *stream, DeprecatedString ind) const
{
    *stream << " str=\"" << String(str).utf8().data() << "\"";

    EventTargetNode::dump(stream,ind);
}
#endif

} // namespace WebCore
