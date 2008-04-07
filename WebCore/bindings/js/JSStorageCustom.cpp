/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "JSStorage.h"

#if ENABLE(DOM_STORAGE)

#include "PlatformString.h"
#include "Storage.h"

using namespace KJS;

namespace WebCore {

bool JSStorage::canGetItemsForName(ExecState*, Storage* impl, const Identifier& propertyName)
{
    return impl->contains(propertyName);
}

JSValue* JSStorage::nameGetter(ExecState* exec, JSObject* originalObject, const Identifier& propertyName, const PropertySlot& slot)
{
    JSStorage* thisObj = static_cast<JSStorage*>(slot.slotBase());
    return jsStringOrNull(thisObj->impl()->getItem(propertyName));
}

bool JSStorage::customPut(ExecState* exec, const Identifier& propertyName, JSValue* value)
{
    // We need to only perform the custom put if the object doesn't have a native property by this name.
    // since hasProperty() would end up calling canGetItemsForName() and be fooled, we need to check
    // the native property slots manually.
    PropertySlot slot;
    if (getStaticValueSlot<JSStorage, Base>(exec, s_info.propHashTable, this, propertyName, slot))
        return false;
        
    JSValue* prototype = this->prototype();
    if (prototype->isObject() && static_cast<JSObject*>(prototype)->hasProperty(exec, propertyName))
        return false;
    
    String stringValue = valueToStringWithNullCheck(exec, value);
    if (exec->hadException())
        return true;
    
    ExceptionCode ec = 0;
    impl()->setItem(propertyName, stringValue, ec);
    setDOMException(exec, ec);

    return true;
}

} // namespace WebCore

#endif // ENABLE(DOM_STORAGE)
