// -*- mode: c++; c-basic-offset: 4 -*-
/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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

#ifndef StructureID_h
#define StructureID_h

#include "JSType.h"
#include "JSValue.h"
#include "PropertyMap.h"
#include "StructureIDTransitionTable.h"
#include "TypeInfo.h"
#include "ustring.h"
#include <wtf/HashFunctions.h>
#include <wtf/HashTraits.h>
#include <wtf/OwnArrayPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

#define DUMP_STRUCTURE_ID_STATISTICS 0

namespace JSC {

    class PropertyNameArray;
    class PropertyNameArrayData;
    class StructureIDChain;

    class StructureID : public RefCounted<StructureID> {
    public:
        friend class CTI;
        static PassRefPtr<StructureID> create(JSValue* prototype, const TypeInfo& typeInfo)
        {
            return adoptRef(new StructureID(prototype, typeInfo));
        }

        static void startIgnoringLeaks();
        static void stopIgnoringLeaks();

#if DUMP_STRUCTURE_ID_STATISTICS
        static void dumpStatistics();
#endif

        static PassRefPtr<StructureID> changePrototypeTransition(StructureID*, JSValue* prototype);
        static PassRefPtr<StructureID> addPropertyTransition(StructureID*, const Identifier& propertyName, unsigned attributes, size_t& offset);
        static PassRefPtr<StructureID> getterSetterTransition(StructureID*);
        static PassRefPtr<StructureID> toDictionaryTransition(StructureID*);
        static PassRefPtr<StructureID> fromDictionaryTransition(StructureID*);

        ~StructureID();

        void mark()
        {
            if (!m_prototype->marked())
                m_prototype->mark();
        }

        // These should be used with caution.  
        size_t addPropertyWithoutTransition(const Identifier& propertyName, unsigned attributes);
        void setPrototypeWithoutTransition(JSValue* prototype) { m_prototype = prototype; }

        bool isDictionary() const { return m_isDictionary; }

        const TypeInfo& typeInfo() const { return m_typeInfo; }

        // For use when first creating a new structure.
        TypeInfo& mutableTypeInfo() { return m_typeInfo; }

        JSValue* storedPrototype() const { return m_prototype; }
        JSValue* prototypeForLookup(ExecState*); 

        StructureID* previousID() const { return m_previous.get(); }

        StructureIDChain* createCachedPrototypeChain();
        void setCachedPrototypeChain(PassRefPtr<StructureIDChain> cachedPrototypeChain) { m_cachedPrototypeChain = cachedPrototypeChain; }
        StructureIDChain* cachedPrototypeChain() const { return m_cachedPrototypeChain.get(); }

        const PropertyMap& propertyMap() const { return m_propertyMap; }
        PropertyMap& propertyMap() { return m_propertyMap; }

        void setCachedTransistionOffset(size_t offset) { m_cachedTransistionOffset = offset; }
        size_t cachedTransistionOffset() const { return m_cachedTransistionOffset; }

        void growPropertyStorageCapacity();
        size_t propertyStorageCapacity() const { return m_propertyStorageCapacity; }

        void getEnumerablePropertyNames(ExecState*, PropertyNameArray&, JSObject*);
        void clearEnumerationCache();

        bool hasGetterSetterProperties() const { return m_hasGetterSetterProperties; }
        void setHasGetterSetterProperties(bool hasGetterSetterProperties) { m_hasGetterSetterProperties = hasGetterSetterProperties; }

    private:
        StructureID(JSValue* prototype, const TypeInfo&);
        
        static const size_t s_maxTransitionLength = 64;

        TypeInfo m_typeInfo;

        JSValue* m_prototype;
        RefPtr<StructureIDChain> m_cachedPrototypeChain;

        RefPtr<StructureID> m_previous;
        UString::Rep* m_nameInPrevious;

        size_t m_transitionCount;
        union {
            StructureID* singleTransition;
            StructureIDTransitionTable* table;
        } m_transitions;

        RefPtr<PropertyNameArrayData> m_cachedPropertyNameArrayData;

        PropertyMap m_propertyMap;
        size_t m_propertyStorageCapacity;

        size_t m_cachedTransistionOffset;

        bool m_isDictionary : 1;
        bool m_hasGetterSetterProperties : 1;
        bool m_usingSingleTransitionSlot : 1;
        unsigned m_attributesInPrevious : 5;
    };

    class StructureIDChain : public RefCounted<StructureIDChain> {
    public:
        static PassRefPtr<StructureIDChain> create(StructureID* structureID) { return adoptRef(new StructureIDChain(structureID)); }

        RefPtr<StructureID>* head() { return m_vector.get(); }

    private:
        StructureIDChain(StructureID* structureID);

        OwnArrayPtr<RefPtr<StructureID> > m_vector;
    };

    bool structureIDChainsAreEqual(StructureIDChain*, StructureIDChain*);

} // namespace JSC

#endif // StructureID_h
