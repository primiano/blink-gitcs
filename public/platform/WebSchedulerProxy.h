// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebSchedulerProxy_h
#define WebSchedulerProxy_h

#include "WebThread.h"

namespace WebCore {
class Scheduler;
}

namespace blink {

class WebSchedulerProxy {
public:
    BLINK_PLATFORM_EXPORT ~WebSchedulerProxy();

    BLINK_PLATFORM_EXPORT static WebSchedulerProxy create();

    // Schedule an input processing task to be run on the blink main thread.
    // Takes ownership of |Task|. Can be called from any thread.
    BLINK_PLATFORM_EXPORT void postInputTask(WebThread::Task*);

    // Schedule a compositor task to run on the blink main thread. Takes
    // ownership of |Task|. Can be called from any thread.
    BLINK_PLATFORM_EXPORT void postCompositorTask(WebThread::Task*);

private:
    WebSchedulerProxy();

    WebCore::Scheduler* m_scheduler;
};

} // namespace blink

#endif // WebSchedulerProxy_h
