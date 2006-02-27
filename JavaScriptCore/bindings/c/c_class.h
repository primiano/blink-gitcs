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
#ifndef _BINDINGS_C_CLASS_H_
#define _BINDINGS_C_CLASS_H_

#include <CoreFoundation/CoreFoundation.h>


#include <runtime.h>
#include <npruntime.h>
#include <c_runtime.h>

namespace KJS {

namespace Bindings {

class CClass : public KJS::Bindings::Class
{
protected:
    CClass(NPClass *aClass); // Use classForIsA to create a CClass.
    
public:
    ~CClass();

    // Return the cached ObjC of the specified name.
    static CClass *classForIsA(NPClass *aClass);
    
    virtual const char *name() const;
    
    virtual MethodList methodsNamed(const char *name, Instance *instance) const;
    
    virtual Field *fieldNamed(const char *name, Instance *instance) const;
    
    virtual Constructor *constructorAt(int) const {
        return 0;
    };
    
    virtual int numConstructors() const { return 0; };
        
private:
    CClass(); // prohibit default construction
    CClass(const CClass &other); // prohibit copying
    CClass &operator=(const CClass &other); // ditto
    
    NPClass *_isa;
    CFDictionaryRef _methods;
    CFDictionaryRef _fields;
};

} // namespace Bindings

} // namespace KJS

#endif