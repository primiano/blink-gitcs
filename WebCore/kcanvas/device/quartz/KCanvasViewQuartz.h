/*
 * Copyright (C) 2005 Apple Computer, Inc.  All rights reserved.
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

#import "KCanvasView.h"

#ifdef __OBJC__
@class DrawView;
#else
class DrawView;
#endif

namespace khtml {
    class RenderKCanvasWrapper;
}

class KCanvasViewQuartz : public KCanvasView {
public:
    KCanvasViewQuartz();
    ~KCanvasViewQuartz();
    
    DrawView *view();
    void setView(DrawView *view);

    khtml::RenderKCanvasWrapper *renderObject();
    void setRenderObject(khtml::RenderKCanvasWrapper *renderObject);

    virtual void invalidateCanvasRect(const QRect &rect) const;

protected:
    virtual KCanvasMatrix viewToCanvasMatrix() const;
    virtual int viewHeight() const;
    virtual int viewWidth() const;
    
private:
    DrawView *m_view;
    khtml::RenderKCanvasWrapper *m_renderObject;
    
    virtual void canvasSizeChanged(int width, int height);
};
