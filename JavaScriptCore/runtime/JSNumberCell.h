/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2004, 2005, 2007, 2008 Apple Inc. All rights reserved.
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

#ifndef JSNumberCell_h
#define JSNumberCell_h

#include "CallFrame.h"
#include "JSCell.h"
#include "JSImmediate.h"
#include "Collector.h"
#include "UString.h"
#include <stddef.h> // for size_t

namespace JSC {

    extern const double NaN;
    extern const double Inf;

    JSValue jsNumberCell(ExecState*, double);

#if !USE(ALTERNATE_JSIMMEDIATE)

    class Identifier;
    class JSCell;
    class JSObject;
    class JSString;
    class PropertySlot;

    struct ClassInfo;
    struct Instruction;

    class JSNumberCell : public JSCell {
        friend class JIT;
        friend JSValue jsNumberCell(JSGlobalData*, double);
        friend JSValue jsNumberCell(ExecState*, double);
        friend JSValue jsAPIMangledNumber(ExecState*, double);
    public:
        double value() const { return m_value; }

        virtual JSValue toPrimitive(ExecState*, PreferredPrimitiveType) const;
        virtual bool getPrimitiveNumber(ExecState*, double& number, JSValue& value);
        virtual bool toBoolean(ExecState*) const;
        virtual double toNumber(ExecState*) const;
        virtual UString toString(ExecState*) const;
        virtual JSObject* toObject(ExecState*) const;

        virtual UString toThisString(ExecState*) const;
        virtual JSObject* toThisObject(ExecState*) const;
        virtual JSValue getJSNumber();

        static const uintptr_t JSAPIMangledMagicNumber = 0xbbadbeef;
        bool isAPIMangledNumber() const { return m_structure == reinterpret_cast<Structure*>(JSAPIMangledMagicNumber); }

        void* operator new(size_t size, ExecState* exec)
        {
    #ifdef JAVASCRIPTCORE_BUILDING_ALL_IN_ONE_FILE
            return exec->heap()->inlineAllocateNumber(size);
    #else
            return exec->heap()->allocateNumber(size);
    #endif
        }

        void* operator new(size_t size, JSGlobalData* globalData)
        {
    #ifdef JAVASCRIPTCORE_BUILDING_ALL_IN_ONE_FILE
            return globalData->heap.inlineAllocateNumber(size);
    #else
            return globalData->heap.allocateNumber(size);
    #endif
        }

        static PassRefPtr<Structure> createStructure(JSValue proto) { return Structure::create(proto, TypeInfo(NumberType, NeedsThisConversion)); }

    private:
        JSNumberCell(JSGlobalData* globalData, double value)
            : JSCell(globalData->numberStructure.get())
            , m_value(value)
        {
        }

        JSNumberCell(ExecState* exec, double value)
            : JSCell(exec->globalData().numberStructure.get())
            , m_value(value)
        {
        }

        enum APIMangledTag { APIMangled };
        JSNumberCell(APIMangledTag, double value)
            : JSCell(reinterpret_cast<Structure*>(JSAPIMangledMagicNumber))
            , m_value(value)
        {
        }

        virtual bool getUInt32(uint32_t&) const;
        virtual bool getTruncatedInt32(int32_t&) const;
        virtual bool getTruncatedUInt32(uint32_t&) const;

        double m_value;
    };

    JSValue jsNumberCell(JSGlobalData*, double);

    inline bool isNumberCell(JSValue v)
    {
        return v.isCell() && v.asCell()->isNumber();
    }

    inline JSNumberCell* asNumberCell(JSValue v)
    {
        ASSERT(isNumberCell(v));
        return static_cast<JSNumberCell*>(v.asCell());
    }

    ALWAYS_INLINE JSValue jsNumber(ExecState* exec, double d)
    {
        JSValue v = JSImmediate::from(d);
        return v ? v : jsNumberCell(exec, d);
    }

    ALWAYS_INLINE JSValue jsNumber(ExecState* exec, int i)
    {
        JSValue v = JSImmediate::from(i);
        return v ? v : jsNumberCell(exec, i);
    }

    ALWAYS_INLINE JSValue jsNumber(ExecState* exec, unsigned i)
    {
        JSValue v = JSImmediate::from(i);
        return v ? v : jsNumberCell(exec, i);
    }

    ALWAYS_INLINE JSValue jsNumber(ExecState* exec, long i)
    {
        JSValue v = JSImmediate::from(i);
        return v ? v : jsNumberCell(exec, i);
    }

