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

#include "value.h"
#include "object.h"
#include "types.h"
#include "interpreter.h"
#include "operations.h"
#include "runtime_object.h"
#include "error_object.h"

#include "runtime_object.h"

#include <assert.h>

using namespace KJS;
using namespace Bindings;

const ClassInfo *RuntimeObjectImp::classInfo() const
{
    return &_classInfo;
}

RuntimeObjectImp::RuntimeObjectImp(ObjectImp *proto)
  : ObjectImp(proto)
{
    instance = 0;
    _classInfo.className = 0;
    _classInfo.parentClass = 0;
    _classInfo.propHashTable = 0;
}

RuntimeObjectImp::~RuntimeObjectImp()
{
    delete instance;
}

RuntimeObjectImp::RuntimeObjectImp(Bindings::Instance *i) : ObjectImp ((ObjectImp *)0)
{
    instance = i;
    _initializeClassInfoFromInstance();
}

Value RuntimeObjectImp::get(ExecState *exec, const Identifier &propertyName) const
{
    printf ("%s: NOT FULLY IMPLEMENTED %p: propertyName %s\n", __PRETTY_FUNCTION__, instance, propertyName.ascii());
    // Get the value of the RuntimeObject's property.
    
    Field *aField = instance->getClass()->fieldNamed(propertyName.ascii());
    if (aField){
        printf ("%s: found field = %p, type = %s\n", __PRETTY_FUNCTION__, aField, aField->type());
        return instance->getValueOfField (aField); 
    }
    
    return Undefined();
}

void RuntimeObjectImp::put(ExecState *exec, const Identifier &propertyName,
                    const Value &value, int attr)
{
    printf ("%s: NOT YET IMPLEMENTED %p: propertyName %s\n", __PRETTY_FUNCTION__, instance, propertyName.ascii());
    // Set the value of the property.
}

bool RuntimeObjectImp::canPut(ExecState *exec, const Identifier &propertyName) const
{
    printf ("%s: NOT YET IMPLEMENTED %p: propertyName %s\n", __PRETTY_FUNCTION__, instance, propertyName.ascii());
    // Returns true if the propertyName is a public ivar of the object.
    return false;
}

bool RuntimeObjectImp::hasProperty(ExecState *exec,
                            const Identifier &propertyName) const
{
    printf ("%s: NOT YET IMPLEMENTED %p: propertyName %s\n", __PRETTY_FUNCTION__, instance, propertyName.ascii());
    // Returns true if the propertyName is a function or ivar of the instance
    // represented by this RuntimeObject.
    return false;
}

bool RuntimeObjectImp::deleteProperty(ExecState *exec,
                            const Identifier &propertyName)
{
    printf ("%s: NOT YET IMPLEMENTED %p: propertyName %s\n", __PRETTY_FUNCTION__, instance, propertyName.ascii());
    // Can never remove a propert of a RuntimeObject.
    return false;
}

Value RuntimeObjectImp::defaultValue(ExecState *exec, Type hint) const
{
    printf ("%s: NOT YET IMPLEMENTED %p\n", __PRETTY_FUNCTION__, instance);
    return ObjectImp::defaultValue(exec, hint);
}
    
void RuntimeObjectImp::_initializeClassInfoFromInstance()
{
    if (!instance)
        return;
}
