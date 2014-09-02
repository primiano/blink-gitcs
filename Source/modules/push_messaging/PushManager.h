// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PushManager_h
#define PushManager_h

#include "bindings/core/v8/ScriptWrappable.h"
#include "platform/heap/Handle.h"
#include "wtf/text/WTFString.h"

namespace blink {

class ExecutionContext;
class ScriptPromise;
class ScriptState;

class PushManager FINAL : public GarbageCollected<PushManager>, public ScriptWrappable {
    DEFINE_WRAPPERTYPEINFO();
public:
    static PushManager* create()
    {
        return new PushManager();
    }

    ScriptPromise registerPushMessaging(ScriptState*, const String& senderId);

    void trace(Visitor*) { }

private:
    PushManager();
};

} // namespace blink

#endif // PushManager_h
