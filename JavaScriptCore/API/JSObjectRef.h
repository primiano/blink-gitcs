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

#ifndef JSObjectRef_h
#define JSObjectRef_h

#include "JSBase.h"

#ifdef __cplusplus
extern "C" {
#endif

enum { 
    kJSPropertyAttributeNone         = 0,
    kJSPropertyAttributeReadOnly     = 1 << 1,
    kJSPropertyAttributeDontEnum     = 1 << 2,
    kJSPropertyAttributeDontDelete   = 1 << 3
};
typedef unsigned JSPropertyAttributes;

typedef void
(*JSInitializeCallback)         (JSObjectRef object);

typedef void            
(*JSFinalizeCallback)           (JSObjectRef object);

typedef JSCharBufferRef 
(*JSCopyDescriptionCallback)    (JSObjectRef object);

typedef bool
(*JSHasPropertyCallback)        (JSObjectRef object, JSCharBufferRef propertyName);

typedef bool
(*JSGetPropertyCallback)        (JSObjectRef object, JSCharBufferRef propertyName, JSValueRef* returnValue);

typedef bool
(*JSSetPropertyCallback)        (JSObjectRef object, JSCharBufferRef propertyName, JSValueRef value);

typedef bool
(*JSDeletePropertyCallback)     (JSObjectRef object, JSCharBufferRef propertyName);

typedef void
(*JSGetPropertyListCallback)    (JSObjectRef object, JSPropertyListRef propertyList);

typedef JSValueRef 
(*JSCallAsFunctionCallback)     (JSContextRef context, JSObjectRef object, JSObjectRef thisObject, size_t argc, JSValueRef argv[]);

typedef JSObjectRef 
(*JSCallAsConstructorCallback)  (JSContextRef context, JSObjectRef object, size_t argc, JSValueRef argv[]);

typedef bool
(*JSConvertToTypeCallback)      (JSObjectRef object, JSTypeCode typeCode, JSValueRef* returnValue);

typedef struct __JSObjectCallbacks {
    int                         version; // current (and only) version is 0
    struct __JSObjectCallbacks* parentCallbacks; // pass NULL for the default object callbacks
    JSInitializeCallback        initialize;
    JSFinalizeCallback          finalize;
    JSCopyDescriptionCallback   copyDescription;
    JSHasPropertyCallback       hasProperty;
    JSGetPropertyCallback       getProperty;
    JSSetPropertyCallback       setProperty;
    JSDeletePropertyCallback    deleteProperty;
    JSGetPropertyListCallback   getPropertyList;
    JSCallAsFunctionCallback    callAsFunction;
    JSCallAsConstructorCallback callAsConstructor;
    JSConvertToTypeCallback     convertToType;
} JSObjectCallbacks;

extern const JSObjectCallbacks kJSObjectCallbacksNone;

// pass NULL as prototype to get the built-in object prototype
JSObjectRef JSObjectMake(JSContextRef context, const JSObjectCallbacks* callbacks, JSObjectRef prototype);

// Will be assigned the built-in function prototype
JSObjectRef JSFunctionMake(JSContextRef context, JSCallAsFunctionCallback callback);

JSCharBufferRef JSObjectGetDescription(JSObjectRef object);

JSValueRef JSObjectGetPrototype(JSObjectRef object);
void JSObjectSetPrototype(JSObjectRef object, JSValueRef value);

bool JSObjectHasProperty(JSContextRef context, JSObjectRef object, JSCharBufferRef propertyName);
bool JSObjectGetProperty(JSContextRef context, JSObjectRef object, JSCharBufferRef propertyName, JSValueRef* value);
bool JSObjectSetProperty(JSContextRef context, JSObjectRef object, JSCharBufferRef propertyName, JSValueRef value, JSPropertyAttributes attributes);
bool JSObjectDeleteProperty(JSContextRef context, JSObjectRef object, JSCharBufferRef propertyName);

void* JSObjectGetPrivate(JSObjectRef object);
bool JSObjectSetPrivate(JSObjectRef object, void* data);

bool JSObjectIsFunction(JSObjectRef object);
bool JSObjectCallAsFunction(JSContextRef context, JSObjectRef object, JSObjectRef thisObject, size_t argc, JSValueRef argv[], JSValueRef* returnValue);
bool JSObjectIsConstructor(JSObjectRef object);
bool JSObjectCallAsConstructor(JSContextRef context, JSObjectRef object, size_t argc, JSValueRef argv[], JSValueRef* returnValue);

// Used for enumerating the names of an object's properties like a for...in loop would
JSPropertyListEnumeratorRef JSObjectCreatePropertyEnumerator(JSContextRef context, JSObjectRef object);
JSPropertyListEnumeratorRef JSPropertyEnumeratorRetain(JSPropertyListEnumeratorRef enumerator);
void JSPropertyEnumeratorRelease(JSPropertyListEnumeratorRef enumerator);
JSCharBufferRef JSPropertyEnumeratorGetNext(JSContextRef context, JSPropertyListEnumeratorRef enumerator);

// Used for adding property names to a for...in enumeration
void JSPropertyListAdd(JSPropertyListRef propertyList, JSObjectRef thisObject, JSCharBufferRef propertyName);

#ifdef __cplusplus
}
#endif

#endif // JSObjectRef_h
