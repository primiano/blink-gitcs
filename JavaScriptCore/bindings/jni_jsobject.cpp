/*
 * Copyright (C) 2003 Apple Computer, Inc.  All rights reserved.
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
#include <jni_jsobject.h>
 
jlong KJS_JSCreateNativeJSObject (JNIEnv *env, jclass clazz, jstring jurl, jboolean ctx)
{
    fprintf (stderr, "%s:\n", __PRETTY_FUNCTION__);
    return POINTER_JNI_JLONG(0);
}

void KJS_JSObject_JSFinalize (JNIEnv *env, jclass jsClass, jlong nativeJSObject)
{
    fprintf (stderr, "%s:\n", __PRETTY_FUNCTION__);
}

jobject KJS_JSObject_JSObjectCall (JNIEnv *env, jclass jsClass, jlong nativeJSObject, jstring jurl, jstring methodName, jobjectArray args, jboolean ctx)
{
    fprintf (stderr, "%s:\n", __PRETTY_FUNCTION__);
    return 0;
}

jobject KJS_JSObject_JSObjectEval (JNIEnv *env, jclass jsClass, jlong nativeJSObject, jstring jurl, jstring jscript, jboolean ctx)
{
    fprintf (stderr, "%s:\n", __PRETTY_FUNCTION__);
    return 0;
}

jobject KJS_JSObject_JSObjectGetMember (JNIEnv *env, jclass jsClass, jlong nativeJSObject, jstring jurl, jstring jname, jboolean ctx)
{
    fprintf (stderr, "%s:\n", __PRETTY_FUNCTION__);
    return 0;
}

void KJS_JSObject_JSObjectSetMember (JNIEnv *env, jclass jsClass, jlong nativeJSObject, jstring jurl, jstring jname, jobject value, jboolean ctx)
{
    fprintf (stderr, "%s:\n", __PRETTY_FUNCTION__);
}

void KJS_JSObject_JSObjectRemoveMember (JNIEnv *env, jclass jsClass, jlong nativeJSObject, jstring jurl, jstring jname, jboolean ctx)
{
    fprintf (stderr, "%s:\n", __PRETTY_FUNCTION__);
}

jobject KJS_JSObject_JSObjectGetSlot (JNIEnv *env, jclass jsClass, jlong nativeJSObject, jstring jurl, jint jindex, jboolean ctx)
{
    fprintf (stderr, "%s:\n", __PRETTY_FUNCTION__);
    return 0;
}

void KJS_JSObject_JSObjectSetSlot (JNIEnv *env, jclass jsClass, jlong nativeJSObject, jstring jurl, jint jindex, jobject value, jboolean ctx)
{
    fprintf (stderr, "%s:\n", __PRETTY_FUNCTION__);
}

jstring KJS_JSObject_JSObjectToString (JNIEnv *env, jclass clazz, jlong nativeJSObject)
{
    fprintf (stderr, "%s:\n", __PRETTY_FUNCTION__);
    return 0;
}

