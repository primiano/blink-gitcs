// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"
#include "core/testing/GarbageCollectedScriptWrappable.h"

namespace WebCore {

GarbageCollectedScriptWrappable::GarbageCollectedScriptWrappable(const String& string)
    : m_string(string)
{
    ScriptWrappable::init(this);
}

GarbageCollectedScriptWrappable::~GarbageCollectedScriptWrappable()
{
}

} // namespace WebCore

