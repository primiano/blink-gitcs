// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ScreenOrientationController_h
#define ScreenOrientationController_h

#include "core/dom/DocumentSupplementable.h"
#include "public/platform/WebScreenOrientationType.h"

namespace WebCore {

class ScreenOrientationController FINAL : public NoBaseWillBeGarbageCollected<ScreenOrientationController>, public DocumentSupplement {
    WILL_BE_USING_GARBAGE_COLLECTED_MIXIN(ScreenOrientationController);
public:
#if !ENABLE(OILPAN)
    virtual ~ScreenOrientationController();
#endif

    void didChangeScreenOrientation(blink::WebScreenOrientationType);

    blink::WebScreenOrientationType orientation() const { return m_orientation; }

    // DocumentSupplement API.
    static ScreenOrientationController& from(Document&);
    static const char* supplementName();

private:
    explicit ScreenOrientationController(Document&);

    void dispatchOrientationChangeEvent();

    Document& m_document;
    blink::WebScreenOrientationType m_orientation;
};

} // namespace WebCore

#endif // ScreenOrientationController_h
