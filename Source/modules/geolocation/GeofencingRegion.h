// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GeofencingRegion_h
#define GeofencingRegion_h

#include "bindings/core/v8/ScriptWrappable.h"
#include "platform/heap/Handle.h"
#include "wtf/text/WTFString.h"

namespace WebCore {

class GeofencingRegion : public GarbageCollectedFinalized<GeofencingRegion>, public ScriptWrappable {
    WTF_MAKE_NONCOPYABLE(GeofencingRegion);
public:
    virtual ~GeofencingRegion() { }

    String id() const { return m_id; }

    virtual void trace(Visitor*) { }

protected:
    GeofencingRegion(const String& id) : m_id(id) { }

private:
    String m_id;
};

} // namespace WebCore

#endif
