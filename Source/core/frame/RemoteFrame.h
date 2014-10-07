// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RemoteFrame_h
#define RemoteFrame_h

#include "core/frame/Frame.h"

namespace blink {

class RemoteFrameClient;
class RemoteFrameView;

class RemoteFrame: public Frame {
public:
    static PassRefPtrWillBeRawPtr<RemoteFrame> create(RemoteFrameClient*, FrameHost*, FrameOwner*);
    virtual bool isRemoteFrame() const override { return true; }

    virtual ~RemoteFrame();

    virtual void navigate(Document& originDocument, const KURL&, bool lockBackForwardList) override;
    virtual void detach() override;

    void setView(PassRefPtr<RemoteFrameView>);
    void createView();

    RemoteFrameView* view() const;

private:
    RemoteFrame(RemoteFrameClient*, FrameHost*, FrameOwner*);

    RemoteFrameClient* remoteFrameClient() const;

    RefPtr<RemoteFrameView> m_view;
};

inline RemoteFrameView* RemoteFrame::view() const
{
    return m_view.get();
}

DEFINE_TYPE_CASTS(RemoteFrame, Frame, remoteFrame, remoteFrame->isRemoteFrame(), remoteFrame.isRemoteFrame());

} // namespace blink

#endif // RemoteFrame_h