    ALWAYS_INLINE JSValue jsNumber(ExecState* exec, unsigned long i)
    {
        JSValue v = JSImmediate::from(i);
        return v ? v : jsNumberCell(exec, i);
    }

    ALWAYS_INLINE JSValue jsNumber(ExecState* exec, long long i)
    {
        JSValue v = JSImmediate::from(i);
        return v ? v : jsNumberCell(exec, static_cast<double>(i));
    }

    ALWAYS_INLINE JSValue jsNumber(ExecState* exec, unsigned long long i)
    {
        JSValue v = JSImmediate::from(i);
        return v ? v : jsNumberCell(exec, static_cast<double>(i));
    }

    ALWAYS_INLINE JSValue jsNumber(JSGlobalData* globalData, double d)
    {
        JSValue v = JSImmediate::from(d);
        return v ? v : jsNumberCell(globalData, d);
    }

    ALWAYS_INLINE JSValue jsNumber(JSGlobalData* globalData, int i)
    {
        JSValue v = JSImmediate::from(i);
        return v ? v : jsNumberCell(globalData, i);
    }

    ALWAYS_INLINE JSValue jsNumber(JSGlobalData* globalData, unsigned i)
    {
        JSValue v = JSImmediate::from(i);
        return v ? v : jsNumberCell(globalData, i);
    }

    ALWAYS_INLINE JSValue jsNumber(JSGlobalData* globalData, long i)
    {
        JSValue v = JSImmediate::from(i);
        return v ? v : jsNumberCell(globalData, i);
    }

    ALWAYS_INLINE JSValue jsNumber(JSGlobalData* globalData, unsigned long i)
    {
        JSValue v = JSImmediate::from(i);
        return v ? v : jsNumberCell(globalData, i);
    }

    ALWAYS_INLINE JSValue jsNumber(JSGlobalData* globalData, long long i)
    {
        JSValue v = JSImmediate::from(i);
        return v ? v : jsNumberCell(globalData, static_cast<double>(i));
    }

    ALWAYS_INLINE JSValue jsNumber(JSGlobalData* globalData, unsigned long long i)
    {
        JSValue v = JSImmediate::from(i);
        return v ? v : jsNumberCell(globalData, static_cast<double>(i));
    }

    inline bool JSValue::isDoubleNumber() const
    {
        return isNumberCell(asValue());
    }

    inline double JSValue::getDoubleNumber() const
    {
        return asNumberCell(asValue())->value();
    }

    inline bool JSValue::isNumber() const
    {
        return JSImmediate::isNumber(asValue()) || isDoubleNumber();
    }

    inline double JSValue::uncheckedGetNumber() const
    {
        ASSERT(isNumber());
        return JSImmediate::isImmediate(asValue()) ? JSImmediate::toDouble(asValue()) : getDoubleNumber();
    }

    JSValue jsAPIMangledNumber(ExecState* exec, double);

    inline bool JSValue::isAPIMangledNumber()
    {
        ASSERT(isNumber());
        return JSImmediate::isImmediate(asValue()) ? false : asNumberCell(asValue())->isAPIMangledNumber();
    }

#else

    ALWAYS_INLINE JSValue jsNumber(ExecState*, double d)
    {
        JSValue v = JSImmediate::from(d);
        ASSERT(v);
        return v;
    }

    ALWAYS_INLINE JSValue jsNumber(ExecState*, int i)
    {
        JSValue v = JSImmediate::from(i);
        ASSERT(v);
        return v;
    }

    ALWAYS_INLINE JSValue jsNumber(ExecState*, unsigned i)
    {
        JSValue v = JSImmediate::from(i);
        ASSERT(v);
        return v;
    }

    ALWAYS_INLINE JSValue jsNumber(ExecState*, long i)
    {
        JSValue v = JSImmediate::from(i);
        ASSERT(v);
        return v;
    }

    ALWAYS_INLINE JSValue jsNumber(ExecState*, unsigned long i)
    {
        JSValue v = JSImmediate::from(i);
        ASSERT(v);
        return v;
    }

    ALWAYS_INLINE JSValue jsNumber(ExecState*, long long i)
    {
        JSValue v = JSImmediate::from(static_cast<double>(i));
        ASSERT(v);
        return v;
    }

    ALWAYS_INLINE JSValue jsNumber(ExecState*, unsigned long long i)
    {
        JSValue v = JSImmediate::from(static_cast<double>(i));
        ASSERT(v);
        return v;
    }

    ALWAYS_INLINE JSValue jsNumber(JSGlobalData*, double d)
    {
        JSValue v = JSImmediate::from(d);
        ASSERT(v);
        return v;
    }

