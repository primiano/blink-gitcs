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
#include <internal.h>
#include <ustring.h>
#include <value.h>

#include <jni_utility.h>
#include <jni_runtime.h>

#include <runtime_object.h>

using namespace KJS;
using namespace Bindings;


JavaParameter::JavaParameter (JNIEnv *env, jstring type)
{
    _type = JavaString (env, type);
    _JNIType = primitiveTypeFromClassName (_type.characters());
};

JavaField::JavaField (JNIEnv *env, jobject aField)
{
    // Get field type
    jobject fieldType = callJNIObjectMethod (aField, "getType", "()Ljava/lang/Class;");
    jstring fieldTypeName = (jstring)callJNIObjectMethod (fieldType, "toString", "()Ljava/lang/String;");
    _type = JavaString(env, fieldTypeName);
    _primitiveType = primitiveTypeFromClassName (_type.characters());

    // Get field name
    jstring fieldName = (jstring)callJNIObjectMethod (aField, "getName", "()Ljava/lang/String;");
    _name = JavaString(env, fieldName);
    
    _field = new JavaInstance(aField);
}

KJS::Value JavaField::valueFromInstance(const Instance *i) const 
{
    const JavaInstance *instance = static_cast<const JavaInstance *>(i);
    jobject jinstance = instance->javaInstance();
    jobject fieldJInstance = _field->javaInstance();

    switch (_primitiveType) {
        case object_type: {
            jobject anObject = callJNIObjectMethod(_field->javaInstance(), "get", "(Ljava/lang/Object;)Ljava/lang/Object;", jinstance);
            return KJS::Object(new RuntimeObjectImp(new JavaInstance ((jobject)anObject)));
        }
        break;
            
        case boolean_type: {
            jboolean value = callJNIBooleanMethod(fieldJInstance, "getBoolean", "(Ljava/lang/Object;)Z", jinstance);
            return KJS::Boolean((bool)value);
        }
        break;
            
        case byte_type:
        case char_type:
        case short_type:
        
        case int_type:
            jint value;
            value = callJNIIntMethod(fieldJInstance, "getInt", "(Ljava/lang/Object;)I", jinstance);
            return Number((int)value);

        case long_type:
        case float_type:
        case double_type: {
            jdouble value;
            value = callJNIDoubleMethod(fieldJInstance, "getDouble", "(Ljava/lang/Object;)D", jinstance);
            return Number((double)value);
        }
        break;
        default:
        break;
    }
    return Undefined();
}


JavaConstructor::JavaConstructor (JNIEnv *env, jobject aConstructor)
{
    // Get parameters
    jarray jparameters = (jarray)callJNIObjectMethod (aConstructor, "getParameterTypes", "()[Ljava/lang/Class;");
    _numParameters = env->GetArrayLength (jparameters);
    _parameters = new JavaParameter[_numParameters];
    
    int i;
    for (i = 0; i < _numParameters; i++) {
        jobject aParameter = env->GetObjectArrayElement ((jobjectArray)jparameters, i);
        jstring parameterName = (jstring)callJNIObjectMethod (aParameter, "getName", "()Ljava/lang/String;");
        _parameters[i] = JavaParameter(env, parameterName);
    }
}

JavaMethod::JavaMethod (JNIEnv *env, jobject aMethod)
{
    // Get return type
    jobject returnType = callJNIObjectMethod (aMethod, "getReturnType", "()Ljava/lang/Class;");
    jstring returnTypeName = (jstring)callJNIObjectMethod (returnType, "getName", "()Ljava/lang/String;");
    _returnType =JavaString (env, returnTypeName);
    _JNIReturnType = primitiveTypeFromClassName (_returnType.characters());

    // Get method name
    jstring methodName = (jstring)callJNIObjectMethod (aMethod, "getName", "()Ljava/lang/String;");
    _name = JavaString (env, methodName);

    // Get parameters
    jarray jparameters = (jarray)callJNIObjectMethod (aMethod, "getParameterTypes", "()[Ljava/lang/Class;");
    _numParameters = env->GetArrayLength (jparameters);
    _parameters = new JavaParameter[_numParameters];
    
    int i;
    for (i = 0; i < _numParameters; i++) {
        jobject aParameter = env->GetObjectArrayElement ((jobjectArray)jparameters, i);
        jstring parameterName = (jstring)callJNIObjectMethod (aParameter, "getName", "()Ljava/lang/String;");
        _parameters[i] = JavaParameter(env, parameterName);
    }

    
    // Created lazily.
    _signature = 0;
}

// JNI method signatures use '/' between components of a class name, but
// we get '.' between components from the reflection API.
static void appendClassName (UString *aString, const char *className)
{
    char *result, *cp = strdup(className);
    
    result = cp;
    while (*cp) {
        if (*cp == '.')
            *cp = '/';
        cp++;
    }
        
    aString->append(result);

    free (result);
}

const char *JavaMethod::signature() const 
{
    if (_signature == 0){
        int i;
        
        _signature = new UString("(");
        for (i = 0; i < _numParameters; i++) {
            JavaParameter *aParameter = static_cast<JavaParameter *>(parameterAt(i));
            JNIType _JNIType = aParameter->getJNIType();
            _signature->append(signatureFromPrimitiveType (_JNIType));
            if (_JNIType == object_type) {
                appendClassName (_signature, aParameter->type());
                _signature->append(";");
            }
        }
        _signature->append(")");
        
        _signature->append(signatureFromPrimitiveType (_JNIReturnType));
        if (_JNIReturnType == object_type) {
            appendClassName (_signature, _returnType.characters());
            _signature->append(";");
        }
    }
    
    return _signature->ascii();
}

JNIType JavaMethod::JNIReturnType() const
{
    return _JNIReturnType;
}
