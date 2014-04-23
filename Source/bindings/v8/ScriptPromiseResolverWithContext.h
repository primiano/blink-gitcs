// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ScriptPromiseResolverWithContext_h
#define ScriptPromiseResolverWithContext_h

#include "bindings/v8/NewScriptState.h"
#include "bindings/v8/ScopedPersistent.h"
#include "bindings/v8/ScriptPromise.h"
#include "bindings/v8/ScriptPromiseResolver.h"
#include "bindings/v8/V8Binding.h"
#include "core/dom/ActiveDOMObject.h"
#include "core/dom/ExecutionContext.h"
#include "platform/Timer.h"
#include "wtf/RefCounted.h"
#include "wtf/Vector.h"
#include <v8.h>

namespace WebCore {

// This class wraps ScriptPromiseResolver and provides the following
// functionalities in addition to ScriptPromiseResolver's.
//  - A ScriptPromiseResolverWithContext retains a ScriptState. A caller
//    can call resolve or reject from outside of a V8 context.
//  - This class is an ActiveDOMObject and keeps track of the associated
//    ExecutionContext state. When the ExecutionContext is suspended,
//    resolve or reject will be delayed. When it is stopped, resolve or reject
//    will be ignored.
class ScriptPromiseResolverWithContext FINAL : public ActiveDOMObject, public RefCounted<ScriptPromiseResolverWithContext> {
    WTF_MAKE_NONCOPYABLE(ScriptPromiseResolverWithContext);

public:
    static PassRefPtr<ScriptPromiseResolverWithContext> create(NewScriptState* scriptState)
    {
        RefPtr<ScriptPromiseResolverWithContext> resolver = adoptRef(new ScriptPromiseResolverWithContext(scriptState));
        resolver->suspendIfNeeded();
        return resolver.release();
    }

    // Anything that can be passed to toV8Value can be passed to this function.
    template <typename T>
    void resolve(T value)
    {
        if (m_state != Pending || !executionContext() || executionContext()->activeDOMObjectsAreStopped())
            return;
        m_state = Resolving;
        NewScriptState::Scope scope(m_scriptState.get());
        m_value.set(m_scriptState->isolate(), toV8Value(value));
        if (!executionContext()->activeDOMObjectsAreSuspended())
            resolveOrRejectImmediately(&m_timer);
    }

    // Anything that can be passed to toV8Value can be passed to this function.
    template <typename T>
    void reject(T value)
    {
        if (m_state != Pending || !executionContext() || executionContext()->activeDOMObjectsAreStopped())
            return;
        m_state = Rejecting;
        NewScriptState::Scope scope(m_scriptState.get());
        m_value.set(m_scriptState->isolate(), toV8Value(value));
        if (!executionContext()->activeDOMObjectsAreSuspended())
            resolveOrRejectImmediately(&m_timer);
    }

    NewScriptState* scriptState() { return m_scriptState.get(); }

    // Note that an empty ScriptPromise will be returned after resolve or
    // reject is called.
    ScriptPromise promise()
    {
        return m_resolver ? m_resolver->promise() : ScriptPromise();
    }

    NewScriptState* scriptState() const { return m_scriptState.get(); }

    // ActiveDOMObject implementation.
    virtual void suspend() OVERRIDE;
    virtual void resume() OVERRIDE;
    virtual void stop() OVERRIDE;

    // Used by ToV8Value<ScriptPromiseResolverWithContext, NewScriptState*>.
    static v8::Handle<v8::Object> getCreationContext(NewScriptState* scriptState)
    {
        return scriptState->context()->Global();
    }

private:
    enum ResolutionState {
        Pending,
        Resolving,
        Rejecting,
        ResolvedOrRejected,
    };

    explicit ScriptPromiseResolverWithContext(NewScriptState*);

    template<typename T>
    v8::Handle<v8::Value> toV8Value(const T& value)
    {
        return ToV8Value<ScriptPromiseResolverWithContext, NewScriptState*>::toV8Value(value, m_scriptState.get(), m_scriptState->isolate());
    }

    void resolveOrRejectImmediately(Timer<ScriptPromiseResolverWithContext>*);
    void clear();

    ResolutionState m_state;
    const RefPtr<NewScriptState> m_scriptState;
    Timer<ScriptPromiseResolverWithContext> m_timer;
    RefPtr<ScriptPromiseResolver> m_resolver;
    ScopedPersistent<v8::Value> m_value;
};

} // namespace WebCore

#endif // #ifndef ScriptPromiseResolverWithContext_h
