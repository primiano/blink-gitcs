/*
 * Copyright (C) 2009, 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
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

#include "config.h"

#include "core/rendering/compositing/RenderLayerCompositor.h"

#include "CSSPropertyNames.h"
#include "HTMLNames.h"
#include "RuntimeEnabledFeatures.h"
#include "core/animation/ActiveAnimations.h"
#include "core/animation/DocumentAnimations.h"
#include "core/dom/FullscreenElementStack.h"
#include "core/dom/NodeList.h"
#include "core/dom/ScriptForbiddenScope.h"
#include "core/frame/DeprecatedScheduleStyleRecalcDuringCompositingUpdate.h"
#include "core/frame/FrameView.h"
#include "core/frame/LocalFrame.h"
#include "core/frame/Settings.h"
#include "core/html/HTMLCanvasElement.h"
#include "core/html/HTMLIFrameElement.h"
#include "core/html/HTMLMediaElement.h"
#include "core/html/canvas/CanvasRenderingContext.h"
#include "core/inspector/InspectorInstrumentation.h"
#include "core/page/Chrome.h"
#include "core/page/Page.h"
#include "core/page/scrolling/ScrollingConstraints.h"
#include "core/page/scrolling/ScrollingCoordinator.h"
#include "core/rendering/HitTestResult.h"
#include "core/rendering/RenderApplet.h"
#include "core/rendering/RenderEmbeddedObject.h"
#include "core/rendering/RenderFullScreen.h"
#include "core/rendering/RenderIFrame.h"
#include "core/rendering/RenderLayerStackingNode.h"
#include "core/rendering/RenderLayerStackingNodeIterator.h"
#include "core/rendering/RenderReplica.h"
#include "core/rendering/RenderVideo.h"
#include "core/rendering/RenderView.h"
#include "core/rendering/compositing/CompositedLayerMapping.h"
#include "core/rendering/compositing/CompositingLayerAssigner.h"
#include "core/rendering/compositing/CompositingRequirementsUpdater.h"
#include "core/rendering/compositing/GraphicsLayerUpdater.h"
#include "platform/OverscrollTheme.h"
#include "platform/TraceEvent.h"
#include "platform/geometry/TransformState.h"
#include "platform/graphics/GraphicsLayer.h"
#include "platform/scroll/ScrollbarTheme.h"
#include "public/platform/Platform.h"
#include "wtf/TemporaryChange.h"

#ifndef NDEBUG
#include "core/rendering/RenderTreeAsText.h"
#endif

namespace WebCore {

using namespace HTMLNames;

class DeprecatedDirtyCompositingDuringCompositingUpdate {
    WTF_MAKE_NONCOPYABLE(DeprecatedDirtyCompositingDuringCompositingUpdate);
public:
    DeprecatedDirtyCompositingDuringCompositingUpdate(DocumentLifecycle& lifecycle)
        : m_lifecycle(lifecycle)
        , m_deprecatedTransition(lifecycle.state(), DocumentLifecycle::LayoutClean)
        , m_originalState(lifecycle.state())
    {
    }

    ~DeprecatedDirtyCompositingDuringCompositingUpdate()
    {
        if (m_originalState != DocumentLifecycle::InCompositingUpdate)
            return;
        if (m_lifecycle.state() != m_originalState) {
            // FIXME: It's crazy that we can trigger a style recalc from inside
            // the compositing update, but that happens in compositing/visibility/hidden-iframe.html.
            ASSERT(m_lifecycle.state() == DocumentLifecycle::LayoutClean || m_lifecycle.state() == DocumentLifecycle::VisualUpdatePending);
            m_lifecycle.advanceTo(m_originalState);
        }
    }

private:
    DocumentLifecycle& m_lifecycle;
    DocumentLifecycle::DeprecatedTransition m_deprecatedTransition;
    DocumentLifecycle::State m_originalState;
};

RenderLayerCompositor::RenderLayerCompositor(RenderView& renderView)
    : m_renderView(renderView)
    , m_compositingReasonFinder(renderView)
    , m_pendingUpdateType(CompositingUpdateNone)
    , m_hasAcceleratedCompositing(true)
    , m_needsToRecomputeCompositingRequirements(false)
    , m_compositing(false)
    , m_compositingLayersNeedRebuild(false)
    , m_rootShouldAlwaysCompositeDirty(true)
    , m_needsUpdateCompositingRequirementsState(false)
    , m_needsUpdateFixedBackground(false)
    , m_isTrackingRepaints(false)
    , m_rootLayerAttachment(RootLayerUnattached)
{
    updateAcceleratedCompositingSettings();
}

RenderLayerCompositor::~RenderLayerCompositor()
{
    ASSERT(m_rootLayerAttachment == RootLayerUnattached);
}

bool RenderLayerCompositor::inCompositingMode() const
{
    // FIXME: This should assert that lificycle is >= CompositingClean since
    // the last step of updateIfNeeded can set this bit to false.
    ASSERT(!m_rootShouldAlwaysCompositeDirty);
    return m_compositing;
}

bool RenderLayerCompositor::staleInCompositingMode() const
{
    return m_compositing;
}

void RenderLayerCompositor::setCompositingModeEnabled(bool enable)
{
    if (enable == m_compositing)
        return;

    m_compositing = enable;

    if (m_compositing)
        ensureRootLayer();
    else
        destroyRootLayer();

    notifyIFramesOfCompositingChange();
}

void RenderLayerCompositor::enableCompositingModeIfNeeded()
{
    if (!m_rootShouldAlwaysCompositeDirty)
        return;

    m_rootShouldAlwaysCompositeDirty = false;
    if (m_compositing)
        return;

    if (rootShouldAlwaysComposite()) {
        // FIXME: Is this needed? It was added in https://bugs.webkit.org/show_bug.cgi?id=26651.
        // No tests fail if it's deleted.
        setCompositingLayersNeedRebuild();
        setCompositingModeEnabled(true);
    }
}

bool RenderLayerCompositor::compositingLayersNeedRebuild()
{
    // enableCompositingModeIfNeeded can set the m_compositingLayersNeedRebuild bit.
    ASSERT(!m_rootShouldAlwaysCompositeDirty);
    return m_compositingLayersNeedRebuild;
}

bool RenderLayerCompositor::rootShouldAlwaysComposite() const
{
    Settings* settings = m_renderView.document().settings();
    bool shouldComposite = settings->forceCompositingMode() && m_hasAcceleratedCompositing;
    if (shouldComposite && !m_renderView.frame()->isMainFrame())
        return m_compositingReasonFinder.requiresCompositingForScrollableFrame();
    return shouldComposite;
}

void RenderLayerCompositor::updateAcceleratedCompositingSettings()
{
    m_compositingReasonFinder.updateTriggers();

    bool hasAcceleratedCompositing = m_renderView.document().settings()->acceleratedCompositingEnabled();

    // FIXME: Is this needed? It was added in https://bugs.webkit.org/show_bug.cgi?id=26651.
    // No tests fail if it's deleted.
    if (hasAcceleratedCompositing != m_hasAcceleratedCompositing)
        setCompositingLayersNeedRebuild();

    m_hasAcceleratedCompositing = hasAcceleratedCompositing;
    m_rootShouldAlwaysCompositeDirty = true;
}

bool RenderLayerCompositor::layerSquashingEnabled() const
{
    if (!RuntimeEnabledFeatures::layerSquashingEnabled())
        return false;
    if (Settings* settings = m_renderView.document().settings())
        return settings->layerSquashingEnabled();
    return true;
}

bool RenderLayerCompositor::legacyOrCurrentAcceleratedCompositingForOverflowScrollEnabled() const
{
    return legacyAcceleratedCompositingForOverflowScrollEnabled() || acceleratedCompositingForOverflowScrollEnabled();
}

bool RenderLayerCompositor::legacyAcceleratedCompositingForOverflowScrollEnabled() const
{
    return m_compositingReasonFinder.hasLegacyOverflowScrollTrigger();
}

bool RenderLayerCompositor::acceleratedCompositingForOverflowScrollEnabled() const
{
    return m_compositingReasonFinder.hasOverflowScrollTrigger();
}

void RenderLayerCompositor::setCompositingLayersNeedRebuild()
{
    // FIXME: crbug.com/332248 ideally this could be merged with setNeedsCompositingUpdate().
    // FIXME: We can remove the staleInCompositingMode check once we get rid of the
    // forceCompositingMode setting.
    if (staleInCompositingMode())
        m_compositingLayersNeedRebuild = true;
    page()->animator().scheduleVisualUpdate();
    lifecycle().ensureStateAtMost(DocumentLifecycle::LayoutClean);
}

void RenderLayerCompositor::updateCompositingRequirementsState()
{
    if (!m_needsUpdateCompositingRequirementsState)
        return;

    TRACE_EVENT0("blink_rendering,comp-scroll", "RenderLayerCompositor::updateCompositingRequirementsState");

    m_needsUpdateCompositingRequirementsState = false;

    if (!rootRenderLayer())
        return;

    if (!legacyOrCurrentAcceleratedCompositingForOverflowScrollEnabled())
        return;

    for (HashSet<RenderLayer*>::iterator it = m_outOfFlowPositionedLayers.begin(); it != m_outOfFlowPositionedLayers.end(); ++it)
        (*it)->updateHasUnclippedDescendant();

    const FrameView::ScrollableAreaSet* scrollableAreas = m_renderView.frameView()->scrollableAreas();
    if (!scrollableAreas)
        return;

    for (FrameView::ScrollableAreaSet::iterator it = scrollableAreas->begin(); it != scrollableAreas->end(); ++it)
        (*it)->updateNeedsCompositedScrolling();
}

static RenderVideo* findFullscreenVideoRenderer(Document& document)
{
    Element* fullscreenElement = FullscreenElementStack::fullscreenElementFrom(document);
    while (fullscreenElement && fullscreenElement->isFrameOwnerElement()) {
        Document* contentDocument = toHTMLFrameOwnerElement(fullscreenElement)->contentDocument();
        if (!contentDocument)
            return 0;
        fullscreenElement = FullscreenElementStack::fullscreenElementFrom(*contentDocument);
    }
    if (!isHTMLVideoElement(fullscreenElement))
        return 0;
    RenderObject* renderer = fullscreenElement->renderer();
    if (!renderer)
        return 0;
    return toRenderVideo(renderer);
}

void RenderLayerCompositor::updateIfNeededRecursive()
{
    for (LocalFrame* child = m_renderView.frameView()->frame().tree().firstChild(); child; child = child->tree().nextSibling())
        child->contentRenderer()->compositor()->updateIfNeededRecursive();

    TRACE_EVENT0("blink_rendering", "RenderLayerCompositor::updateIfNeededRecursive");

    ASSERT(!m_renderView.needsLayout());

    ScriptForbiddenScope forbidScript;

    lifecycle().advanceTo(DocumentLifecycle::InCompositingUpdate);

    updateIfNeeded();

    lifecycle().advanceTo(DocumentLifecycle::CompositingClean);

    DocumentAnimations::startPendingAnimations(m_renderView.document());
    // TODO: Figure out why this fails on Chrome OS login page. crbug.com/365507
    // ASSERT(lifecycle().state() == DocumentLifecycle::CompositingClean);
}

void RenderLayerCompositor::setNeedsCompositingUpdate(CompositingUpdateType updateType)
{
    ASSERT(updateType != CompositingUpdateNone);
    // FIXME: Technically we only need to do this when the FrameView's isScrollable method
    // would return a different value.
    if (updateType == CompositingUpdateAfterLayout)
        m_rootShouldAlwaysCompositeDirty = true;

    // FIXME: This function should only set dirty bits. We shouldn't
    // enable compositing mode here.
    // We check needsLayout here because we don't know if we need to enable
    // compositing mode until layout is up-to-date because we need to know
    // if this frame scrolls.
    if (!m_renderView.needsLayout())
        enableCompositingModeIfNeeded();

    m_pendingUpdateType = std::max(m_pendingUpdateType, updateType);

    page()->animator().scheduleVisualUpdate();
    lifecycle().ensureStateAtMost(DocumentLifecycle::LayoutClean);
}

void RenderLayerCompositor::scheduleAnimationIfNeeded()
{
    LocalFrame* localFrame = &m_renderView.frameView()->frame();
    for (LocalFrame* currentFrame = localFrame; currentFrame; currentFrame = currentFrame->tree().traverseNext(localFrame)) {
        if (currentFrame->contentRenderer()) {
            RenderLayerCompositor* childCompositor = currentFrame->contentRenderer()->compositor();
            if (childCompositor && childCompositor->hasUnresolvedDirtyBits()) {
                m_renderView.frameView()->scheduleAnimation();
                return;
            }
        }
    }
}

bool RenderLayerCompositor::hasUnresolvedDirtyBits()
{
    return m_needsToRecomputeCompositingRequirements || compositingLayersNeedRebuild() || m_needsUpdateCompositingRequirementsState || m_pendingUpdateType != CompositingUpdateNone;
}

void RenderLayerCompositor::updateIfNeeded()
{
    {
        // FIXME: Notice that we call this function before checking the dirty bits below.
        // We'll need to remove DeprecatedDirtyCompositingDuringCompositingUpdate
        // before moving this function after checking the dirty bits.
        DeprecatedDirtyCompositingDuringCompositingUpdate marker(lifecycle());
        updateCompositingRequirementsState();

        // FIXME: enableCompositingModeIfNeeded can call setCompositingLayersNeedRebuild,
        // which asserts that it's not InCompositingUpdate.
        enableCompositingModeIfNeeded();
    }

    CompositingUpdateType updateType = m_pendingUpdateType;
    bool needCompositingRequirementsUpdate = m_needsToRecomputeCompositingRequirements || updateType >= CompositingUpdateAfterCompositingInputChange;
    bool needHierarchyAndGeometryUpdate = compositingLayersNeedRebuild();

    m_pendingUpdateType = CompositingUpdateNone;
    m_compositingLayersNeedRebuild = false;
    m_needsToRecomputeCompositingRequirements = false;

    if (!hasAcceleratedCompositing() || (!needCompositingRequirementsUpdate && !m_compositing))
        return;

    bool needsToUpdateScrollingCoordinator = scrollingCoordinator() ? scrollingCoordinator()->needsToUpdateAfterCompositingChange() : false;
    if (updateType == CompositingUpdateNone && !needCompositingRequirementsUpdate && !needHierarchyAndGeometryUpdate && !needsToUpdateScrollingCoordinator)
        return;

    GraphicsLayerUpdater::UpdateType graphicsLayerUpdateType = GraphicsLayerUpdater::DoNotForceUpdate;
    CompositingPropertyUpdater::UpdateType compositingPropertyUpdateType = CompositingPropertyUpdater::DoNotForceUpdate;

    // FIXME: Teach non-style compositing updates how to do partial tree walks.
    if (updateType >= CompositingUpdateAfterLayout) {
        graphicsLayerUpdateType = GraphicsLayerUpdater::ForceUpdate;
        compositingPropertyUpdateType = CompositingPropertyUpdater::ForceUpdate;
    }

    RenderLayer* updateRoot = rootRenderLayer();

    if (needCompositingRequirementsUpdate) {
        bool layersChanged = false;

        {
            TRACE_EVENT0("blink_rendering", "CompositingPropertyUpdater::updateAncestorDependentProperties");
            CompositingPropertyUpdater(updateRoot).updateAncestorDependentProperties(updateRoot, compositingPropertyUpdateType, 0);
#if ASSERT_ENABLED
            CompositingPropertyUpdater::assertNeedsToUpdateAncestorDependantPropertiesBitsCleared(updateRoot);
#endif
        }

        CompositingRequirementsUpdater(m_renderView, m_compositingReasonFinder).update(updateRoot);

        {
            TRACE_EVENT0("blink_rendering", "CompositingLayerAssigner::assign");
            CompositingLayerAssigner(this).assign(updateRoot, layersChanged);
        }

        {
            TRACE_EVENT0("blink_rendering", "RenderLayerCompositor::updateAfterCompositingChange");
            const FrameView::ScrollableAreaSet* scrollableAreas = m_renderView.frameView()->scrollableAreas();
            if (scrollableAreas) {
                for (FrameView::ScrollableAreaSet::iterator it = scrollableAreas->begin(); it != scrollableAreas->end(); ++it)
                    (*it)->updateAfterCompositingChange();
            }
        }

        if (layersChanged)
            needHierarchyAndGeometryUpdate = true;
    }

    if (updateType != CompositingUpdateNone || needHierarchyAndGeometryUpdate) {
        TRACE_EVENT0("blink_rendering", "GraphicsLayerUpdater::updateRecursive");
        GraphicsLayerUpdater updater;
        updater.update(*updateRoot, graphicsLayerUpdateType);

        if (updater.needsRebuildTree())
            needHierarchyAndGeometryUpdate = true;

#if !ASSERT_DISABLED
        // FIXME: Move this check to the end of the compositing update.
        GraphicsLayerUpdater::assertNeedsToUpdateGraphicsLayerBitsCleared(*updateRoot);
#endif
    }

    if (needHierarchyAndGeometryUpdate) {
        // Update the hierarchy of the compositing layers.
        GraphicsLayerVector childList;
        {
            TRACE_EVENT0("blink_rendering", "GraphicsLayerUpdater::rebuildTree");
            GraphicsLayerUpdater().rebuildTree(*updateRoot, childList);
        }

        // Host the document layer in the RenderView's root layer.
        if (RuntimeEnabledFeatures::overlayFullscreenVideoEnabled() && m_renderView.frame()->isMainFrame()) {
            RenderVideo* video = findFullscreenVideoRenderer(m_renderView.document());
            GraphicsLayer* backgroundLayer = fixedRootBackgroundLayer();
            if (video && video->hasCompositedLayerMapping()) {
                childList.clear();
                childList.append(video->compositedLayerMapping()->mainGraphicsLayer());
                if (backgroundLayer && backgroundLayer->parent())
                    backgroundLayer->removeFromParent();
            } else {
                if (backgroundLayer && !backgroundLayer->parent())
                    rootFixedBackgroundsChanged();
            }
        }

        if (childList.isEmpty())
            destroyRootLayer();
        else
            m_rootContentLayer->setChildren(childList);
    }

    if (m_needsUpdateFixedBackground) {
        rootFixedBackgroundsChanged();
        m_needsUpdateFixedBackground = false;
    }

    ASSERT(updateRoot || !compositingLayersNeedRebuild());

    // The scrolling coordinator may realize that it needs updating while compositing was being updated in this function.
    needsToUpdateScrollingCoordinator |= scrollingCoordinator() && scrollingCoordinator()->needsToUpdateAfterCompositingChange();
    if (needsToUpdateScrollingCoordinator && m_renderView.frame()->isMainFrame() && scrollingCoordinator() && inCompositingMode())
        scrollingCoordinator()->updateAfterCompositingChange();

    // Inform the inspector that the layer tree has changed.
    if (m_renderView.frame()->isMainFrame())
        InspectorInstrumentation::layerTreeDidChange(m_renderView.frame());
}

void RenderLayerCompositor::addOutOfFlowPositionedLayer(RenderLayer* layer)
{
    m_outOfFlowPositionedLayers.add(layer);
}

void RenderLayerCompositor::removeOutOfFlowPositionedLayer(RenderLayer* layer)
{
    m_outOfFlowPositionedLayers.remove(layer);
}

bool RenderLayerCompositor::allocateOrClearCompositedLayerMapping(RenderLayer* layer, const CompositingStateTransitionType compositedLayerUpdate)
{
    bool compositedLayerMappingChanged = false;
    bool nonCompositedReasonChanged = updateLayerIfViewportConstrained(layer);

    // FIXME: It would be nice to directly use the layer's compositing reason,
    // but allocateOrClearCompositedLayerMapping also gets called without having updated compositing
    // requirements fully.
    switch (compositedLayerUpdate) {
    case AllocateOwnCompositedLayerMapping:
        ASSERT(!layer->hasCompositedLayerMapping());
        {
            // FIXME: This can go away once we get rid of the forceCompositingMode setting.
            // It's needed because setCompositingModeEnabled call ensureRootLayer, which
            // eventually calls WebViewImpl::enterForceCompositingMode.
            DeprecatedDirtyCompositingDuringCompositingUpdate marker(lifecycle());
            setCompositingModeEnabled(true);
        }

        // If this layer was previously squashed, we need to remove its reference to a groupedMapping right away, so
        // that computing repaint rects will know the layer's correct compositingState.
        // FIXME: do we need to also remove the layer from it's location in the squashing list of its groupedMapping?
        // Need to create a test where a squashed layer pops into compositing. And also to cover all other
        // sorts of compositingState transitions.
        layer->setLostGroupedMapping(false);
        layer->setGroupedMapping(0);

        // If we need to repaint, do so before allocating the compositedLayerMapping
        repaintOnCompositingChange(layer);
        layer->ensureCompositedLayerMapping();
        compositedLayerMappingChanged = true;

        // At this time, the ScrollingCooridnator only supports the top-level frame.
        if (layer->isRootLayer() && m_renderView.frame()->isMainFrame()) {
            if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
                scrollingCoordinator->frameViewRootLayerDidChange(m_renderView.frameView());
        }

        // FIXME: it seems premature to compute this before all compositing state has been updated?
        // This layer and all of its descendants have cached repaints rects that are relative to
        // the repaint container, so change when compositing changes; we need to update them here.
        if (layer->parent())
            layer->repainter().computeRepaintRectsIncludingDescendants();

        break;
    case RemoveOwnCompositedLayerMapping:
    // PutInSquashingLayer means you might have to remove the composited layer mapping first.
    case PutInSquashingLayer:
        if (layer->hasCompositedLayerMapping()) {
            // If we're removing the compositedLayerMapping from a reflection, clear the source GraphicsLayer's pointer to
            // its replica GraphicsLayer. In practice this should never happen because reflectee and reflection
            // are both either composited, or not composited.
            if (layer->isReflection()) {
                RenderLayer* sourceLayer = toRenderLayerModelObject(layer->renderer()->parent())->layer();
                if (sourceLayer->hasCompositedLayerMapping()) {
                    ASSERT(sourceLayer->compositedLayerMapping()->mainGraphicsLayer()->replicaLayer() == layer->compositedLayerMapping()->mainGraphicsLayer());
                    sourceLayer->compositedLayerMapping()->mainGraphicsLayer()->setReplicatedByLayer(0);
                }
            }

            layer->clearCompositedLayerMapping();
            compositedLayerMappingChanged = true;

            // This layer and all of its descendants have cached repaints rects that are relative to
            // the repaint container, so change when compositing changes; we need to update them here.
            layer->repainter().computeRepaintRectsIncludingDescendants();

            // If we need to repaint, do so now that we've removed the compositedLayerMapping
            repaintOnCompositingChange(layer);
        }

        break;
    case RemoveFromSquashingLayer:
    case NoCompositingStateChange:
        // Do nothing.
        break;
    }

    if (layer->hasCompositedLayerMapping() && layer->compositedLayerMapping()->updateRequiresOwnBackingStoreForIntrinsicReasons())
        compositedLayerMappingChanged = true;

    if (compositedLayerMappingChanged && layer->renderer()->isRenderPart()) {
        RenderLayerCompositor* innerCompositor = frameContentsCompositor(toRenderPart(layer->renderer()));
        if (innerCompositor && innerCompositor->staleInCompositingMode())
            innerCompositor->updateRootLayerAttachment();
    }

    if (compositedLayerMappingChanged)
        layer->clipper().clearClipRectsIncludingDescendants(PaintingClipRects);

    // If a fixed position layer gained/lost a compositedLayerMapping or the reason not compositing it changed,
    // the scrolling coordinator needs to recalculate whether it can do fast scrolling.
    if (compositedLayerMappingChanged || nonCompositedReasonChanged) {
        if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
            scrollingCoordinator->frameViewFixedObjectsDidChange(m_renderView.frameView());
    }

    return compositedLayerMappingChanged || nonCompositedReasonChanged;
}

bool RenderLayerCompositor::updateLayerIfViewportConstrained(RenderLayer* layer)
{
    RenderLayer::ViewportConstrainedNotCompositedReason viewportConstrainedNotCompositedReason = RenderLayer::NoNotCompositedReason;
    m_compositingReasonFinder.requiresCompositingForPosition(layer->renderer(), layer, &viewportConstrainedNotCompositedReason, &m_needsToRecomputeCompositingRequirements);

    if (layer->viewportConstrainedNotCompositedReason() != viewportConstrainedNotCompositedReason) {
        ASSERT(viewportConstrainedNotCompositedReason == RenderLayer::NoNotCompositedReason || layer->renderer()->style()->position() == FixedPosition);
        layer->setViewportConstrainedNotCompositedReason(viewportConstrainedNotCompositedReason);
        return true;
    }
    return false;
}

// These are temporary hacks to work around chicken-egg issues while we continue to refactor the compositing code.
// See crbug.com/339892 for a list of tests that fail if this method is removed.
void RenderLayerCompositor::applyUpdateLayerCompositingStateChickenEggHacks(RenderLayer* layer, CompositingStateTransitionType compositedLayerUpdate)
{
    if (compositedLayerUpdate != NoCompositingStateChange)
        allocateOrClearCompositedLayerMapping(layer, compositedLayerUpdate);
}

void RenderLayerCompositor::updateLayerCompositingState(RenderLayer* layer, UpdateLayerCompositingStateOptions options)
{
    updateDirectCompositingReasons(layer);
    CompositingStateTransitionType compositedLayerUpdate = CompositingLayerAssigner(this).computeCompositedLayerUpdate(layer);

    if (compositedLayerUpdate != NoCompositingStateChange) {
        setCompositingLayersNeedRebuild();
        setNeedsToRecomputeCompositingRequirements();
    }

    if (options == UseChickenEggHacks)
        applyUpdateLayerCompositingStateChickenEggHacks(layer, compositedLayerUpdate);
}

void RenderLayerCompositor::repaintOnCompositingChange(RenderLayer* layer)
{
    // If the renderer is not attached yet, no need to repaint.
    if (layer->renderer() != &m_renderView && !layer->renderer()->parent())
        return;

    const RenderLayerModelObject* repaintContainer = layer->renderer()->containerForRepaint();
    ASSERT(repaintContainer);
    layer->repainter().repaintIncludingNonCompositingDescendants(repaintContainer);
}

// This method assumes that layout is up-to-date, unlike repaintOnCompositingChange().
void RenderLayerCompositor::repaintInCompositedAncestor(RenderLayer* layer, const LayoutRect& rect)
{
    RenderLayer* compositedAncestor = layer->enclosingCompositingLayerForRepaint(ExcludeSelf);
    if (!compositedAncestor)
        return;
    ASSERT(compositedAncestor->compositingState() == PaintsIntoOwnBacking || compositedAncestor->compositingState() == PaintsIntoGroupedBacking);

    LayoutPoint offset;
    layer->convertToLayerCoords(compositedAncestor, offset);
    LayoutRect repaintRect = rect;
    repaintRect.moveBy(offset);
    compositedAncestor->repainter().setBackingNeedsRepaintInRect(repaintRect);
}

void RenderLayerCompositor::layerWasAdded(RenderLayer* /*parent*/, RenderLayer* /*child*/)
{
    setCompositingLayersNeedRebuild();
}

