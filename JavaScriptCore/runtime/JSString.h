/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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

#ifndef JSString_h
#define JSString_h

#include "CallFrame.h"
#include "CommonIdentifiers.h"
#include "Identifier.h"
#include "JSNumberCell.h"
#include "PropertyDescriptor.h"
#include "PropertySlot.h"

namespace JSC {

    class JSString;

    JSString* jsEmptyString(JSGlobalData*);
    JSString* jsEmptyString(ExecState*);
    JSString* jsString(JSGlobalData*, const UString&); // returns empty string if passed null string
    JSString* jsString(ExecState*, const UString&); // returns empty string if passed null string

    JSString* jsSingleCharacterString(JSGlobalData*, UChar);
    JSString* jsSingleCharacterString(ExecState*, UChar);
    JSString* jsSingleCharacterSubstring(JSGlobalData*, const UString&, unsigned offset);
    JSString* jsSingleCharacterSubstring(ExecState*, const UString&, unsigned offset);
    JSString* jsSubstring(JSGlobalData*, const UString&, unsigned offset, unsigned length);
    JSString* jsSubstring(ExecState*, const UString&, unsigned offset, unsigned length);

    // Non-trivial strings are two or more characters long.
    // These functions are faster than just calling jsString.
    JSString* jsNontrivialString(JSGlobalData*, const UString&);
    JSString* jsNontrivialString(ExecState*, const UString&);
    JSString* jsNontrivialString(JSGlobalData*, const char*);
    JSString* jsNontrivialString(ExecState*, const char*);

    // Should be used for strings that are owned by an object that will
    // likely outlive the JSValue this makes, such as the parse tree or a
    // DOM object that contains a UString
    JSString* jsOwnedString(JSGlobalData*, const UString&); 
    JSString* jsOwnedString(ExecState*, const UString&); 

    class JSString : public JSCell {
    public:
        friend class JIT;
        friend struct VPtrSet;

        // A Rope is a string composed of a set of substrings.
        class Rope : public RefCounted<Rope> {
        public:
            // A Rope is composed from a set of smaller strings called Fibers.
            // Each Fiber in a rope is either UString::Rep or another Rope.
            class Fiber {
            public:
                Fiber() {}
                Fiber(UString::Rep* string) : m_value(reinterpret_cast<intptr_t>(string)) {}
                Fiber(Rope* rope) : m_value(reinterpret_cast<intptr_t>(rope) | 1) {}

                bool isRope() { return m_value & 1; }
                Rope* rope() { return reinterpret_cast<Rope*>(m_value & ~1); }
                bool isString() { return !isRope(); }
                UString::Rep* string() { return reinterpret_cast<UString::Rep*>(m_value); }

            private:
                intptr_t m_value;
            };

            // Creates a Rope comprising of 'ropeLength' Fibers.
            // The Rope is constructed in an uninitialized state - initialize must be called for each Fiber in the Rope.
            static PassRefPtr<Rope> createOrNull(unsigned ropeLength)
            {
                void* allocation;
                if (tryFastMalloc(sizeof(Rope) + (ropeLength - 1) * sizeof(Fiber)).getValue(allocation))
                    return adoptRef(new (allocation) Rope(ropeLength));
                return 0;
            }

            ~Rope();
            void destructNonRecursive();

            void initializeFiber(unsigned index, const UString& string)
            {
                UString::Rep* rep = string.rep();
                rep->ref();
                m_fibers[index] = Fiber(rep);
                m_stringLength += rep->len;
            }
            void initializeFiber(unsigned index, Rope* rope)
            {
                rope->ref();
                m_fibers[index] = Fiber(rope);
                m_stringLength += rope->stringLength();
            }
            void initializeFiber(unsigned index, JSString* jsString)
            {
                if (jsString->isRope())
                    initializeFiber(index, jsString->rope());
                else
                    initializeFiber(index, jsString->string());
            }

            unsigned ropeLength() { return m_ropeLength; }
            unsigned stringLength() { return m_stringLength; }
            Fiber& fibers(unsigned index) { return m_fibers[index]; }

        private:
            Rope(unsigned ropeLength) : m_ropeLength(ropeLength), m_stringLength(0) {}
            void* operator new(size_t, void* inPlace) { return inPlace; }
            
            unsigned m_ropeLength;
            unsigned m_stringLength;
            Fiber m_fibers[1];
        };

        JSString(JSGlobalData* globalData, const UString& value)
            : JSCell(globalData->stringStructure.get())
            , m_length(value.size())
            , m_value(value)
        {
            Heap::heap(this)->reportExtraMemoryCost(value.cost());
        }

