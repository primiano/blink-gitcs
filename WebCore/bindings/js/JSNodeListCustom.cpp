/*
 * Copyright (C) 2007 Apple Inc. All rights reserved.
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
#include "JSNodeList.h"

#include "AtomicString.h"
#include "JSNode.h"
#include "Node.h"
#include "NodeList.h"

using namespace KJS;

namespace WebCore {

// Need to support call so that list(0) works.
static JSValue* callNodeList(ExecState* exec, JSObject* function, JSValue*, const ArgList& args)
{
    bool ok;
    unsigned index = args[0]->toString(exec).toUInt32(&ok);
    if (!ok)
        return jsUndefined();
    return toJS(exec, static_cast<JSNodeList*>(function)->impl()->item(index));
}

CallType JSNodeList::getCallData(CallData& callData)
{
    callData.native.function = callNodeList;
    return CallTypeNative;
}

bool JSNodeList::canGetItemsForName(ExecState*, NodeList* impl, const Identifier& propertyName)
{
    return impl->itemWithName(propertyName);
}

JSValue* JSNodeList::nameGetter(ExecState* exec, const Identifier& propertyName, const PropertySlot& slot)
{
    JSNodeList* thisObj = static_cast<JSNodeList*>(slot.slotBase());
    return toJS(exec, thisObj->impl()->itemWithName(propertyName));
}

} // namespace WebCore