void RenderLayerCompositor::layerWillBeRemoved(RenderLayer* parent, RenderLayer* child)
{
    if (!child->hasCompositedLayerMapping() || parent->renderer()->documentBeingDestroyed())
        return;

    {
        // FIXME: This is called from within RenderLayer::removeChild, which is called from RenderObject::RemoveChild.
        // There's no guarantee that compositor state is up to date.
        DisableCompositingQueryAsserts disabler;
        repaintInCompositedAncestor(child, child->compositedLayerMapping()->compositedBounds());
    }

    setCompositingLayersNeedRebuild();
}

void RenderLayerCompositor::frameViewDidChangeLocation(const IntPoint& contentsOffset)
{
    if (m_overflowControlsHostLayer)
        m_overflowControlsHostLayer->setPosition(contentsOffset);
}

void RenderLayerCompositor::frameViewDidChangeSize()
{
    if (m_containerLayer) {
        FrameView* frameView = m_renderView.frameView();
        m_containerLayer->setSize(frameView->unscaledVisibleContentSize());

        frameViewDidScroll();
        updateOverflowControlsLayers();
    }
}

enum AcceleratedFixedRootBackgroundHistogramBuckets {
    ScrolledMainFrameBucket = 0,
    ScrolledMainFrameWithAcceleratedFixedRootBackground = 1,
    ScrolledMainFrameWithUnacceleratedFixedRootBackground = 2,
    AcceleratedFixedRootBackgroundHistogramMax = 3
};

