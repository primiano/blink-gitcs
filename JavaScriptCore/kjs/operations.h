// -*- c-basic-offset: 2 -*-
/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
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

#ifndef _KJS_OPERATIONS_H_
#define _KJS_OPERATIONS_H_

#include <math.h>

namespace KJS {

  class ExecState;
  class JSValue;

#if PLATFORM(DARWIN)
  inline bool isNaN(double d) { return isnan(d); }
  inline bool isInf(double d) { return isinf(d); }
  inline bool isPosInf(double d) { return isinf(d) && d > 0; }
  inline bool isNegInf(double d) { return isinf(d) && d < 0; }
#else
  bool isNaN(double d);
  bool isInf(double d);
  bool isPosInf(double d);
  bool isNegInf(double d);
#endif

  bool equal(ExecState *exec, JSValue *v1, JSValue *v2);
  bool strictEqual(ExecState *exec, JSValue *v1, JSValue *v2);
  int maxInt(int d1, int d2);
  int minInt(int d1, int d2);

}

#endif
