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
#include "interpreter.h"

#include "jni_runtime.h"
#include "jni_utility.h"
#include "runtime_object.h"

static JavaVM *jvm;

JavaVM *getJavaVM()
{
    if (jvm)
        return jvm;

    JavaVM *jvmArray[1];
    jsize bufLen = 1;
    jsize nJVMs = 0;
    jint jniError = 0;

    // Assumes JVM is already running ..., one per process
    jniError = JNI_GetCreatedJavaVMs(jvmArray, bufLen, &nJVMs);
    if ( jniError == JNI_OK && nJVMs > 0 ) {
        jvm = jvmArray[0];
    }
    else 
        fprintf(stderr, "%s: JNI_GetCreatedJavaVMs failed, returned %d", __PRETTY_FUNCTION__, jniError);
        
    return jvm;
}

JNIEnv *getJNIEnv()
{
    JNIEnv *env;
    jint jniError = 0;

    jniError = (getJavaVM())->AttachCurrentThread((void**)&env, (void *)NULL);
    if ( jniError == JNI_OK )
        return env;
    else
        fprintf(stderr, "%s: AttachCurrentThread failed, returned %d", __PRETTY_FUNCTION__, jniError);
    return NULL;
}

static jvalue callJNIMethod( JNIType type, jobject obj, const char *name, const char *sig, va_list args)
{
    JavaVM *jvm = getJavaVM();
    JNIEnv *env = getJNIEnv();
    jvalue result;

    if ( obj != NULL && jvm != NULL && env != NULL) {
        jclass cls = env->GetObjectClass(obj);
        if ( cls != NULL ) {
            jmethodID mid = env->GetMethodID(cls, name, sig);
            if ( mid != NULL )
            {
                switch (type) {
                case void_type:
                    env->functions->CallVoidMethodV(env, obj, mid, args);
                    break;
                case object_type:
                    result.l = env->functions->CallObjectMethodV(env, obj, mid, args);
                    break;
                case boolean_type:
                    result.z = env->functions->CallBooleanMethodV(env, obj, mid, args);
                    break;
                case byte_type:
                    result.b = env->functions->CallByteMethodV(env, obj, mid, args);
                    break;
                case char_type:
                    result.c = env->functions->CallCharMethodV(env, obj, mid, args);
                    break;
                case short_type:
                    result.s = env->functions->CallShortMethodV(env, obj, mid, args);
                    break;
                case int_type:
                    result.i = env->functions->CallIntMethodV(env, obj, mid, args);
                    break;
                case long_type:
                    result.j = env->functions->CallLongMethodV(env, obj, mid, args);
                    break;
                case float_type:
                    result.f = env->functions->CallFloatMethodV(env, obj, mid, args);
                    break;
                case double_type:
                    result.d = env->functions->CallDoubleMethodV(env, obj, mid, args);
                    break;
                default:
                    fprintf(stderr, "%s: invalid function type (%d)", __PRETTY_FUNCTION__, (int)type);
                }
            }
            else
            {
                fprintf(stderr, "%s: Could not find method: %s!", __PRETTY_FUNCTION__, name);
                env->ExceptionDescribe();
                env->ExceptionClear();
            }

            env->DeleteLocalRef(cls);
        }
        else {
            fprintf(stderr, "%s: Could not find class for object!", __PRETTY_FUNCTION__);
        }
    }

    return result;
}

static jvalue callJNIMethodA( JNIType type, jobject obj, const char *name, const char *sig, jvalue *args)
{
    JavaVM *jvm = getJavaVM();
    JNIEnv *env = getJNIEnv();
    jvalue result;
    
    if ( obj != NULL && jvm != NULL && env != NULL) {
        jclass cls = env->GetObjectClass(obj);
        if ( cls != NULL ) {
            jmethodID mid = env->GetMethodID(cls, name, sig);
            if ( mid != NULL )
            {
                switch (type) {
                case void_type:
                    env->functions->CallVoidMethodA(env, obj, mid, args);
                    break;
                case object_type:
                    result.l = env->functions->CallObjectMethodA(env, obj, mid, args);
                    break;
                case boolean_type:
                    result.z = env->functions->CallBooleanMethodA(env, obj, mid, args);
                    break;
                case byte_type:
                    result.b = env->functions->CallByteMethodA(env, obj, mid, args);
                    break;
                case char_type:
                    result.c = env->functions->CallCharMethodA(env, obj, mid, args);
                    break;
                case short_type:
                    result.s = env->functions->CallShortMethodA(env, obj, mid, args);
                    break;
                case int_type:
                    result.i = env->functions->CallIntMethodA(env, obj, mid, args);
                    break;
                case long_type:
                    result.j = env->functions->CallLongMethodA(env, obj, mid, args);
                    break;
                case float_type:
                    result.f = env->functions->CallFloatMethodA(env, obj, mid, args);
                    break;
                case double_type:
                    result.d = env->functions->CallDoubleMethodA(env, obj, mid, args);
                    break;
                default:
                    fprintf(stderr, "%s: invalid function type (%d)", __PRETTY_FUNCTION__, (int)type);
                }
            }
            else
            {
                fprintf(stderr, "%s: Could not find method: %s!", __PRETTY_FUNCTION__, name);
                env->ExceptionDescribe();
                env->ExceptionClear();
            }

            env->DeleteLocalRef(cls);
        }
        else {
            fprintf(stderr, "%s: Could not find class for object!", __PRETTY_FUNCTION__);
        }
    }

    return result;
}

