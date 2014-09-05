// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebServiceWorkerRegistrationProxy_h
#define WebServiceWorkerRegistrationProxy_h

#if INSIDE_BLINK
#include "platform/heap/Handle.h"
#endif
#include "public/platform/WebCommon.h"

namespace blink {

class ServiceWorkerRegistration;
class WebServiceWorker;

// A proxy interface, passed via WebServiceWorkerRegistration.setProxy() from
// blink to the embedder, to talk to the ServiceWorkerRegistration object from
// embedder.
class WebServiceWorkerRegistrationProxy {
public:
    WebServiceWorkerRegistrationProxy() : m_private(0) { }
    virtual ~WebServiceWorkerRegistrationProxy() { }

    // Notifies that the registration entered the installation process.
    // The installing worker should be accessible via
    // WebServiceWorkerRegistration.installing.
    virtual void dispatchUpdateFoundEvent() = 0;

    virtual void setInstalling(WebServiceWorker*) = 0;
    virtual void setWaiting(WebServiceWorker*) = 0;
    virtual void setActive(WebServiceWorker*) = 0;

#if INSIDE_BLINK
    BLINK_PLATFORM_EXPORT WebServiceWorkerRegistrationProxy(ServiceWorkerRegistration*);
    BLINK_PLATFORM_EXPORT operator ServiceWorkerRegistration*() const;
#endif

protected:
#if INSIDE_BLINK
    // This is a back pointer to |this| object.
    // The ServiceWorkerRegistration inherits from this WebServiceWorkerRegistrationProxy.
    GC_PLUGIN_IGNORE("crbug.com/410257")
#endif
    ServiceWorkerRegistration* m_private;
};

} // namespace blink

#endif // WebServiceWorkerRegistrationProxy_h
