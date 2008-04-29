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

#include "DumpRenderTree.h"
#include "EditingDelegate.h"

#include "LayoutTestController.h"
#include <WebCore/COMPtr.h>
#include <wtf/Platform.h>
#include <JavaScriptCore/Assertions.h>
#include <string>
#include <tchar.h>

using std::wstring;

EditingDelegate::EditingDelegate()
    : m_refCount(1)
    , m_acceptsEditing(true)
{
}

// IUnknown
HRESULT STDMETHODCALLTYPE EditingDelegate::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<IWebEditingDelegate*>(this);
    else if (IsEqualGUID(riid, IID_IWebEditingDelegate))
        *ppvObject = static_cast<IWebEditingDelegate*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE EditingDelegate::AddRef(void)
{
    return ++m_refCount;
}

ULONG STDMETHODCALLTYPE EditingDelegate::Release(void)
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete this;

    return newRef;
}

static wstring dumpPath(IDOMNode* node)
{
    ASSERT(node);

    wstring result;

    BSTR name;
    if (FAILED(node->nodeName(&name)))
        return result;
    result.assign(name, SysStringLen(name));
    SysFreeString(name);

    COMPtr<IDOMNode> parent;
    if (SUCCEEDED(node->parentNode(parent.adoptionPointer())))
        result += TEXT(" > ") + dumpPath(parent.get());

    return result;
}

static wstring dump(IDOMRange* range)
{
    ASSERT(range);

    int startOffset;
    if (FAILED(range->startOffset(&startOffset)))
        return 0;

    int endOffset;
    if (FAILED(range->endOffset(&endOffset)))
        return 0;

    COMPtr<IDOMNode> startContainer;
    if (FAILED(range->startContainer(startContainer.adoptionPointer())))
        return 0;

    COMPtr<IDOMNode> endContainer;
    if (FAILED(range->endContainer(endContainer.adoptionPointer())))
        return 0;

    wchar_t buffer[1024];
    _snwprintf(buffer, ARRAYSIZE(buffer), L"range from %ld of %s to %ld of %s", startOffset, dumpPath(startContainer.get()), endOffset, dumpPath(endContainer.get()));
    return buffer;
}

