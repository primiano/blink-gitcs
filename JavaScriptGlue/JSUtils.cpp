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

#include "config.h"
#include "JSUtils.h"
#include "JSBase.h"
#include "JSObject.h"
#include "JSRun.h"
#include "UserObjectImp.h"
#include "JSValueWrapper.h"
#include "JSObject.h"
#include <JavaScriptCore/PropertyNameArray.h>

struct ObjectImpList {
    JSObject* imp;
    ObjectImpList* next;
    CFTypeRef data;
};

static CFTypeRef KJSValueToCFTypeInternal(JSValue *inValue, ExecState *exec, ObjectImpList* inImps);


//--------------------------------------------------------------------------
// CFStringToUString
//--------------------------------------------------------------------------

UString CFStringToUString(CFStringRef inCFString)
{
    JSLock lock;

    UString result;
    if (inCFString) {
        CFIndex len = CFStringGetLength(inCFString);
        UniChar* buffer = (UniChar*)malloc(sizeof(UniChar) * len);
        if (buffer)
        {
            CFStringGetCharacters(inCFString, CFRangeMake(0, len), buffer);
            result = UString((const UChar *)buffer, len);
            free(buffer);
        }
    }
    return result;
}


//--------------------------------------------------------------------------
// UStringToCFString
//--------------------------------------------------------------------------
// Caller is responsible for releasing the returned CFStringRef
CFStringRef UStringToCFString(const UString& inUString)
{
    return CFStringCreateWithCharacters(0, (const UniChar*)inUString.data(), inUString.size());
}


//--------------------------------------------------------------------------
// CFStringToIdentifier
//--------------------------------------------------------------------------

Identifier CFStringToIdentifier(CFStringRef inCFString)
{
    return Identifier(CFStringToUString(inCFString));
}


//--------------------------------------------------------------------------
// IdentifierToCFString
//--------------------------------------------------------------------------
// Caller is responsible for releasing the returned CFStringRef
CFStringRef IdentifierToCFString(const Identifier& inIdentifier)
{
    return UStringToCFString(inIdentifier.ustring());
}


//--------------------------------------------------------------------------
// KJSValueToJSObject
//--------------------------------------------------------------------------
JSUserObject* KJSValueToJSObject(JSValue *inValue, ExecState *exec)
{
    JSUserObject* result = 0;

    if (inValue->isObject(&UserObjectImp::info)) {
        UserObjectImp* userObjectImp = static_cast<UserObjectImp *>(inValue);
        result = userObjectImp->GetJSUserObject();
        if (result)
            result->Retain();
    } else {
        JSValueWrapper* wrapperValue = new JSValueWrapper(inValue);
        if (wrapperValue) {
            JSObjectCallBacks callBacks;
            JSValueWrapper::GetJSObectCallBacks(callBacks);
            result = (JSUserObject*)JSObjectCreate(wrapperValue, &callBacks);
            if (!result) {
                delete wrapperValue;
            }
        }
    }
    return result;
}

//--------------------------------------------------------------------------
// JSObjectKJSValue
//--------------------------------------------------------------------------
JSValue *JSObjectKJSValue(JSUserObject* ptr)
{
    JSLock lock;

    JSValue *result = jsUndefined();
    if (ptr)
    {
        bool handled = false;

        switch (ptr->DataType())
        {
            case kJSUserObjectDataTypeJSValueWrapper:
            {
                JSValueWrapper* wrapper = (JSValueWrapper*)ptr->GetData();
                if (wrapper)
                {
                    result = wrapper->GetValue();
                    handled = true;
                }
                break;
            }

            case kJSUserObjectDataTypeCFType:
            {
                CFTypeRef cfType = (CFTypeRef*)ptr->GetData();
                if (cfType)
                {
                    CFTypeID typeID = CFGetTypeID(cfType);
                    if (typeID == CFStringGetTypeID())
                    {
                        result = jsString(CFStringToUString((CFStringRef)cfType));
                        handled = true;
                    }
                    else if (typeID == CFNumberGetTypeID())
                    {
                        double num;
                        CFNumberGetValue((CFNumberRef)cfType, kCFNumberDoubleType, &num);
                        result = jsNumber(num);
                        handled = true;
                    }
                    else if (typeID == CFBooleanGetTypeID())
                    {
                        result = jsBoolean(CFBooleanGetValue((CFBooleanRef)cfType));
                        handled = true;
                    }
                    else if (typeID == CFNullGetTypeID())
                    {
                        result = jsNull();
                        handled = true;
                    }
                }
                break;
            }
        }
        if (!handled)
        {
            result = new UserObjectImp(ptr);
        }
    }
    return result;
}