#define CALL_JNI_METHOD(function_type,obj,name,sig) \
    va_list args;\
    va_start (args, sig);\
    \
    jvalue result = callJNIMethod(function_type, obj, name, sig, args);\
    \
    va_end (args);

void callJNIVoidMethod (jobject obj, const char *name, const char *sig, ... )
{
    CALL_JNI_METHOD (void_type, obj, name, sig);
}

jobject callJNIObjectMethod (jobject obj, const char *name, const char *sig, ... )
{
    CALL_JNI_METHOD (object_type, obj, name, sig);
    return result.l;
}

jboolean callJNIBooleanMethod( jobject obj, const char *name, const char *sig, ... )
{
    CALL_JNI_METHOD (boolean_type, obj, name, sig);
    return result.z;
}

jbyte callJNIByteMethod( jobject obj, const char *name, const char *sig, ... )
{
    CALL_JNI_METHOD (byte_type, obj, name, sig);
    return result.b;
}

jchar callJNICharMethod (jobject obj, const char *name, const char *sig, ... )
{
    CALL_JNI_METHOD (char_type, obj, name, sig);
    return result.c;
}

jshort callJNIShortMethod (jobject obj, const char *name, const char *sig, ... )
{
    CALL_JNI_METHOD (short_type, obj, name, sig);
    return result.s;
}

jint callJNIIntMethod (jobject obj, const char *name, const char *sig, ... )
{
    CALL_JNI_METHOD (int_type, obj, name, sig);
    return result.i;
}

jlong callJNILongMethod (jobject obj, const char *name, const char *sig, ... )
{
    CALL_JNI_METHOD (long_type, obj, name, sig);
    return result.j;
}

jfloat callJNIFloatMethod (jobject obj, const char *name, const char *sig, ... )
{
    CALL_JNI_METHOD (float_type, obj, name, sig);
    return result.f;
}

jdouble callJNIDoubleMethod (jobject obj, const char *name, const char *sig, ... )
{
    CALL_JNI_METHOD (double_type, obj, name, sig);
    return result.d;
}

void callJNIVoidMethodA (jobject obj, const char *name, const char *sig, jvalue *args)
{
    jvalue result = callJNIMethodA (void_type, obj, name, sig, args);
}

jobject callJNIObjectMethodA (jobject obj, const char *name, const char *sig, jvalue *args)
{
    jvalue result = callJNIMethodA (object_type, obj, name, sig, args);
    return result.l;
}

jbyte callJNIByteMethodA ( jobject obj, const char *name, const char *sig, jvalue *args)
{
    jvalue result = callJNIMethodA (byte_type, obj, name, sig, args);
    return result.b;
}

jchar callJNICharMethodA (jobject obj, const char *name, const char *sig, jvalue *args)
{
    jvalue result = callJNIMethodA (char_type, obj, name, sig, args);
    return result.c;
}

jshort callJNIShortMethodA (jobject obj, const char *name, const char *sig, jvalue *args)
{
    jvalue result = callJNIMethodA (short_type, obj, name, sig, args);
    return result.s;
}

jint callJNIIntMethodA (jobject obj, const char *name, const char *sig, jvalue *args)
{
    jvalue result = callJNIMethodA (int_type, obj, name, sig, args);
    return result.i;
}

jlong callJNILongMethodA (jobject obj, const char *name, const char *sig, jvalue *args)
{
    jvalue result = callJNIMethodA (long_type, obj, name, sig, args);
    return result.j;
}

jfloat callJNIFloatMethodA (jobject obj, const char *name, const char *sig, jvalue *args)
{
    jvalue result = callJNIMethodA  (float_type, obj, name, sig, args);
    return result.f;
}

jdouble callJNIDoubleMethodA (jobject obj, const char *name, const char *sig, jvalue *args)
{
    jvalue result = callJNIMethodA (double_type, obj, name, sig, args);
    return result.d;
}

jboolean callJNIBooleanMethodA (jobject obj, const char *name, const char *sig, jvalue *args)
{
    jvalue result = callJNIMethodA (boolean_type, obj, name, sig, args);
    return result.z;
}

const char *getCharactersFromJString (jstring aJString)
{
    return getCharactersFromJStringInEnv (getJNIEnv(), aJString);
}

void releaseCharactersForJString (jstring aJString, const char *s)
{
    releaseCharactersForJStringInEnv (getJNIEnv(), aJString, s);
}

