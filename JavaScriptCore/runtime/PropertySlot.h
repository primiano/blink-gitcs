/*
 *  Copyright (C) 2005, 2007, 2008 Apple Inc. All rights reserved.
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

#ifndef PropertySlot_h
#define PropertySlot_h

#include "Identifier.h"
#include "JSValue.h"
#include "Register.h"
#include <wtf/Assertions.h>
#include <wtf/NotFound.h>

namespace JSC {

    class ExecState;
    class JSObject;

#define JSC_VALUE_SLOT_MARKER 0
#define JSC_REGISTER_SLOT_MARKER reinterpret_cast<GetValueFunc>(1)
#define INDEX_GETTER_MARKER reinterpret_cast<GetValueFunc>(2)

    class PropertySlot {
    public:
        PropertySlot()
        {
            clearBase();
            clearOffset();
            clearValue();
        }

        explicit PropertySlot(const JSValue base)
            : m_slotBase(base)
        {
            clearOffset();
            clearValue();
        }

        typedef JSValue (*GetValueFunc)(ExecState*, const Identifier&, const PropertySlot&);
        typedef JSValue (*GetIndexValueFunc)(ExecState*, JSValue slotBase, unsigned);

        JSValue getValue(ExecState* exec, const Identifier& propertyName) const
        {
            if (m_getValue == JSC_VALUE_SLOT_MARKER)
                return *m_data.valueSlot;
            if (m_getValue == JSC_REGISTER_SLOT_MARKER)
                return (*m_data.registerSlot).jsValue();
            if (m_getValue == INDEX_GETTER_MARKER)
                return m_getIndexValue(exec, slotBase(), index());
            return m_getValue(exec, propertyName, *this);
        }

        JSValue getValue(ExecState* exec, unsigned propertyName) const
        {
            if (m_getValue == JSC_VALUE_SLOT_MARKER)
                return *m_data.valueSlot;
            if (m_getValue == JSC_REGISTER_SLOT_MARKER)
                return (*m_data.registerSlot).jsValue();
            if (m_getValue == INDEX_GETTER_MARKER)
                return m_getIndexValue(exec, m_slotBase, m_data.index);
            return m_getValue(exec, Identifier::from(exec, propertyName), *this);
        }

        bool isGetter() const { return m_isGetter; }
        bool isCacheable() const { return m_isCacheable; }
        bool isCacheableValue() const { return m_isCacheable && !m_isGetter; }
        size_t cachedOffset() const
        {
            ASSERT(isCacheable());
            return m_offset;
        }

        void setValueSlot(JSValue* valueSlot) 
        {
            ASSERT(valueSlot);
            clearBase();
            clearOffset();
            m_getValue = JSC_VALUE_SLOT_MARKER;
            m_data.valueSlot = valueSlot;
        }
        
        void setValueSlot(JSValue slotBase, JSValue* valueSlot)
        {
            ASSERT(valueSlot);
            m_getValue = JSC_VALUE_SLOT_MARKER;
            m_slotBase = slotBase;
            m_data.valueSlot = valueSlot;
        }
        
        void setValueSlot(JSValue slotBase, JSValue* valueSlot, size_t offset)
        {
            ASSERT(valueSlot);
            m_getValue = JSC_VALUE_SLOT_MARKER;
            m_slotBase = slotBase;
            m_data.valueSlot = valueSlot;
            m_offset = offset;
            m_isCacheable = true;
            m_isGetter = false;
        }
        
        void setValue(JSValue value)
        {
            ASSERT(value);
            clearBase();
            clearOffset();
            m_getValue = JSC_VALUE_SLOT_MARKER;
            m_value = value;
            m_data.valueSlot = &m_value;
        }

        void setRegisterSlot(Register* registerSlot)
        {
            ASSERT(registerSlot);
            clearBase();
            clearOffset();
            m_getValue = JSC_REGISTER_SLOT_MARKER;
            m_data.registerSlot = registerSlot;
        }

        void setCustom(JSValue slotBase, GetValueFunc getValue)
        {
            ASSERT(slotBase);
            ASSERT(getValue);
            m_getValue = getValue;
            m_getIndexValue = 0;
            m_slotBase = slotBase;
        }

        void setCustomIndex(JSValue slotBase, unsigned index, GetIndexValueFunc getIndexValue)
        {
            ASSERT(slotBase);
            ASSERT(getIndexValue);
            m_getValue = INDEX_GETTER_MARKER;
            m_getIndexValue = getIndexValue;
            m_slotBase = slotBase;
            m_data.index = index;
        }

        void setGetterSlot(JSObject* getterFunc)
        {
            ASSERT(getterFunc);
            m_thisValue = m_slotBase;
            m_getValue = functionGetter;
            m_data.getterFunc = getterFunc;
            m_isGetter = true;
        }

        void setCacheableGetterSlot(JSValue slotBase, JSObject* getterFunc, unsigned offset)
        {
            ASSERT(getterFunc);
            m_getValue = functionGetter;
            m_thisValue = m_slotBase;
            m_slotBase = slotBase;
            m_data.getterFunc = getterFunc;
            m_offset = offset;
            m_isCacheable = true;
            m_isGetter = true;
        }

        void setUndefined()
        {
            setValue(jsUndefined());
        }

        JSValue slotBase() const
        {
            return m_slotBase;
        }

        void setBase(JSValue base)
        {
            ASSERT(m_slotBase);
            ASSERT(base);
            m_slotBase = base;
        }

        void clearBase()
        {
#ifndef NDEBUG
            m_slotBase = JSValue();
#endif
        }

        void clearValue()
        {
#ifndef NDEBUG
            m_value = JSValue();
#endif
        }

        void clearOffset()
        {
            // Clear offset even in release builds, in case this PropertySlot has been used before.
            // (For other data members, we don't need to clear anything because reuse would meaningfully overwrite them.)
            m_offset = 0;
            m_isCacheable = false;
            m_isGetter = false;
        }

        unsigned index() const { return m_data.index; }

        JSValue thisValue() const { return m_thisValue; }
    private:
        static JSValue functionGetter(ExecState*, const Identifier&, const PropertySlot&);

        GetValueFunc m_getValue;
        GetIndexValueFunc m_getIndexValue;
        
        JSValue m_slotBase;
        union {
            JSObject* getterFunc;
            JSValue* valueSlot;
            Register* registerSlot;
            unsigned index;
        } m_data;

        JSValue m_value;
        JSValue m_thisValue;

        size_t m_offset;
        bool m_isCacheable : 1;
        bool m_isGetter : 1;
    };

} // namespace JSC

#endif // PropertySlot_h
