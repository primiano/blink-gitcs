/*
 * Copyright (C) 2005 Apple Computer, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "JSRun.h"

JSRun::JSRun(CFStringRef source, JSFlags inFlags)
    :   JSBase(kJSRunTypeID),
        fSource(CFStringToUString(source)),
        fGlobalObject(new JSGlobalObject()),
        fFlags(inFlags)
{
    Interpreter* interpreter = new JSInterpreter(fGlobalObject, inFlags);
    interpreter->setGlobalObject(fGlobalObject); // fGlobalObject now owns interpreter
}

JSRun::~JSRun()
{
}

JSFlags JSRun::Flags() const
{
    return fFlags;
}

UString JSRun::GetSource() const
{
    return fSource;
}

JSGlobalObject* JSRun::GlobalObject() const
{
    return fGlobalObject;
}

Completion JSRun::Evaluate()
{
    return fGlobalObject->interpreter()->evaluate(UString(), 0, fSource.data(), fSource.size());
}

bool JSRun::CheckSyntax()
{
    return fGlobalObject->interpreter()->checkSyntax(UString(), 0, fSource.data(), fSource.size()).complType() != Throw;
}
