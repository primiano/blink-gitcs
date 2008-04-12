/*
 * Copyright (C) 2008 Apple Computer, Inc.  All rights reserved.
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
#include "GeneratedImage.h"

#include "FloatRect.h"
#include "GraphicsContext.h"
#include "ImageBuffer.h"

using namespace std;

namespace WebCore {

void GeneratedImage::draw(GraphicsContext* context, const FloatRect& dstRect, const FloatRect& srcRect, CompositeOperator compositeOp)
{
    // Generated images should never be scaled, since it was the front end's responsibility to update our size to the correct
    // resolution.
    ASSERT(dstRect.size() == srcRect.size());
    ASSERT(srcRect.x() + srcRect.width() == m_size.width());
    ASSERT(srcRect.y() + srcRect.height() == m_size.height());
    
    context->save();
    context->setCompositeOperation(compositeOp);
    context->clip(dstRect);
    context->translate(dstRect.x() - srcRect.x(), dstRect.y() - srcRect.y());
    context->fillRect(FloatRect(FloatPoint(), m_size), *m_generator.get());
    context->restore();
}

void GeneratedImage::drawPattern(GraphicsContext* context, const FloatRect& srcRect, const AffineTransform& patternTransform,
                                 const FloatPoint& phase, CompositeOperator compositeOp, const FloatRect& destRect)
{
    // Create a BitmapImage and call drawPattern on it.
    auto_ptr<ImageBuffer> imageBuffer = ImageBuffer::create(m_size, false);
    ASSERT(imageBuffer.get());
    
    // Fill with the gradient.
    GraphicsContext* graphicsContext = imageBuffer->context();
    graphicsContext->fillRect(FloatRect(FloatPoint(), m_size), *m_generator.get());
    
    // Grab the final image from the image buffer.
    Image* bitmap = imageBuffer->image(true);
    
    // Now just call drawTiled on that image.
    bitmap->drawPattern(context, srcRect, patternTransform, phase, compositeOp, destRect);
}

}