const char *getCharactersFromJStringInEnv (JNIEnv *env, jstring aJString)
{
    jboolean isCopy;
    const char *s = env->GetStringUTFChars((jstring)aJString, &isCopy);
    if (!s) {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
    return s;
}

void releaseCharactersForJStringInEnv (JNIEnv *env, jstring aJString, const char *s)
{
    env->ReleaseStringUTFChars (aJString, s);
}

JNIType primitiveTypeFromClassName(const char *name)
{
    JNIType type;
    
    if (strcmp("byte",name) == 0)
        type = byte_type;
    else if (strcmp("short",name) == 0)
        type = short_type;
    else if (strcmp("int",name) == 0)
        type = int_type;
    else if (strcmp("long",name) == 0)
        type = long_type;
    else if (strcmp("float",name) == 0)
        type = float_type;
    else if (strcmp("double",name) == 0)
        type = double_type;
    else if (strcmp("char",name) == 0)
        type = char_type;
    else if (strcmp("boolean",name) == 0)
        type = boolean_type;
    else if (strcmp("void",name) == 0)
        type = void_type;
    else
        type = object_type;
        
    return type;
}

const char *signatureFromPrimitiveType(JNIType type)
{
    switch (type){
        case void_type: 
            return "V";
        
        case object_type:
            return "L";
        
        case boolean_type:
            return "Z";
        
        case byte_type:
            return "B";
            
        case char_type:
            return "C";
        
        case short_type:
            return "S";
        
        case int_type:
            return "I";
        
        case long_type:
            return "J";
        
        case float_type:
            return "F";
        
        case double_type:
            return "D";

        case invalid_type:
        default:
        break;
    }
    return "";
}

jvalue getJNIField( jobject obj, JNIType type, const char *name, const char *signature)
{
    JavaVM *jvm = getJavaVM();
    JNIEnv *env = getJNIEnv();
    jvalue result;

    if ( obj != NULL && jvm != NULL && env != NULL) {
        jclass cls = env->GetObjectClass(obj);
        if ( cls != NULL ) {
            jfieldID field = env->GetFieldID(cls, name, signature);
            if ( field != NULL ) {
                switch (type) {
                case object_type:
                    result.l = env->functions->GetObjectField(env, obj, field);
                    break;
                case boolean_type:
                    result.z = env->functions->GetBooleanField(env, obj, field);
                    break;
                case byte_type:
                    result.b = env->functions->GetByteField(env, obj, field);
                    break;
                case char_type:
                    result.c = env->functions->GetCharField(env, obj, field);
                    break;
                case short_type:
                    result.s = env->functions->GetShortField(env, obj, field);
                    break;
                case int_type:
                    result.i = env->functions->GetIntField(env, obj, field);
                    break;
                case long_type:
                    result.j = env->functions->GetLongField(env, obj, field);
                    break;
                case float_type:
                    result.f = env->functions->GetFloatField(env, obj, field);
                    break;
                case double_type:
                    result.d = env->functions->GetDoubleField(env, obj, field);
                    break;
                default:
                    fprintf(stderr, "%s: invalid field type (%d)", __PRETTY_FUNCTION__, (int)type);
                }
            }
            else
            {
                fprintf(stderr, "%s: Could not find field: %s!", __PRETTY_FUNCTION__, name);
                env->ExceptionDescribe();
                env->ExceptionClear();
            }

            env->DeleteLocalRef(cls);
        }
        else {
            fprintf(stderr, "%s: Could not find class for object!", __PRETTY_FUNCTION__);
        }
    }

    return result;
}

jvalue convertValueToJValue (KJS::ExecState *exec, KJS::Value value, Bindings::JavaParameter *aParameter)
{
    jvalue result;
    double d = 0;
   
    d = value.toNumber(exec);
    switch (aParameter->getJNIType()){
        case object_type: {
            KJS::RuntimeObjectImp *imp = static_cast<KJS::RuntimeObjectImp*>(value.imp());
            if (imp) {
                Bindings::JavaInstance *instance = static_cast<Bindings::JavaInstance*>(imp->getInternalInstance());
                result.l = instance->javaInstance();
            }
            else
                result.l = (jobject)0;
        }
        break;
        
        case boolean_type: {
            result.z = (jboolean)d;
        }
        break;
            
        case byte_type: {
            result.b = (jbyte)d;
        }
        break;
        
        case char_type: {
            result.c = (jchar)d;
        }
        break;

        case short_type: {
            result.s = (jshort)d;
        }
        break;

        case int_type: {
            result.i = (jint)d;
        }
        break;

        case long_type: {
            result.j = (jlong)d;
        }
        break;

        case float_type: {
            result.f = (jfloat)d;
        }
        break;

        case double_type: {
            result.d = (jdouble)d;
        }
        break;
            
        break;

        case invalid_type:
        default:
        case void_type: {
            bzero (&result, sizeof(jvalue));
        }
        break;
    }
    return result;
}