void RenderLayerCompositor::frameViewDidScroll()
{
    FrameView* frameView = m_renderView.frameView();
    IntPoint scrollPosition = frameView->scrollPosition();

    if (!m_scrollLayer)
        return;

    bool scrollingCoordinatorHandlesOffset = false;
    if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator()) {
        if (Settings* settings = m_renderView.document().settings()) {
            if (m_renderView.frame()->isMainFrame() || settings->compositedScrollingForFramesEnabled())
                scrollingCoordinatorHandlesOffset = scrollingCoordinator->scrollableAreaScrollLayerDidChange(frameView);
        }
    }

    // Scroll position = scroll minimum + scroll offset. Adjust the layer's
    // position to handle whatever the scroll coordinator isn't handling.
    // The minimum scroll position is non-zero for RTL pages with overflow.
    if (scrollingCoordinatorHandlesOffset)
        m_scrollLayer->setPosition(-frameView->minimumScrollPosition());
    else
        m_scrollLayer->setPosition(-scrollPosition);


    blink::Platform::current()->histogramEnumeration("Renderer.AcceleratedFixedRootBackground",
        ScrolledMainFrameBucket,
        AcceleratedFixedRootBackgroundHistogramMax);
}

void RenderLayerCompositor::frameViewScrollbarsExistenceDidChange()
{
    if (m_containerLayer)
        updateOverflowControlsLayers();
}

