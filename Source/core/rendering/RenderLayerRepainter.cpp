/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011, 2012 Apple Inc. All rights reserved.
 *
 * Portions are Copyright (C) 1998 Netscape Communications Corporation.
 *
 * Other contributors:
 *   Robert O'Callahan <roc+@cs.cmu.edu>
 *   David Baron <dbaron@fas.harvard.edu>
 *   Christian Biesinger <cbiesinger@web.de>
 *   Randall Jesup <rjesup@wgate.com>
 *   Roland Mainz <roland.mainz@informatik.med.uni-giessen.de>
 *   Josh Soref <timeless@mac.com>
 *   Boris Zbarsky <bzbarsky@mit.edu>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Alternatively, the contents of this file may be used under the terms
 * of either the Mozilla Public License Version 1.1, found at
 * http://www.mozilla.org/MPL/ (the "MPL") or the GNU General Public
 * License Version 2.0, found at http://www.fsf.org/copyleft/gpl.html
 * (the "GPL"), in which case the provisions of the MPL or the GPL are
 * applicable instead of those above.  If you wish to allow use of your
 * version of this file only under the terms of one of those two
 * licenses (the MPL or the GPL) and not to allow others to use your
 * version of this file under the LGPL, indicate your decision by
 * deletingthe provisions above and replace them with the notice and
 * other provisions required by the MPL or the GPL, as the case may be.
 * If you do not delete the provisions above, a recipient may use your
 * version of this file under any of the LGPL, the MPL or the GPL.
 */

#include "config.h"
#include "core/rendering/RenderLayerRepainter.h"

#include "core/rendering/FilterEffectRenderer.h"
#include "core/rendering/RenderLayer.h"
#include "core/rendering/RenderView.h"
#include "core/rendering/compositing/CompositedLayerMapping.h"

