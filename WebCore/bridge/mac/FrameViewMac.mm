/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#import "config.h"
#import "FrameView.h"

#import "DocumentImpl.h"
#import "KWQExceptions.h"
#import "KWQWindowWidget.h"
#import "MacFrame.h"
#import "WebCoreFrameBridge.h"
#import "render_object.h"

namespace WebCore {

Widget* FrameView::topLevelWidget() const 
{
    return Mac(frame())->topLevelWidget();
}

void FrameView::updateBorder()
{
    KWQ_BLOCK_EXCEPTIONS;
    [Mac(m_frame.get())->bridge() setHasBorder:hasBorder()];
    KWQ_UNBLOCK_EXCEPTIONS;
}

void FrameView::updateDashboardRegions()
{
    DocumentImpl* document = m_frame->document();
    if (document->hasDashboardRegions()) {
        QValueList<DashboardRegionValue> newRegions = document->renderer()->computeDashboardRegions();
        QValueList<DashboardRegionValue> currentRegions = document->dashboardRegions();
        document->setDashboardRegions(newRegions);
        Mac(m_frame.get())->dashboardRegionsChanged();
    }
}

IntPoint FrameView::viewportToGlobal(const IntPoint &p) const
{
    return static_cast<KWQWindowWidget*>(topLevelWidget())->viewportToGlobal(p);
}

}