void RenderLayerCompositor::rootFixedBackgroundsChanged()
{
    if (!supportsFixedRootBackgroundCompositing())
        return;

    // To avoid having to make the fixed root background layer fixed positioned to
    // stay put, we position it in the layer tree as follows:
    //
    // + Overflow controls host
    //   + LocalFrame clip
    //     + (Fixed root background) <-- Here.
    //     + LocalFrame scroll
    //       + Root content layer
    //   + Scrollbars
    //
    // That is, it needs to be the first child of the frame clip, the sibling of
    // the frame scroll layer. The compositor does not own the background layer, it
    // just positions it (like the foreground layer).
    if (GraphicsLayer* backgroundLayer = fixedRootBackgroundLayer())
        m_containerLayer->addChildBelow(backgroundLayer, m_scrollLayer.get());
}

bool RenderLayerCompositor::scrollingLayerDidChange(RenderLayer* layer)
{
    if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
        return scrollingCoordinator->scrollableAreaScrollLayerDidChange(layer->scrollableArea());
    return false;
}

String RenderLayerCompositor::layerTreeAsText(LayerTreeFlags flags)
{
    ASSERT(lifecycle().state() >= DocumentLifecycle::CompositingClean);

    if (!m_rootContentLayer)
        return String();

    // We skip dumping the scroll and clip layers to keep layerTreeAsText output
    // similar between platforms (unless we explicitly request dumping from the
    // root.
    GraphicsLayer* rootLayer = m_rootContentLayer.get();
    if (flags & LayerTreeIncludesRootLayer)
        rootLayer = rootGraphicsLayer();

    String layerTreeText = rootLayer->layerTreeAsText(flags);

    // The true root layer is not included in the dump, so if we want to report
    // its repaint rects, they must be included here.
    if (flags & LayerTreeIncludesRepaintRects)
        return m_renderView.frameView()->trackedRepaintRectsAsText() + layerTreeText;

    return layerTreeText;
}