    ALWAYS_INLINE JSValue jsNumber(JSGlobalData*, int i)
    {
        JSValue v = JSImmediate::from(i);
        ASSERT(v);
        return v;
    }

    ALWAYS_INLINE JSValue jsNumber(JSGlobalData*, unsigned i)
    {
        JSValue v = JSImmediate::from(i);
        ASSERT(v);
        return v;
    }

    ALWAYS_INLINE JSValue jsNumber(JSGlobalData*, long i)
    {
        JSValue v = JSImmediate::from(i);
        ASSERT(v);
        return v;
    }

    ALWAYS_INLINE JSValue jsNumber(JSGlobalData*, unsigned long i)
    {
        JSValue v = JSImmediate::from(i);
        ASSERT(v);
        return v;
    }

    ALWAYS_INLINE JSValue jsNumber(JSGlobalData*, long long i)
    {
        JSValue v = JSImmediate::from(static_cast<double>(i));
        ASSERT(v);
        return v;
    }

    ALWAYS_INLINE JSValue jsNumber(JSGlobalData*, unsigned long long i)
    {
        JSValue v = JSImmediate::from(static_cast<double>(i));
        ASSERT(v);
        return v;
    }

    inline bool JSValue::isDoubleNumber() const
    {
        return JSImmediate::isDoubleNumber(asValue());
    }

    inline double JSValue::getDoubleNumber() const
    {
        return JSImmediate::doubleValue(asValue());
    }

    inline bool JSValue::isNumber() const
    {
        return JSImmediate::isNumber(asValue());
    }

    inline double JSValue::uncheckedGetNumber() const
    {
        ASSERT(isNumber());
        return JSImmediate::toDouble(asValue());
    }

#endif

    ALWAYS_INLINE JSValue jsNumber(ExecState*, char i)
    {
        ASSERT(JSImmediate::from(i));
        return JSImmediate::from(i);
    }

    ALWAYS_INLINE JSValue jsNumber(ExecState*, unsigned char i)
    {
        ASSERT(JSImmediate::from(i));
        return JSImmediate::from(i);
    }

    ALWAYS_INLINE JSValue jsNumber(ExecState*, short i)
    {
        ASSERT(JSImmediate::from(i));
        return JSImmediate::from(i);
    }

    ALWAYS_INLINE JSValue jsNumber(ExecState*, unsigned short i)
    {
        ASSERT(JSImmediate::from(i));
        return JSImmediate::from(i);
    }

    ALWAYS_INLINE JSValue jsNumber(JSGlobalData*, short i)
    {
        ASSERT(JSImmediate::from(i));
        return JSImmediate::from(i);
    }

    ALWAYS_INLINE JSValue jsNumber(JSGlobalData*, unsigned short i)
    {
        ASSERT(JSImmediate::from(i));
        return JSImmediate::from(i);
    }

    inline JSValue jsNaN(ExecState* exec)
    {
        return jsNumber(exec, NaN);
    }

    inline JSValue jsNaN(JSGlobalData* globalData)
    {
        return jsNumber(globalData, NaN);
    }

    // --- JSValue inlines ----------------------------

    ALWAYS_INLINE JSValue JSValue::toJSNumber(ExecState* exec) const
    {
        return isNumber() ? asValue() : jsNumber(exec, this->toNumber(exec));
    }

    inline bool JSValue::getNumber(double &result) const
    {
        if (isInt32Fast())
            result = getInt32Fast();
        else if (LIKELY(isDoubleNumber()))
            result = getDoubleNumber();
        else {
            ASSERT(!isNumber());
            return false;
        }
        return true;
    }

    inline bool JSValue::numberToInt32(int32_t& arg)
    {
        if (isInt32Fast())
            arg = getInt32Fast();
        else if (LIKELY(isDoubleNumber()))
            arg = JSC::toInt32(getDoubleNumber());
        else {
            ASSERT(!isNumber());
            return false;
        }
        return true;
    }

    inline bool JSValue::numberToUInt32(uint32_t& arg)
    {
        if (isUInt32Fast())
            arg = getUInt32Fast();
        else if (LIKELY(isDoubleNumber()))
            arg = JSC::toUInt32(getDoubleNumber());
        else if (isInt32Fast()) {
            // FIXME: I think this case can be merged with the uint case; toUInt32SlowCase
            // on a negative value is equivalent to simple static_casting.
            bool ignored;
            arg = toUInt32SlowCase(getInt32Fast(), ignored);
        } else {
            ASSERT(!isNumber());
            return false;
        }
        return true;
    }

} // namespace JSC

#endif // JSNumberCell_h
