// -*- c-basic-offset: 2 -*-
/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.
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

#ifndef INTERNAL_H
#define INTERNAL_H

#include "JSType.h"
#include "interpreter.h"
#include "object.h"
#include "protect.h"
#include "scope_chain.h"
#include "types.h"
#include "ustring.h"

#include <wtf/Noncopyable.h>

#define I18N_NOOP(s) s

namespace KJS {

  class FunctionPrototype;

  // ---------------------------------------------------------------------------
  //                            Primitive impls
  // ---------------------------------------------------------------------------

  class StringImp : public JSCell {
  public:
    StringImp(const UString& v) : val(v) { Collector::reportExtraMemoryCost(v.cost()); }
    enum HasOtherOwnerType { HasOtherOwner };
    StringImp(const UString& value, HasOtherOwnerType) : val(value) { }
    UString value() const { return val; }

    JSType type() const { return StringType; }

    JSValue* toPrimitive(ExecState*, JSType preferred = UnspecifiedType) const;
    bool getPrimitiveNumber(ExecState*, double& number) const;
    bool toBoolean(ExecState *exec) const;
    double toNumber(ExecState *exec) const;
    UString toString(ExecState *exec) const;
    JSObject *toObject(ExecState *exec) const;

  private:
    UString val;
  };

  class NumberImp : public JSCell {
    friend class ConstantValues;
    friend JSValue *jsNumberCell(double);
  public:
    double value() const { return val; }

    JSType type() const { return NumberType; }

    JSValue* toPrimitive(ExecState*, JSType preferred = UnspecifiedType) const;
    bool getPrimitiveNumber(ExecState*, double& number) const;
    bool toBoolean(ExecState *exec) const;
    double toNumber(ExecState *exec) const;
    UString toString(ExecState *exec) const;
    JSObject *toObject(ExecState *exec) const;

  private:
    NumberImp(double v) : val(v) { }

    virtual bool getUInt32(uint32_t&) const;
    virtual bool getTruncatedInt32(int32_t&) const;
    virtual bool getTruncatedUInt32(uint32_t&) const;

    double val;
  };


  // ---------------------------------------------------------------------------
  //                            Evaluation
  // ---------------------------------------------------------------------------

  struct AttachedInterpreter;
  class DebuggerImp {
  public:

    DebuggerImp() {
      interps = 0;
      isAborted = false;
    }

    void abort() { isAborted = true; }
    bool aborted() const { return isAborted; }

    AttachedInterpreter *interps;
    bool isAborted;
  };

#ifndef NDEBUG
  void printInfo(ExecState *exec, const char *s, JSValue *, int lineno = -1);
#endif

} // namespace

#endif //  INTERNAL_H