RenderLayerCompositor* RenderLayerCompositor::frameContentsCompositor(RenderPart* renderer)
{
    if (!renderer->node()->isFrameOwnerElement())
        return 0;

    HTMLFrameOwnerElement* element = toHTMLFrameOwnerElement(renderer->node());
    if (Document* contentDocument = element->contentDocument()) {
        if (RenderView* view = contentDocument->renderView())
            return view->compositor();
    }
    return 0;
}

// FIXME: What does this function do? It needs a clearer name.
bool RenderLayerCompositor::parentFrameContentLayers(RenderPart* renderer)
{
    RenderLayerCompositor* innerCompositor = frameContentsCompositor(renderer);
    if (!innerCompositor || !innerCompositor->staleInCompositingMode() || innerCompositor->rootLayerAttachment() != RootLayerAttachedViaEnclosingFrame)
        return false;

    RenderLayer* layer = renderer->layer();
    if (!layer->hasCompositedLayerMapping())
        return false;

    CompositedLayerMappingPtr compositedLayerMapping = layer->compositedLayerMapping();
    GraphicsLayer* hostingLayer = compositedLayerMapping->parentForSublayers();
    GraphicsLayer* rootLayer = innerCompositor->rootGraphicsLayer();
    if (hostingLayer->children().size() != 1 || hostingLayer->children()[0] != rootLayer) {
        hostingLayer->removeAllChildren();
        hostingLayer->addChild(rootLayer);
    }
    return true;
}

void RenderLayerCompositor::repaintCompositedLayers()
{
    recursiveRepaintLayer(rootRenderLayer());
}

void RenderLayerCompositor::recursiveRepaintLayer(RenderLayer* layer)
{
    // FIXME: This method does not work correctly with transforms.
    if (layer->compositingState() == PaintsIntoOwnBacking)
        layer->repainter().setBackingNeedsRepaint();

#if !ASSERT_DISABLED
    LayerListMutationDetector mutationChecker(layer->stackingNode());
#endif

    unsigned childrenToVisit = NormalFlowChildren;
    if (layer->hasCompositingDescendant())
        childrenToVisit |= PositiveZOrderChildren | NegativeZOrderChildren;
    RenderLayerStackingNodeIterator iterator(*layer->stackingNode(), childrenToVisit);
    while (RenderLayerStackingNode* curNode = iterator.next())
        recursiveRepaintLayer(curNode->layer());
}

RenderLayer* RenderLayerCompositor::rootRenderLayer() const
{
    return m_renderView.layer();
}

GraphicsLayer* RenderLayerCompositor::rootGraphicsLayer() const
{
    if (m_overflowControlsHostLayer)
        return m_overflowControlsHostLayer.get();
    return m_rootContentLayer.get();
}

GraphicsLayer* RenderLayerCompositor::scrollLayer() const
{
    return m_scrollLayer.get();
}

GraphicsLayer* RenderLayerCompositor::containerLayer() const
{
    return m_containerLayer.get();
}

GraphicsLayer* RenderLayerCompositor::ensureRootTransformLayer()
{
    ASSERT(rootGraphicsLayer());

    if (!m_rootTransformLayer.get()) {
        m_rootTransformLayer = GraphicsLayer::create(graphicsLayerFactory(), this);
        m_overflowControlsHostLayer->addChild(m_rootTransformLayer.get());
        m_rootTransformLayer->addChild(m_containerLayer.get());
        updateOverflowControlsLayers();
    }

    return m_rootTransformLayer.get();
}

void RenderLayerCompositor::setIsInWindow(bool isInWindow)
{
    if (!staleInCompositingMode())
        return;

    if (isInWindow) {
        if (m_rootLayerAttachment != RootLayerUnattached)
            return;

        RootLayerAttachment attachment = m_renderView.frame()->isMainFrame() ? RootLayerAttachedViaChromeClient : RootLayerAttachedViaEnclosingFrame;
        attachRootLayer(attachment);
    } else {
        if (m_rootLayerAttachment == RootLayerUnattached)
            return;

        detachRootLayer();
    }
}

