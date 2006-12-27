/*
    Copyright (C) 2006 Nikolas Zimmermann <wildfox@kde.org>

    This file is part of the KDE project

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    aint with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include "config.h"

#ifdef SVG_SUPPORT
#include "SVGPaintServerPattern.h"

#include "GraphicsContext.h"
#include "RenderObject.h"
#include "CgSupport.h"

namespace WebCore {

static void patternCallback(void* info, CGContextRef context)
{
    ImageBuffer* patternImage = reinterpret_cast<ImageBuffer*>(info);

    IntSize patternContentSize = patternImage->size();
    CGContextDrawImage(context, CGRectMake(0, 0, patternContentSize.width(), patternContentSize.height()), patternImage->cgImage());
}

bool SVGPaintServerPattern::setup(GraphicsContext*& context, const RenderObject* object, SVGPaintTargetType type, bool isPaintingText) const
{
    if (listener()) // this seems like bad design to me, should be in a common baseclass. -- ecs 8/6/05
        listener()->resourceNotification();

    if (!tile())
        return false;

    if (m_pattern && m_tileIsDirty) {
        m_tileIsDirty = false;
 
        CGPatternRelease(m_pattern);
        m_pattern = 0;
    }

    RenderStyle* style = object->style();
    CGContextRef contextRef = context->platformContext();

    context->save();

    CGSize cellSize = CGSize(tile()->size());
    CGFloat alpha = 1; // canvasStyle->opacity(); //which?

    if (boundingBoxMode()) {
        // Compute destination object bounding box
        CGRect objectBBox = CGContextGetPathBoundingBox(contextRef);

        // Choose default pattern bounding box
        CGRect patternBBox = CGRectMake(0, 0, 100, 100);

        // Generate a transform to map between both bounding boxes
        CGAffineTransform patternIntoObjectBBox = CGAffineTransformMakeMapBetweenRects(patternBBox, objectBBox);
        CGContextConcatCTM(contextRef, patternIntoObjectBBox);
    }

    // Repesct local pattern transformations
    CGContextConcatCTM(contextRef, patternTransform());

    CGSize phase = CGSizeMake(bbox().x(), -bbox().y()); // Pattern space seems to start in the lower-left, so we flip the Y here.
    CGContextSetPatternPhase(contextRef, phase);

    CGContextSetAlpha(contextRef, style->opacity()); // or do I set the alpha above?

    if (!m_pattern) {
        CGPatternCallbacks callbacks = {0, patternCallback, NULL};
        m_pattern = CGPatternCreate(tile(),
                                    CGRectMake(0, 0, cellSize.width, cellSize.height),
                                    CGContextGetCTM(contextRef),
                                    bbox().width(),
                                    bbox().height(),
                                    kCGPatternTilingConstantSpacing, // FIXME: should ask CG guys.
                                    true, // has color
                                    &callbacks);
    }

    m_patternSpace = CGColorSpaceCreatePattern(0);

    if ((type & ApplyToFillTargetType) && style->svgStyle()->hasFill()) {
        CGContextSetFillColorSpace(contextRef, m_patternSpace);
        CGContextSetFillPattern(contextRef, m_pattern, &alpha);
 
        if (isPaintingText) 
            context->setTextDrawingMode(cTextFill);
    }

    if ((type & ApplyToStrokeTargetType) && style->svgStyle()->hasStroke()) {
        CGContextSetStrokeColorSpace(contextRef, m_patternSpace);
        CGContextSetStrokePattern(contextRef, m_pattern, &alpha);
        applyStrokeStyleToContext(contextRef, style, object);

        if (isPaintingText) 
            context->setTextDrawingMode(cTextStroke);
    }

    return true;
}

void SVGPaintServerPattern::teardown(GraphicsContext*& context, const RenderObject*, SVGPaintTargetType, bool) const
{
    context->restore();
}

} // namespace WebCore

#endif

// vim:ts=4:noet
