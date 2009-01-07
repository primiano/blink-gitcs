/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2008 Apple Inc. All rights reserved.
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

#ifndef DatePrototype_h
#define DatePrototype_h

#include "DateInstance.h"

namespace JSC {

    class ObjectPrototype;

    class DatePrototype : public DateInstance {
    public:
        DatePrototype(ExecState*, PassRefPtr<Structure>);

        virtual bool getOwnPropertySlot(ExecState*, const Identifier&, PropertySlot&);

        virtual const ClassInfo* classInfo() const { return &info; }
        static const ClassInfo info;

        static PassRefPtr<Structure> createStructure(JSValuePtr prototype)
        {
            return Structure::create(prototype, TypeInfo(ObjectType));
        }
    };

} // namespace JSC

#endif // DatePrototype_h
