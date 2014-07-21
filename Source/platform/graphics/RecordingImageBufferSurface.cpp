// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"

#include "platform/graphics/RecordingImageBufferSurface.h"

#include "platform/graphics/GraphicsContext.h"
#include "platform/graphics/ImageBuffer.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkPictureRecorder.h"
#include "wtf/PassOwnPtr.h"
#include "wtf/PassRefPtr.h"

namespace WebCore {

RecordingImageBufferSurface::RecordingImageBufferSurface(const IntSize& size, OpacityMode opacityMode)
    : ImageBufferSurface(size, opacityMode)
    , m_graphicsContext(0)
    , m_frameWasCleared(true)
{
    initializeCurrentFrame();
}

RecordingImageBufferSurface::~RecordingImageBufferSurface()
{ }

void RecordingImageBufferSurface::initializeCurrentFrame()
{
    static SkRTreeFactory rTreeFactory;
    m_currentFrame = adoptPtr(new SkPictureRecorder);
    m_currentFrame->beginRecording(size().width(), size().height(), &rTreeFactory);
    if (m_graphicsContext) {
        m_graphicsContext->resetCanvas(m_currentFrame->getRecordingCanvas());
        m_graphicsContext->setTrackOpaqueRegion(true);
    }
}

void RecordingImageBufferSurface::setImageBuffer(ImageBuffer* imageBuffer)
{
    m_graphicsContext = imageBuffer ? imageBuffer->context() : 0;
    if (m_currentFrame && m_graphicsContext) {
        m_graphicsContext->setTrackOpaqueRegion(true);
        m_graphicsContext->resetCanvas(m_currentFrame->getRecordingCanvas());
    }
}

void RecordingImageBufferSurface::willReadback()
{
    fallBackToRasterCanvas();
}

void RecordingImageBufferSurface::fallBackToRasterCanvas()
{
    if (m_rasterCanvas) {
        ASSERT(!m_currentFrame);
        return;
    }

    m_rasterCanvas = adoptPtr(SkCanvas::NewRasterN32(size().width(), size().height()));

    if (m_previousFrame) {
        m_previousFrame->draw(m_rasterCanvas.get());
        m_previousFrame.clear();
    }
    if (m_currentFrame) {
        RefPtr<SkPicture> currentPicture = adoptRef(m_currentFrame->endRecording());
        currentPicture->draw(m_rasterCanvas.get());
        m_currentFrame.clear();
    }

    if (m_graphicsContext) {
        m_graphicsContext->setTrackOpaqueRegion(false);
        m_graphicsContext->resetCanvas(m_rasterCanvas.get());
    }
}

SkCanvas* RecordingImageBufferSurface::canvas() const
{
    if (m_rasterCanvas)
        return m_rasterCanvas.get();

    ASSERT(m_currentFrame->getRecordingCanvas());
    return m_currentFrame->getRecordingCanvas();
}

PassRefPtr<SkPicture> RecordingImageBufferSurface::getPicture()
{
    if (handleOpaqueFrame())
        return m_previousFrame;

    if (!m_rasterCanvas)
        fallBackToRasterCanvas();
    return nullptr;
}

void RecordingImageBufferSurface::didClearCanvas()
{
    m_frameWasCleared = true;
}

bool RecordingImageBufferSurface::handleOpaqueFrame()
{
    if (!m_currentFrame)
        return false;
    IntRect canvasRect(IntPoint(0, 0), size());
    if (!m_frameWasCleared && !m_graphicsContext->opaqueRegion().asRect().contains(canvasRect))
        return false;

    SkCanvas* oldCanvas = m_currentFrame->getRecordingCanvas(); // Could be raster or picture

    // FIXME(crbug.com/392614): handle transferring complex state from the current picture to the new one.
    if (oldCanvas->getSaveCount())
        return false;
    if (!oldCanvas->isClipRect())
        return false;

    SkMatrix ctm = oldCanvas->getTotalMatrix();
    SkRect clip;
    oldCanvas->getClipBounds(&clip);

    m_previousFrame = adoptRef(m_currentFrame->endRecording());
    initializeCurrentFrame();

    SkCanvas* newCanvas = m_currentFrame->getRecordingCanvas();
    newCanvas->concat(ctm);
    newCanvas->clipRect(clip);

    m_frameWasCleared = false;
    return true;
}

} // namespace WebCore