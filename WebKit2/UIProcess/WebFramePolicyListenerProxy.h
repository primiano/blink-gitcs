/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebFramePolicyListenerProxy_h
#define WebFramePolicyListenerProxy_h

#include <WebCore/FrameLoaderTypes.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebKit {

class WebFrameProxy;

class WebFramePolicyListenerProxy : public RefCounted<WebFramePolicyListenerProxy> {
public:
    static PassRefPtr<WebFramePolicyListenerProxy> create(WebFrameProxy* frame, uint64_t listenerID)
    {
        return adoptRef(new WebFramePolicyListenerProxy(frame, listenerID));
    }
    ~WebFramePolicyListenerProxy();

    void use();
    void download();
    void ignore();

    void invalidate();

    uint64_t listenerID() const { return m_listenerID; }

private:
    WebFramePolicyListenerProxy(WebFrameProxy*, uint64_t listenerID);

    void receivedPolicyDecision(WebCore::PolicyAction);

    RefPtr<WebFrameProxy> m_frame;
    uint64_t m_listenerID;
};

} // namespace WebKit

#endif // WebFramePolicyListenerProxy_h
