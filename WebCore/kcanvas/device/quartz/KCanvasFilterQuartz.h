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

#ifdef SVG_SUPPORT

#import "KCanvasFilters.h"

#ifdef __OBJC__
@class CIFilter;
@class CIImage;
#else
class CIFilter;
class CIImage;
#endif

namespace WebCore {

class KRenderingDevice;

class KCanvasFilterQuartz : public KCanvasFilter {
public:
    KCanvasFilterQuartz();
    virtual ~KCanvasFilterQuartz();
    
    virtual void prepareFilter(const FloatRect& bbox);
    virtual void applyFilter(const FloatRect& bbox);
    
    CIImage* imageForName(const String&) const;
    void setImageForName(CIImage*, const String&);
    
    void setOutputImage(const KCanvasFilterEffect*, CIImage*);
    CIImage* inputImage(const KCanvasFilterEffect*);
    
private:
    NSArray* getCIFilterStack(CIImage* inputImage);

    CIContext* m_filterCIContext;
    CGLayerRef m_filterCGLayer;
    NSMutableDictionary* m_imagesByName;
};

class KCanvasFEBlendQuartz : public KCanvasFEBlend {
public:
    virtual CIFilter* getCIFilter(KCanvasFilterQuartz*) const;
};

class KCanvasFEColorMatrixQuartz : public KCanvasFEColorMatrix {
public:
    virtual CIFilter* getCIFilter(KCanvasFilterQuartz*) const;
};

class KCanvasFEComponentTransferQuartz : public KCanvasFEComponentTransfer {
public:
    virtual CIFilter* getCIFilter(KCanvasFilterQuartz*) const;
private:
    CIFilter* getFunctionFilter(KCChannelSelectorType, CIImage* inputImage) const;
};

class KCanvasFEConvolveMatrixQuartz : public KCanvasFEConvolveMatrix {
public:
    virtual CIFilter* getCIFilter(KCanvasFilterQuartz*) const;
};

class KCanvasFECompositeQuartz : public KCanvasFEComposite {
public:
    virtual CIFilter* getCIFilter(KCanvasFilterQuartz*) const;
};

class KCanvasFEDiffuseLightingQuartz : public KCanvasFEDiffuseLighting {
public:
    virtual CIFilter* getCIFilter(KCanvasFilterQuartz*) const;
};

class KCanvasFEFloodQuartz : public KCanvasFEFlood {
public:
    virtual CIFilter* getCIFilter(KCanvasFilterQuartz*) const;
};

class KCanvasFEGaussianBlurQuartz : public KCanvasFEGaussianBlur {
public:
    virtual CIFilter* getCIFilter(KCanvasFilterQuartz*) const;
};

class KCanvasFEImageQuartz : public KCanvasFEImage {
public:
    virtual CIFilter* getCIFilter(KCanvasFilterQuartz*) const;
};

class KCanvasFEDisplacementMapQuartz : public KCanvasFEDisplacementMap {
public:
    virtual CIFilter* getCIFilter(KCanvasFilterQuartz*) const;
};

class KCanvasFEMergeQuartz : public KCanvasFEMerge {
public:
    virtual CIFilter* getCIFilter(KCanvasFilterQuartz*) const;
};

class KCanvasFEOffsetQuartz : public KCanvasFEOffset {
public:
    virtual CIFilter* getCIFilter(KCanvasFilterQuartz*) const;
};

class KCanvasFESpecularLightingQuartz : public KCanvasFESpecularLighting {
public:
    virtual CIFilter* getCIFilter(KCanvasFilterQuartz*) const;
};

class KCanvasFETileQuartz : public KCanvasFETile {
public:
    virtual CIFilter* getCIFilter(KCanvasFilterQuartz*) const;
};

}

#endif // SVG_SUPPORT
