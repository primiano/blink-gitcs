/*
 * Copyright (C) 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2009 Apple, Inc. All rights reserved.
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

#include "config.h"

#if ENABLE(SVG)
#include "JSSVGElementInstance.h"

#include "JSDOMWindow.h"
#include "JSEventListener.h"
#include "JSSVGElement.h"
#include "SVGElementInstance.h"

using namespace JSC;

namespace WebCore {

void JSSVGElementInstance::markChildren(MarkStack& markStack)
{
    Base::markChildren(markStack);

    // Mark the wrapper for our corresponding element, so it can mark its event handlers.
    JSNode* correspondingWrapper = getCachedDOMNodeWrapper(impl()->correspondingElement()->document(), impl()->correspondingElement());
    if (correspondingWrapper)
        markStack.append(correspondingWrapper);
}

JSValue JSSVGElementInstance::addEventListener(ExecState* exec, const ArgList& args)
{
    JSDOMGlobalObject* globalObject = toJSDOMGlobalObject(impl()->scriptExecutionContext());
    if (!globalObject)
        return jsUndefined();

    JSValue listener = args.at(1);
    if (!listener.isObject())
        return jsUndefined();

    impl()->addEventListener(args.at(0).toString(exec), JSEventListener::create(asObject(listener), globalObject, false), args.at(2).toBoolean(exec));
    return jsUndefined();
}

JSValue JSSVGElementInstance::removeEventListener(ExecState* exec, const ArgList& args)
{
    JSDOMGlobalObject* globalObject = toJSDOMGlobalObject(impl()->scriptExecutionContext());
    if (!globalObject)
        return jsUndefined();

    JSValue listener = args.at(1);
    if (!listener.isObject())
        return jsUndefined();

    impl()->removeEventListener(args.at(0).toString(exec), JSEventListener::create(asObject(listener), globalObject, false).get(), args.at(2).toBoolean(exec));
    return jsUndefined();
}

void JSSVGElementInstance::pushEventHandlerScope(ExecState*, ScopeChain&) const
{
}

JSC::JSValue toJS(JSC::ExecState* exec, JSDOMGlobalObject* globalObject, SVGElementInstance* object)
{
    JSValue result = getDOMObjectWrapper<JSSVGElementInstance>(exec, globalObject, object);

    // Ensure that our corresponding element has a JavaScript wrapper to keep its event handlers alive.
    if (object)
        toJS(exec, object->correspondingElement());

    return result;
}

} // namespace WebCore

#endif // ENABLE(SVG)
