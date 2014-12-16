// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RemoteFrame_h
#define RemoteFrame_h

#include "core/dom/RemoteSecurityContext.h"
#include "core/frame/Frame.h"

namespace blink {

class Event;
class RemoteDOMWindow;
class RemoteFrameClient;
class RemoteFrameView;

class RemoteFrame: public Frame {
public:
    static PassRefPtrWillBeRawPtr<RemoteFrame> create(RemoteFrameClient*, FrameHost*, FrameOwner*);

    virtual ~RemoteFrame();

    // Frame overrides:
    void trace(Visitor*) override;
    virtual bool isRemoteFrame() const override { return true; }
    virtual DOMWindow* domWindow() const override;
    virtual void navigate(Document& originDocument, const KURL&, bool lockBackForwardList) override;
    virtual void reload(ReloadPolicy, ClientRedirectPolicy) override;
    virtual void detach() override;
    virtual RemoteSecurityContext* securityContext() const override;
    bool checkLoadComplete() override;

    // FIXME: Remove this method once we have input routing in the browser
    // process. See http://crbug.com/339659.
    void forwardInputEvent(Event*);

    void setView(PassRefPtrWillBeRawPtr<RemoteFrameView>);
    void createView();
    void setIsLoading(bool isLoading) { m_isLoading = isLoading; }

    RemoteFrameView* view() const;


private:
    RemoteFrame(RemoteFrameClient*, FrameHost*, FrameOwner*);

    RemoteFrameClient* remoteFrameClient() const;

    RefPtrWillBeMember<RemoteFrameView> m_view;
    RefPtr<RemoteSecurityContext> m_securityContext;
    RefPtrWillBeMember<RemoteDOMWindow> m_domWindow;

    bool m_isLoading;
};

inline RemoteFrameView* RemoteFrame::view() const
{
    return m_view.get();
}

DEFINE_TYPE_CASTS(RemoteFrame, Frame, remoteFrame, remoteFrame->isRemoteFrame(), remoteFrame.isRemoteFrame());

} // namespace blink

#endif // RemoteFrame_h
