/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc.  All rights reserved.
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
#include "Image.h"
#include "BitmapImage.h"
#include "GraphicsContext.h"
#include <ApplicationServices/ApplicationServices.h>

#include <windows.h>
#include "PlatformString.h"

namespace WebCore {

bool BitmapImage::getHBITMAPOfSize(HBITMAP bmp, LPSIZE size)
{
    ASSERT(bmp);

    BITMAP bmpInfo;
    GetObject(bmp, sizeof(BITMAP), &bmpInfo);

    ASSERT(bmpInfo.bmBitsPixel == 32);
    int bufferSize = bmpInfo.bmWidthBytes * bmpInfo.bmHeight;
    
    CGColorSpaceRef deviceRGB = CGColorSpaceCreateDeviceRGB();
    CGContextRef cgContext = CGBitmapContextCreate(bmpInfo.bmBits, bmpInfo.bmWidth, bmpInfo.bmHeight,
        8, bmpInfo.bmWidthBytes, deviceRGB, kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst);
  
    GraphicsContext gc(cgContext);

    IntSize imageSize = BitmapImage::size();
    if (size)
        drawFrameMatchingSourceSize(&gc, FloatRect(0.0f, 0.0f, bmpInfo.bmWidth, bmpInfo.bmHeight), IntSize(*size), CompositeCopy);
    else
        draw(&gc, FloatRect(0.0f, 0.0f, bmpInfo.bmWidth, bmpInfo.bmHeight), FloatRect(0.0f, 0.0f, imageSize.width(), imageSize.height()), CompositeCopy);

    // Do cleanup
    CGContextRelease(cgContext);
    CGColorSpaceRelease(deviceRGB);

    return true;
}

void BitmapImage::drawFrameMatchingSourceSize(GraphicsContext* ctxt, const FloatRect& dstRect, const IntSize& srcSize, CompositeOperator compositeOp)
{
    size_t frames = frameCount();
    for (size_t i = 0; i < frames; ++i) {
        CGImageRef image = frameAtIndex(i);
        if (CGImageGetHeight(image) == static_cast<size_t>(srcSize.height()) && CGImageGetWidth(image) == static_cast<size_t>(srcSize.width())) {
            size_t currentFrame = m_currentFrame;
            m_currentFrame = i;
            draw(ctxt, dstRect, FloatRect(0.0f, 0.0f, srcSize.width(), srcSize.height()), compositeOp);
            m_currentFrame = currentFrame;
            return;
        }
    }

    // No image of the correct size was found, fallback to drawing the current frame
    IntSize imageSize = BitmapImage::size();
    draw(ctxt, dstRect, FloatRect(0.0f, 0.0f, imageSize.width(), imageSize.height()), compositeOp);
}

} // namespace WebCore
