// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"
#include "core/inspector/PromiseTracker.h"

#include "bindings/core/v8/ScopedPersistent.h"
#include "bindings/core/v8/ScriptCallStackFactory.h"
#include "bindings/core/v8/ScriptState.h"
#include "wtf/PassOwnPtr.h"
#include "wtf/WeakPtr.h"

using blink::TypeBuilder::Array;
using blink::TypeBuilder::Console::CallFrame;
using blink::TypeBuilder::Debugger::PromiseDetails;

namespace blink {

class PromiseTracker::PromiseData : public RefCounted<PromiseData> {
public:
    PromiseData(v8::Isolate* isolate, int promiseHash, int promiseId, v8::Handle<v8::Object> promise)
        : m_promiseHash(promiseHash)
        , m_promiseId(promiseId)
        , m_promise(isolate, promise)
        , m_parentPromiseId(0)
        , m_status(0)
        , m_weakPtrFactory(this)
    {
    }

    int promiseHash() const { return m_promiseHash; }
    ScopedPersistent<v8::Object>& promise() { return m_promise; }

private:
    friend class PromiseTracker;

    int m_promiseHash;
    int m_promiseId;
    ScopedPersistent<v8::Object> m_promise;
    int m_parentPromiseId;
    int m_status;
    WeakPtrFactory<PromiseData> m_weakPtrFactory;
    RefPtr<ScriptCallStack> m_callStack;
    ScopedPersistent<v8::Object> m_parentPromise;
};

static int indexOf(PromiseTracker::PromiseDataVector* vector, const ScopedPersistent<v8::Object>& promise)
{
    for (size_t index = 0; index < vector->size(); ++index) {
        if (vector->at(index)->promise() == promise)
            return index;
    }
    return -1;
}

namespace {

class PromiseDataWrapper {
public:
    PromiseDataWrapper(WeakPtr<PromiseTracker::PromiseData> data, PromiseTracker::PromiseDataMap* map)
        : m_data(data)
        , m_promiseDataMap(map)
    {
    }

    static void didRemovePromise(const v8::WeakCallbackData<v8::Object, PromiseDataWrapper>& data)
    {
        OwnPtr<PromiseDataWrapper> wrapper = adoptPtr(data.GetParameter());
        WeakPtr<PromiseTracker::PromiseData> promiseData = wrapper->m_data;
        if (!promiseData)
            return;
        PromiseTracker::PromiseDataMap* map = wrapper->m_promiseDataMap;
        int promiseHash = promiseData->promiseHash();
        PromiseTracker::PromiseDataVector* vector = &map->find(promiseHash)->value;
        int index = indexOf(vector, promiseData->promise());
        ASSERT(index >= 0);
        vector->remove(index);
        if (vector->size() == 0)
            map->remove(promiseHash);
    }

private:
    WeakPtr<PromiseTracker::PromiseData> m_data;
    PromiseTracker::PromiseDataMap* m_promiseDataMap;
};

}

PromiseTracker::PromiseTracker()
    : m_isEnabled(false)
    , m_circularSequentialId(0)
{
}

PromiseTracker::~PromiseTracker()
{
}

void PromiseTracker::setEnabled(bool enabled)
{
    m_isEnabled = enabled;
    if (!enabled)
        clear();
}

void PromiseTracker::clear()
{
    m_promiseDataMap.clear();
}

int PromiseTracker::circularSequentialId()
{
    ++m_circularSequentialId;
    if (m_circularSequentialId <= 0)
        m_circularSequentialId = 1;
    return m_circularSequentialId;
}

PassRefPtr<PromiseTracker::PromiseData> PromiseTracker::createPromiseDataIfNeeded(v8::Isolate* isolate, v8::Handle<v8::Object> promise)
{
    int promiseHash = promise->GetIdentityHash();
    PromiseDataVector* vector;
    PromiseDataMap::iterator it = m_promiseDataMap.find(promiseHash);
    if (it != m_promiseDataMap.end())
        vector = &it->value;
    else
        vector = &m_promiseDataMap.add(promiseHash, PromiseDataVector()).storedValue->value;

    RefPtr<PromiseData> data;
    int index = indexOf(vector, ScopedPersistent<v8::Object>(isolate, promise));
    if (index == -1) {
        data = adoptRef(new PromiseData(isolate, promiseHash, circularSequentialId(), promise));
        OwnPtr<PromiseDataWrapper> wrapper = adoptPtr(new PromiseDataWrapper(data->m_weakPtrFactory.createWeakPtr(), &m_promiseDataMap));
        data->m_promise.setWeak(wrapper.leakPtr(), &PromiseDataWrapper::didRemovePromise);
        vector->append(data);
    } else {
        data = vector->at(index);
    }

    return data.release();
}

void PromiseTracker::didReceiveV8PromiseEvent(ScriptState* scriptState, v8::Handle<v8::Object> promise, v8::Handle<v8::Value> parentPromise, int status)
{
    ASSERT(isEnabled());

    v8::Isolate* isolate = scriptState->isolate();
    RefPtr<PromiseData> data = createPromiseDataIfNeeded(isolate, promise);
    if (!parentPromise.IsEmpty() && parentPromise->IsObject()) {
        v8::Handle<v8::Object> handle = parentPromise->ToObject();
        RefPtr<PromiseData> parentData = createPromiseDataIfNeeded(isolate, handle);
        data->m_parentPromiseId = parentData->m_promiseId;
        data->m_parentPromise.set(isolate, handle);
    } else {
        data->m_status = status;
        if (!status && !data->m_callStack) {
            v8::Handle<v8::StackTrace> stackTrace(v8::StackTrace::CurrentStackTrace(isolate, 1));
            RefPtrWillBeRawPtr<ScriptCallStack> stack = createScriptCallStack(stackTrace, 1, isolate);
            if (stack->size())
                data->m_callStack = stack;
        }
    }
}

PassRefPtr<Array<PromiseDetails> > PromiseTracker::promises()
{
    ASSERT(isEnabled());

    RefPtr<Array<PromiseDetails> > result = Array<PromiseDetails>::create();
    for (PromiseDataMap::iterator it = m_promiseDataMap.begin(); it != m_promiseDataMap.end(); ++it) {
        PromiseDataVector* vector = &it->value;
        for (size_t index = 0; index < vector->size(); ++index) {
            RefPtr<PromiseData> data = vector->at(index);
            PromiseDetails::Status::Enum status;
            if (!data->m_status)
                status = PromiseDetails::Status::Pending;
            else if (data->m_status == 1)
                status = PromiseDetails::Status::Resolved;
            else
                status = PromiseDetails::Status::Rejected;
            RefPtr<PromiseDetails> promiseDetails = PromiseDetails::create()
                .setId(data->m_promiseId)
                .setStatus(status);
            if (data->m_parentPromiseId)
                promiseDetails->setParentId(data->m_parentPromiseId);
            if (data->m_callStack)
                promiseDetails->setCallFrame(data->m_callStack->at(0).buildInspectorObject());
            result->addItem(promiseDetails);
        }
    }

    return result.release();
}

} // namespace blink