void RenderLayerCompositor::updateRootLayerPosition()
{
    if (m_rootContentLayer) {
        const IntRect& documentRect = m_renderView.documentRect();
        m_rootContentLayer->setSize(documentRect.size());
        m_rootContentLayer->setPosition(documentRect.location());
#if USE(RUBBER_BANDING)
        if (m_layerForOverhangShadow)
            OverscrollTheme::theme()->updateOverhangShadowLayer(m_layerForOverhangShadow.get(), m_rootContentLayer.get());
#endif
    }
    if (m_containerLayer) {
        FrameView* frameView = m_renderView.frameView();
        m_containerLayer->setSize(frameView->unscaledVisibleContentSize());
    }
}

void RenderLayerCompositor::updateStyleDeterminedCompositingReasons(RenderLayer* layer)
{
    CompositingReasons reasons = m_compositingReasonFinder.styleDeterminedReasons(layer->renderer());
    layer->setStyleDeterminedCompositingReasons(reasons);
}

void RenderLayerCompositor::updateDirectCompositingReasons(RenderLayer* layer)
{
    CompositingReasons reasons = m_compositingReasonFinder.directReasons(layer, &m_needsToRecomputeCompositingRequirements);
    layer->setCompositingReasons(reasons, CompositingReasonComboAllDirectReasons);
}

bool RenderLayerCompositor::canBeComposited(const RenderLayer* layer) const
{
    // FIXME: We disable accelerated compositing for elements in a RenderFlowThread as it doesn't work properly.
    // See http://webkit.org/b/84900 to re-enable it.
    return m_hasAcceleratedCompositing && layer->isSelfPaintingLayer() && !layer->subtreeIsInvisible() && layer->renderer()->flowThreadState() == RenderObject::NotInsideFlowThread;
}

// Return true if the given layer has some ancestor in the RenderLayer hierarchy that clips,
// up to the enclosing compositing ancestor. This is required because compositing layers are parented
// according to the z-order hierarchy, yet clipping goes down the renderer hierarchy.
// Thus, a RenderLayer can be clipped by a RenderLayer that is an ancestor in the renderer hierarchy,
// but a sibling in the z-order hierarchy.
bool RenderLayerCompositor::clippedByNonAncestorInStackingTree(const RenderLayer* layer) const
{
    if (!layer->hasCompositedLayerMapping() || !layer->parent())
        return false;

    const RenderLayer* compositingAncestor = layer->ancestorCompositingLayer();
    if (!compositingAncestor)
        return false;

    RenderObject* clippingContainer = layer->renderer()->clippingContainer();
    if (!clippingContainer)
        return false;

    if (compositingAncestor->renderer()->isDescendantOf(clippingContainer))
        return false;

    return true;
}

// Return true if the given layer is a stacking context and has compositing child
// layers that it needs to clip. In this case we insert a clipping GraphicsLayer
// into the hierarchy between this layer and its children in the z-order hierarchy.
bool RenderLayerCompositor::clipsCompositingDescendants(const RenderLayer* layer) const
{
    return layer->hasCompositingDescendant() && layer->renderer()->hasClipOrOverflowClip();
}

// If an element has negative z-index children, those children render in front of the
// layer background, so we need an extra 'contents' layer for the foreground of the layer
// object.
bool RenderLayerCompositor::needsContentsCompositingLayer(const RenderLayer* layer) const
{
    return layer->stackingNode()->hasNegativeZOrderList();
}

static void paintScrollbar(Scrollbar* scrollbar, GraphicsContext& context, const IntRect& clip)
{
    if (!scrollbar)
        return;

    context.save();
    const IntRect& scrollbarRect = scrollbar->frameRect();
    context.translate(-scrollbarRect.x(), -scrollbarRect.y());
    IntRect transformedClip = clip;
    transformedClip.moveBy(scrollbarRect.location());
    scrollbar->paint(&context, transformedClip);
    context.restore();
}

void RenderLayerCompositor::paintContents(const GraphicsLayer* graphicsLayer, GraphicsContext& context, GraphicsLayerPaintingPhase, const IntRect& clip)
{
    if (graphicsLayer == layerForHorizontalScrollbar())
        paintScrollbar(m_renderView.frameView()->horizontalScrollbar(), context, clip);
    else if (graphicsLayer == layerForVerticalScrollbar())
        paintScrollbar(m_renderView.frameView()->verticalScrollbar(), context, clip);
    else if (graphicsLayer == layerForScrollCorner()) {
        const IntRect& scrollCorner = m_renderView.frameView()->scrollCornerRect();
        context.save();
        context.translate(-scrollCorner.x(), -scrollCorner.y());
        IntRect transformedClip = clip;
        transformedClip.moveBy(scrollCorner.location());
        m_renderView.frameView()->paintScrollCorner(&context, transformedClip);
        context.restore();
    }
}

bool RenderLayerCompositor::supportsFixedRootBackgroundCompositing() const
{
    if (Settings* settings = m_renderView.document().settings()) {
        if (settings->acceleratedCompositingForFixedRootBackgroundEnabled())
            return true;
    }
    return false;
}

bool RenderLayerCompositor::needsFixedRootBackgroundLayer(const RenderLayer* layer) const
{
    if (layer != m_renderView.layer())
        return false;

    return supportsFixedRootBackgroundCompositing() && m_renderView.rootBackgroundIsEntirelyFixed();
}

GraphicsLayer* RenderLayerCompositor::fixedRootBackgroundLayer() const
{
    // Get the fixed root background from the RenderView layer's compositedLayerMapping.
    RenderLayer* viewLayer = m_renderView.layer();
    if (!viewLayer)
        return 0;

    if (viewLayer->compositingState() == PaintsIntoOwnBacking && viewLayer->compositedLayerMapping()->backgroundLayerPaintsFixedRootBackground())
        return viewLayer->compositedLayerMapping()->backgroundLayer();

    return 0;
}

static void resetTrackedRepaintRectsRecursive(GraphicsLayer* graphicsLayer)
{
    if (!graphicsLayer)
        return;

    graphicsLayer->resetTrackedRepaints();

    for (size_t i = 0; i < graphicsLayer->children().size(); ++i)
        resetTrackedRepaintRectsRecursive(graphicsLayer->children()[i]);

    if (GraphicsLayer* replicaLayer = graphicsLayer->replicaLayer())
        resetTrackedRepaintRectsRecursive(replicaLayer);

    if (GraphicsLayer* maskLayer = graphicsLayer->maskLayer())
        resetTrackedRepaintRectsRecursive(maskLayer);

    if (GraphicsLayer* clippingMaskLayer = graphicsLayer->contentsClippingMaskLayer())
        resetTrackedRepaintRectsRecursive(clippingMaskLayer);
}

void RenderLayerCompositor::resetTrackedRepaintRects()
{
    if (GraphicsLayer* rootLayer = rootGraphicsLayer())
        resetTrackedRepaintRectsRecursive(rootLayer);
}

void RenderLayerCompositor::setTracksRepaints(bool tracksRepaints)
{
    ASSERT(lifecycle().state() == DocumentLifecycle::CompositingClean);
    m_isTrackingRepaints = tracksRepaints;
}

bool RenderLayerCompositor::isTrackingRepaints() const
{
    return m_isTrackingRepaints;
}