//--------------------------------------------------------------------------
// KJSValueToCFTypeInternal
//--------------------------------------------------------------------------
// Caller is responsible for releasing the returned CFTypeRef
CFTypeRef KJSValueToCFTypeInternal(JSValue *inValue, ExecState *exec, ObjectImpList* inImps)
{
    if (!inValue)
        return 0;

    CFTypeRef result = 0;

    JSLock lock;

    switch (inValue->type())
    {
        case BooleanType:
            {
                result = inValue->toBoolean(exec) ? kCFBooleanTrue : kCFBooleanFalse;
                RetainCFType(result);
            }
            break;

        case StringType:
            {
                UString uString = inValue->toString(exec);
                result = UStringToCFString(uString);
            }
            break;

        case NumberType:
            {
                double number1 = inValue->toNumber(exec);
                double number2 = (double)inValue->toInteger(exec);
                if (number1 ==  number2)
                {
                    int intValue = (int)number2;
                    result = CFNumberCreate(0, kCFNumberIntType, &intValue);
                }
                else
                {
                    result = CFNumberCreate(0, kCFNumberDoubleType, &number1);
                }
            }
            break;

        case ObjectType:
            {
                            if (inValue->isObject(&UserObjectImp::info)) {
                                UserObjectImp* userObjectImp = static_cast<UserObjectImp *>(inValue);
                    JSUserObject* ptr = userObjectImp->GetJSUserObject();
                    if (ptr)
                    {
                        result = ptr->CopyCFValue();
                    }
                }
                else
                {
                    JSObject *object = inValue->toObject(exec);
                    UInt8 isArray = false;

                    // if two objects reference each
                    JSObject* imp = object;
                    ObjectImpList* temp = inImps;
                    while (temp) {
                        if (imp == temp->imp) {
                            return CFRetain(GetCFNull());
                        }
                        temp = temp->next;
                    }

                    ObjectImpList imps;
                    imps.next = inImps;
                    imps.imp = imp;


//[...] HACK since we do not have access to the class info we use class name instead
#if 0
                    if (object->inherits(&ArrayInstanceImp::info))
#else
                    if (object->className() == "Array")
#endif
                    {
                        isArray = true;
                        JSInterpreter* intrepreter = (JSInterpreter*)exec->dynamicInterpreter();
                        if (intrepreter && (intrepreter->Flags() & kJSFlagConvertAssociativeArray)) {
                            PropertyNameArray propNames;
                            object->getPropertyNames(exec, propNames);
                            PropertyNameArrayIterator iter = propNames.begin();
                            PropertyNameArrayIterator end = propNames.end();
                            while(iter != end && isArray)
                            {
                                Identifier propName = *iter;
                                UString ustr = propName.ustring();
                                const UniChar* uniChars = (const UniChar*)ustr.data();
                                int size = ustr.size();
                                while (size--) {
                                    if (uniChars[size] < '0' || uniChars[size] > '9') {
                                        isArray = false;
                                        break;
                                    }
                                }
                                iter++;
                            }
                        }
                    }

                    if (isArray)
                    {
                        // This is an KJS array
                        unsigned int length = object->get(exec, "length")->toUInt32(exec);
                        result = CFArrayCreateMutable(0, 0, &kCFTypeArrayCallBacks);
                        if (result)
                        {
                            for (unsigned i = 0; i < length; i++)
                            {
                                CFTypeRef cfValue = KJSValueToCFTypeInternal(object->get(exec, i), exec, &imps);
                                CFArrayAppendValue((CFMutableArrayRef)result, cfValue);
                                ReleaseCFType(cfValue);
                            }
                        }
                    }
                    else
                    {
                        // Not an array, just treat it like a dictionary which contains (property name, property value) pairs
                        PropertyNameArray propNames;
                        object->getPropertyNames(exec, propNames);
                        {
                            result = CFDictionaryCreateMutable(0,
                                                               0,
                                                               &kCFTypeDictionaryKeyCallBacks,
                                                               &kCFTypeDictionaryValueCallBacks);
                            if (result)
                            {
                                PropertyNameArrayIterator iter = propNames.begin();
                                PropertyNameArrayIterator end = propNames.end();
                                while(iter != end)
                                {
                                    Identifier propName = *iter;
                                    if (object->hasProperty(exec, propName))
                                    {
                                        CFStringRef cfKey = IdentifierToCFString(propName);
                                        CFTypeRef cfValue = KJSValueToCFTypeInternal(object->get(exec, propName), exec, &imps);
                                        if (cfKey && cfValue)
                                        {
                                            CFDictionaryAddValue((CFMutableDictionaryRef)result, cfKey, cfValue);
                                        }
                                        ReleaseCFType(cfKey);
                                        ReleaseCFType(cfValue);
                                    }
                                    iter++;
                                }
                            }
                        }
                    }
                }
            }
            break;

        case NullType:
        case UndefinedType:
        case UnspecifiedType:
            result = RetainCFType(GetCFNull());
            break;

        default:
            fprintf(stderr, "KJSValueToCFType: wrong value type %d\n", inValue->type());
            break;
    }

    return result;
}

CFTypeRef KJSValueToCFType(JSValue *inValue, ExecState *exec)
{
    return KJSValueToCFTypeInternal(inValue, exec, 0);
}

CFTypeRef GetCFNull(void)
{
    static CFArrayRef sCFNull = CFArrayCreate(0, 0, 0, 0);
    CFTypeRef result = JSGetCFNull();
    if (!result)
    {
        result = sCFNull;
    }
    return result;
}

