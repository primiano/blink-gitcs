/*
 * Copyright (C) 2007, 2008, 2009 Apple Inc. All rights reserved.
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

#ifndef CheckedRadioButtons_h
#define CheckedRadioButtons_h

#include <wtf/HashMap.h>
#include <wtf/OwnPtr.h>

namespace WebCore {

class AtomicString;
class AtomicStringImpl;
class HTMLInputElement;
class HTMLFormControlElement;

class CheckedRadioButtons {
public:
    void addButton(HTMLFormControlElement*);
    void removeButton(HTMLFormControlElement*);
    HTMLInputElement* checkedButtonForGroup(const AtomicString& groupName) const;

private:
    typedef HashMap<AtomicStringImpl*, HTMLInputElement*> NameToInputMap;
    OwnPtr<NameToInputMap> m_nameToCheckedRadioButtonMap;
};

} // namespace WebCore

#endif // CheckedRadioButtons_h
