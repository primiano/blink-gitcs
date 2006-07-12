// -*- mode: c++; c-basic-offset: 4 -*-
/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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

#include "APICast.h"
#include "JSCallbackObject.h"
#include "JSValueRef.h"

#include <kjs/JSType.h>
#include <kjs/internal.h>
#include <kjs/operations.h>
#include <kjs/protect.h>
#include <kjs/ustring.h>
#include <kjs/value.h>

#include <wtf/Assertions.h>

#include <algorithm> // for std::min

JSType JSValueGetType(JSValueRef value)
{
    KJS::JSValue* jsValue = toJS(value);
    switch (jsValue->type()) {
        case KJS::UndefinedType:
            return kJSTypeUndefined;
        case KJS::NullType:
            return kJSTypeNull;
        case KJS::BooleanType:
            return kJSTypeBoolean;
        case KJS::NumberType:
            return kJSTypeNumber;
        case KJS::StringType:
            return kJSTypeString;
        case KJS::ObjectType:
            return kJSTypeObject;
        default:
            ASSERT(!"JSValueGetType: unknown type code.\n");
            return kJSTypeUndefined;
    }
}

using namespace KJS; // placed here to avoid conflict between KJS::JSType and JSType, above.

bool JSValueIsUndefined(JSValueRef value)
{
    JSValue* jsValue = toJS(value);
    return jsValue->isUndefined();
}

bool JSValueIsNull(JSValueRef value)
{
    JSValue* jsValue = toJS(value);
    return jsValue->isNull();
}

bool JSValueIsBoolean(JSValueRef value)
{
    JSValue* jsValue = toJS(value);
    return jsValue->isBoolean();
}

bool JSValueIsNumber(JSValueRef value)
{
    JSValue* jsValue = toJS(value);
    return jsValue->isNumber();
}

bool JSValueIsString(JSValueRef value)
{
    JSValue* jsValue = toJS(value);
    return jsValue->isString();
}

bool JSValueIsObject(JSValueRef value)
{
    JSValue* jsValue = toJS(value);
    return jsValue->isObject();
}

bool JSValueIsObjectOfClass(JSValueRef value, JSClassRef jsClass)
{
    JSValue* jsValue = toJS(value);
    
    if (JSObject* o = jsValue->getObject())
        if (o->inherits(&JSCallbackObject::info))
            return static_cast<JSCallbackObject*>(o)->inherits(jsClass);
    return false;
}

bool JSValueIsEqual(JSContextRef context, JSValueRef a, JSValueRef b)
{
    JSLock lock;
    ExecState* exec = toJS(context);
    JSValue* jsA = toJS(a);
    JSValue* jsB = toJS(b);

    bool result = equal(exec, jsA, jsB);
    if (exec->hadException())
        exec->clearException();
    return result;
}

bool JSValueIsStrictEqual(JSContextRef context, JSValueRef a, JSValueRef b)
{
    JSLock lock;
    ExecState* exec = toJS(context);
    JSValue* jsA = toJS(a);
    JSValue* jsB = toJS(b);
    
    bool result = strictEqual(exec, jsA, jsB);
    if (exec->hadException())
        exec->clearException();
    return result;
}

bool JSValueIsInstanceOfConstructor(JSContextRef context, JSValueRef value, JSObjectRef constructor)
{
    ExecState* exec = toJS(context);
    JSValue* jsValue = toJS(value);
    JSObject* jsConstructor = toJS(constructor);
    if (!jsConstructor->implementsHasInstance())
        return false;
    bool result = jsConstructor->hasInstance(exec, jsValue);
    if (exec->hadException())
        exec->clearException();
    return result;
}

JSValueRef JSValueMakeUndefined()
{
    return toRef(jsUndefined());
}

JSValueRef JSValueMakeNull()
{
    return toRef(jsNull());
}

JSValueRef JSValueMakeBoolean(bool value)
{
    return toRef(jsBoolean(value));
}

JSValueRef JSValueMakeNumber(double value)
{
    JSLock lock;
    return toRef(jsNumber(value));
}

bool JSValueToBoolean(JSContextRef context, JSValueRef value)
{
    JSLock lock;
    ExecState* exec = toJS(context);
    JSValue* jsValue = toJS(value);
    
    bool boolean = jsValue->toBoolean(exec);
    if (exec->hadException()) {
        exec->clearException();
        boolean = false;
    }
    return boolean;
}

double JSValueToNumber(JSContextRef context, JSValueRef value)
{
    JSLock lock;
    JSValue* jsValue = toJS(value);
    ExecState* exec = toJS(context);

    double number = jsValue->toNumber(exec);
    if (exec->hadException()) {
        exec->clearException();
        number = NaN;
    }
    return number;
}

JSObjectRef JSValueToObject(JSContextRef context, JSValueRef value)
{
    JSLock lock;
    ExecState* exec = toJS(context);
    JSValue* jsValue = toJS(value);
    
    JSObjectRef objectRef = toRef(jsValue->toObject(exec));
    if (exec->hadException()) {
        exec->clearException();
        objectRef = 0;
    }
    return objectRef;
}    

void JSValueProtect(JSValueRef value)
{
    JSLock lock;
    JSValue* jsValue = toJS(value);
    gcProtect(jsValue);
}

void JSValueUnprotect(JSValueRef value)
{
    JSLock lock;
    JSValue* jsValue = toJS(value);
    gcUnprotect(jsValue);
}

void JSGarbageCollect()
{
    JSLock lock;
    Collector::collect();
}