static bool shouldCompositeOverflowControls(FrameView* view)
{
    if (Page* page = view->frame().page()) {
        if (ScrollingCoordinator* scrollingCoordinator = page->scrollingCoordinator())
            if (scrollingCoordinator->coordinatesScrollingForFrameView(view))
                return true;
    }

    return true;
}

bool RenderLayerCompositor::requiresHorizontalScrollbarLayer() const
{
    FrameView* view = m_renderView.frameView();
    return shouldCompositeOverflowControls(view) && view->horizontalScrollbar();
}

bool RenderLayerCompositor::requiresVerticalScrollbarLayer() const
{
    FrameView* view = m_renderView.frameView();
    return shouldCompositeOverflowControls(view) && view->verticalScrollbar();
}

bool RenderLayerCompositor::requiresScrollCornerLayer() const
{
    FrameView* view = m_renderView.frameView();
    return shouldCompositeOverflowControls(view) && view->isScrollCornerVisible();
}

#if USE(RUBBER_BANDING)
bool RenderLayerCompositor::requiresOverhangLayers() const
{
    // We don't want a layer if this is a subframe.
    if (!m_renderView.frame()->isMainFrame())
        return false;

    // We do want a layer if we have a scrolling coordinator and can scroll.
    if (scrollingCoordinator() && m_renderView.frameView()->hasOpaqueBackground())
        return true;

    // Chromium always wants a layer.
    return true;
}
#endif

void RenderLayerCompositor::updateOverflowControlsLayers()
{
#if USE(RUBBER_BANDING)
    if (requiresOverhangLayers()) {
        if (!m_layerForOverhangShadow) {
            m_layerForOverhangShadow = GraphicsLayer::create(graphicsLayerFactory(), this);
            OverscrollTheme::theme()->setUpOverhangShadowLayer(m_layerForOverhangShadow.get());
            OverscrollTheme::theme()->updateOverhangShadowLayer(m_layerForOverhangShadow.get(), m_rootContentLayer.get());
            m_scrollLayer->addChild(m_layerForOverhangShadow.get());
        }
    } else {
        if (m_layerForOverhangShadow) {
            m_layerForOverhangShadow->removeFromParent();
            m_layerForOverhangShadow = nullptr;
        }
    }
#endif
    GraphicsLayer* controlsParent = m_rootTransformLayer.get() ? m_rootTransformLayer.get() : m_overflowControlsHostLayer.get();

    if (requiresHorizontalScrollbarLayer()) {
        if (!m_layerForHorizontalScrollbar) {
            m_layerForHorizontalScrollbar = GraphicsLayer::create(graphicsLayerFactory(), this);
        }

        if (m_layerForHorizontalScrollbar->parent() != controlsParent) {
            controlsParent->addChild(m_layerForHorizontalScrollbar.get());

            if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
                scrollingCoordinator->scrollableAreaScrollbarLayerDidChange(m_renderView.frameView(), HorizontalScrollbar);
        }
    } else if (m_layerForHorizontalScrollbar) {
        m_layerForHorizontalScrollbar->removeFromParent();
        m_layerForHorizontalScrollbar = nullptr;

        if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
            scrollingCoordinator->scrollableAreaScrollbarLayerDidChange(m_renderView.frameView(), HorizontalScrollbar);
    }

    if (requiresVerticalScrollbarLayer()) {
        if (!m_layerForVerticalScrollbar) {
            m_layerForVerticalScrollbar = GraphicsLayer::create(graphicsLayerFactory(), this);
        }

        if (m_layerForVerticalScrollbar->parent() != controlsParent) {
            controlsParent->addChild(m_layerForVerticalScrollbar.get());

            if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
                scrollingCoordinator->scrollableAreaScrollbarLayerDidChange(m_renderView.frameView(), VerticalScrollbar);
        }
    } else if (m_layerForVerticalScrollbar) {
        m_layerForVerticalScrollbar->removeFromParent();
        m_layerForVerticalScrollbar = nullptr;

        if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
            scrollingCoordinator->scrollableAreaScrollbarLayerDidChange(m_renderView.frameView(), VerticalScrollbar);
    }

    if (requiresScrollCornerLayer()) {
        if (!m_layerForScrollCorner) {
            m_layerForScrollCorner = GraphicsLayer::create(graphicsLayerFactory(), this);
            controlsParent->addChild(m_layerForScrollCorner.get());
        }
    } else if (m_layerForScrollCorner) {
        m_layerForScrollCorner->removeFromParent();
        m_layerForScrollCorner = nullptr;
    }

    m_renderView.frameView()->positionScrollbarLayers();
}

void RenderLayerCompositor::ensureRootLayer()
{
    RootLayerAttachment expectedAttachment = m_renderView.frame()->isMainFrame() ? RootLayerAttachedViaChromeClient : RootLayerAttachedViaEnclosingFrame;
    if (expectedAttachment == m_rootLayerAttachment)
         return;

    if (!m_rootContentLayer) {
        m_rootContentLayer = GraphicsLayer::create(graphicsLayerFactory(), this);
        IntRect overflowRect = m_renderView.pixelSnappedLayoutOverflowRect();
        m_rootContentLayer->setSize(FloatSize(overflowRect.maxX(), overflowRect.maxY()));
        m_rootContentLayer->setPosition(FloatPoint());

        // Need to clip to prevent transformed content showing outside this frame
        m_rootContentLayer->setMasksToBounds(true);
    }

    if (!m_overflowControlsHostLayer) {
        ASSERT(!m_scrollLayer);
        ASSERT(!m_containerLayer);

        // Create a layer to host the clipping layer and the overflow controls layers.
        m_overflowControlsHostLayer = GraphicsLayer::create(graphicsLayerFactory(), this);

        // Create a clipping layer if this is an iframe or settings require to clip.
        m_containerLayer = GraphicsLayer::create(graphicsLayerFactory(), this);
        bool containerMasksToBounds = !m_renderView.frame()->isMainFrame();
        if (Settings* settings = m_renderView.document().settings()) {
            if (settings->mainFrameClipsContent())
                containerMasksToBounds = true;
        }
        m_containerLayer->setMasksToBounds(containerMasksToBounds);

        m_scrollLayer = GraphicsLayer::create(graphicsLayerFactory(), this);
        if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
            scrollingCoordinator->setLayerIsContainerForFixedPositionLayers(m_scrollLayer.get(), true);

        // Hook them up
        m_overflowControlsHostLayer->addChild(m_containerLayer.get());
        m_containerLayer->addChild(m_scrollLayer.get());
        m_scrollLayer->addChild(m_rootContentLayer.get());

        frameViewDidChangeSize();
    }

    // Check to see if we have to change the attachment
    if (m_rootLayerAttachment != RootLayerUnattached)
        detachRootLayer();

    attachRootLayer(expectedAttachment);
}

