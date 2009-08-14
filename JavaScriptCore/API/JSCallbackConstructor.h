/*
 * Copyright (C) 2006, 2008 Apple Inc. All rights reserved.
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

#ifndef JSCallbackConstructor_h
#define JSCallbackConstructor_h

#include "JSObjectRef.h"
#include <runtime/JSObject.h>

namespace JSC {

class JSCallbackConstructor : public JSObject {
public:
    JSCallbackConstructor(PassRefPtr<Structure>, JSClassRef, JSObjectCallAsConstructorCallback);
    virtual ~JSCallbackConstructor();
    JSClassRef classRef() const { return m_class; }
    JSObjectCallAsConstructorCallback callback() const { return m_callback; }
    static const ClassInfo info;
    
    static PassRefPtr<Structure> createStructure(JSValue proto) 
    { 
        return Structure::create(proto, TypeInfo(ObjectType, ImplementsHasInstance | HasStandardGetOwnPropertySlot | HasDefaultMark)); 
    }

private:
    virtual ConstructType getConstructData(ConstructData&);
    virtual const ClassInfo* classInfo() const { return &info; }

    JSClassRef m_class;
    JSObjectCallAsConstructorCallback m_callback;
};

} // namespace JSC

#endif // JSCallbackConstructor_h
