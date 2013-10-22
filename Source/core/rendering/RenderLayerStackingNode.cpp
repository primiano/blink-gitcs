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
#include "core/rendering/RenderLayerStackingNode.h"

#include "core/platform/HistogramSupport.h"
#include "core/rendering/RenderLayer.h"
#include "core/rendering/RenderLayerCompositor.h"
#include "core/rendering/RenderView.h"

namespace WebCore {

// FIXME: This should not require RenderLayer. There is currently a cycle where
// in order to determine if we shoulBeNormalFlowOnly() and isStackingContainer()
// we have to ask the render layer about some of its state.
RenderLayerStackingNode::RenderLayerStackingNode(RenderLayer* layer)
    : m_layer(layer)
    , m_normalFlowListDirty(true)
    , m_descendantsAreContiguousInStackingOrder(false)
    , m_descendantsAreContiguousInStackingOrderDirty(true)
    , m_needsToBeStackingContainer(false)
    , m_needsToBeStackingContainerHasBeenRecorded(false)
#if !ASSERT_DISABLED
    , m_layerListMutationAllowed(true)
#endif
{
    m_isNormalFlowOnly = shouldBeNormalFlowOnly();

    // Non-stacking containers should have empty z-order lists. As this is already the case,
    // there is no need to dirty / recompute these lists.
    m_zOrderListsDirty = isStackingContainer();
}

bool RenderLayerStackingNode::isStackingContext(const RenderStyle* style) const
{
    return !style->hasAutoZIndex() || layer()->isRootLayer();
}

// Helper for the sorting of layers by z-index.
static inline bool compareZIndex(RenderLayer* first, RenderLayer* second)
{
    return first->stackingNode()->zIndex() < second->stackingNode()->zIndex();
}

RenderLayerCompositor* RenderLayerStackingNode::compositor() const
{
    if (!renderer()->view())
        return 0;
    return renderer()->view()->compositor();
}

void RenderLayerStackingNode::dirtyNormalFlowListCanBePromotedToStackingContainer()
{
    m_descendantsAreContiguousInStackingOrderDirty = true;

    if (m_normalFlowListDirty || !normalFlowList())
        return;

    for (size_t index = 0; index < normalFlowList()->size(); ++index)
        normalFlowList()->at(index)->stackingNode()->dirtyNormalFlowListCanBePromotedToStackingContainer();
}

void RenderLayerStackingNode::dirtySiblingStackingNodeCanBePromotedToStackingContainer()
{
    RenderLayerStackingNode* ancestorStackingNode = layer()->ancestorStackingNode();
    if (!ancestorStackingNode)
        return;

    if (!ancestorStackingNode->zOrderListsDirty() && ancestorStackingNode->posZOrderList()) {
        for (size_t index = 0; index < ancestorStackingNode->posZOrderList()->size(); ++index)
            ancestorStackingNode->posZOrderList()->at(index)->stackingNode()->setDescendantsAreContiguousInStackingOrderDirty(true);
    }

    ancestorStackingNode->dirtyNormalFlowListCanBePromotedToStackingContainer();

    if (!ancestorStackingNode->zOrderListsDirty() && ancestorStackingNode->negZOrderList()) {
        for (size_t index = 0; index < ancestorStackingNode->negZOrderList()->size(); ++index)
            ancestorStackingNode->negZOrderList()->at(index)->stackingNode()->setDescendantsAreContiguousInStackingOrderDirty(true);
    }
}

void RenderLayerStackingNode::dirtyZOrderLists()
{
    ASSERT(m_layerListMutationAllowed);
    ASSERT(isStackingContainer());

    if (m_posZOrderList)
        m_posZOrderList->clear();
    if (m_negZOrderList)
        m_negZOrderList->clear();
    m_zOrderListsDirty = true;

    m_descendantsAreContiguousInStackingOrderDirty = true;

    if (!renderer()->documentBeingDestroyed()) {
        compositor()->setNeedsUpdateCompositingRequirementsState();
        compositor()->setCompositingLayersNeedRebuild();
        if (layer()->acceleratedCompositingForOverflowScrollEnabled())
            compositor()->setShouldReevaluateCompositingAfterLayout();
    }
}

void RenderLayerStackingNode::dirtyStackingContainerZOrderLists()
{
    // Any siblings in the ancestor stacking context could also be affected.
    // Changing z-index, for example, could cause us to stack in between a
    // sibling's descendants, meaning that we have to recompute
    // m_descendantsAreContiguousInStackingOrder for that sibling.
    dirtySiblingStackingNodeCanBePromotedToStackingContainer();

    RenderLayerStackingNode* stackingContainerNode = layer()->ancestorStackingContainerNode();
    if (stackingContainerNode)
        stackingContainerNode->dirtyZOrderLists();

    // Any change that could affect our stacking container's z-order list could
    // cause other RenderLayers in our stacking context to either opt in or out
    // of composited scrolling. It is important that we make our stacking
    // context aware of these z-order changes so the appropriate updating can
    // happen.
    RenderLayerStackingNode* stackingNode = layer()->ancestorStackingNode();
    if (stackingNode && stackingNode != stackingContainerNode)
        stackingNode->dirtyZOrderLists();
}

void RenderLayerStackingNode::dirtyNormalFlowList()
{
    ASSERT(m_layerListMutationAllowed);

    if (m_normalFlowList)
        m_normalFlowList->clear();
    m_normalFlowListDirty = true;

    if (!renderer()->documentBeingDestroyed()) {
        compositor()->setCompositingLayersNeedRebuild();
        if (layer()->acceleratedCompositingForOverflowScrollEnabled())
            compositor()->setShouldReevaluateCompositingAfterLayout();
    }
}

void RenderLayerStackingNode::rebuildZOrderLists()
{
    ASSERT(m_layerListMutationAllowed);
    ASSERT(isDirtyStackingContainer());
    rebuildZOrderLists(m_posZOrderList, m_negZOrderList);
    m_zOrderListsDirty = false;
}

void RenderLayerStackingNode::rebuildZOrderLists(OwnPtr<Vector<RenderLayer*> >& posZOrderList,
    OwnPtr<Vector<RenderLayer*> >& negZOrderList, const RenderLayer* layerToForceAsStackingContainer,
    CollectLayersBehavior collectLayersBehavior)
{
    bool includeHiddenLayers = compositor()->inCompositingMode();
    for (RenderLayer* child = layer()->firstChild(); child; child = child->nextSibling()) {
        if (!layer()->reflection() || layer()->reflectionLayer() != child)
            child->stackingNode()->collectLayers(includeHiddenLayers, posZOrderList, negZOrderList, layerToForceAsStackingContainer, collectLayersBehavior);
    }

    // Sort the two lists.
    if (posZOrderList)
        std::stable_sort(posZOrderList->begin(), posZOrderList->end(), compareZIndex);

    if (negZOrderList)
        std::stable_sort(negZOrderList->begin(), negZOrderList->end(), compareZIndex);

    // Append layers for top layer elements after normal layer collection, to ensure they are on top regardless of z-indexes.
    // The renderers of top layer elements are children of the view, sorted in top layer stacking order.
    if (layer()->isRootLayer()) {
        RenderObject* view = renderer()->view();
        for (RenderObject* child = view->firstChild(); child; child = child->nextSibling()) {
            Element* childElement = (child->node() && child->node()->isElementNode()) ? toElement(child->node()) : 0;
            if (childElement && childElement->isInTopLayer()) {
                RenderLayer* layer = toRenderLayerModelObject(child)->layer();
                // Create the buffer if it doesn't exist yet.
                if (!posZOrderList)
                    posZOrderList = adoptPtr(new Vector<RenderLayer*>);
                posZOrderList->append(layer);
            }
        }
    }
}

void RenderLayerStackingNode::updateNormalFlowList()
{
    if (!m_normalFlowListDirty)
        return;

    ASSERT(m_layerListMutationAllowed);

    for (RenderLayer* child = layer()->firstChild(); child; child = child->nextSibling()) {
        // Ignore non-overflow layers and reflections.
        if (child->stackingNode()->isNormalFlowOnly() && (!layer()->reflection() || layer()->reflectionLayer() != child)) {
            if (!m_normalFlowList)
                m_normalFlowList = adoptPtr(new Vector<RenderLayer*>);
            m_normalFlowList->append(child);
        }
    }

    m_normalFlowListDirty = false;
}

void RenderLayerStackingNode::collectLayers(bool includeHiddenLayers,
    OwnPtr<Vector<RenderLayer*> >& posBuffer, OwnPtr<Vector<RenderLayer*> >& negBuffer,
    const RenderLayer* layerToForceAsStackingContainer, CollectLayersBehavior collectLayersBehavior)
{
    if (layer()->isInTopLayer())
        return;

    layer()->updateDescendantDependentFlags();

    bool isStacking = false;
    bool isNormalFlow = false;

    switch (collectLayersBehavior) {
    case ForceLayerToStackingContainer:
        ASSERT(layerToForceAsStackingContainer);
        if (layer() == layerToForceAsStackingContainer) {
            isStacking = true;
            isNormalFlow = false;
        } else {
            isStacking = isStackingContext();
            isNormalFlow = shouldBeNormalFlowOnlyIgnoringCompositedScrolling();
        }
        break;
    case OverflowScrollCanBeStackingContainers:
        ASSERT(!layerToForceAsStackingContainer);
        isStacking = isStackingContainer();
        isNormalFlow = isNormalFlowOnly();
        break;
    case OnlyStackingContextsCanBeStackingContainers:
        isStacking = isStackingContext();
        isNormalFlow = shouldBeNormalFlowOnlyIgnoringCompositedScrolling();
        break;
    }

    // Overflow layers are just painted by their enclosing layers, so they don't get put in zorder lists.
    bool includeHiddenLayer = includeHiddenLayers || (layer()->hasVisibleContent() || (layer()->hasVisibleDescendant() && isStacking));
    if (includeHiddenLayer && !isNormalFlow && !layer()->isOutOfFlowRenderFlowThread()) {
        // Determine which buffer the child should be in.
        OwnPtr<Vector<RenderLayer*> >& buffer = (zIndex() >= 0) ? posBuffer : negBuffer;

        // Create the buffer if it doesn't exist yet.
        if (!buffer)
            buffer = adoptPtr(new Vector<RenderLayer*>);

        // Append ourselves at the end of the appropriate buffer.
        buffer->append(layer());
    }

    // Recur into our children to collect more layers, but only if we don't establish
    // a stacking context/container.
    if ((includeHiddenLayers || layer()->hasVisibleDescendant()) && !isStacking) {
        for (RenderLayer* child = layer()->firstChild(); child; child = child->nextSibling()) {
            // Ignore reflections.
            if (!layer()->reflection() || layer()->reflectionLayer() != child)
                child->stackingNode()->collectLayers(includeHiddenLayers, posBuffer, negBuffer, layerToForceAsStackingContainer, collectLayersBehavior);
        }
    }
}

void RenderLayerStackingNode::updateLayerListsIfNeeded()
{
    updateZOrderLists();
    updateNormalFlowList();

    if (RenderLayer* reflectionLayer = layer()->reflectionLayer()) {
        reflectionLayer->stackingNode()->updateZOrderLists();
        reflectionLayer->stackingNode()->updateNormalFlowList();
    }
}

void RenderLayerStackingNode::updateStackingNodesAfterStyleChange(const RenderStyle* oldStyle)
{
    bool wasStackingContext = oldStyle ? isStackingContext(oldStyle) : false;
    EVisibility oldVisibility = oldStyle ? oldStyle->visibility() : VISIBLE;
    int oldZIndex = oldStyle ? oldStyle->zIndex() : 0;

    // FIXME: RenderLayer already handles visibility changes through our visiblity dirty bits. This logic could
    // likely be folded along with the rest.
    bool isStackingContext = this->isStackingContext();
    if (isStackingContext == wasStackingContext && oldVisibility == renderer()->style()->visibility() && oldZIndex == zIndex())
        return;

    dirtyStackingContainerZOrderLists();

    if (isStackingContainer())
        dirtyZOrderLists();
    else
        clearZOrderLists();

    compositor()->setNeedsUpdateCompositingRequirementsState();
}

bool RenderLayerStackingNode::shouldBeNormalFlowOnly() const
{
    return shouldBeNormalFlowOnlyIgnoringCompositedScrolling() && !layer()->needsCompositedScrolling();
}

bool RenderLayerStackingNode::shouldBeNormalFlowOnlyIgnoringCompositedScrolling() const
{
    const bool couldBeNormalFlow = renderer()->hasOverflowClip()
        || renderer()->hasReflection()
        || renderer()->hasMask()
        || renderer()->isCanvas()
        || renderer()->isVideo()
        || renderer()->isEmbeddedObject()
        || renderer()->isRenderIFrame()
        || (renderer()->style()->specifiesColumns() && !layer()->isRootLayer());
    const bool preventsElementFromBeingNormalFlow = renderer()->isPositioned()
        || renderer()->hasTransform()
        || renderer()->hasClipPath()
        || renderer()->hasFilter()
        || renderer()->hasBlendMode()
        || layer()->isTransparent()
        || renderer()->isRenderRegion();

    return couldBeNormalFlow && !preventsElementFromBeingNormalFlow;
}

void RenderLayerStackingNode::updateIsNormalFlowOnly()
{
    bool isNormalFlowOnly = shouldBeNormalFlowOnly();
    if (isNormalFlowOnly == m_isNormalFlowOnly)
        return;

    m_isNormalFlowOnly = isNormalFlowOnly;
    if (RenderLayer* p = layer()->parent())
        p->stackingNode()->dirtyNormalFlowList();
    dirtyStackingContainerZOrderLists();
}

bool RenderLayerStackingNode::needsToBeStackingContainer() const
{
    return layer()->adjustForForceCompositedScrollingMode(m_needsToBeStackingContainer);
}

// Determine whether the current layer can be promoted to a stacking container.
// We do this by computing what positive and negative z-order lists would look
// like before and after promotion, and ensuring that proper stacking order is
// preserved between the two sets of lists.
void RenderLayerStackingNode::updateDescendantsAreContiguousInStackingOrder()
{
    TRACE_EVENT0("blink_rendering,comp-scroll", "RenderLayerStackingNode::updateDescendantsAreContiguousInStackingOrder");

    const RenderLayer* currentLayer = layer();
    if (isStackingContext() || !m_descendantsAreContiguousInStackingOrderDirty || !currentLayer->acceleratedCompositingForOverflowScrollEnabled())
        return;

    if (!currentLayer->scrollsOverflow())
        return;

    RenderLayerStackingNode* ancestorStackingNode = currentLayer->ancestorStackingNode();
    if (!ancestorStackingNode)
        return;

    OwnPtr<Vector<RenderLayer*> > posZOrderListBeforePromote = adoptPtr(new Vector<RenderLayer*>);
    OwnPtr<Vector<RenderLayer*> > negZOrderListBeforePromote = adoptPtr(new Vector<RenderLayer*>);
    OwnPtr<Vector<RenderLayer*> > posZOrderListAfterPromote = adoptPtr(new Vector<RenderLayer*>);
    OwnPtr<Vector<RenderLayer*> > negZOrderListAfterPromote = adoptPtr(new Vector<RenderLayer*>);

    collectBeforePromotionZOrderList(ancestorStackingNode, posZOrderListBeforePromote, negZOrderListBeforePromote);
    collectAfterPromotionZOrderList(ancestorStackingNode, posZOrderListAfterPromote, negZOrderListAfterPromote);

    size_t maxIndex = std::min(posZOrderListAfterPromote->size() + negZOrderListAfterPromote->size(), posZOrderListBeforePromote->size() + negZOrderListBeforePromote->size());

    m_descendantsAreContiguousInStackingOrderDirty = false;
    m_descendantsAreContiguousInStackingOrder = false;

    const RenderLayer* layerAfterPromote = 0;
    for (size_t i = 0; i < maxIndex && layerAfterPromote != currentLayer; ++i) {
        const RenderLayer* layerBeforePromote = i < negZOrderListBeforePromote->size()
            ? negZOrderListBeforePromote->at(i)
            : posZOrderListBeforePromote->at(i - negZOrderListBeforePromote->size());
        layerAfterPromote = i < negZOrderListAfterPromote->size()
            ? negZOrderListAfterPromote->at(i)
            : posZOrderListAfterPromote->at(i - negZOrderListAfterPromote->size());

        if (layerBeforePromote != layerAfterPromote && (layerAfterPromote != currentLayer || renderer()->hasBackground()))
            return;
    }

    layerAfterPromote = 0;
    for (size_t i = 0; i < maxIndex && layerAfterPromote != currentLayer; ++i) {
        const RenderLayer* layerBeforePromote = i < posZOrderListBeforePromote->size()
            ? posZOrderListBeforePromote->at(posZOrderListBeforePromote->size() - i - 1)
            : negZOrderListBeforePromote->at(negZOrderListBeforePromote->size() + posZOrderListBeforePromote->size() - i - 1);
        layerAfterPromote = i < posZOrderListAfterPromote->size()
            ? posZOrderListAfterPromote->at(posZOrderListAfterPromote->size() - i - 1)
            : negZOrderListAfterPromote->at(negZOrderListAfterPromote->size() + posZOrderListAfterPromote->size() - i - 1);

        if (layerBeforePromote != layerAfterPromote && layerAfterPromote != currentLayer)
            return;
    }

    m_descendantsAreContiguousInStackingOrder = true;
}

void RenderLayerStackingNode::collectBeforePromotionZOrderList(RenderLayerStackingNode* ancestorStackingNode,
    OwnPtr<Vector<RenderLayer*> >& posZOrderList, OwnPtr<Vector<RenderLayer*> >& negZOrderList)
{
    ancestorStackingNode->rebuildZOrderLists(posZOrderList, negZOrderList, layer(), OnlyStackingContextsCanBeStackingContainers);

    const RenderLayer* currentLayer = layer();
    const RenderLayer* positionedAncestor = currentLayer->parent();
    while (positionedAncestor && !positionedAncestor->isPositionedContainer() && !positionedAncestor->stackingNode()->isStackingContext())
        positionedAncestor = positionedAncestor->parent();
    if (positionedAncestor && (!positionedAncestor->isPositionedContainer() || positionedAncestor->stackingNode()->isStackingContext()))
        positionedAncestor = 0;

    if (!posZOrderList)
        posZOrderList = adoptPtr(new Vector<RenderLayer*>());
    else if (posZOrderList->find(currentLayer) != kNotFound)
        return;

    // The current layer will appear in the z-order lists after promotion, so
    // for a meaningful comparison, we must insert it in the z-order lists
    // before promotion if it does not appear there already.
    if (!positionedAncestor) {
        posZOrderList->prepend(layer());
        return;
    }

    for (size_t index = 0; index < posZOrderList->size(); index++) {
        if (posZOrderList->at(index) == positionedAncestor) {
            posZOrderList->insert(index + 1, layer());
            return;
        }
    }
}

void RenderLayerStackingNode::collectAfterPromotionZOrderList(RenderLayerStackingNode* ancestorStackingNode,
    OwnPtr<Vector<RenderLayer*> >& posZOrderList, OwnPtr<Vector<RenderLayer*> >& negZOrderList)
{
    ancestorStackingNode->rebuildZOrderLists(posZOrderList, negZOrderList, layer(), ForceLayerToStackingContainer);
}

// Compute what positive and negative z-order lists would look like before and
// after promotion, so we can later ensure that proper stacking order is
// preserved between the two sets of lists.
//
// A few examples:
// c = currentLayer
// - = negative z-order child of currentLayer
// + = positive z-order child of currentLayer
// a = positioned ancestor of currentLayer
// x = any other RenderLayer in the list
//
// (a) xxxxx-----++a+++x
// (b) xxx-----c++++++xx
//
// Normally the current layer would be painted in the normal flow list if it
// doesn't already appear in the positive z-order list. However, in the case
// that the layer has a positioned ancestor, it will paint directly after the
// positioned ancestor. In example (a), the current layer would be painted in
// the middle of its own positive z-order children, so promoting would cause a
// change in paint order (since a promoted layer will paint all of its positive
// z-order children strictly after it paints itself).
//
// In example (b), it is ok to promote the current layer only if it does not
// have a background. If it has a background, the background gets painted before
// the layer's negative z-order children, so again, a promotion would cause a
// change in paint order (causing the background to get painted after the
// negative z-order children instead of before).
//
void RenderLayerStackingNode::computePaintOrderList(PaintOrderListType type, Vector<RefPtr<Node> >& list)
{
    OwnPtr<Vector<RenderLayer*> > posZOrderList;
    OwnPtr<Vector<RenderLayer*> > negZOrderList;

    RenderLayerStackingNode* stackingNode = layer()->ancestorStackingNode();
    if (!stackingNode)
        return;

    switch (type) {
    case BeforePromote:
        collectBeforePromotionZOrderList(stackingNode, posZOrderList, negZOrderList);
        break;
    case AfterPromote:
        collectAfterPromotionZOrderList(stackingNode, posZOrderList, negZOrderList);
        break;
    }

    if (negZOrderList) {
        for (size_t index = 0; index < negZOrderList->size(); ++index)
            list.append(negZOrderList->at(index)->renderer()->node());
    }

    if (posZOrderList) {
        for (size_t index = 0; index < posZOrderList->size(); ++index)
            list.append(posZOrderList->at(index)->renderer()->node());
    }
}

bool RenderLayerStackingNode::descendantsAreContiguousInStackingOrder() const
{
    if (isStackingContext() || !layer()->ancestorStackingContainerLayer())
        return true;

    ASSERT(!m_descendantsAreContiguousInStackingOrderDirty);
    return m_descendantsAreContiguousInStackingOrder;
}

bool RenderLayerStackingNode::setNeedsToBeStackingContainer(bool needsToBeStackingContainer)
{
    if (m_needsToBeStackingContainer == needsToBeStackingContainer)
        return false;

    // Count the total number of RenderLayers which need to be stacking
    // containers some point. This should be recorded at most once per
    // RenderLayer, so we check m_needsToBeStackingContainerHasBeenRecorded.
    if (layer()->acceleratedCompositingForOverflowScrollEnabled() && !m_needsToBeStackingContainerHasBeenRecorded) {
        HistogramSupport::histogramEnumeration("Renderer.CompositedScrolling", RenderLayer::NeedsToBeStackingContainerBucket, RenderLayer::CompositedScrollingHistogramMax);
        m_needsToBeStackingContainerHasBeenRecorded = true;
    }

    m_needsToBeStackingContainer = needsToBeStackingContainer;

    return true;
}

RenderLayerModelObject* RenderLayerStackingNode::renderer() const
{
    return m_layer->renderer();
}

} // namespace WebCore
