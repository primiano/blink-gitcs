/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
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

#ifndef WebScriptCallFrame_h
#define WebScriptCallFrame_h

#include "IWebScriptCallFrame.h"

#include <JavaScriptCore/ExecState.h>
#pragma warning(push, 0)
#include <WebCore/COMPtr.h>
#pragma warning(pop)

class WebScriptCallFrame : public IWebScriptCallFrame {
public:
    static WebScriptCallFrame* createInstance();

private:
    WebScriptCallFrame();
    virtual ~WebScriptCallFrame();

public:
    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface( 
        /* [in] */ REFIID riid,
        /* [retval][out] */ void** ppvObject);
    
    virtual ULONG STDMETHODCALLTYPE AddRef();

    virtual ULONG STDMETHODCALLTYPE Release();

    // IWebScriptCallFrame
    virtual HRESULT STDMETHODCALLTYPE caller(
        /* [out, retval] */ IWebScriptCallFrame**);

    virtual HRESULT STDMETHODCALLTYPE scopeChain(
        /* [out, retval] */ IEnumVARIANT**);

    virtual HRESULT STDMETHODCALLTYPE functionName(
        /* [out, retval] */ BSTR*);

    virtual HRESULT STDMETHODCALLTYPE stringByEvaluatingJavaScriptFromString(
        /* [in] */ BSTR script,
        /* [out, retval] */ BSTR*);

private:
    KJS::ExecState* m_state;
    COMPtr<WebScriptCallFrame> m_caller;

    ULONG m_refCount;
};

#endif