void RenderLayerCompositor::destroyRootLayer()
{
    if (!m_rootContentLayer)
        return;

    detachRootLayer();

#if USE(RUBBER_BANDING)
    if (m_layerForOverhangShadow) {
        m_layerForOverhangShadow->removeFromParent();
        m_layerForOverhangShadow = nullptr;
    }
#endif

    if (m_layerForHorizontalScrollbar) {
        m_layerForHorizontalScrollbar->removeFromParent();
        m_layerForHorizontalScrollbar = nullptr;
        if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
            scrollingCoordinator->scrollableAreaScrollbarLayerDidChange(m_renderView.frameView(), HorizontalScrollbar);
        if (Scrollbar* horizontalScrollbar = m_renderView.frameView()->verticalScrollbar())
            m_renderView.frameView()->invalidateScrollbar(horizontalScrollbar, IntRect(IntPoint(0, 0), horizontalScrollbar->frameRect().size()));
    }

    if (m_layerForVerticalScrollbar) {
        m_layerForVerticalScrollbar->removeFromParent();
        m_layerForVerticalScrollbar = nullptr;
        if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
            scrollingCoordinator->scrollableAreaScrollbarLayerDidChange(m_renderView.frameView(), VerticalScrollbar);
        if (Scrollbar* verticalScrollbar = m_renderView.frameView()->verticalScrollbar())
            m_renderView.frameView()->invalidateScrollbar(verticalScrollbar, IntRect(IntPoint(0, 0), verticalScrollbar->frameRect().size()));
    }

    if (m_layerForScrollCorner) {
        m_layerForScrollCorner = nullptr;
        m_renderView.frameView()->invalidateScrollCorner(m_renderView.frameView()->scrollCornerRect());
    }

    if (m_overflowControlsHostLayer) {
        m_overflowControlsHostLayer = nullptr;
        m_containerLayer = nullptr;
        m_scrollLayer = nullptr;
    }
    ASSERT(!m_scrollLayer);
    m_rootContentLayer = nullptr;
    m_rootTransformLayer = nullptr;
}

void RenderLayerCompositor::attachRootLayer(RootLayerAttachment attachment)
{
    if (!m_rootContentLayer)
        return;

    switch (attachment) {
        case RootLayerUnattached:
            ASSERT_NOT_REACHED();
            break;
        case RootLayerAttachedViaChromeClient: {
            LocalFrame& frame = m_renderView.frameView()->frame();
            Page* page = frame.page();
            if (!page)
                return;
            page->chrome().client().attachRootGraphicsLayer(rootGraphicsLayer());
            break;
        }
        case RootLayerAttachedViaEnclosingFrame: {
            HTMLFrameOwnerElement* ownerElement = m_renderView.document().ownerElement();
            ASSERT(ownerElement);
            DeprecatedScheduleStyleRecalcDuringCompositingUpdate marker(ownerElement->document().lifecycle());
            // The layer will get hooked up via CompositedLayerMapping::updateGraphicsLayerConfiguration()
            // for the frame's renderer in the parent document.
            ownerElement->scheduleLayerUpdate();
            break;
        }
    }

    m_rootLayerAttachment = attachment;
}

void RenderLayerCompositor::detachRootLayer()
{
    if (!m_rootContentLayer || m_rootLayerAttachment == RootLayerUnattached)
        return;

    switch (m_rootLayerAttachment) {
    case RootLayerAttachedViaEnclosingFrame: {
        // The layer will get unhooked up via CompositedLayerMapping::updateGraphicsLayerConfiguration()
        // for the frame's renderer in the parent document.
        if (m_overflowControlsHostLayer)
            m_overflowControlsHostLayer->removeFromParent();
        else
            m_rootContentLayer->removeFromParent();

        if (HTMLFrameOwnerElement* ownerElement = m_renderView.document().ownerElement()) {
            DeprecatedScheduleStyleRecalcDuringCompositingUpdate marker(ownerElement->document().lifecycle());
            ownerElement->scheduleLayerUpdate();
        }
        break;
    }
    case RootLayerAttachedViaChromeClient: {
        LocalFrame& frame = m_renderView.frameView()->frame();
        Page* page = frame.page();
        if (!page)
            return;
        page->chrome().client().attachRootGraphicsLayer(0);
    }
    break;
    case RootLayerUnattached:
        break;
    }

    m_rootLayerAttachment = RootLayerUnattached;
}

void RenderLayerCompositor::updateRootLayerAttachment()
{
    ensureRootLayer();
}

// IFrames are special, because we hook compositing layers together across iframe boundaries
// when both parent and iframe content are composited. So when this frame becomes composited, we have
// to use a synthetic style change to get the iframes into RenderLayers in order to allow them to composite.
void RenderLayerCompositor::notifyIFramesOfCompositingChange()
{
    if (!m_renderView.frameView())
        return;
    LocalFrame& frame = m_renderView.frameView()->frame();

    for (LocalFrame* child = frame.tree().firstChild(); child; child = child->tree().traverseNext(&frame)) {
        if (!child->document())
            continue; // FIXME: Can this happen?
        if (HTMLFrameOwnerElement* ownerElement = child->document()->ownerElement()) {
            DeprecatedScheduleStyleRecalcDuringCompositingUpdate marker(ownerElement->document().lifecycle());
            ownerElement->scheduleLayerUpdate();
        }
    }

    // Compositing also affects the answer to RenderIFrame::requiresAcceleratedCompositing(), so
    // we need to schedule a style recalc in our parent document.
    if (HTMLFrameOwnerElement* ownerElement = m_renderView.document().ownerElement()) {
        ownerElement->document().renderView()->compositor()->setNeedsToRecomputeCompositingRequirements();
        DeprecatedScheduleStyleRecalcDuringCompositingUpdate marker(ownerElement->document().lifecycle());
        ownerElement->scheduleLayerUpdate();
    }
}

ScrollingCoordinator* RenderLayerCompositor::scrollingCoordinator() const
{
    if (Page* page = this->page())
        return page->scrollingCoordinator();

    return 0;
}

GraphicsLayerFactory* RenderLayerCompositor::graphicsLayerFactory() const
{
    if (Page* page = this->page())
        return page->chrome().client().graphicsLayerFactory();
    return 0;
}

Page* RenderLayerCompositor::page() const
{
    return m_renderView.frameView()->frame().page();
}

DocumentLifecycle& RenderLayerCompositor::lifecycle() const
{
    return m_renderView.document().lifecycle();
}

String RenderLayerCompositor::debugName(const GraphicsLayer* graphicsLayer)
{
    String name;
    if (graphicsLayer == m_rootContentLayer.get()) {
        name = "Content Root Layer";
    } else if (graphicsLayer == m_rootTransformLayer.get()) {
        name = "Root Transform Layer";
#if USE(RUBBER_BANDING)
    } else if (graphicsLayer == m_layerForOverhangShadow.get()) {
        name = "Overhang Areas Shadow";
#endif
    } else if (graphicsLayer == m_overflowControlsHostLayer.get()) {
        name = "Overflow Controls Host Layer";
    } else if (graphicsLayer == m_layerForHorizontalScrollbar.get()) {
        name = "Horizontal Scrollbar Layer";
    } else if (graphicsLayer == m_layerForVerticalScrollbar.get()) {
        name = "Vertical Scrollbar Layer";
    } else if (graphicsLayer == m_layerForScrollCorner.get()) {
        name = "Scroll Corner Layer";
    } else if (graphicsLayer == m_containerLayer.get()) {
        name = "LocalFrame Clipping Layer";
    } else if (graphicsLayer == m_scrollLayer.get()) {
        name = "LocalFrame Scrolling Layer";
    } else {
        ASSERT_NOT_REACHED();
    }

    return name;
}

} // namespace WebCore