HRESULT STDMETHODCALLTYPE EditingDelegate::shouldBeginEditingInDOMRange( 
    /* [in] */ IWebView* webView,
    /* [in] */ IDOMRange* range,
    /* [retval][out] */ BOOL* result)
{
    if (!result) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    if (::layoutTestController->dumpEditingCallbacks() && !done)
        _tprintf(TEXT("EDITING DELEGATE: shouldBeginEditingInDOMRange:%s\n"), dump(range));

    *result = m_acceptsEditing;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE EditingDelegate::shouldEndEditingInDOMRange( 
    /* [in] */ IWebView* webView,
    /* [in] */ IDOMRange* range,
    /* [retval][out] */ BOOL* result)
{
    if (!result) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    if (::layoutTestController->dumpEditingCallbacks() && !done)
        _tprintf(TEXT("EDITING DELEGATE: shouldEndEditingInDOMRange:%s\n"), dump(range));

    *result = m_acceptsEditing;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE EditingDelegate::shouldInsertNode( 
    /* [in] */ IWebView* webView,
    /* [in] */ IDOMNode* node,
    /* [in] */ IDOMRange* range,
    /* [in] */ WebViewInsertAction action)
{
    static LPCTSTR insertactionstring[] = {
        TEXT("WebViewInsertActionTyped"),
        TEXT("WebViewInsertActionPasted"),
        TEXT("WebViewInsertActionDropped"),
    };

    if (::layoutTestController->dumpEditingCallbacks() && !done)
        _tprintf(TEXT("EDITING DELEGATE: shouldInsertNode:%s replacingDOMRange:%s givenAction:%s\n"), dumpPath(node), dump(range), insertactionstring[action]);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE EditingDelegate::shouldInsertText( 
    /* [in] */ IWebView* webView,
    /* [in] */ BSTR text,
    /* [in] */ IDOMRange* range,
    /* [in] */ WebViewInsertAction action,
    /* [retval][out] */ BOOL* result)
{
    if (!result) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    static LPCTSTR insertactionstring[] = {
        TEXT("WebViewInsertActionTyped"),
        TEXT("WebViewInsertActionPasted"),
        TEXT("WebViewInsertActionDropped"),
    };

    if (::layoutTestController->dumpEditingCallbacks() && !done)
        _tprintf(TEXT("EDITING DELEGATE: shouldInsertText:%s replacingDOMRange:%s givenAction:%s\n"), text ? text : TEXT(""), dump(range), insertactionstring[action]);

    *result = m_acceptsEditing;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE EditingDelegate::shouldDeleteDOMRange( 
    /* [in] */ IWebView* webView,
    /* [in] */ IDOMRange* range,
    /* [retval][out] */ BOOL* result)
{
    if (!result) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    if (::layoutTestController->dumpEditingCallbacks() && !done)
        _tprintf(TEXT("EDITING DELEGATE: shouldDeleteDOMRange:%s\n"), dump(range));

    *result = m_acceptsEditing;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE EditingDelegate::shouldChangeSelectedDOMRange( 
    /* [in] */ IWebView* webView,
    /* [in] */ IDOMRange* currentRange,
    /* [in] */ IDOMRange* proposedRange,
    /* [in] */ WebSelectionAffinity selectionAffinity,
    /* [in] */ BOOL stillSelecting,
    /* [retval][out] */ BOOL* result)
{
    if (!result) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    static LPCTSTR affinitystring[] = {
        TEXT("NSSelectionAffinityUpstream"),
        TEXT("NSSelectionAffinityDownstream")
    };
    static LPCTSTR boolstring[] = {
        TEXT("FALSE"),
        TEXT("TRUE")
    };

    if (::layoutTestController->dumpEditingCallbacks() && !done)
        _tprintf(TEXT("EDITING DELEGATE: shouldChangeSelectedDOMRange:%s toDOMRange:%s affinity:%s stillSelecting:%s\n"), dump(currentRange), dump(proposedRange), affinitystring[selectionAffinity], boolstring[stillSelecting]);

    *result = m_acceptsEditing;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE EditingDelegate::shouldApplyStyle( 
    /* [in] */ IWebView* webView,
    /* [in] */ IDOMCSSStyleDeclaration* style,
    /* [in] */ IDOMRange* range,
    /* [retval][out] */ BOOL* result)
{
    if (!result) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    if (::layoutTestController->dumpEditingCallbacks() && !done)
        _tprintf(TEXT("EDITING DELEGATE: shouldApplyStyle:%s toElementsInDOMRange:%s\n"), TEXT("'style description'")/*[[style description] UTF8String]*/, dump(range));

    *result = m_acceptsEditing;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE EditingDelegate::shouldChangeTypingStyle( 
    /* [in] */ IWebView* webView,
    /* [in] */ IDOMCSSStyleDeclaration* currentStyle,
    /* [in] */ IDOMCSSStyleDeclaration* proposedStyle,
    /* [retval][out] */ BOOL* result)
{
    if (!result) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    if (::layoutTestController->dumpEditingCallbacks() && !done)
        _tprintf(TEXT("EDITING DELEGATE: shouldChangeTypingStyle:%s toStyle:%s\n"), TEXT("'currentStyle description'"), TEXT("'proposedStyle description'"));

    *result = m_acceptsEditing;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE EditingDelegate::doPlatformCommand( 
    /* [in] */ IWebView *webView,
    /* [in] */ BSTR command,
    /* [retval][out] */ BOOL *result)
{
    if (!result) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    if (::layoutTestController->dumpEditingCallbacks() && !done)
        _tprintf(TEXT("EDITING DELEGATE: doPlatformCommand:%s\n"), command ? command : TEXT(""));

    *result = m_acceptsEditing;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE EditingDelegate::webViewDidBeginEditing( 
    /* [in] */ IWebNotification* notification)
{
    if (::layoutTestController->dumpEditingCallbacks() && !done) {
        BSTR name;
        notification->name(&name);
        _tprintf(TEXT("EDITING DELEGATE: webViewDidBeginEditing:%s\n"), name ? name : TEXT(""));
        SysFreeString(name);
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE EditingDelegate::webViewDidChange( 
    /* [in] */ IWebNotification *notification)
{
    if (::layoutTestController->dumpEditingCallbacks() && !done) {
        BSTR name;
        notification->name(&name);
        _tprintf(TEXT("EDITING DELEGATE: webViewDidBeginEditing:%s\n"), name ? name : TEXT(""));
        SysFreeString(name);
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE EditingDelegate::webViewDidEndEditing( 
    /* [in] */ IWebNotification *notification)
{
    if (::layoutTestController->dumpEditingCallbacks() && !done) {
        BSTR name;
        notification->name(&name);
        _tprintf(TEXT("EDITING DELEGATE: webViewDidEndEditing:%s\n"), name ? name : TEXT(""));
        SysFreeString(name);
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE EditingDelegate::webViewDidChangeTypingStyle( 
    /* [in] */ IWebNotification *notification)
{
    if (::layoutTestController->dumpEditingCallbacks() && !done) {
        BSTR name;
        notification->name(&name);
        _tprintf(TEXT("EDITING DELEGATE: webViewDidChangeTypingStyle:%s\n"), name ? name : TEXT(""));
        SysFreeString(name);
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE EditingDelegate::webViewDidChangeSelection( 
    /* [in] */ IWebNotification *notification)
{
    if (::layoutTestController->dumpEditingCallbacks() && !done) {
        BSTR name;
        notification->name(&name);
        _tprintf(TEXT("EDITING DELEGATE: webViewDidChangeSelection:%s\n"), name ? name : TEXT(""));
        SysFreeString(name);
    }
    return S_OK;
}
