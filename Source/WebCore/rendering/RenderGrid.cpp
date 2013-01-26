/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
#include "RenderGrid.h"

#include "LayoutRepainter.h"
#include "NotImplemented.h"
#include "RenderLayer.h"
#include "RenderView.h"

namespace WebCore {

static const int infinity = intMaxForLayoutUnit;

class GridTrack {
public:
    GridTrack()
        : m_usedBreadth(0)
        , m_maxBreadth(0)
    {
    }

    void growUsedBreadth(LayoutUnit growth)
    {
        ASSERT(growth >= 0);
        m_usedBreadth += growth;
    }
    LayoutUnit usedBreadth() const { return m_usedBreadth; }

    void growMaxBreadth(LayoutUnit growth)
    {
        if (m_maxBreadth == infinity)
            m_maxBreadth = m_usedBreadth + growth;
        else
            m_maxBreadth += growth;
    }
    LayoutUnit maxBreadthIfNotInfinite() const
    {
        return (m_maxBreadth == infinity) ? m_usedBreadth : m_maxBreadth;
    }

    LayoutUnit m_usedBreadth;
    LayoutUnit m_maxBreadth;
};

RenderGrid::RenderGrid(Element* element)
    : RenderBlock(element)
{
    // All of our children must be block level.
    setChildrenInline(false);
}

RenderGrid::~RenderGrid()
{
}

void RenderGrid::layoutBlock(bool relayoutChildren, LayoutUnit)
{
    ASSERT(needsLayout());

    if (!relayoutChildren && simplifiedLayout())
        return;

    // FIXME: Much of this method is boiler plate that matches RenderBox::layoutBlock and Render*FlexibleBox::layoutBlock.
    // It would be nice to refactor some of the duplicate code.
    LayoutRepainter repainter(*this, checkForRepaintDuringLayout());
    LayoutStateMaintainer statePusher(view(), this, locationOffset(), hasTransform() || hasReflection() || style()->isFlippedBlocksWritingMode());

    if (inRenderFlowThread()) {
        // Regions changing widths can force us to relayout our children.
        if (logicalWidthChangedInRegions())
            relayoutChildren = true;
    }
    updateRegionsAndExclusionsLogicalSize();

    LayoutSize previousSize = size();

    setLogicalHeight(0);
    updateLogicalWidth();

    m_overflow.clear();

    layoutGridItems();

    LayoutUnit oldClientAfterEdge = clientLogicalBottom();
    updateLogicalHeight();

    if (size() != previousSize)
        relayoutChildren = true;

    layoutPositionedObjects(relayoutChildren || isRoot());

    computeRegionRangeForBlock();

    computeOverflow(oldClientAfterEdge);
    statePusher.pop();

    updateLayerTransform();

    // Update our scroll information if we're overflow:auto/scroll/hidden now that we know if
    // we overflow or not.
    if (hasOverflowClip())
        layer()->updateScrollInfoAfterLayout();

    repainter.repaintAfterLayout();

    setNeedsLayout(false);
}

void RenderGrid::computePreferredLogicalWidths()
{
    ASSERT(preferredLogicalWidthsDirty());

    m_minPreferredLogicalWidth = 0;
    m_maxPreferredLogicalWidth = 0;

    // FIXME: We don't take our own logical width into account.

    const Vector<GridTrackSize>& trackStyles = style()->gridColumns();

    for (size_t i = 0; i < trackStyles.size(); ++i) {
        const Length& minTrackLength = trackStyles[i].minTrackBreadth();
        const Length& maxTrackLength = trackStyles[i].maxTrackBreadth();
        // FIXME: Handle only one fixed length properly (e.g minmax(100px, max-content)).
        if (!minTrackLength.isFixed() || !maxTrackLength.isFixed()) {
            notImplemented();
            continue;
        }

        LayoutUnit minTrackBreadth = minTrackLength.intValue();
        LayoutUnit maxTrackBreadth = maxTrackLength.intValue();

        maxTrackBreadth = std::max(maxTrackBreadth, minTrackBreadth);

        m_minPreferredLogicalWidth += minTrackBreadth;
        m_maxPreferredLogicalWidth += maxTrackBreadth;

        // FIXME: This should add in the scrollbarWidth (e.g. see RenderFlexibleBox).
    }

    // FIXME: We should account for min / max logical width.

    LayoutUnit borderAndPaddingInInlineDirection = borderAndPaddingLogicalWidth();
    m_minPreferredLogicalWidth += borderAndPaddingInInlineDirection;
    m_maxPreferredLogicalWidth += borderAndPaddingInInlineDirection;

    setPreferredLogicalWidthsDirty(false);
}

void RenderGrid::computedUsedBreadthOfGridTracks(TrackSizingDirection direction, Vector<GridTrack>& columnTracks, Vector<GridTrack>& rowTracks)
{
    const Vector<GridTrackSize>& trackStyles = (direction == ForColumns) ? style()->gridColumns() : style()->gridRows();
    LayoutUnit availableLogicalSpace = (direction == ForColumns) ? availableLogicalWidth() : availableLogicalHeight(IncludeMarginBorderPadding);
    Vector<GridTrack>& tracks = (direction == ForColumns) ? columnTracks : rowTracks;
    for (size_t i = 0; i < trackStyles.size(); ++i) {
        GridTrack track;
        const Length& minTrackBreadth = trackStyles[i].minTrackBreadth();
        const Length& maxTrackBreadth = trackStyles[i].maxTrackBreadth();

        track.m_usedBreadth = computeUsedBreadthOfMinLength(direction, minTrackBreadth);
        track.m_maxBreadth = computeUsedBreadthOfMaxLength(direction, maxTrackBreadth);

        track.m_maxBreadth = std::max(track.m_maxBreadth, track.m_usedBreadth);

        availableLogicalSpace -= track.m_usedBreadth;

        tracks.append(track);
    }

    // FIXME: We shouldn't call resolveContentBasedTrackSizingFunctions if we have no min-content / max-content tracks.
    resolveContentBasedTrackSizingFunctions(direction, columnTracks, rowTracks, availableLogicalSpace);

    if (availableLogicalSpace <= 0)
        return;

    distributeSpaceToTracks(direction, tracks, availableLogicalSpace);
}

LayoutUnit RenderGrid::computeUsedBreadthOfMinLength(TrackSizingDirection direction, const Length& trackLength) const
{
    if (trackLength.isFixed() || trackLength.isPercent() || trackLength.isViewportPercentage())
        return computeUsedBreadthOfSpecifiedLength(direction, trackLength);

    ASSERT(trackLength.isMinContent() || trackLength.isMaxContent() || trackLength.isAuto());
    return 0;
}

LayoutUnit RenderGrid::computeUsedBreadthOfMaxLength(TrackSizingDirection direction, const Length& trackLength) const
{
    if (trackLength.isFixed() || trackLength.isPercent() || trackLength.isViewportPercentage()) {
        LayoutUnit computedBreadth = computeUsedBreadthOfSpecifiedLength(direction, trackLength);
        // FIXME: We should ASSERT that computedBreadth cannot return infinity but it's currently
        // possible. See https://bugs.webkit.org/show_bug.cgi?id=107053
        return computedBreadth;
    }

    ASSERT(trackLength.isMinContent() || trackLength.isMaxContent() || trackLength.isAuto());
    return infinity;
}

LayoutUnit RenderGrid::computeUsedBreadthOfSpecifiedLength(TrackSizingDirection direction, const Length& trackLength) const
{
    // FIXME: We still need to support calc() here (https://webkit.org/b/103761).
    ASSERT(trackLength.isFixed() || trackLength.isPercent() || trackLength.isViewportPercentage());
    return valueForLength(trackLength, direction == ForColumns ? logicalWidth() : computeContentLogicalHeight(MainOrPreferredSize, style()->logicalHeight()), view());
}

static bool sortByGridTrackGrowthPotential(GridTrack* track1, GridTrack* track2)
{
    return (track1->m_maxBreadth - track1->m_usedBreadth) <= (track2->m_maxBreadth - track2->m_usedBreadth);
}

LayoutUnit RenderGrid::minContentForChild(RenderBox* child, TrackSizingDirection direction, Vector<GridTrack>& columnTracks)
{
    bool hasOrthogonalWritingMode = child->isHorizontalWritingMode() != isHorizontalWritingMode();
    // FIXME: Properly support orthogonal writing mode.
    if (hasOrthogonalWritingMode)
        return 0;

    if (direction == ForColumns) {
        // FIXME: It's unclear if we should return the intrinsic width or the preferred width.
        // See http://lists.w3.org/Archives/Public/www-style/2013Jan/0245.html
        return child->minPreferredLogicalWidth();
    }

    if (child->needsLayout()) {
        size_t columnTrack = resolveGridPosition(ForColumns, child);
        child->setOverrideContainingBlockContentLogicalWidth(columnTracks[columnTrack].m_usedBreadth);
        child->layout();
    }
    return child->logicalHeight();
}

LayoutUnit RenderGrid::maxContentForChild(RenderBox* child, TrackSizingDirection direction, Vector<GridTrack>& columnTracks)
{
    bool hasOrthogonalWritingMode = child->isHorizontalWritingMode() != isHorizontalWritingMode();
    // FIXME: Properly support orthogonal writing mode.
    if (hasOrthogonalWritingMode)
        return LayoutUnit();

    if (direction == ForColumns) {
        // FIXME: It's unclear if we should return the intrinsic width or the preferred width.
        // See http://lists.w3.org/Archives/Public/www-style/2013Jan/0245.html
        return child->maxPreferredLogicalWidth();
    }

    if (child->needsLayout()) {
        size_t columnTrack = resolveGridPosition(ForColumns, child);
        child->setOverrideContainingBlockContentLogicalWidth(columnTracks[columnTrack].m_usedBreadth);
        child->layout();
    }
    return child->logicalHeight();
}

void RenderGrid::resolveContentBasedTrackSizingFunctions(TrackSizingDirection direction, Vector<GridTrack>& columnTracks, Vector<GridTrack>& rowTracks, LayoutUnit& availableLogicalSpace)
{
    // FIXME: Split the grid tracks once we support spanning or fractions (step 1 and 2 of the algorithm).

    const Vector<GridTrackSize>& trackStyles = (direction == ForColumns) ? style()->gridColumns() : style()->gridRows();
    Vector<GridTrack>& tracks = (direction == ForColumns) ? columnTracks : rowTracks;

    for (size_t i = 0; i < tracks.size(); ++i) {
        GridTrack& track = tracks[i];
        const Length& minTrackBreadth = trackStyles[i].minTrackBreadth();
        if (minTrackBreadth.isMinContent() || minTrackBreadth.isMaxContent()) {
            LayoutUnit oldUsedBreadth = track.m_usedBreadth;
            resolveContentBasedTrackSizingFunctionsForItems(direction, columnTracks, rowTracks, i, &RenderGrid::minContentForChild, &GridTrack::usedBreadth, &GridTrack::growUsedBreadth);
            availableLogicalSpace -= (track.m_usedBreadth - oldUsedBreadth);
        }

        if (minTrackBreadth.isMaxContent()) {
            LayoutUnit oldUsedBreadth = track.m_usedBreadth;
            resolveContentBasedTrackSizingFunctionsForItems(direction, columnTracks, rowTracks, i, &RenderGrid::maxContentForChild, &GridTrack::usedBreadth, &GridTrack::growUsedBreadth);
            availableLogicalSpace -= (track.m_usedBreadth - oldUsedBreadth);
        }

        const Length& maxTrackBreadth = trackStyles[i].maxTrackBreadth();
        if (maxTrackBreadth.isMinContent() || maxTrackBreadth.isMaxContent())
            resolveContentBasedTrackSizingFunctionsForItems(direction, columnTracks, rowTracks, i, &RenderGrid::minContentForChild, &GridTrack::maxBreadthIfNotInfinite, &GridTrack::growMaxBreadth);

        if (maxTrackBreadth.isMaxContent())
            resolveContentBasedTrackSizingFunctionsForItems(direction, columnTracks, rowTracks, i, &RenderGrid::maxContentForChild, &GridTrack::maxBreadthIfNotInfinite, &GridTrack::growMaxBreadth);
    }

    // FIXME: The spec says to update maxBreadth if it is Infinity.
}

void RenderGrid::resolveContentBasedTrackSizingFunctionsForItems(TrackSizingDirection direction, Vector<GridTrack>& columnTracks, Vector<GridTrack>& rowTracks, size_t i, SizingFunction sizingFunction, AccumulatorGetter trackGetter, AccumulatorGrowFunction trackGrowthFunction)
{
    // FIXME: The specification re-uses distributeSpaceToTrack, which we should probably do.
    GridTrack& track = (direction == ForColumns) ? columnTracks[i] : rowTracks[i];
    for (RenderBox* child = firstChildBox(); child; child = child->nextSiblingBox()) {
        size_t cellIndex = resolveGridPosition(direction, child);
        if (cellIndex != i)
            continue;

        LayoutUnit contentSize = (this->*sizingFunction)(child, direction, columnTracks);
        LayoutUnit additionalBreadthSpace = contentSize - (track.*trackGetter)();
        LayoutUnit share = additionalBreadthSpace;
        std::min(additionalBreadthSpace, track.m_maxBreadth - (track.*trackGetter)());
        (track.*trackGrowthFunction)(share);
    }
}

void RenderGrid::distributeSpaceToTracks(TrackSizingDirection, Vector<GridTrack>& tracks, LayoutUnit availableLogicalSpace)
{
    const size_t tracksSize = tracks.size();
    Vector<GridTrack*> sortedTracks(tracksSize);
    for (size_t i = 0; i < tracksSize; ++i)
        sortedTracks[i] = tracks.data() + i;

    std::sort(sortedTracks.begin(), sortedTracks.end(), sortByGridTrackGrowthPotential);

    for (size_t i = 0; i < tracksSize; ++i) {
        GridTrack& track = *sortedTracks[i];
        LayoutUnit availableLogicalSpaceShare = availableLogicalSpace / (tracksSize - i);
        // We never shrink the used breadth by clamping the difference between max and used breadth. The spec uses
        // 2 extra-variables and 2 extra iterations over the tracks to achieve that (because distributeSpaceToTracks
        // is shared with other methods). If we start sharing the method, we should probably remove this clamping.
        LayoutUnit growthShare = std::min(availableLogicalSpaceShare, std::max(LayoutUnit(0), track.m_maxBreadth - track.m_usedBreadth));
        track.m_usedBreadth += growthShare;
        availableLogicalSpace -= growthShare;
    }

    // FIXME: We don't implement the last 2 steps of the algorithm as we don't implement content based sizing.
}

#ifndef NDEBUG
bool RenderGrid::tracksAreWiderThanMinTrackBreadth(TrackSizingDirection direction, const Vector<GridTrack>& tracks)
{
    const Vector<GridTrackSize>& trackStyles = (direction == ForColumns) ? style()->gridColumns() : style()->gridRows();
    for (size_t i = 0; i < trackStyles.size(); ++i) {
        const Length& minTrackBreadth = trackStyles[i].minTrackBreadth();
        if (computeUsedBreadthOfMinLength(direction, minTrackBreadth) > tracks[i].m_usedBreadth)
            return false;
    }
    return true;
}
#endif

void RenderGrid::layoutGridItems()
{
    Vector<GridTrack> columnTracks, rowTracks;
    computedUsedBreadthOfGridTracks(ForColumns, columnTracks, rowTracks);
    ASSERT(tracksAreWiderThanMinTrackBreadth(ForColumns, columnTracks));
    computedUsedBreadthOfGridTracks(ForRows, columnTracks, rowTracks);
    ASSERT(tracksAreWiderThanMinTrackBreadth(ForRows, rowTracks));

    for (RenderBox* child = firstChildBox(); child; child = child->nextSiblingBox()) {
        LayoutPoint childPosition = findChildLogicalPosition(child, columnTracks, rowTracks);

        size_t columnTrack = resolveGridPosition(child->style()->gridItemColumn());
        size_t rowTrack = resolveGridPosition(child->style()->gridItemRow());

        // FIXME: Properly support implicit rows and columns (bug 103573).
        if (columnTrack < columnTracks.size() && rowTrack < rowTracks.size()) {
            // Because the grid area cannot be styled, we don't need to adjust
            // the grid breadth to account for 'box-sizing'.
            LayoutUnit oldOverrideContainingBlockContentLogicalWidth = child->hasOverrideContainingBlockLogicalWidth() ? child->overrideContainingBlockContentLogicalWidth() : LayoutUnit();
            LayoutUnit oldOverrideContainingBlockContentLogicalHeight = child->hasOverrideContainingBlockLogicalHeight() ? child->overrideContainingBlockContentLogicalHeight() : LayoutUnit();

            if (oldOverrideContainingBlockContentLogicalWidth != columnTracks[columnTrack].m_usedBreadth || oldOverrideContainingBlockContentLogicalHeight != rowTracks[rowTrack].m_usedBreadth)
                child->setNeedsLayout(true, MarkOnlyThis);

            child->setOverrideContainingBlockContentLogicalWidth(columnTracks[columnTrack].m_usedBreadth);
            child->setOverrideContainingBlockContentLogicalHeight(rowTracks[rowTrack].m_usedBreadth);

        }

        // FIXME: Grid items should stretch to fill their cells. Once we
        // implement grid-{column,row}-align, we can also shrink to fit. For
        // now, just size as if we were a regular child.
        child->layoutIfNeeded();

        // FIXME: Handle border & padding on the grid element.
        child->setLogicalLocation(childPosition);
    }

    for (size_t i = 0; i < rowTracks.size(); ++i)
        setLogicalHeight(logicalHeight() + rowTracks[i].m_usedBreadth);

    // FIXME: We should handle min / max logical height.

    setLogicalHeight(logicalHeight() + borderAndPaddingLogicalHeight());
}

size_t RenderGrid::resolveGridPosition(TrackSizingDirection direction, const RenderObject* gridItem) const
{
    const GridPosition& position = (direction == ForColumns) ? gridItem->style()->gridItemColumn() : gridItem->style()->gridItemRow();
    return resolveGridPosition(position);
}

size_t RenderGrid::resolveGridPosition(const GridPosition& position) const
{
    // FIXME: Handle other values for grid-{row,column} like ranges or line names.
    switch (position.type()) {
    case IntegerPosition:
        // FIXME: What does a non-positive integer mean for a column/row?
        if (!position.isPositive())
            return 0;

        return position.integerPosition() - 1;
    case AutoPosition:
        // FIXME: We should follow 'grid-auto-flow' for resolution.
        // Until then, we use the 'grid-auto-flow: none' behavior (which is the default)
        // and resolve 'auto' as the first row / column.
        return 0;
    }
    ASSERT_NOT_REACHED();
    return 0;
}

LayoutPoint RenderGrid::findChildLogicalPosition(RenderBox* child, const Vector<GridTrack>& columnTracks, const Vector<GridTrack>& rowTracks)
{
    size_t columnTrack = resolveGridPosition(child->style()->gridItemColumn());
    size_t rowTrack = resolveGridPosition(child->style()->gridItemRow());

    LayoutPoint offset;
    // FIXME: |columnTrack| and |rowTrack| should be smaller than our column / row count.
    for (size_t i = 0; i < columnTrack && i < columnTracks.size(); ++i)
        offset.setX(offset.x() + columnTracks[i].m_usedBreadth);
    for (size_t i = 0; i < rowTrack && i < rowTracks.size(); ++i)
        offset.setY(offset.y() + rowTracks[i].m_usedBreadth);

    // FIXME: Handle margins on the grid item.
    return offset;
}

const char* RenderGrid::renderName() const
{
    if (isFloating())
        return "RenderGrid (floating)";
    if (isOutOfFlowPositioned())
        return "RenderGrid (positioned)";
    if (isAnonymous())
        return "RenderGrid (generated)";
    if (isRelPositioned())
        return "RenderGrid (relative positioned)";
    return "RenderGrid";
}

} // namespace WebCore
