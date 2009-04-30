/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ScriptFunctionCall_h
#define ScriptFunctionCall_h

#include "PlatformString.h"
#include "ScriptObject.h"
#include "ScriptState.h"

#include <runtime/ArgList.h>

namespace JSC {
    class UString;
    class JSValuePtr;
}

namespace WebCore {
    class ScriptValue;
    class ScriptString;

    class ScriptFunctionCall {
    public:
        ScriptFunctionCall(ScriptState* exec, const ScriptObject& thisObject, const String& name);
        virtual ~ScriptFunctionCall() {};

        void appendArgument(const ScriptObject&);
        void appendArgument(const ScriptString&);
        void appendArgument(const ScriptValue&);
        void appendArgument(const String&);
        void appendArgument(const JSC::UString&);
        void appendArgument(JSC::JSValuePtr);
        void appendArgument(long long);
        void appendArgument(unsigned int);
        void appendArgument(int);
        void appendArgument(bool);
        ScriptValue call(bool& hadException, bool reportExceptions = true);
        ScriptValue call();
        ScriptObject construct(bool& hadException, bool reportExceptions = true);

    protected:
        ScriptState* m_exec;
        ScriptObject m_thisObject;
        String m_name;
        JSC::MarkedArgumentBuffer m_arguments;
    };

} // namespace WebCore

#endif // ScriptFunctionCall
