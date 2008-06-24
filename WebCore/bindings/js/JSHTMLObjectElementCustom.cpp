/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
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
#include "JSHTMLObjectElementCustom.h"

#include "HTMLObjectElement.h"
#include "JSPluginElementFunctions.h"

namespace WebCore {

using namespace KJS;

bool JSHTMLObjectElement::customGetOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    return runtimeObjectCustomGetOwnPropertySlot(exec, propertyName, slot, this);
}

bool JSHTMLObjectElement::customPut(ExecState* exec, const Identifier& propertyName, JSValue* value)
{
    return runtimeObjectCustomPut(exec, propertyName, value, impl());
}

CallType JSHTMLObjectElement::getCallData(CallData& callData)
{
    return runtimeObjectGetCallData(impl(), callData);
}

bool JSHTMLObjectElement::canGetItemsForName(ExecState*, HTMLObjectElement*, const Identifier& propertyName)
{
    return propertyName == "__apple_runtime_object";
}

JSValue* JSHTMLObjectElement::nameGetter(ExecState* exec, const Identifier& propertyName, const PropertySlot& slot)
{
    return runtimeObjectGetter(exec, propertyName, slot);
}

} // namespace WebCore
