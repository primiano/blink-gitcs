/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Google Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
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

#ifndef CustomElementScheduler_h
#define CustomElementScheduler_h

#include "core/dom/custom/CustomElementCallbackQueue.h"
#include "core/dom/custom/CustomElementMicrotaskQueue.h"
#include "wtf/HashMap.h"
#include "wtf/OwnPtr.h"
#include "wtf/PassOwnPtr.h"
#include "wtf/PassRefPtr.h"
#include "wtf/text/AtomicString.h"

namespace WebCore {

class CustomElementDescriptor;
class CustomElementLifecycleCallbacks;
class CustomElementMicrotaskImportStep;
class Element;
class HTMLImport;

class CustomElementScheduler {
public:
    static void scheduleAttributeChangedCallback(PassRefPtr<CustomElementLifecycleCallbacks>, PassRefPtr<Element>, const AtomicString& name, const AtomicString& oldValue, const AtomicString& newValue);
    static void scheduleAttachedCallback(PassRefPtr<CustomElementLifecycleCallbacks>, PassRefPtr<Element>);
    static void scheduleDetachedCallback(PassRefPtr<CustomElementLifecycleCallbacks>, PassRefPtr<Element>);
    static void scheduleResolutionStep(const CustomElementDescriptor&, PassRefPtr<Element>);
    static CustomElementMicrotaskImportStep* scheduleImport(HTMLImportChild*);

    static bool dispatchMicrotaskProcessingSteps() { return instance().dispatch(); }

private:
    CustomElementScheduler() { }

    static CustomElementScheduler& instance();

    CustomElementCallbackQueue* ensureCallbackQueue(PassRefPtr<Element>);
    CustomElementCallbackQueue* schedule(PassRefPtr<Element>);

    bool dispatch();
    void clearElementCallbackQueueMap();

    typedef HashMap<Element*, OwnPtr<CustomElementCallbackQueue> > ElementCallbackQueueMap;
    ElementCallbackQueueMap m_elementCallbackQueueMap;

    CustomElementMicrotaskQueue m_baseMicrotaskQueue;
};

}

#endif // CustomElementScheduler_h
