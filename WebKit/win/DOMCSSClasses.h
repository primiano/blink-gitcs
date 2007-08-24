/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
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

#ifndef DOMCSSClasses_H
#define DOMCSSClasses_H

#include "DOMCSS.h"
#include "DOMCoreClasses.h"

#include <WebCore/CSSStyleDeclaration.h>

class DeprecatedDOMCSSStyleDeclaration : public DeprecatedDOMObject, public IDeprecatedDOMCSSStyleDeclaration
{
protected:
    DeprecatedDOMCSSStyleDeclaration(WebCore::CSSStyleDeclaration* d);
    ~DeprecatedDOMCSSStyleDeclaration();

public:
    static IDeprecatedDOMCSSStyleDeclaration* createInstance(WebCore::CSSStyleDeclaration* d);

public:
    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void) { return DeprecatedDOMObject::AddRef(); }
    virtual ULONG STDMETHODCALLTYPE Release(void) { return DeprecatedDOMObject::Release(); }

    // IWebScriptObject
    virtual HRESULT STDMETHODCALLTYPE throwException( 
        /* [in] */ BSTR exceptionMessage,
        /* [retval][out] */ BOOL *result) { return DeprecatedDOMObject::throwException(exceptionMessage, result); }
    
    virtual HRESULT STDMETHODCALLTYPE callWebScriptMethod( 
        /* [in] */ BSTR name,
        /* [size_is][in] */ const VARIANT args[  ],
        /* [in] */ int cArgs,
        /* [retval][out] */ VARIANT *result) { return DeprecatedDOMObject::callWebScriptMethod(name, args, cArgs, result); }
    
    virtual HRESULT STDMETHODCALLTYPE evaluateWebScript( 
        /* [in] */ BSTR script,
        /* [retval][out] */ VARIANT *result) { return DeprecatedDOMObject::evaluateWebScript(script, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeWebScriptKey( 
        /* [in] */ BSTR name) { return DeprecatedDOMObject::removeWebScriptKey(name); }
    
    virtual HRESULT STDMETHODCALLTYPE stringRepresentation( 
        /* [retval][out] */ BSTR* stringRepresentation) { return DeprecatedDOMObject::stringRepresentation(stringRepresentation); }
    
    virtual HRESULT STDMETHODCALLTYPE webScriptValueAtIndex( 
        /* [in] */ unsigned int index,
        /* [retval][out] */ VARIANT *result) { return DeprecatedDOMObject::webScriptValueAtIndex(index, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setWebScriptValueAtIndex( 
        /* [in] */ unsigned int index,
        /* [in] */ VARIANT val) { return DeprecatedDOMObject::setWebScriptValueAtIndex(index, val); }
    
    virtual HRESULT STDMETHODCALLTYPE setException( 
        /* [in] */ BSTR description) { return DeprecatedDOMObject::setException(description); }

    // IDeprecatedDOMCSSStyleDeclaration
    virtual HRESULT STDMETHODCALLTYPE cssText( 
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE setCssText( 
        /* [in] */ BSTR cssText);
    
    virtual HRESULT STDMETHODCALLTYPE getPropertyValue( 
        /* [in] */ BSTR propertyName,
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE getPropertyCSSValue( 
        /* [in] */ BSTR propertyName,
        /* [retval][out] */ IDeprecatedDOMCSSValue **result);
    
    virtual HRESULT STDMETHODCALLTYPE removeProperty( 
        /* [in] */ BSTR propertyName,
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE getPropertyPriority( 
        /* [in] */ BSTR propertyName,
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE setProperty( 
        /* [in] */ BSTR propertyName,
        /* [in] */ BSTR value,
        /* [in] */ BSTR priority);
    
    virtual HRESULT STDMETHODCALLTYPE length( 
        /* [retval][out] */ UINT *result);
    
    virtual HRESULT STDMETHODCALLTYPE item( 
        /* [in] */ UINT index,
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE parentRule( 
        /* [retval][out] */ IDeprecatedDOMCSSRule **result);

protected:
    ULONG m_refCount;
    WebCore::CSSStyleDeclaration* m_style;
};

#endif
