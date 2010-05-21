/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2006, 2008 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef FunctionPrototype_h
#define FunctionPrototype_h

#include "InternalFunction.h"

namespace JSC {

    class PrototypeFunction;

    class FunctionPrototype : public InternalFunction {
    public:
        FunctionPrototype(ExecState*, JSGlobalObject*, NonNullPassRefPtr<Structure>);
        void addFunctionProperties(ExecState*, JSGlobalObject*, Structure* prototypeFunctionStructure, NativeFunctionWrapper** callFunction, NativeFunctionWrapper** applyFunction);

        static PassRefPtr<Structure> createStructure(JSValue proto)
        {
            return Structure::create(proto, TypeInfo(ObjectType, StructureFlags), AnonymousSlotCount);
        }

    private:
        virtual CallType getCallData(CallData&);
    };

} // namespace JSC

#endif // FunctionPrototype_h
