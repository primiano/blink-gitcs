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

#ifndef KCanvasResourcesQuartz_h
#define KCanvasResourcesQuartz_h

#import "KCanvasImage.h"
#import "KCanvasResources.h"
#import "KCanvasContainer.h"
#import <qmatrix.h>

typedef struct CGContext *CGContextRef;
typedef struct CGLayer *CGLayerRef;

namespace WebCore {

class KCanvasContainerQuartz : public KCanvasContainer {
public:
    KCanvasContainerQuartz(SVGStyledElementImpl *node) : KCanvasContainer(node) { }
    
    virtual bool canHaveChildren() const;
    
    virtual bool requiresLayer();
    virtual short lineHeight(bool b, bool isRootLineBox = false) const;
    virtual short baselinePosition(bool b, bool isRootLineBox = false) const;
    
    virtual void calcMinMaxWidth();
    virtual void layout();
    virtual void paint(PaintInfo &paintInfo, int parentX, int parentY);
    
    virtual IntRect getAbsoluteRepaintRect();
    virtual QMatrix absoluteTransform() const;

    virtual void setViewport(const FloatRect&);
    virtual FloatRect viewport() const;

    virtual void setViewBox(const FloatRect&);
    virtual FloatRect viewBox() const;

    virtual void setAlign(KCAlign align);
    virtual KCAlign align() const;
    
private:
    QMatrix viewportTransform() const;

    FloatRect m_viewport;
    FloatRect m_viewBox;
    KCAlign m_align;
};

class KCanvasClipperQuartz : public KCanvasClipper {
public:
    KCanvasClipperQuartz() { }
    
    virtual void applyClip(const FloatRect& boundingBox) const;
};

class KCanvasImageQuartz : public KCanvasImage {
public:
    KCanvasImageQuartz() : m_cgLayer(0) { }
    ~KCanvasImageQuartz();
    void init(const Image &) { }
    void init(IntSize size) { m_size = size; }
    
    CGLayerRef cgLayer();
    void setCGLayer(CGLayerRef layer);

    IntSize size() { return m_size; }
    
private:
    IntSize m_size;
    CGLayerRef m_cgLayer;
};

}

#endif
