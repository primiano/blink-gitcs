/*
    Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
                  2004, 2005, 2007 Rob Buis <buis@kde.org>
    Copyright (C) 2009 Google, Inc.  All rights reserved.

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
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef RenderSVGContainer_h
#define RenderSVGContainer_h

#if ENABLE(SVG)

#include "RenderSVGModelObject.h"

namespace WebCore {

class SVGElement;

class RenderSVGContainer : public RenderSVGModelObject {
public:
    RenderSVGContainer(SVGStyledElement*);
    ~RenderSVGContainer();

    virtual RenderObjectChildList* virtualChildren() { return children(); }
    virtual const RenderObjectChildList* virtualChildren() const { return children(); }
    const RenderObjectChildList* children() const { return &m_children; }
    RenderObjectChildList* children() { return &m_children; }

    // Some containers do not want it's children
    // to be drawn, because they may be 'referenced'
    // Example: <marker> children in SVG
    void setDrawsContents(bool);
    bool drawsContents() const;

    virtual bool isSVGContainer() const { return true; }
    virtual const char* renderName() const { return "RenderSVGContainer"; }

    virtual void layout();
    virtual void paint(PaintInfo&, int parentX, int parentY);

    virtual IntRect clippedOverflowRectForRepaint(RenderBoxModelObject* repaintContainer);
    virtual void absoluteRects(Vector<IntRect>& rects, int tx, int ty, bool topLevel = true);
    virtual void absoluteQuads(Vector<FloatQuad>&, bool topLevel = true);
    virtual void addFocusRingRects(GraphicsContext*, int tx, int ty);

    virtual FloatRect objectBoundingBox() const;
    virtual FloatRect repaintRectInLocalCoordinates() const;

    // FIXME: This only exists to allow computeContainerBoundingBox's broken transformation
    // Once localToParentTransform() is landed, this will be removed, and layout test results updated.
    virtual TransformationMatrix viewportTransform() const { return TransformationMatrix(); }

    virtual bool nodeAtPoint(const HitTestRequest&, HitTestResult&, int x, int y, int tx, int ty, HitTestAction);

protected:
    virtual bool calculateLocalTransform();
    virtual void applyContentTransforms(PaintInfo&);
    virtual void applyAdditionalTransforms(PaintInfo&);

    virtual IntRect outlineBoundsForRepaint(RenderBoxModelObject* /*repaintContainer*/) const;

private:
    RenderObjectChildList m_children;
    
    bool selfWillPaint() const;

    bool m_drawsContents : 1;
    
protected:    
    IntRect m_absoluteBounds;
};
  
} // namespace WebCore

#endif // ENABLE(SVG)
#endif // RenderSVGContainer_h

// vim:ts=4:noet
