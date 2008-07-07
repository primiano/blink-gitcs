/*
 *  Copyright (C) 1999-2002 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2004, 2007, 2008 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "JSString.h"

#include "JSGlobalObject.h"
#include "JSObject.h"
#include "StringObject.h"
#include "StringPrototype.h"

namespace KJS {

JSValue* JSString::toPrimitive(ExecState*, JSType) const
{
    return const_cast<JSString*>(this);
}

bool JSString::getPrimitiveNumber(ExecState*, double& number, JSValue*& value)
{
    value = this;
    number = m_value.toDouble();
    return false;
}

bool JSString::toBoolean(ExecState*) const
{
    return !m_value.isEmpty();
}

double JSString::toNumber(ExecState*) const
{
    return m_value.toDouble();
}

UString JSString::toString(ExecState*) const
{
    return m_value;
}

UString JSString::toThisString(ExecState*) const
{
    return m_value;
}

JSString* JSString::toThisJSString(ExecState*)
{
    return this;
}

inline StringObject* StringObject::create(ExecState* exec, JSString* string)
{
    return new (exec) StringObject(exec->lexicalGlobalObject()->stringPrototype(), string);
}

JSObject* JSString::toObject(ExecState* exec) const
{
    return StringObject::create(exec, const_cast<JSString*>(this));
}

JSObject* JSString::toThisObject(ExecState* exec) const
{
    return StringObject::create(exec, const_cast<JSString*>(this));
}

JSValue* JSString::lengthGetter(ExecState* exec, const Identifier&, const PropertySlot& slot)
{
    return jsNumber(exec, static_cast<JSString*>(slot.slotBase())->value().size());
}

JSValue* JSString::indexGetter(ExecState* exec, const Identifier&, const PropertySlot& slot)
{
    return jsString(exec, static_cast<JSString*>(slot.slotBase())->value().substr(slot.index(), 1));
}

JSValue* JSString::indexNumericPropertyGetter(ExecState* exec, unsigned index, const PropertySlot& slot)
{
    return jsString(exec, static_cast<JSString*>(slot.slotBase())->value().substr(index, 1));
}

bool JSString::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    // The semantics here are really getPropertySlot, not getOwnPropertySlot.
    // This function should only be called by JSValue::get.
    if (getStringPropertySlot(exec, propertyName, slot))
        return true;
    slot.setBase(this);
    JSObject* object;
    for (JSValue* prototype = exec->lexicalGlobalObject()->stringPrototype(); prototype != jsNull(); prototype = object->prototype()) {
        ASSERT(prototype->isObject());
        object = static_cast<JSObject*>(prototype);
        if (object->getOwnPropertySlot(exec, propertyName, slot))
            return true;
    }
    slot.setUndefined();
    return true;
}

bool JSString::getOwnPropertySlot(ExecState* exec, unsigned propertyName, PropertySlot& slot)
{
    // The semantics here are really getPropertySlot, not getOwnPropertySlot.
    // This function should only be called by JSValue::get.
    if (getStringPropertySlot(propertyName, slot))
        return true;
    return JSString::getOwnPropertySlot(exec, Identifier::from(exec, propertyName), slot);
}

JSString* jsString(ExecState* exec, const char* s)
{
    return new (exec) JSString(s ? s : "");
}

JSString* jsString(ExecState* exec, const UString& s)
{
    return s.isNull() ? new (exec) JSString("") : new (exec) JSString(s);
}

JSString* jsOwnedString(ExecState* exec, const UString& s)
{
    return s.isNull() ? new (exec) JSString("", JSString::HasOtherOwner) : new (exec) JSString(s, JSString::HasOtherOwner);
}

} // namespace KJS
