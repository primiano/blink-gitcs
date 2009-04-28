/**
 * Copyright (C) 2007 Rob Buis <buis@kde.org>
 *           (C) 2007 Nikolas Zimmermann <zimmermann@kde.org>
 *           (C) 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Google, Inc.  All rights reserved.
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
 *
 */

#if ENABLE(SVG)
#include "RenderObject.h"

namespace WebCore {

// FIXME: Most of this code should move to RenderSVGModelObject once
// all SVG renderers inherit from RenderSVGModelObject

class SVGResourceFilter;
void prepareToRenderSVGContent(RenderObject*, RenderObject::PaintInfo&, const FloatRect& boundingBox, SVGResourceFilter*&, SVGResourceFilter* rootFilter = 0);
void finishRenderSVGContent(RenderObject*, RenderObject::PaintInfo&, const FloatRect& boundingBox, SVGResourceFilter*&, GraphicsContext* savedContext);

// This offers a way to render parts of a WebKit rendering tree into a ImageBuffer.
class ImageBuffer;
void renderSubtreeToImage(ImageBuffer*, RenderObject*);

void clampImageBufferSizeToViewport(RenderObject*, IntSize&);

// Used to share the "walk all the children" logic between objectBoundingBox
// and repaintRectInLocalCoordinates in RenderSVGRoot and RenderSVGContainer
FloatRect computeContainerBoundingBox(const RenderObject* container, bool includeAllPaintedContent);

// returns the filter bounding box (or the empty rect if no filter) in local coordinates
FloatRect filterBoundingBoxForRenderer(const RenderObject*);

// Used for transforming the GraphicsContext and damage rect before passing PaintInfo to child renderers.
void applyTransformToPaintInfo(RenderObject::PaintInfo& paintInfo, const TransformationMatrix& localToChildTransform);

}

#endif
