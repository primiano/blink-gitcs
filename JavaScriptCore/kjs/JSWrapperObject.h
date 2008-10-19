/*
 *  Copyright (C) 2006 Maks Orlovich
 *  Copyright (C) 2006, 2008 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#ifndef KJS_JSWrapperObject_h
#define KJS_JSWrapperObject_h

#include "JSObject.h"

namespace JSC {
    
    // This class is used as a base for classes such as String,
    // Number, Boolean and Date which are wrappers for primitive types.
    class JSWrapperObject : public JSObject {
    public:
        explicit JSWrapperObject(PassRefPtr<StructureID>);
        
        JSValuePtr internalValue() const { return m_internalValue; }
        void setInternalValue(JSValuePtr);
        
        virtual void mark();
        
    private:
        JSValuePtr m_internalValue;
    };
    
    inline JSWrapperObject::JSWrapperObject(PassRefPtr<StructureID> structure)
        : JSObject(structure)
        , m_internalValue(noValue())
    {
    }
    
    inline void JSWrapperObject::setInternalValue(JSValuePtr value)
    {
        ASSERT(value);
        ASSERT(!value->isObject());
        m_internalValue = value;
    }

} // namespace JSC

#endif // KJS_JSWrapperObject_h
