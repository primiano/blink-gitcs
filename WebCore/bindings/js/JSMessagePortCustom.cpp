/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "JSMessagePort.h"

#include "AtomicString.h"
#include "Event.h"
#include "Frame.h"
#include "JSDOMWindowCustom.h"
#include "JSEvent.h"
#include "JSEventListener.h"
#include "MessagePort.h"

using namespace JSC;

namespace WebCore {
    
JSValue* JSMessagePort::startConversation(ExecState* exec, const ArgList& args)
{
    DOMWindow* window = asJSDOMWindow(exec->lexicalGlobalObject())->impl();
    const UString& message = args.at(exec, 0)->toString(exec);

    return toJS(exec, impl()->startConversation(window->document(), message).get());
}

JSValue* JSMessagePort::addEventListener(ExecState* exec, const ArgList& args)
{
    Document* document = impl()->document();
    if (!document)
        return jsUndefined();
    JSDOMWindow* window = toJSDOMWindow(document->frame());
    if (!window)
        return jsUndefined();
    RefPtr<JSUnprotectedEventListener> listener = window->findOrCreateJSUnprotectedEventListener(exec, args.at(exec, 1), true);
    if (!listener)
        return jsUndefined();
    impl()->addEventListener(args.at(exec, 0)->toString(exec), listener.release(), args.at(exec, 2)->toBoolean(exec));
    return jsUndefined();
}

JSValue* JSMessagePort::removeEventListener(ExecState* exec, const ArgList& args)
{
    Document* document = impl()->document();
    if (!document)
        return jsUndefined();
    JSDOMWindow* window = toJSDOMWindow(document->frame());
    if (!window)
        return jsUndefined();
    JSUnprotectedEventListener* listener = window->findJSUnprotectedEventListener(exec, args.at(exec, 1), true);
    if (!listener)
        return jsUndefined();
    impl()->removeEventListener(args.at(exec, 0)->toString(exec), listener, args.at(exec, 2)->toBoolean(exec));
    return jsUndefined();
    
}
    
JSValue* JSMessagePort::dispatchEvent(JSC::ExecState* exec, const ArgList& args)
{
    ExceptionCode ec = 0;
    
    bool result = impl()->dispatchEvent(toEvent(args.at(exec, 0)), ec);
    setDOMException(exec, ec);
    return jsBoolean(result);    
}

void JSMessagePort::setOnmessage(ExecState* exec, JSValue* value)
{
    Document* document = impl()->document();
    if (!document)
        return;
    JSDOMWindow* window = toJSDOMWindow(document->frame());
    if (!window)
        return;
    impl()->setOnMessageListener(window->findOrCreateJSUnprotectedEventListener(exec, value, true));
}

JSValue* JSMessagePort::onmessage(ExecState*) const
{
    if (JSUnprotectedEventListener* listener = static_cast<JSUnprotectedEventListener*>(impl()->onMessageListener()))
        if (JSObject* listenerObj = listener->listenerObj())
            return listenerObj;
    return jsNull();
}

void JSMessagePort::setOnclose(ExecState* exec, JSValue* value)
{
    Document* document = impl()->document();
    if (!document)
        return;
    JSDOMWindow* window = toJSDOMWindow(document->frame());
    if (!window)
        return;
    impl()->setOnCloseListener(window->findOrCreateJSUnprotectedEventListener(exec, value, true));
}

JSValue* JSMessagePort::onclose(ExecState*) const
{
    if (JSUnprotectedEventListener* listener = static_cast<JSUnprotectedEventListener*>(impl()->onCloseListener()))
        if (JSObject* listenerObj = listener->listenerObj())
            return listenerObj;
    return jsNull();
}

void JSMessagePort::mark()
{
    DOMObject::mark();
 
    if (JSUnprotectedEventListener* listener = static_cast<JSUnprotectedEventListener*>(m_impl->onMessageListener()))
        listener->mark();

    if (JSUnprotectedEventListener* listener = static_cast<JSUnprotectedEventListener*>(m_impl->onCloseListener()))
        listener->mark();

    if (MessagePort* entangledPort = m_impl->entangledPort()) {
        DOMObject* wrapper = getCachedDOMObjectWrapper(entangledPort);
        if (wrapper && !wrapper->marked())
            wrapper->mark();
    }

    typedef MessagePort::EventListenersMap EventListenersMap;
    typedef MessagePort::ListenerVector ListenerVector;
    EventListenersMap& eventListeners = m_impl->eventListeners();
    for (EventListenersMap::iterator mapIter = eventListeners.begin(); mapIter != eventListeners.end(); ++mapIter) {
        for (ListenerVector::iterator vecIter = mapIter->second.begin(); vecIter != mapIter->second.end(); ++vecIter) {
            JSUnprotectedEventListener* listener = static_cast<JSUnprotectedEventListener*>(vecIter->get());
            listener->mark();
        }
    }
}

} // namespace WebCore
