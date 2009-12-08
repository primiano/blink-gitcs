/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(NOTIFICATIONS)

#include "NotImplemented.h"
#include "Notification.h"
#include "NotificationCenter.h"
#include "V8Binding.h"
#include "V8CustomBinding.h"
#include "V8CustomEventListener.h"
#include "V8CustomVoidCallback.h"
#include "V8Proxy.h"
#include "V8Utilities.h"
#include "WorkerContext.h"
#include "WorkerContextExecutionProxy.h"

namespace WebCore {

CALLBACK_FUNC_DECL(NotificationAddEventListener)
{
    INC_STATS("DOM.Notification.addEventListener()");
    Notification* notification = V8DOMWrapper::convertToNativeObject<Notification>(V8ClassIndex::NOTIFICATION, args.Holder());

    RefPtr<EventListener> listener = V8DOMWrapper::getEventListener(notification, args[1], false, ListenerFindOrCreate);
    if (listener) {
        String type = toWebCoreString(args[0]);
        bool useCapture = args[2]->BooleanValue();
        notification->addEventListener(type, listener, useCapture);
        createHiddenDependency(args.Holder(), args[1], V8Custom::kNotificationRequestCacheIndex);
    }

    return v8::Undefined();
}

CALLBACK_FUNC_DECL(NotificationRemoveEventListener)
{
    INC_STATS("DOM.Notification.removeEventListener()");
    Notification* notification = V8DOMWrapper::convertToNativeObject<Notification>(V8ClassIndex::NOTIFICATION, args.Holder());

    RefPtr<EventListener> listener = V8DOMWrapper::getEventListener(notification, args[1], false, ListenerFindOnly);
    if (listener) {
        String type = toWebCoreString(args[0]);
        bool useCapture = args[2]->BooleanValue();
        notification->removeEventListener(type, listener.get(), useCapture);
        removeHiddenDependency(args.Holder(), args[1], V8Custom::kNotificationRequestCacheIndex);
    }

    return v8::Undefined();
}

CALLBACK_FUNC_DECL(NotificationCenterCreateHTMLNotification)
{
    INC_STATS(L"DOM.NotificationCenter.CreateHTMLNotification()");
    NotificationCenter* notificationCenter = V8DOMWrapper::convertToNativeObject<NotificationCenter>(V8ClassIndex::NOTIFICATIONCENTER, args.Holder());

    ExceptionCode ec = 0;
    String url = toWebCoreString(args[0]);
    RefPtr<Notification> notification = notificationCenter->createHTMLNotification(url, ec);

    if (ec)
        return throwError(ec);

    if (notificationCenter->context()->isWorkerContext())
        return WorkerContextExecutionProxy::convertToV8Object(V8ClassIndex::NOTIFICATION, notification.get());

    return V8DOMWrapper::convertToV8Object(V8ClassIndex::NOTIFICATION, notification.get());
}

CALLBACK_FUNC_DECL(NotificationCenterCreateNotification)
{
    INC_STATS(L"DOM.NotificationCenter.CreateNotification()");
    NotificationCenter* notificationCenter = V8DOMWrapper::convertToNativeObject<NotificationCenter>(V8ClassIndex::NOTIFICATIONCENTER, args.Holder());

    ExceptionCode ec = 0;
    RefPtr<Notification> notification = notificationCenter->createNotification(toWebCoreString(args[0]), toWebCoreString(args[1]), toWebCoreString(args[2]), ec);

    if (ec)
        return throwError(ec);

    if (notificationCenter->context()->isWorkerContext())
        return WorkerContextExecutionProxy::convertToV8Object(V8ClassIndex::NOTIFICATION, notification.get());

    return V8DOMWrapper::convertToV8Object(V8ClassIndex::NOTIFICATION, notification.get());
}

CALLBACK_FUNC_DECL(NotificationCenterRequestPermission)
{
    INC_STATS(L"DOM.NotificationCenter.RequestPermission()");
    NotificationCenter* notificationCenter = V8DOMWrapper::convertToNativeObject<NotificationCenter>(V8ClassIndex::NOTIFICATIONCENTER, args.Holder());
    ScriptExecutionContext* context = notificationCenter->context();

    // Requesting permission is only valid from a page context.
    if (context->isWorkerContext())
        return throwError(NOT_SUPPORTED_ERR);

    RefPtr<V8CustomVoidCallback> callback;
    if (args.Length() > 0) {
        if (!args[0]->IsObject())
            return throwError("Callback must be of valid type.", V8Proxy::TypeError);
 
        callback = V8CustomVoidCallback::create(args[0], V8Proxy::retrieveFrameForCurrentContext());
    }

    notificationCenter->requestPermission(callback.release());
    return v8::Undefined();
}


}  // namespace WebCore

#endif  // ENABLE(NOTIFICATIONS)
