/*
 * Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2007 Rob Buis <buis@kde.org>
 * Copyright (C) 2009 Google, Inc.  All rights reserved.
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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

#ifndef RenderSVGContainer_h
#define RenderSVGContainer_h

#include "core/rendering/svg/RenderSVGModelObject.h"

namespace blink {

class SVGElement;

class RenderSVGContainer : public RenderSVGModelObject {
public:
    explicit RenderSVGContainer(SVGElement*);
    virtual ~RenderSVGContainer();
    virtual void trace(Visitor*) override;

    // If you have a RenderSVGContainer, use firstChild or lastChild instead.
    void slowFirstChild() const = delete;
    void slowLastChild() const = delete;

    RenderObject* firstChild() const { ASSERT(children() == virtualChildren()); return children()->firstChild(); }
    RenderObject* lastChild() const { ASSERT(children() == virtualChildren()); return children()->lastChild(); }

    virtual void paint(const PaintInfo&, const LayoutPoint&) override;
    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle) override;
    virtual void setNeedsBoundariesUpdate() override final { m_needsBoundariesUpdate = true; }
    virtual bool didTransformToRootUpdate() { return false; }
    bool isObjectBoundingBoxValid() const { return m_objectBoundingBoxValid; }

    bool selfWillPaint();

    virtual bool hasNonIsolatedBlendingDescendants() const override final;
protected:
    virtual RenderObjectChildList* virtualChildren() override final { return children(); }
    virtual const RenderObjectChildList* virtualChildren() const override final { return children(); }

    virtual bool isOfType(RenderObjectType type) const override { return type == RenderObjectSVGContainer || RenderSVGModelObject::isOfType(type); }
    virtual const char* renderName() const override { return "RenderSVGContainer"; }

    virtual void layout() override;

    virtual void addChild(RenderObject* child, RenderObject* beforeChild = 0) override final;
    virtual void removeChild(RenderObject*) override final;
    virtual void addFocusRingRects(Vector<LayoutRect>&, const LayoutPoint& additionalOffset) const override final;

    virtual FloatRect objectBoundingBox() const override final { return m_objectBoundingBox; }
    virtual FloatRect strokeBoundingBox() const override final { return m_strokeBoundingBox; }

    virtual bool nodeAtFloatPoint(const HitTestRequest&, HitTestResult&, const FloatPoint& pointInParent, HitTestAction) override;

    // Allow RenderSVGTransformableContainer to hook in at the right time in layout().
    virtual bool calculateLocalTransform() { return false; }

    // Allow RenderSVGViewportContainer to hook in at the right times in layout() and nodeAtFloatPoint().
    virtual void calcViewport() { }
    virtual bool pointIsInsideViewportClip(const FloatPoint& /*pointInParent*/) { return true; }

    virtual void determineIfLayoutSizeChanged() { }

    void updateCachedBoundaries();

    virtual void descendantIsolationRequirementsChanged(DescendantIsolationState) override final;

private:
    const RenderObjectChildList* children() const { return &m_children; }
    RenderObjectChildList* children() { return &m_children; }

    RenderObjectChildList m_children;
    FloatRect m_objectBoundingBox;
    FloatRect m_strokeBoundingBox;
    bool m_objectBoundingBoxValid;
    bool m_needsBoundariesUpdate;
    mutable bool m_hasNonIsolatedBlendingDescendants : 1;
    mutable bool m_hasNonIsolatedBlendingDescendantsDirty : 1;
};

DEFINE_RENDER_OBJECT_TYPE_CASTS(RenderSVGContainer, isSVGContainer());

} // namespace blink

#endif // RenderSVGContainer_h