namespace WebCore {

RenderLayerRepainter::RenderLayerRepainter(RenderLayerModelObject& renderer)
    : m_renderer(renderer)
{
}

void RenderLayerRepainter::computeRepaintRectsIncludingNonCompositingDescendants()
{
    // FIXME: boundsRectForPaintInvalidation() has to walk up the parent chain
    // for every layer to compute the rects. We should make this more efficient.
    // FIXME: it's wrong to call this when layout is not up-to-date, which we do.
    m_renderer.setPreviousPaintInvalidationRect(m_renderer.boundsRectForPaintInvalidation(m_renderer.containerForPaintInvalidation()));
    // FIXME: We are only updating the paint invalidation bounds but not
    // the positionFromPaintInvalidationContainer. This means that we may
    // forcing a full invaliation of the new position. Is this really correct?

    for (RenderLayer* layer = m_renderer.layer()->firstChild(); layer; layer = layer->nextSibling()) {
        if (layer->compositingState() != PaintsIntoOwnBacking && layer->compositingState() != PaintsIntoGroupedBacking)
            layer->repainter().computeRepaintRectsIncludingNonCompositingDescendants();
    }
}

// Since we're only painting non-composited layers, we know that they all share the same repaintContainer.
void RenderLayerRepainter::repaintIncludingNonCompositingDescendants()
{
    repaintIncludingNonCompositingDescendantsInternal(m_renderer.containerForPaintInvalidation());
}

void RenderLayerRepainter::repaintIncludingNonCompositingDescendantsInternal(const RenderLayerModelObject* repaintContainer)
{
    m_renderer.invalidatePaintUsingContainer(repaintContainer, m_renderer.previousPaintInvalidationRect(), InvalidationLayer);

    // Disable for reading compositingState() below.
    DisableCompositingQueryAsserts disabler;

    for (RenderLayer* curr = m_renderer.layer()->firstChild(); curr; curr = curr->nextSibling()) {
        if (curr->compositingState() != PaintsIntoOwnBacking && curr->compositingState() != PaintsIntoGroupedBacking)
            curr->repainter().repaintIncludingNonCompositingDescendantsInternal(repaintContainer);
    }
}

LayoutRect RenderLayerRepainter::repaintRectIncludingNonCompositingDescendants() const
{
    LayoutRect repaintRect = m_renderer.previousPaintInvalidationRect();

    for (RenderLayer* child = m_renderer.layer()->firstChild(); child; child = child->nextSibling()) {
        // Don't include repaint rects for composited child layers; they will paint themselves and have a different origin.
        if (child->compositingState() == PaintsIntoOwnBacking || child->compositingState() == PaintsIntoGroupedBacking)
            continue;

        repaintRect.unite(child->repainter().repaintRectIncludingNonCompositingDescendants());
    }
    return repaintRect;
}

void RenderLayerRepainter::setBackingNeedsRepaintInRect(const LayoutRect& r)
{
    // https://bugs.webkit.org/show_bug.cgi?id=61159 describes an unreproducible crash here,
    // so assert but check that the layer is composited.
    ASSERT(m_renderer.compositingState() != NotComposited);
    if (m_renderer.compositingState() == NotComposited) {
        // If we're trying to repaint the placeholder document layer, propagate the
        // repaint to the native view system.
        LayoutRect absRect(r);
        LayoutPoint delta;
        m_renderer.layer()->convertToLayerCoords(m_renderer.layer()->root(), delta);
        absRect.moveBy(delta);

        if (absRect.isEmpty())
            return;

        RenderView* view = m_renderer.view();
        if (view)
            view->repaintViewRectangle(absRect);
        return;
    }
    // FIXME: generalize accessors to backing GraphicsLayers so that this code is squasphing-agnostic.
    if (m_renderer.layer()->groupedMapping()) {
        LayoutRect repaintRect = r;
        repaintRect.move(m_renderer.layer()->subpixelAccumulation());
        if (GraphicsLayer* squashingLayer = m_renderer.layer()->groupedMapping()->squashingLayer())
            squashingLayer->setNeedsDisplayInRect(pixelSnappedIntRect(repaintRect));
    } else {
        m_renderer.layer()->compositedLayerMapping()->setContentsNeedDisplayInRect(r);
    }
}

void RenderLayerRepainter::setFilterBackendNeedsRepaintingInRect(const LayoutRect& rect)
{
    if (rect.isEmpty())
        return;
    LayoutRect rectForRepaint = rect;
    m_renderer.style()->filterOutsets().expandRect(rectForRepaint);

    RenderLayerFilterInfo* filterInfo = m_renderer.layer()->filterInfo();
    ASSERT(filterInfo);
    filterInfo->expandDirtySourceRect(rectForRepaint);

    RenderLayer* parentLayer = enclosingFilterRepaintLayer();
    ASSERT(parentLayer);
    FloatQuad repaintQuad(rectForRepaint);
    LayoutRect parentLayerRect = m_renderer.localToContainerQuad(repaintQuad, parentLayer->renderer()).enclosingBoundingBox();

    if (parentLayerRect.isEmpty())
        return;

    if (parentLayer->hasCompositedLayerMapping()) {
        parentLayer->repainter().setBackingNeedsRepaintInRect(parentLayerRect);
        return;
    }

    if (parentLayer->paintsWithFilters()) {
        parentLayer->repainter().setFilterBackendNeedsRepaintingInRect(parentLayerRect);
        return;
    }

    if (parentLayer->isRootLayer()) {
        RenderView* view = toRenderView(parentLayer->renderer());
        view->repaintViewRectangle(parentLayerRect);
        return;
    }

    ASSERT_NOT_REACHED();
}

RenderLayer* RenderLayerRepainter::enclosingFilterRepaintLayer() const
{
    for (const RenderLayer* curr = m_renderer.layer(); curr; curr = curr->parent()) {
        if ((curr != m_renderer.layer() && curr->requiresFullLayerImageForFilters()) || curr->compositingState() == PaintsIntoOwnBacking || curr->isRootLayer())
            return const_cast<RenderLayer*>(curr);
    }
    return 0;
}

} // Namespace WebCore
