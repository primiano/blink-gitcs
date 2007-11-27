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

#include "config.h"
#include "WebKitDLL.h"
#include "WebScriptCallFrame.h"

#include "COMEnumVariant.h"
#include "IWebScriptScope.h"
#include "Function.h"
#include "WebScriptScope.h"

#include <JavaScriptCore/JSGlobalObject.h>
#include <JavaScriptCore/JSStringRefBSTR.h>
#include <JavaScriptCore/JSValueRef.h>

#pragma warning(push, 0)
#include <WebCore/BString.h>
#include <WebCore/PlatformString.h>
#pragma warning(pop)

#include <wtf/Assertions.h>

using namespace KJS;

// WebScriptCallFrame -----------------------------------------------------------

WebScriptCallFrame::WebScriptCallFrame(ExecState* state, IWebScriptCallFrame* caller)
    : m_refCount(0)
{
    m_state = state;
    m_caller = caller;

    gClassCount++;
}

WebScriptCallFrame::~WebScriptCallFrame()
{
    gClassCount--;
}

WebScriptCallFrame* WebScriptCallFrame::createInstance(ExecState* state, IWebScriptCallFrame* caller)
{
    WebScriptCallFrame* instance = new WebScriptCallFrame(state, caller);
    instance->AddRef();
    return instance;
}

// IUnknown ------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebScriptCallFrame::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<IWebScriptCallFrame*>(this);
    else if (IsEqualGUID(riid, IID_IWebScriptCallFrame))
        *ppvObject = static_cast<IWebScriptCallFrame*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE WebScriptCallFrame::AddRef()
{
    return ++m_refCount;
}

ULONG STDMETHODCALLTYPE WebScriptCallFrame::Release()
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete(this);

    return newRef;
}

// IWebScriptCallFrame -----------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebScriptCallFrame::caller(
    /* [out, retval] */ IWebScriptCallFrame** callFrame)
{
    return m_caller.copyRefTo(callFrame);
}

template<> struct COMVariantSetter<JSObject*> : COMIUnknownVariantSetter<WebScriptScope, JSObject*> {};

HRESULT STDMETHODCALLTYPE WebScriptCallFrame::scopeChain(
    /* [out, retval] */ IEnumVARIANT** result)
{
    if (!result)
        return E_POINTER;

    // FIXME: If there is no current body do we need to make scope chain from the global object?
    *result = COMEnumVariant<ScopeChain>::createInstance(m_state->scopeChain());

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebScriptCallFrame::functionName(
    /* [out, retval] */ BSTR* funcName)
{
    if (!funcName)
        return E_POINTER;

    *funcName = 0;

    if (!m_state->currentBody())
        return S_OK;

    FunctionImp* func = m_state->function();
    if (!func)
        return E_FAIL;

    const Identifier& funcIdent = func->functionName();
    if (!funcIdent.isEmpty())
        *funcName = WebCore::BString(funcIdent).release();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebScriptCallFrame::stringByEvaluatingJavaScriptFromString(
    /* [in] */ BSTR script,
    /* [out, retval] */ BSTR* result)
{
    if (!script)
        return E_FAIL;

    if (!result)
        return E_POINTER;

    *result = 0;

    JSLock lock;

    JSValue* scriptExecutionResult = valueByEvaluatingJavaScriptFromString(script);

    if (scriptExecutionResult && scriptExecutionResult->isString())
        *result = WebCore::BString(WebCore::String(scriptExecutionResult->getString())).release();
    else
        return E_FAIL;

    return S_OK;
}

JSValue* WebScriptCallFrame::valueByEvaluatingJavaScriptFromString(BSTR script)
{
    ExecState* state = m_state;
    Interpreter* interp  = state->dynamicInterpreter();
    JSObject* globObj = interp->globalObject();

    // find "eval"
    JSObject* eval = 0;
    if (state->currentBody()) {  // "eval" won't work without context (i.e. at global scope)
        JSValue* v = globObj->get(state, "eval");
        if (v->isObject() && static_cast<JSObject*>(v)->implementsCall())
            eval = static_cast<JSObject*>(v);
        else
            // no "eval" - fallback operates on global exec state
            state = interp->globalExec();
    }

    JSValue* savedException = state->exception();
    state->clearException();

    UString code(reinterpret_cast<KJS::UChar*>(script), SysStringLen(script));

    // evaluate
    JSValue* scriptExecutionResult;
    if (eval) {
        List args;
        args.append(jsString(code));
        scriptExecutionResult = eval->call(state, 0, args);
    } else
        // no "eval", or no context (i.e. global scope) - use global fallback
        scriptExecutionResult = interp->evaluate(UString(), 0, code.data(), code.size(), globObj).value();

    if (state->hadException())
        scriptExecutionResult = state->exception();    // (may be redundant depending on which eval path was used)
    state->setException(savedException);

    return scriptExecutionResult;
}