        enum HasOtherOwnerType { HasOtherOwner };
        JSString(JSGlobalData* globalData, const UString& value, HasOtherOwnerType)
            : JSCell(globalData->stringStructure.get())
            , m_length(value.size())
            , m_value(value)
        {
        }
        JSString(JSGlobalData* globalData, PassRefPtr<UString::Rep> value, HasOtherOwnerType)
            : JSCell(globalData->stringStructure.get())
            , m_length(value->size())
            , m_value(value)
        {
        }
        JSString(JSGlobalData* globalData, PassRefPtr<JSString::Rope> rope)
            : JSCell(globalData->stringStructure.get())
            , m_length(rope->stringLength())
            , m_rope(rope)
        {
        }

        const UString& value(ExecState* exec) const
        {
            if (m_rope)
                resolveRope(exec);
            return m_value;
        }
        const UString tryGetValue() const
        {
            if (m_rope)
                UString();
            return m_value;
        }
        unsigned length() { return m_length; }

        bool isRope() const { return m_rope; }
        Rope* rope() { ASSERT(isRope()); return m_rope.get(); }
        UString& string() { ASSERT(!isRope()); return m_value; }

        bool getStringPropertySlot(ExecState*, const Identifier& propertyName, PropertySlot&);
        bool getStringPropertySlot(ExecState*, unsigned propertyName, PropertySlot&);
        bool getStringPropertyDescriptor(ExecState*, const Identifier& propertyName, PropertyDescriptor&);

        bool canGetIndex(unsigned i) { return i < m_length; }
        JSString* getIndex(ExecState*, unsigned);

        static PassRefPtr<Structure> createStructure(JSValue proto) { return Structure::create(proto, TypeInfo(StringType, OverridesGetOwnPropertySlot | NeedsThisConversion)); }

    private:
        enum VPtrStealingHackType { VPtrStealingHack };
        JSString(VPtrStealingHackType) 
            : JSCell(0)
        {
        }

        void resolveRope(ExecState*) const;

        virtual JSValue toPrimitive(ExecState*, PreferredPrimitiveType) const;
        virtual bool getPrimitiveNumber(ExecState*, double& number, JSValue& value);
        virtual bool toBoolean(ExecState*) const;
        virtual double toNumber(ExecState*) const;
        virtual JSObject* toObject(ExecState*) const;
        virtual UString toString(ExecState*) const;

        virtual JSObject* toThisObject(ExecState*) const;
        virtual UString toThisString(ExecState*) const;
        virtual JSString* toThisJSString(ExecState*);

        // Actually getPropertySlot, not getOwnPropertySlot (see JSCell).
        virtual bool getOwnPropertySlot(ExecState*, const Identifier& propertyName, PropertySlot&);
        virtual bool getOwnPropertySlot(ExecState*, unsigned propertyName, PropertySlot&);
        virtual bool getOwnPropertyDescriptor(ExecState*, const Identifier&, PropertyDescriptor&);

        // A string is represented either by a UString or a Rope.
        unsigned m_length;
        mutable UString m_value;
        mutable RefPtr<Rope> m_rope;
    };

    JSString* asString(JSValue);

    inline JSString* asString(JSValue value)
    {
        ASSERT(asCell(value)->isString());
        return static_cast<JSString*>(asCell(value));
    }

    inline JSString* jsEmptyString(JSGlobalData* globalData)
    {
        return globalData->smallStrings.emptyString(globalData);
    }

    inline JSString* jsSingleCharacterString(JSGlobalData* globalData, UChar c)
    {
        if (c <= 0xFF)
            return globalData->smallStrings.singleCharacterString(globalData, c);
        return new (globalData) JSString(globalData, UString(&c, 1));
    }

    inline JSString* jsSingleCharacterSubstring(JSGlobalData* globalData, const UString& s, unsigned offset)
    {
        ASSERT(offset < static_cast<unsigned>(s.size()));
        UChar c = s.data()[offset];
        if (c <= 0xFF)
            return globalData->smallStrings.singleCharacterString(globalData, c);
        return new (globalData) JSString(globalData, UString(UString::Rep::create(s.rep(), offset, 1)));
    }

    inline JSString* jsNontrivialString(JSGlobalData* globalData, const char* s)
    {
        ASSERT(s);
        ASSERT(s[0]);
        ASSERT(s[1]);
        return new (globalData) JSString(globalData, s);
    }

    inline JSString* jsNontrivialString(JSGlobalData* globalData, const UString& s)
    {
        ASSERT(s.size() > 1);
        return new (globalData) JSString(globalData, s);
    }

