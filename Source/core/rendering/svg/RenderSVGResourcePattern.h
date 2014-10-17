/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef RenderSVGResourcePattern_h
#define RenderSVGResourcePattern_h

#include "core/rendering/svg/RenderSVGResourceContainer.h"
#include "core/svg/PatternAttributes.h"
#include "core/svg/SVGPatternElement.h"
#include "core/svg/SVGUnitTypes.h"
#include "platform/geometry/FloatRect.h"
#include "platform/graphics/ImageBuffer.h"
#include "platform/graphics/Pattern.h"
#include "platform/transforms/AffineTransform.h"

#include "wtf/HashMap.h"
#include "wtf/OwnPtr.h"

namespace blink {

struct PatternData {
    WTF_MAKE_FAST_ALLOCATED;
public:
    RefPtr<Pattern> pattern;
    AffineTransform transform;
};

class RenderSVGResourcePattern final : public RenderSVGResourceContainer {
public:
    explicit RenderSVGResourcePattern(SVGPatternElement*);

    virtual const char* renderName() const override { return "RenderSVGResourcePattern"; }

    virtual void removeAllClientsFromCache(bool markForInvalidation = true) override;
    virtual void removeClientFromCache(RenderObject*, bool markForInvalidation = true) override;

    virtual SVGPaintServer preparePaintServer(const RenderObject&) override;

    virtual RenderSVGResourceType resourceType() const override { return s_resourceType; }
    static const RenderSVGResourceType s_resourceType;

private:
    bool buildTileImageTransform(const RenderObject&, const PatternAttributes&, const SVGPatternElement*, FloatRect& patternBoundaries, AffineTransform& tileImageTransform) const;

    PassOwnPtr<ImageBuffer> createTileImage(const PatternAttributes&, const FloatRect& tileBoundaries,
                                            const FloatRect& absoluteTileBoundaries, const AffineTransform& tileImageTransform) const;

    PatternData* buildPattern(const RenderObject&, const SVGPatternElement*);

    bool m_shouldCollectPatternAttributes : 1;
    PatternAttributes m_attributes;
    HashMap<const RenderObject*, OwnPtr<PatternData> > m_patternMap;
};

}

#endif
