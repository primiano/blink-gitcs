/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef WebScriptDebugServer_H
#define WebScriptDebugServer_H

#include "WebKit.h"

// FIXME: <rdar://5932722> Remove this class entirely once we stop exporting progIDForClass.

class WebScriptDebugServer : public IWebScriptDebugServer {
public:
    static WebScriptDebugServer* createInstance();

    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface( 
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void** ppvObject);

    virtual ULONG STDMETHODCALLTYPE AddRef();

    virtual ULONG STDMETHODCALLTYPE Release();

    // IWebScriptDebugServer
    virtual HRESULT STDMETHODCALLTYPE sharedWebScriptDebugServer( 
        /* [retval][out] */ IWebScriptDebugServer** server);

    virtual HRESULT STDMETHODCALLTYPE addListener(
        /* [in] */ IWebScriptDebugListener*);

    virtual HRESULT STDMETHODCALLTYPE removeListener(
        /* [in] */ IWebScriptDebugListener*);

    virtual HRESULT STDMETHODCALLTYPE step();

    virtual HRESULT STDMETHODCALLTYPE pause();

    virtual HRESULT STDMETHODCALLTYPE resume();

    virtual HRESULT STDMETHODCALLTYPE isPaused(
        /* [out, retval] */ BOOL* isPaused);

private:
    WebScriptDebugServer();
    ~WebScriptDebugServer();

    ULONG m_refCount;
};

#endif
