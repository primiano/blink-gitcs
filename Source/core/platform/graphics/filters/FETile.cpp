/*
 * Copyright (C) 2008 Alex Mathews <possessedpenguinbob@gmail.com>
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#include "config.h"

#include "core/platform/graphics/filters/FETile.h"

#include "SkFlattenableBuffers.h"
#include "SkTileImageFilter.h"

#include "core/platform/graphics/GraphicsContext.h"
#include "core/platform/graphics/Pattern.h"
#include "core/platform/graphics/filters/Filter.h"
#include "core/platform/graphics/filters/SkiaImageFilterBuilder.h"
#include "platform/text/TextStream.h"
#include "platform/transforms/AffineTransform.h"
#include "third_party/skia/include/core/SkDevice.h"

namespace WebCore {

FETile::FETile(Filter* filter)
    : FilterEffect(filter)
{
}

PassRefPtr<FETile> FETile::create(Filter* filter)
{
    return adoptRef(new FETile(filter));
}

void FETile::applySoftware()
{
    FilterEffect* in = inputEffect(0);

    ImageBuffer* resultImage = createImageBufferResult();
    if (!resultImage)
        return;

    setIsAlphaImage(in->isAlphaImage());

    // Source input needs more attention. It has the size of the filterRegion but gives the
    // size of the cutted sourceImage back. This is part of the specification and optimization.
    FloatRect tileRect = in->maxEffectRect();
    FloatPoint inMaxEffectLocation = tileRect.location();
    FloatPoint maxEffectLocation = maxEffectRect().location();
    if (in->filterEffectType() == FilterEffectTypeSourceInput) {
        Filter* filter = this->filter();
        tileRect = filter->absoluteFilterRegion();
        tileRect.scale(filter->filterResolution().width(), filter->filterResolution().height());
    }

    OwnPtr<ImageBuffer> tileImage = ImageBuffer::createBufferForTile(tileRect.size(), tileRect.size(), filter()->renderingMode());
    if (!tileImage)
        return;

    GraphicsContext* tileImageContext = tileImage->context();
    tileImageContext->translate(-inMaxEffectLocation.x(), -inMaxEffectLocation.y());
    tileImageContext->drawImageBuffer(in->asImageBuffer(), in->absolutePaintRect().location());

    RefPtr<Pattern> pattern = Pattern::create(tileImage->copyImage(CopyBackingStore), true, true);

    AffineTransform patternTransform;
    patternTransform.translate(inMaxEffectLocation.x() - maxEffectLocation.x(), inMaxEffectLocation.y() - maxEffectLocation.y());
    pattern->setPatternSpaceTransform(patternTransform);
    GraphicsContext* filterContext = resultImage->context();
    filterContext->setFillPattern(pattern);
    filterContext->fillRect(FloatRect(FloatPoint(), absolutePaintRect().size()));
}

PassRefPtr<SkImageFilter> FETile::createImageFilter(SkiaImageFilterBuilder* builder)
{
    RefPtr<SkImageFilter> input(builder->build(inputEffect(0), operatingColorSpace()));
    FloatRect srcRect = inputEffect(0) ? inputEffect(0)->effectBoundaries() : FloatRect();
    return adoptRef(new SkTileImageFilter(srcRect, effectBoundaries(), input.get()));
}

TextStream& FETile::externalRepresentation(TextStream& ts, int indent) const
{
    writeIndent(ts, indent);
    ts << "[feTile";
    FilterEffect::externalRepresentation(ts);
    ts << "]\n";
    inputEffect(0)->externalRepresentation(ts, indent + 1);

    return ts;
}

} // namespace WebCore
