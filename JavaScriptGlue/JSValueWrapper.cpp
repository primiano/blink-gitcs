/*
 * Copyright (C) 2005 Apple Computer, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "JSValueWrapper.h"
#include "JavaScriptCore/reference_list.h"

JSValueWrapper::JSValueWrapper(JSValue *inValue, ExecState *inExec)
    : fValue(inValue), fExec(inExec)
{
}

JSValueWrapper::~JSValueWrapper()
{
}

JSValue *JSValueWrapper::GetValue()
{
    return fValue;
}
ExecState* JSValueWrapper::GetExecState() const
{
    return fExec;
}


void JSValueWrapper::GetJSObectCallBacks(JSObjectCallBacks& callBacks)
{
    callBacks.dispose = (JSObjectDisposeProcPtr)JSValueWrapper::JSObjectDispose;
    callBacks.equal = (JSObjectEqualProcPtr)0;
    callBacks.copyPropertyNames = (JSObjectCopyPropertyNamesProcPtr)JSValueWrapper::JSObjectCopyPropertyNames;
    callBacks.copyCFValue = (JSObjectCopyCFValueProcPtr)JSValueWrapper::JSObjectCopyCFValue;
    callBacks.copyProperty = (JSObjectCopyPropertyProcPtr)JSValueWrapper::JSObjectCopyProperty;
    callBacks.setProperty = (JSObjectSetPropertyProcPtr)JSValueWrapper::JSObjectSetProperty;
    callBacks.callFunction = (JSObjectCallFunctionProcPtr)JSValueWrapper::JSObjectCallFunction;
}

void JSValueWrapper::JSObjectDispose(void *data)
{
    JSValueWrapper* ptr = (JSValueWrapper*)data;
    delete ptr;
}


CFArrayRef JSValueWrapper::JSObjectCopyPropertyNames(void *data)
{
    JSLock lock;

    CFMutableArrayRef result = 0;
    JSValueWrapper* ptr = (JSValueWrapper*)data;
    if (ptr)
    {
        ExecState* exec = ptr->GetExecState();
        JSObject *object = ptr->GetValue()->toObject(exec);
        ReferenceList list = object->propList(exec);
        ReferenceListIterator iterator = list.begin();

        while (iterator != list.end()) {
            Identifier name = iterator->getPropertyName(exec);
            CFStringRef nameStr = IdentifierToCFString(name);

            if (!result)
            {
                result = CFArrayCreateMutable(0, 0, &kCFTypeArrayCallBacks);
            }
            if (result && nameStr)
            {
                CFArrayAppendValue(result, nameStr);
            }
            ReleaseCFType(nameStr);
            iterator++;
        }

    }
    return result;
}


JSObjectRef JSValueWrapper::JSObjectCopyProperty(void *data, CFStringRef propertyName)
{
    JSLock lock;

    JSObjectRef result = 0;
    JSValueWrapper* ptr = (JSValueWrapper*)data;
    if (ptr)
    {
        ExecState* exec = ptr->GetExecState();
        JSValue *propValue = ptr->GetValue()->toObject(exec)->get(exec, CFStringToIdentifier(propertyName));
        JSValueWrapper* wrapperValue = new JSValueWrapper(propValue, exec);

        JSObjectCallBacks callBacks;
        GetJSObectCallBacks(callBacks);
        result = JSObjectCreateInternal(wrapperValue, &callBacks, JSValueWrapper::JSObjectMark, kJSUserObjectDataTypeJSValueWrapper);

        if (!result)
        {
            delete wrapperValue;
        }
    }
    return result;
}

void JSValueWrapper::JSObjectSetProperty(void *data, CFStringRef propertyName, JSObjectRef jsValue)
{
    JSLock lock;

    JSValueWrapper* ptr = (JSValueWrapper*)data;
    if (ptr)
    {
        ExecState* exec = ptr->GetExecState();
        JSValue *value = JSObjectKJSValue((JSUserObject*)jsValue);
        JSObject *objValue = ptr->GetValue()->toObject(exec);
        objValue->put(exec, CFStringToIdentifier(propertyName), value);
    }
}

JSObjectRef JSValueWrapper::JSObjectCallFunction(void *data, JSObjectRef thisObj, CFArrayRef args)
{
    JSLock lock;

    JSObjectRef result = 0;
    JSValueWrapper* ptr = (JSValueWrapper*)data;
    if (ptr)
    {
        ExecState* exec = ptr->GetExecState();

        JSValue *value = JSObjectKJSValue((JSUserObject*)thisObj);
        JSObject *ksjThisObj = value->toObject(exec);
        JSObject *objValue = ptr->GetValue()->toObject(exec);

        List listArgs;
        CFIndex argCount = args ? CFArrayGetCount(args) : 0;
        for (CFIndex i = 0; i < argCount; i++)
        {
            JSObjectRef jsArg = (JSObjectRef)CFArrayGetValueAtIndex(args, i);
            JSValue *kgsArg = JSObjectKJSValue((JSUserObject*)jsArg);
            listArgs.append(kgsArg);
        }

        JSValue *resultValue = objValue->call(exec, ksjThisObj, listArgs);
        JSValueWrapper* wrapperValue = new JSValueWrapper(resultValue, ptr->GetExecState());
        JSObjectCallBacks callBacks;
        GetJSObectCallBacks(callBacks);
        result = JSObjectCreate(wrapperValue, &callBacks);
        if (!result)
        {
            delete wrapperValue;
        }
    }
    return result;
}

CFTypeRef JSValueWrapper::JSObjectCopyCFValue(void *data)
{
    JSLock lock;

    CFTypeRef result = 0;
    JSValueWrapper* ptr = (JSValueWrapper*)data;
    if (ptr)
    {
        result = KJSValueToCFType(ptr->fValue, ptr->fExec);
    }
    return result;
}

void JSValueWrapper::JSObjectMark(void *data)
{
    JSValueWrapper* ptr = (JSValueWrapper*)data;
    if (ptr)
    {
        ptr->fValue->mark();
    }
}
