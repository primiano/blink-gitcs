/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "config.h"
#include "FunctionPrototype.h"

#include "Arguments.h"
#include "JSArray.h"
#include "JSFunction.h"
#include "JSString.h"
#include "PrototypeFunction.h"

namespace JSC {

ASSERT_CLASS_FITS_IN_CELL(FunctionPrototype);

static JSValue* functionProtoFuncToString(ExecState*, JSObject*, JSValue*, const ArgList&);
static JSValue* functionProtoFuncApply(ExecState*, JSObject*, JSValue*, const ArgList&);
static JSValue* functionProtoFuncCall(ExecState*, JSObject*, JSValue*, const ArgList&);

FunctionPrototype::FunctionPrototype(ExecState* exec)
    : InternalFunction(exec)
{
    putDirect(exec->propertyNames().length, jsNumber(exec, 0), DontDelete | ReadOnly | DontEnum);

    putDirectFunction(exec, new (exec) PrototypeFunction(exec, this, 0, exec->propertyNames().toString, functionProtoFuncToString), DontEnum);
    putDirectFunction(exec, new (exec) PrototypeFunction(exec, this, 2, exec->propertyNames().apply, functionProtoFuncApply), DontEnum);
    putDirectFunction(exec, new (exec) PrototypeFunction(exec, this, 1, exec->propertyNames().call, functionProtoFuncCall), DontEnum);
}

static JSValue* callFunctionPrototype(ExecState*, JSObject*, JSValue*, const ArgList&)
{
    return jsUndefined();
}

// ECMA 15.3.4
CallType FunctionPrototype::getCallData(CallData& callData)
{
    callData.native.function = callFunctionPrototype;
    return CallTypeHost;
}

// Functions

JSValue* functionProtoFuncToString(ExecState* exec, JSObject*, JSValue* thisValue, const ArgList&)
{
    if (thisValue->isObject(&JSFunction::info)) {
        JSFunction* function = static_cast<JSFunction*>(thisValue);
        return jsString(exec, "function " + function->name(exec) + "(" + function->m_body->paramString() + ") " + function->m_body->toSourceString());
    }

    if (thisValue->isObject(&InternalFunction::info)) {
        InternalFunction* function = static_cast<InternalFunction*>(thisValue);
        return jsString(exec, "function " + function->name(exec) + "() {\n    [native code]\n}");
    }

    return throwError(exec, TypeError);
}

JSValue* functionProtoFuncApply(ExecState* exec, JSObject*, JSValue* thisValue, const ArgList& args)
{
    CallData callData;
    CallType callType = thisValue->getCallData(callData);
    if (callType == CallTypeNone)
        return throwError(exec, TypeError);

    JSValue* thisArg = args.at(exec, 0);
    JSValue* argArray = args.at(exec, 1);

    JSValue* applyThis;
    if (thisArg->isUndefinedOrNull())
        applyThis = exec->globalThisValue();
    else
        applyThis = thisArg->toObject(exec);

    ArgList applyArgs;
    if (!argArray->isUndefinedOrNull()) {
        if (argArray->isObject() &&
            (static_cast<JSObject*>(argArray)->inherits(&JSArray::info) ||
             static_cast<JSObject*>(argArray)->inherits(&Arguments::info))) {

            JSObject* argArrayObj = static_cast<JSObject*>(argArray);
            unsigned int length = argArrayObj->get(exec, exec->propertyNames().length)->toUInt32(exec);
            for (unsigned int i = 0; i < length; i++)
                applyArgs.append(argArrayObj->get(exec, i));
        } else
            return throwError(exec, TypeError);
    }

    return call(exec, thisValue, callType, callData, applyThis, applyArgs);
}

JSValue* functionProtoFuncCall(ExecState* exec, JSObject*, JSValue* thisValue, const ArgList& args)
{
    CallData callData;
    CallType callType = thisValue->getCallData(callData);
    if (callType == CallTypeNone)
        return throwError(exec, TypeError);

    JSValue* thisArg = args.at(exec, 0);

    JSObject* callThis;
    if (thisArg->isUndefinedOrNull())
        callThis = exec->globalThisValue();
    else
        callThis = thisArg->toObject(exec);

    ArgList argsTail;
    args.getSlice(1, argsTail);
    return call(exec, thisValue, callType, callData, callThis, argsTail);
}

} // namespace JSC
