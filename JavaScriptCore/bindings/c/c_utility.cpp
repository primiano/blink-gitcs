/*
 * Copyright (C) 2004 Apple Computer, Inc.  All rights reserved.
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
#include <c_instance.h> 
#include <c_utility.h> 
#include <internal.h>
#include <runtime.h>
#include <runtime_object.h>
#include <runtime_root.h>
#include <value.h>
#include <NP_jsobject.h>

using namespace KJS;
using namespace KJS::Bindings;

NP_Object *coerceValueToNPString (KJS::ExecState *exec, const KJS::Value &value)
{
    UString ustring = value.toString(exec);
    return NP_CreateStringWithUTF8 (ustring.UTF8String().c_str());
}

NP_Object *convertValueToNPValueType (KJS::ExecState *exec, const KJS::Value &value)
{
    Type type = value.type();
    
    if (type == StringType) {
        UString ustring = value.toString(exec);
        return NP_CreateStringWithUTF8 (ustring.UTF8String().c_str());
    }
    else if (type == NumberType) {
        return NP_CreateNumberWithDouble (value.toNumber(exec));
    }
    else if (type == BooleanType) {
        return NP_CreateBoolean (value.toBoolean(exec));
    }
    else if (type == UnspecifiedType) {
        return NP_GetUndefined();
    }
    else if (type == NullType) {
        return NP_GetNull();
    }
    else if (type == ObjectType) {
        KJS::ObjectImp *objectImp = static_cast<KJS::ObjectImp*>(value.imp());
        if (strcmp(objectImp->classInfo()->className, "RuntimeObject") == 0) {
            KJS::RuntimeObjectImp *imp = static_cast<KJS::RuntimeObjectImp *>(value.imp());
            CInstance *instance = static_cast<CInstance*>(imp->getInternalInstance());
            return instance->getObject();
        }
    }
    
    return 0;
}

Value convertNPValueTypeToValue (KJS::ExecState *exec, const NP_Object *obj)
{
    if (NP_IsKindOfClass (obj, NP_BooleanClass)) {
        if (NP_BoolFromBoolean ((NP_Boolean *)obj))
            return KJS::Boolean (true);
        return KJS::Boolean (false);
    }
    else if (NP_IsKindOfClass (obj, NP_NullClass)) {
        return Null();
    }
    else if (NP_IsKindOfClass (obj, NP_UndefinedClass)) {
        return Undefined();
    }
    else if (NP_IsKindOfClass (obj, NP_ArrayClass)) {
        // FIXME:  Need to implement
    }
    else if (NP_IsKindOfClass (obj, NP_NumberClass)) {
        return Number (NP_DoubleFromNumber((NP_Number *)obj));
    }
    else if (NP_IsKindOfClass (obj, NP_StringClass)) {

        NP_UTF8 *utf8String = NP_UTF8FromString((NP_String *)obj);
        CFStringRef stringRef = CFStringCreateWithCString (NULL, utf8String, kCFStringEncodingUTF8);
        NP_DeallocateUTF8 (utf8String);

        int length = CFStringGetLength (stringRef);
        NP_UTF16 *buffer = (NP_UTF16 *)malloc(sizeof(NP_UTF16)*length);

        // Convert the string to UTF16.
        CFRange range = { 0, length };
        CFStringGetCharacters (stringRef, range, (UniChar *)buffer);
        CFRelease (stringRef);

        String resultString(UString((const UChar *)buffer,length));
        free (buffer);
        
        return resultString;
    }
    else if (NP_IsKindOfClass (obj, NP_JavaScriptObjectClass)) {
        // Get ObjectImp from NP_JavaScriptObject.
        JavaScriptObject *o = (JavaScriptObject *)obj;
        return Object(const_cast<ObjectImp*>(o->imp));
    }
    else {
        //  Wrap NP_Object in a CInstance.
        return Instance::createRuntimeObject(Instance::CLanguage, (void *)obj);
    }
    
    return Undefined();
}

