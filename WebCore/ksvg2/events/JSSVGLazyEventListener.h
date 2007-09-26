/*
    Copyright (C) 2006 Apple Computer, Inc.
                  
    This file is part of the WebKit project

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef JSSVGLazyEventListener_h
#define JSSVGLazyEventListener_h
#if ENABLE(SVG)

#include "kjs_events.h"

namespace WebCore {

    class JSSVGLazyEventListener : public JSLazyEventListener {
    public:
        JSSVGLazyEventListener(const String& functionName, const String& code, KJS::Window*, Node*, int lineno = 0);
    private:
        virtual KJS::JSValue* eventParameterName() const;
    };

}

#endif // ENABLE(SVG)
#endif

// vim:ts=4:noet