    inline JSString* JSString::getIndex(ExecState* exec, unsigned i)
    {
        ASSERT(canGetIndex(i));
        return jsSingleCharacterSubstring(&exec->globalData(), value(exec), i);
    }

    inline JSString* jsString(JSGlobalData* globalData, const UString& s)
    {
        int size = s.size();
        if (!size)
            return globalData->smallStrings.emptyString(globalData);
        if (size == 1) {
            UChar c = s.data()[0];
            if (c <= 0xFF)
                return globalData->smallStrings.singleCharacterString(globalData, c);
        }
        return new (globalData) JSString(globalData, s);
    }
        
    inline JSString* jsSubstring(JSGlobalData* globalData, const UString& s, unsigned offset, unsigned length)
    {
        ASSERT(offset <= static_cast<unsigned>(s.size()));
        ASSERT(length <= static_cast<unsigned>(s.size()));
        ASSERT(offset + length <= static_cast<unsigned>(s.size()));
        if (!length)
            return globalData->smallStrings.emptyString(globalData);
        if (length == 1) {
            UChar c = s.data()[offset];
            if (c <= 0xFF)
                return globalData->smallStrings.singleCharacterString(globalData, c);
        }
        return new (globalData) JSString(globalData, UString(UString::Rep::create(s.rep(), offset, length)));
    }

    inline JSString* jsOwnedString(JSGlobalData* globalData, const UString& s)
    {
        int size = s.size();
        if (!size)
            return globalData->smallStrings.emptyString(globalData);
        if (size == 1) {
            UChar c = s.data()[0];
            if (c <= 0xFF)
                return globalData->smallStrings.singleCharacterString(globalData, c);
        }
        return new (globalData) JSString(globalData, s, JSString::HasOtherOwner);
    }

    inline JSString* jsEmptyString(ExecState* exec) { return jsEmptyString(&exec->globalData()); }
    inline JSString* jsString(ExecState* exec, const UString& s) { return jsString(&exec->globalData(), s); }
    inline JSString* jsSingleCharacterString(ExecState* exec, UChar c) { return jsSingleCharacterString(&exec->globalData(), c); }
    inline JSString* jsSingleCharacterSubstring(ExecState* exec, const UString& s, unsigned offset) { return jsSingleCharacterSubstring(&exec->globalData(), s, offset); }
    inline JSString* jsSubstring(ExecState* exec, const UString& s, unsigned offset, unsigned length) { return jsSubstring(&exec->globalData(), s, offset, length); }
    inline JSString* jsNontrivialString(ExecState* exec, const UString& s) { return jsNontrivialString(&exec->globalData(), s); }
    inline JSString* jsNontrivialString(ExecState* exec, const char* s) { return jsNontrivialString(&exec->globalData(), s); }
    inline JSString* jsOwnedString(ExecState* exec, const UString& s) { return jsOwnedString(&exec->globalData(), s); } 

    ALWAYS_INLINE bool JSString::getStringPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
    {
        if (propertyName == exec->propertyNames().length) {
            slot.setValue(jsNumber(exec, m_length));
            return true;
        }

        bool isStrictUInt32;
        unsigned i = propertyName.toStrictUInt32(&isStrictUInt32);
        if (isStrictUInt32 && i < m_length) {
            slot.setValue(jsSingleCharacterSubstring(exec, value(exec), i));
            return true;
        }

        return false;
    }
        
    ALWAYS_INLINE bool JSString::getStringPropertySlot(ExecState* exec, unsigned propertyName, PropertySlot& slot)
    {
        if (propertyName < m_length) {
            slot.setValue(jsSingleCharacterSubstring(exec, value(exec), propertyName));
            return true;
        }

        return false;
    }

    inline bool isJSString(JSGlobalData* globalData, JSValue v) { return v.isCell() && v.asCell()->vptr() == globalData->jsStringVPtr; }

    // --- JSValue inlines ----------------------------

    inline JSString* JSValue::toThisJSString(ExecState* exec)
    {
        return isCell() ? asCell()->toThisJSString(exec) : jsString(exec, toString(exec));
    }

    inline UString JSValue::toString(ExecState* exec) const
    {
        if (isString())
            return static_cast<JSString*>(asCell())->value(exec);
        if (isInt32())
            return exec->globalData().numericStrings.add(asInt32());
        if (isDouble())
            return exec->globalData().numericStrings.add(asDouble());
        if (isTrue())
            return "true";
        if (isFalse())
            return "false";
        if (isNull())
            return "null";
        if (isUndefined())
            return "undefined";
        ASSERT(isCell());
        return asCell()->toString(exec);
    }

} // namespace JSC

#endif // JSString_h
