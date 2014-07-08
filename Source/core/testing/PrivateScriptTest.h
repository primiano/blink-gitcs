// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PrivateScriptTest_h
#define PrivateScriptTest_h

#include "platform/heap/Handle.h"
#include "wtf/RefCounted.h"

namespace WebCore {

class LocalFrame;

class PrivateScriptTest : public GarbageCollected<PrivateScriptTest> {
public:
    static PrivateScriptTest* create(LocalFrame* frame)
    {
        return new PrivateScriptTest(frame);
    }

    void trace(Visitor*) { }

private:
    PrivateScriptTest(LocalFrame*);
};

} // namespace WebCore

#endif
