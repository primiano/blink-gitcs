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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef StructureStubInfo_h
#define StructureStubInfo_h

#if ENABLE(JIT)

#include "Instruction.h"
#include "MacroAssembler.h"
#include "Opcode.h"
#include "Structure.h"

namespace JSC {

    enum AccessType {
        access_get_by_id_self,
        access_get_by_id_proto,
        access_get_by_id_chain,
        access_get_by_id_self_list,
        access_get_by_id_proto_list,
        access_put_by_id_transition,
        access_put_by_id_replace,
        access_get_by_id,
        access_put_by_id,
        access_get_by_id_generic,
        access_put_by_id_generic,
        access_get_array_length,
        access_get_string_length,
    };

    struct StructureStubInfo {
        StructureStubInfo(AccessType accessType)
            : accessType(accessType)
            , seen(false)
        {
        }

        void initGetByIdSelf(Structure* baseObjectStructure)
        {
            accessType = access_get_by_id_self;

            u.getByIdSelf.baseObjectStructure = baseObjectStructure;
            baseObjectStructure->ref();
        }

        void initGetByIdProto(Structure* baseObjectStructure, Structure* prototypeStructure)
        {
            accessType = access_get_by_id_proto;

            u.getByIdProto.baseObjectStructure = baseObjectStructure;
            baseObjectStructure->ref();

            u.getByIdProto.prototypeStructure = prototypeStructure;
            prototypeStructure->ref();
        }

        void initGetByIdChain(Structure* baseObjectStructure, StructureChain* chain)
        {
            accessType = access_get_by_id_chain;

            u.getByIdChain.baseObjectStructure = baseObjectStructure;
            baseObjectStructure->ref();

            u.getByIdChain.chain = chain;
            chain->ref();
        }

        void initGetByIdSelfList(PolymorphicAccessStructureList* structureList, int listSize)
        {
            accessType = access_get_by_id_self_list;

            u.getByIdProtoList.structureList = structureList;
            u.getByIdProtoList.listSize = listSize;
        }

        void initGetByIdProtoList(PolymorphicAccessStructureList* structureList, int listSize)
        {
            accessType = access_get_by_id_proto_list;

            u.getByIdProtoList.structureList = structureList;
            u.getByIdProtoList.listSize = listSize;
        }

        // PutById*

        void initPutByIdTransition(Structure* previousStructure, Structure* structure, StructureChain* chain)
        {
            accessType = access_put_by_id_transition;

            u.putByIdTransition.previousStructure = previousStructure;
            previousStructure->ref();

            u.putByIdTransition.structure = structure;
            structure->ref();

            u.putByIdTransition.chain = chain;
            chain->ref();
        }

        void initPutByIdReplace(Structure* baseObjectStructure)
        {
            accessType = access_put_by_id_replace;
    
            u.putByIdReplace.baseObjectStructure = baseObjectStructure;
            baseObjectStructure->ref();
        }

        void deref();

        bool seenOnce()
        {
            return seen;
        }

        void setSeen()
        {
            seen = true;
        }

        int accessType : 31;
        int seen : 1;

        union {
            struct {
                Structure* baseObjectStructure;
            } getByIdSelf;
            struct {
                Structure* baseObjectStructure;
                Structure* prototypeStructure;
            } getByIdProto;
            struct {
                Structure* baseObjectStructure;
                StructureChain* chain;
            } getByIdChain;
            struct {
                PolymorphicAccessStructureList* structureList;
                int listSize;
            } getByIdSelfList;
            struct {
                PolymorphicAccessStructureList* structureList;
                int listSize;
            } getByIdProtoList;
            struct {
                Structure* previousStructure;
                Structure* structure;
                StructureChain* chain;
            } putByIdTransition;
            struct {
                Structure* baseObjectStructure;
            } putByIdReplace;
        } u;

        CodeLocationLabel stubRoutine;
        CodeLocationCall callReturnLocation;
        CodeLocationLabel hotPathBegin;
    };

} // namespace JSC

#endif

#endif // StructureStubInfo_h
