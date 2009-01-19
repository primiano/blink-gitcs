/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
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

#include "config.h"
#include "FrameWin.h"

#include <windows.h>

#include "FrameView.h"
#include "GraphicsContext.h"
#include "Settings.h"

#include <CoreGraphics/CoreGraphics.h>

using std::min;

namespace WebCore {

static void drawRectIntoContext(IntRect rect, FrameView* view, GraphicsContext* gc)
{
    IntSize offset = view->scrollOffset();
    rect.move(-offset.width(), -offset.height());
    rect = view->convertToContainingWindow(rect);

    gc->concatCTM(TransformationMatrix().translate(-rect.x(), -rect.y()));

    view->paint(gc, rect);
}

static HBITMAP imageFromRect(const Frame* frame, IntRect& ir)
{
    void* bits;
    HDC hdc = CreateCompatibleDC(0);
    int w = ir.width();
    int h = ir.height();
    BITMAPINFO bmp = { { sizeof(BITMAPINFOHEADER), w, h, 1, 32 } };

    HBITMAP hbmp = CreateDIBSection(0, &bmp, DIB_RGB_COLORS, static_cast<void**>(&bits), 0, 0);
    HBITMAP hbmpOld = static_cast<HBITMAP>(SelectObject(hdc, hbmp));
    CGColorSpaceRef deviceRGB = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = CGBitmapContextCreate(static_cast<void*>(bits), w, h,
        8, w * sizeof(RGBQUAD), deviceRGB, kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst);
    CGColorSpaceRelease(deviceRGB);
    CGContextSaveGState(context);

    GraphicsContext gc(context);

    drawRectIntoContext(ir, frame->view(), &gc);

    CGContextRelease(context);
    SelectObject(hdc, hbmpOld);
    DeleteDC(hdc);

    return hbmp;
}

HBITMAP imageFromSelection(Frame* frame, bool forceBlackText)
{
    frame->document()->updateLayout();

    frame->view()->setPaintRestriction(forceBlackText ? PaintRestrictionSelectionOnlyBlackText : PaintRestrictionSelectionOnly);
    FloatRect fr = frame->selectionBounds();
    IntRect ir(static_cast<int>(fr.x()), static_cast<int>(fr.y()),
               static_cast<int>(fr.width()), static_cast<int>(fr.height()));
    HBITMAP image = imageFromRect(frame, ir);
    frame->view()->setPaintRestriction(PaintRestrictionNone);
    return image;
}

HBITMAP Frame::nodeImage(Node* node) const
{
    RenderObject* renderer = node->renderer();
    if (!renderer)
        return 0;

    IntRect topLevelRect;
    IntRect paintingRect = renderer->paintingRootRect(topLevelRect);

    document()->updateLayout();

    m_view->setNodeToDraw(node); // invoke special sub-tree drawing mode
    HBITMAP result = imageFromRect(this, paintingRect);
    m_view->setNodeToDraw(0);

    return result;
}

} // namespace WebCore
