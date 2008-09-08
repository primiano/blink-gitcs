/*
 *  Copyright (C) 2003, 2008 Apple Inc. All rights reserved.
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

#ifndef KJS_dtoa_h
#define KJS_dtoa_h

namespace WTF {
    class Mutex;
}

namespace JSC {

    extern WTF::Mutex* s_dtoaP5Mutex;

    double strtod(const char* s00, char** se);
    char* dtoa(double d, int ndigits, int* decpt, int* sign, char** rve);
    void freedtoa(char* s);

} // namespace JSC

#endif /* KJS_dtoa_h */
