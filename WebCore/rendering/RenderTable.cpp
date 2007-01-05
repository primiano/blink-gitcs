/**
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1997 Martin Jones (mjones@kde.org)
 *           (C) 1997 Torben Weis (weis@kde.org)
 *           (C) 1998 Waldo Bastian (bastian@kde.org)
 *           (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006 Apple Computer, Inc.
 * Copyright (C) 2006 Alexey Proskuryakov (ap@nypop.com)
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
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include "RenderTable.h"

#include "AutoTableLayout.h"
#include "DeleteButtonController.h"
#include "Document.h"
#include "FixedTableLayout.h"
#include "HTMLNames.h"
#include "RenderTableCell.h"
#include "RenderTableCol.h"
#include "RenderTableSection.h"
#include "TextStream.h"

using namespace std;

namespace WebCore {

using namespace HTMLNames;

RenderTable::RenderTable(Node* node)
    : RenderBlock(node)
    , m_caption(0)
    , m_head(0)
    , m_foot(0)
    , m_firstBody(0)
    , m_tableLayout(0)
    , m_currentBorder(0)
    , m_frame(Void)
    , m_rules(None)
    , m_hasColElements(false)
    , m_padding(0)
    , m_needsSectionRecalc(0)
    , m_hSpacing(0)
    , m_vSpacing(0)
    , m_borderLeft(0)
    , m_borderRight(0)
{
    m_columnPos.fill(0, 2);
    m_columns.fill(ColumnStruct(), 1);
}

RenderTable::~RenderTable()
{
    delete m_tableLayout;
}

void RenderTable::setStyle(RenderStyle* newStyle)
{
    ETableLayout oldTableLayout = style() ? style()->tableLayout() : TAUTO;
    RenderBlock::setStyle(newStyle);

    // In the collapsed border model, there is no cell spacing.
    m_hSpacing = collapseBorders() ? 0 : style()->horizontalBorderSpacing();
    m_vSpacing = collapseBorders() ? 0 : style()->verticalBorderSpacing();
    m_columnPos[0] = m_hSpacing;

    if (!m_tableLayout || style()->tableLayout() != oldTableLayout) {
        delete m_tableLayout;

        // According to the CSS2 spec, you only use fixed table layout if an
        // explicit width is specified on the table.  Auto width implies auto table layout.
        if (style()->tableLayout() == TFIXED && !style()->width().isAuto())
            m_tableLayout = new FixedTableLayout(this);
        else
            m_tableLayout = new AutoTableLayout(this);
    }
}

void RenderTable::addChild(RenderObject* child, RenderObject* beforeChild)
{
    bool wrapInAnonymousSection = true;
    bool isTableElement = element() && element()->hasTagName(tableTag);

    switch (child->style()->display()) {
        case TABLE_CAPTION:
            if (child->isRenderBlock())
                m_caption = static_cast<RenderBlock*>(child);
            wrapInAnonymousSection = false;
            break;
        case TABLE_COLUMN:
        case TABLE_COLUMN_GROUP:
            m_hasColElements = true;
            wrapInAnonymousSection = false;
            break;
        case TABLE_HEADER_GROUP:
            if (!m_head) {
                if (child->isTableSection())
                    m_head = static_cast<RenderTableSection*>(child);
            } else if (!m_firstBody) {
                if (child->isTableSection())
                    m_firstBody = static_cast<RenderTableSection*>(child);
            }
            wrapInAnonymousSection = false;
            break;
        case TABLE_FOOTER_GROUP:
            if (!m_foot) {
                if (child->isTableSection())
                    m_foot = static_cast<RenderTableSection*>(child);
                wrapInAnonymousSection = false;
                break;
            }
            // fall through
        case TABLE_ROW_GROUP:
            if (!m_firstBody)
                if (child->isTableSection())
                    m_firstBody = static_cast<RenderTableSection*>(child);
            wrapInAnonymousSection = false;
            break;
        case TABLE_CELL:
        case TABLE_ROW:
            wrapInAnonymousSection = true;
            break;
        case BLOCK:
        case BOX:
        case COMPACT:
        case INLINE:
        case INLINE_BLOCK:
        case INLINE_BOX:
        case INLINE_TABLE:
        case LIST_ITEM:
        case NONE:
        case RUN_IN:
        case TABLE:
            // Allow a form to just sit at the top level.
            wrapInAnonymousSection = !isTableElement || !child->element() || !(child->element()->hasTagName(formTag) && document()->isHTMLDocument());

            // FIXME: Allow the delete button container element to sit at the top level. This is needed until http://bugs.webkit.org/show_bug.cgi?id=11363 is fixed.
            if (wrapInAnonymousSection && child->element() && child->element()->isHTMLElement() && static_cast<HTMLElement*>(child->element())->id() == DeleteButtonController::containerElementIdentifier)
                wrapInAnonymousSection = false;
            break;
        }

    if (!wrapInAnonymousSection) {
        // If the next renderer is actually wrapped in an anonymous table section, we need to go up and find that.
        while (beforeChild && !beforeChild->isTableSection() && !beforeChild->isTableCol())
            beforeChild = beforeChild->parent();

        RenderContainer::addChild(child, beforeChild);
        return;
    }

    if (!beforeChild && lastChild() && lastChild()->isTableSection() && lastChild()->isAnonymous()) {
        lastChild()->addChild(child);
        return;
    }

    RenderObject* lastBox = beforeChild;
    RenderObject* nextToLastBox = beforeChild;
    while (lastBox && lastBox->parent()->isAnonymous() && !lastBox->isTableSection() && lastBox->style()->display() != TABLE_CAPTION) {
        nextToLastBox = lastBox;
        lastBox = lastBox->parent();
    }
    if (lastBox && lastBox->isAnonymous()) {
        lastBox->addChild(child, nextToLastBox);
        return;
    }

    if (beforeChild && !beforeChild->isTableSection())
        beforeChild = 0;
    RenderTableSection* section = new (renderArena()) RenderTableSection(document() /* anonymous */);
    RenderStyle* newStyle = new (renderArena()) RenderStyle();
    newStyle->inheritFrom(style());
    newStyle->setDisplay(TABLE_ROW_GROUP);
    section->setStyle(newStyle);
    addChild(section, beforeChild);
    section->addChild(child);
}

void RenderTable::calcWidth()
{
    if (isPositioned())
        calcAbsoluteHorizontal();

    RenderBlock* cb = containingBlock();
    int availableWidth = cb->contentWidth();

    LengthType widthType = style()->width().type();
    if (widthType > Relative && style()->width().isPositive()) {
        // Percent or fixed table
        m_width = style()->width().calcMinValue(availableWidth);
        if (m_minWidth > m_width)
            m_width = m_minWidth;
    } else {
        // An auto width table should shrink to fit within the line width if necessary in order to 
        // avoid overlapping floats.
        availableWidth = cb->lineWidth(m_y);
        
        // Subtract out any fixed margins from our available width for auto width tables.
        int marginTotal = 0;
        if (!style()->marginLeft().isAuto())
            marginTotal += style()->marginLeft().calcValue(availableWidth);
        if (!style()->marginRight().isAuto())
            marginTotal += style()->marginRight().calcValue(availableWidth);
            
        // Subtract out our margins to get the available content width.
        int availContentWidth = max(0, availableWidth - marginTotal);
        
        // Ensure we aren't bigger than our max width or smaller than our min width.
        m_width = min(availContentWidth, m_maxWidth);
    }
    
    m_width = max(m_width, m_minWidth);

    // Finally, with our true width determined, compute our margins for real.
    m_marginRight = 0;
    m_marginLeft = 0;
    calcHorizontalMargins(style()->marginLeft(), style()->marginRight(), availableWidth);
}

void RenderTable::layout()
{
    ASSERT(needsLayout());
    ASSERT(minMaxKnown());
    ASSERT(!m_needsSectionRecalc);

    if (posChildNeedsLayout() && !normalChildNeedsLayout() && !selfNeedsLayout()) {
        // All we have to do is lay out our positioned objects.
        layoutPositionedObjects(false);
        setNeedsLayout(false);
        return;
    }

    IntRect oldBounds;
    IntRect oldFullBounds;
    bool checkForRepaint = checkForRepaintDuringLayout();
    if (checkForRepaint)
        getAbsoluteRepaintRectIncludingFloats(oldBounds, oldFullBounds);
    
    m_height = 0;
    m_overflowHeight = 0;
    initMaxMarginValues();
    
    //int oldWidth = m_width;
    calcWidth();

    // FIXME: The optimisation below doesn't work since the internal table
    // layout could have changed.  we need to add a flag to the table
    // layout that tells us if something has changed in the min max
    // calculations to do it correctly.
//     if ( oldWidth != m_width || columns.size() + 1 != columnPos.size() )
    m_tableLayout->layout();

    setCellWidths();

    // layout child objects
    int calculatedHeight = 0;

    RenderObject* child = firstChild();
    while (child) {
        // FIXME: What about a form that has a display value that makes it a table section?
        if (child->needsLayout() && !(child->element() && child->element()->hasTagName(formTag)))
            child->layout();
        if (child->isTableSection()) {
            static_cast<RenderTableSection*>(child)->calcRowHeight();
            calculatedHeight += static_cast<RenderTableSection*>(child)->layoutRows(0);
        }
        child = child->nextSibling();
    }

    m_overflowWidth = m_width + (collapseBorders() ? outerBorderRight() - borderRight() : 0);
    m_overflowLeft = collapseBorders() ? borderLeft() - outerBorderLeft() : 0;

    // FIXME: Collapse caption margin.
    if (m_caption && m_caption->style()->captionSide() != CAPBOTTOM) {
        m_caption->setPos(m_caption->marginLeft(), m_height);
        m_height += m_caption->height() + m_caption->marginTop() + m_caption->marginBottom();
    }

    int bpTop = borderTop() + (collapseBorders() ? 0 : paddingTop());
    int bpBottom = borderBottom() + (collapseBorders() ? 0 : paddingBottom());
    
    m_height += bpTop;

    int oldHeight = m_height;
    if (!isPositioned())
        calcHeight();
    m_height = oldHeight;

    Length h = style()->height();
    int th = 0;
    if (h.isFixed())
        // Tables size as though CSS height includes border/padding.
        th = h.value() - (bpTop + bpBottom);
    else if (h.isPercent())
        th = calcPercentageHeight(h);
    th = max(0, th);

    // layout rows
    if (th > calculatedHeight) {
        // we have to redistribute that height to get the constraint correctly
        // just force the first body to the height needed
        // FIXME: This should take height constraints on all table sections into account and distribute
        // accordingly. For now this should be good enough.
        if (m_firstBody) {
            m_firstBody->calcRowHeight();
            m_firstBody->layoutRows(th - calculatedHeight);
        } else if (!style()->htmlHacks())
            // Completely empty tables (with no sections or anything) should at least honor specified height
            // in strict mode.
            m_height += th;
    }
    
    int bl = borderLeft();
    if (!collapseBorders())
        bl += paddingLeft();

    // position the table sections
    if (m_head) {
        m_head->setPos(bl, m_height);
        m_height += m_head->height();
    }
    for (RenderObject* body = m_firstBody; body; body = body->nextSibling()) {
        if (body != m_head && body != m_foot && body->isTableSection()) {
            body->setPos(bl, m_height);
            m_height += body->height();
        }
    }
    if (m_foot) {
        m_foot->setPos(bl, m_height);
        m_height += m_foot->height();
    }

    m_height += bpBottom;
               
    if (m_caption && m_caption->style()->captionSide() == CAPBOTTOM) {
        m_caption->setPos(m_caption->marginLeft(), m_height);
        m_height += m_caption->height() + m_caption->marginTop() + m_caption->marginBottom();
    }

    if (isPositioned())
        calcHeight();

    // table can be containing block of positioned elements.
    // FIXME: Only pass true if width or height changed.
    layoutPositionedObjects(true);

    // Repaint with our new bounds if they are different from our old bounds.
    if (checkForRepaint)
        repaintAfterLayoutIfNeeded(oldBounds, oldFullBounds);
    
    setNeedsLayout(false);
}

void RenderTable::setCellWidths()
{
    for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
        if (child->isTableSection())
            static_cast<RenderTableSection*>(child)->setCellWidths();
    }
}

void RenderTable::paint(PaintInfo& paintInfo, int tx, int ty)
{
    tx += xPos();
    ty += yPos();

    PaintPhase paintPhase = paintInfo.phase;

    int os = 2 * maximalOutlineSize(paintPhase);
    if (ty >= paintInfo.rect.bottom() + os || ty + height() <= paintInfo.rect.y() - os)
        return;
    if (tx >= paintInfo.rect.right() + os || tx + width() <= paintInfo.rect.x() - os)
        return;

    if ((paintPhase == PaintPhaseBlockBackground || paintPhase == PaintPhaseChildBlockBackground) && shouldPaintBackgroundOrBorder() && style()->visibility() == VISIBLE)
        paintBoxDecorations(paintInfo, tx, ty);

    // We're done.  We don't bother painting any children.
    if (paintPhase == PaintPhaseBlockBackground)
        return;

    // We don't paint our own background, but we do let the kids paint their backgrounds.
    if (paintPhase == PaintPhaseChildBlockBackgrounds)
        paintPhase = PaintPhaseChildBlockBackground;
    PaintInfo info(paintInfo);
    info.phase = paintPhase;
    info.paintingRoot = paintingRootForChildren(paintInfo);

    for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
        if (!child->layer() && (child->isTableSection() || child == m_caption))
            child->paint(info, tx, ty);
    }

    if (collapseBorders() && paintPhase == PaintPhaseChildBlockBackground && style()->visibility() == VISIBLE) {
        // Collect all the unique border styles that we want to paint in a sorted list.  Once we
        // have all the styles sorted, we then do individual passes, painting each style of border
        // from lowest precedence to highest precedence.
        info.phase = PaintPhaseCollapsedTableBorders;
        DeprecatedValueList<CollapsedBorderValue> borderStyles;
        collectBorders(borderStyles);
        DeprecatedValueListIterator<CollapsedBorderValue> it = borderStyles.begin();
        DeprecatedValueListIterator<CollapsedBorderValue> end = borderStyles.end();
        for (; it != end; ++it) {
            m_currentBorder = &(*it);
            for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
                if (child->isTableSection())
                    child->paint(info, tx, ty);
            }
        }
    }
}

void RenderTable::paintBoxDecorations(PaintInfo& paintInfo, int tx, int ty)
{
    int w = width();
    int h = height();

    // Account for the caption.
    if (m_caption) {
        int captionHeight = (m_caption->height() + m_caption->marginBottom() +  m_caption->marginTop());
        h -= captionHeight;
        if (m_caption->style()->captionSide() != CAPBOTTOM)
            ty += captionHeight;
    }

    int my = max(ty, paintInfo.rect.y());
    int mh;
    if (ty < paintInfo.rect.y())
        mh = max(0, h - (paintInfo.rect.y() - ty));
    else
        mh = min(paintInfo.rect.height(), h);

    paintBackground(paintInfo.context, style()->backgroundColor(), style()->backgroundLayers(), my, mh, tx, ty, w, h);

    if (style()->hasBorder() && !collapseBorders())
        paintBorder(paintInfo.context, tx, ty, w, h, style());
}

void RenderTable::calcMinMaxWidth()
{
    ASSERT(!minMaxKnown());

    recalcSectionsIfNeeded();
    recalcHorizontalBorders();

    m_tableLayout->calcMinMaxWidth(m_minWidth, m_maxWidth);

    if (m_caption)
        m_minWidth = max(m_minWidth, m_caption->minWidth());

    setMinMaxKnown();
}

void RenderTable::splitColumn(int pos, int firstSpan)
{
    // we need to add a new columnStruct
    int oldSize = m_columns.size();
    m_columns.resize(oldSize + 1);
    int oldSpan = m_columns[pos].span;
    ASSERT(oldSpan > firstSpan);
    m_columns[pos].span = firstSpan;
    memmove(m_columns.data() + pos + 1, m_columns.data() + pos, (oldSize - pos) * sizeof(ColumnStruct));
    m_columns[pos + 1].span = oldSpan - firstSpan;

    // change width of all rows.
    for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
        if (child->isTableSection())
            static_cast<RenderTableSection*>(child)->splitColumn(pos, oldSize + 1);
    }

    m_columnPos.resize(numEffCols() + 1);
    setNeedsLayoutAndMinMaxRecalc();
}

void RenderTable::appendColumn(int span)
{
    // easy case.
    int pos = m_columns.size();
    int newSize = pos + 1;
    m_columns.resize(newSize);
    m_columns[pos].span = span;

    // change width of all rows.
    for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
        if (child->isTableSection())
            static_cast<RenderTableSection*>(child)->appendColumn(pos);
    }

    m_columnPos.resize(numEffCols() + 1);
    setNeedsLayoutAndMinMaxRecalc();
}

RenderTableCol* RenderTable::colElement(int col) const
{
    if (!m_hasColElements)
        return 0;
    RenderObject* child = firstChild();
    int cCol = 0;

    while (child) {
        if (child->isTableCol()) {
            RenderTableCol* colElem = static_cast<RenderTableCol*>(child);
            int span = colElem->span();
            if (!colElem->firstChild()) {
                cCol += span;
                if (cCol > col)
                    return colElem;
            }

            RenderObject* next = child->firstChild();
            if (!next)
                next = child->nextSibling();
            if (!next && child->parent()->isTableCol())
                next = child->parent()->nextSibling();
            child = next;
        } else if (child == m_caption)
            child = child->nextSibling();
        else
            break;
    }

    return 0;
}

void RenderTable::recalcSections()
{
    m_caption = 0;
    m_head = 0;
    m_foot = 0;
    m_firstBody = 0;
    m_hasColElements = false;

    // We need to get valid pointers to caption, head, foot and first body again
    for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
        switch (child->style()->display()) {
            case TABLE_CAPTION:
                if (!m_caption && child->isRenderBlock()) {
                    m_caption = static_cast<RenderBlock*>(child);
                    m_caption->setNeedsLayout(true);
                }
                break;
            case TABLE_COLUMN:
            case TABLE_COLUMN_GROUP:
                m_hasColElements = true;
                break;
            case TABLE_HEADER_GROUP:
                if (child->isTableSection()) {
                    RenderTableSection* section = static_cast<RenderTableSection*>(child);
                    if (!m_head)
                        m_head = section;
                    else if (!m_firstBody)
                        m_firstBody = section;
                    section->recalcCellsIfNeeded();
                }
                break;
            case TABLE_FOOTER_GROUP:
                if (child->isTableSection()) {
                    RenderTableSection* section = static_cast<RenderTableSection*>(child);
                    if (!m_foot)
                        m_foot = section;
                    else if (!m_firstBody)
                        m_firstBody = section;
                    section->recalcCellsIfNeeded();
                }
                break;
            case TABLE_ROW_GROUP:
                if (child->isTableSection()) {
                    RenderTableSection* section = static_cast<RenderTableSection*>(child);
                    if (!m_firstBody)
                        m_firstBody = section;
                    section->recalcCellsIfNeeded();
                }
                break;
            default:
                break;
        }
    }

    // repair column count (addChild can grow it too much, because it always adds elements to the last row of a section)
    int maxCols = 0;
    for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
        if (child->isTableSection()) {
            RenderTableSection* section = static_cast<RenderTableSection*>(child);
            int sectionCols = section->numColumns();
            if (sectionCols > maxCols)
                maxCols = sectionCols;
        }
    }
    
    m_columns.resize(maxCols);
    m_columnPos.resize(maxCols + 1);
    
    m_needsSectionRecalc = false;
    setNeedsLayout(true);
}

RenderObject* RenderTable::removeChildNode(RenderObject* child)
{
    setNeedsSectionRecalc();
    return RenderContainer::removeChildNode(child);
}

int RenderTable::calcBorderLeft() const
{
    if (collapseBorders()) {
        // Determined by the first cell of the first row. See the CSS 2.1 spec, section 17.6.2.
        if (!numEffCols())
            return 0;

        unsigned borderWidth = 0;

        const BorderValue& tb = style()->borderLeft();
        if (tb.style() == BHIDDEN)
            return 0;
        if (tb.style() > BHIDDEN)
            borderWidth = tb.width;

        int leftmostColumn = style()->direction() == RTL ? numEffCols() - 1 : 0;
        RenderTableCol* colGroup = colElement(leftmostColumn);
        if (colGroup) {
            const BorderValue& gb = style()->borderLeft();
            if (gb.style() == BHIDDEN)
                return 0;
            if (gb.style() > BHIDDEN)
                borderWidth = max(borderWidth, gb.width);
        }
        
        RenderObject* child = firstChild();
        while (child && !child->isTableSection())
            child = child->nextSibling();
        if (child && child->isTableSection()) {
            RenderTableSection* section = static_cast<RenderTableSection*>(child);
            
            if (!section->numRows())
                return borderWidth / 2;
            
            const BorderValue& sb = section->style()->borderLeft();
            if (sb.style() == BHIDDEN)
                return 0;
            if (sb.style() > BHIDDEN)
                borderWidth = max(borderWidth, sb.width);

            const RenderTableSection::CellStruct& cs = section->cellAt(0, leftmostColumn);
            
            if (cs.cell) {
                const BorderValue& cb = cs.cell->style()->borderLeft();
                if (cb.style() == BHIDDEN)
                    return 0;
                const BorderValue& rb = cs.cell->parent()->style()->borderLeft();
                if (rb.style() == BHIDDEN)
                    return 0;

                if (cb.style() > BHIDDEN)
                    borderWidth = max(borderWidth, cb.width);
                if (rb.style() > BHIDDEN)
                    borderWidth = max(borderWidth, rb.width);
            }
        }
        return borderWidth / 2;
    }
    return RenderBlock::borderLeft();
}
    
int RenderTable::calcBorderRight() const
{
    if (collapseBorders()) {
        // Determined by the last cell of the first row. See the CSS 2.1 spec, section 17.6.2.
        if (!numEffCols())
            return 0;

        unsigned borderWidth = 0;

        const BorderValue& tb = style()->borderRight();
        if (tb.style() == BHIDDEN)
            return 0;
        if (tb.style() > BHIDDEN)
            borderWidth = tb.width;

        int rightmostColumn = style()->direction() == RTL ? 0 : numEffCols() - 1;
        RenderTableCol* colGroup = colElement(rightmostColumn);
        if (colGroup) {
            const BorderValue& gb = style()->borderRight();
            if (gb.style() == BHIDDEN)
                return 0;
            if (gb.style() > BHIDDEN)
                borderWidth = max(borderWidth, gb.width);
        }
        
        RenderObject* child = firstChild();
        while (child && !child->isTableSection())
            child = child->nextSibling();
        if (child && child->isTableSection()) {
            RenderTableSection* section = static_cast<RenderTableSection*>(child);
            
            if (!section->numRows())
                return (borderWidth + 1) / 2;
            
            const BorderValue& sb = section->style()->borderRight();
            if (sb.style() == BHIDDEN)
                return 0;
            if (sb.style() > BHIDDEN)
                borderWidth = max(borderWidth, sb.width);

            const RenderTableSection::CellStruct& cs = section->cellAt(0, rightmostColumn);
            
            if (cs.cell) {
                const BorderValue& cb = cs.cell->style()->borderRight();
                if (cb.style() == BHIDDEN)
                    return 0;
                const BorderValue& rb = cs.cell->parent()->style()->borderRight();
                if (rb.style() == BHIDDEN)
                    return 0;

                if (cb.style() > BHIDDEN)
                    borderWidth = max(borderWidth, cb.width);
                if (rb.style() > BHIDDEN)
                    borderWidth = max(borderWidth, rb.width);
            }
        }
        return (borderWidth + 1) / 2;
    }
    return RenderBlock::borderRight();
}

void RenderTable::recalcHorizontalBorders()
{
    m_borderLeft = calcBorderLeft();
    m_borderRight = calcBorderRight();
}

int RenderTable::borderTop() const
{
    if (collapseBorders())
        return outerBorderTop();
    return RenderBlock::borderTop();
}

int RenderTable::borderBottom() const
{
    if (collapseBorders())
        return outerBorderBottom();
    return RenderBlock::borderBottom();
}

int RenderTable::outerBorderTop() const
{
    if (!collapseBorders())
        return 0;
    int borderWidth = 0;
    RenderTableSection* topSection;
    if (m_head)
        topSection = m_head;
    else if (m_firstBody)
        topSection = m_firstBody;
    else if (m_foot)
        topSection = m_foot;
    else
        topSection = 0;
    if (topSection) {
        borderWidth = topSection->outerBorderTop();
        if (borderWidth == -1)
            return 0;   // Overridden by hidden
    }
    const BorderValue& tb = style()->borderTop();
    if (tb.style() == BHIDDEN)
        return 0;
    if (tb.style() > BHIDDEN)
        borderWidth = max(borderWidth, static_cast<int>(tb.width / 2));
    return borderWidth;
}

int RenderTable::outerBorderBottom() const
{
    if (!collapseBorders())
        return 0;
    int borderWidth = 0;
    RenderTableSection* bottomSection;
    if (m_foot)
        bottomSection = m_foot;
    else {
        RenderObject* child;
        for (child = lastChild(); child && !child->isTableSection(); child = child->previousSibling());
        bottomSection = child ? static_cast<RenderTableSection*>(child) : 0;
    }
    if (bottomSection) {
        borderWidth = bottomSection->outerBorderBottom();
        if (borderWidth == -1)
            return 0;   // Overridden by hidden
    }
    const BorderValue& tb = style()->borderBottom();
    if (tb.style() == BHIDDEN)
        return 0;
    if (tb.style() > BHIDDEN)
        borderWidth = max(borderWidth, static_cast<int>((tb.width + 1) / 2));
    return borderWidth;
}

int RenderTable::outerBorderLeft() const
{
    if (!collapseBorders())
        return 0;

    int borderWidth = 0;

    const BorderValue& tb = style()->borderLeft();
    if (tb.style() == BHIDDEN)
        return 0;
    if (tb.style() > BHIDDEN)
        borderWidth = tb.width / 2;

    bool allHidden = true;
    for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
        if (!child->isTableSection())
            continue;
        int sw = static_cast<RenderTableSection*>(child)->outerBorderLeft();
        if (sw == -1)
            continue;
        else
            allHidden = false;
        borderWidth = max(borderWidth, sw);
    }
    if (allHidden)
        return 0;

    return borderWidth;
}

int RenderTable::outerBorderRight() const
{
    if (!collapseBorders())
        return 0;

    int borderWidth = 0;

    const BorderValue& tb = style()->borderRight();
    if (tb.style() == BHIDDEN)
        return 0;
    if (tb.style() > BHIDDEN)
        borderWidth = (tb.width + 1) / 2;

    bool allHidden = true;
    for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
        if (!child->isTableSection())
            continue;
        int sw = static_cast<RenderTableSection*>(child)->outerBorderRight();
        if (sw == -1)
            continue;
        else
            allHidden = false;
        borderWidth = max(borderWidth, sw);
    }
    if (allHidden)
        return 0;

    return borderWidth;
}

RenderTableSection* RenderTable::sectionAbove(const RenderTableSection* section, bool skipEmptySections) const
{
    if (section == m_head)
        return 0;
    RenderObject* prevSection = (section == m_foot ? lastChild() : section)->previousSibling();
    while (prevSection) {
        if (prevSection->isTableSection() && prevSection != m_head && prevSection != m_foot && (!skipEmptySections || static_cast<RenderTableSection*>(prevSection)->numRows()))
            break;
        prevSection = prevSection->previousSibling();
    }
    if (!prevSection && m_head && (!skipEmptySections || m_head->numRows()))
        prevSection = m_head;
    return static_cast<RenderTableSection*>(prevSection);
}

RenderTableSection* RenderTable::sectionBelow(const RenderTableSection* section, bool skipEmptySections) const
{
    if (section == m_foot)
        return 0;
    RenderObject* nextSection = (section == m_head ? firstChild() : section)->nextSibling();
    while (nextSection) {
        if (nextSection->isTableSection() && nextSection != m_head && nextSection != m_foot && (!skipEmptySections || static_cast<RenderTableSection*>(nextSection)->numRows()))
            break;
        nextSection = nextSection->nextSibling();
    }
    if (!nextSection && m_foot && (!skipEmptySections || m_foot->numRows()))
        nextSection = m_foot;
    return static_cast<RenderTableSection*>(nextSection);
}

RenderTableCell* RenderTable::cellAbove(const RenderTableCell* cell) const
{
    // Find the section and row to look in
    int r = cell->row();
    RenderTableSection* section = 0;
    int rAbove = 0;
    if (r > 0) {
        // cell is not in the first row, so use the above row in its own section
        section = cell->section();
        rAbove = r - 1;
    } else {
        section = sectionAbove(cell->section(), true);
        if (section)
            rAbove = section->numRows() - 1;
    }

    // Look up the cell in the section's grid, which requires effective col index
    if (section) {
        int effCol = colToEffCol(cell->col());
        RenderTableSection::CellStruct aboveCell;
        // If we hit a span back up to a real cell.
        do {
            aboveCell = section->cellAt(rAbove, effCol);
            effCol--;
        } while (!aboveCell.cell && aboveCell.inColSpan && effCol >= 0);
        return aboveCell.cell;
    } else
        return 0;
}

RenderTableCell* RenderTable::cellBelow(const RenderTableCell* cell) const
{
    // Find the section and row to look in
    int r = cell->row() + cell->rowSpan() - 1;
    RenderTableSection* section = 0;
    int rBelow = 0;
    if (r < cell->section()->numRows() - 1) {
        // The cell is not in the last row, so use the next row in the section.
        section = cell->section();
        rBelow = r + 1;
    } else {
        section = sectionBelow(cell->section(), true);
        if (section)
            rBelow = 0;
    }

    // Look up the cell in the section's grid, which requires effective col index
    if (section) {
        int effCol = colToEffCol(cell->col());
        RenderTableSection::CellStruct belowCell;
        // If we hit a colspan back up to a real cell.
        do {
            belowCell = section->cellAt(rBelow, effCol);
            effCol--;
        } while (!belowCell.cell && belowCell.inColSpan && effCol >= 0);
        return belowCell.cell;
    } else
        return 0;
}

RenderTableCell* RenderTable::cellBefore(const RenderTableCell* cell) const
{
    RenderTableSection* section = cell->section();
    int effCol = colToEffCol(cell->col());
    if (!effCol)
        return 0;
    
    // If we hit a colspan back up to a real cell.
    RenderTableSection::CellStruct prevCell;
    do {
        prevCell = section->cellAt(cell->row(), effCol - 1);
        effCol--;
    } while (!prevCell.cell && prevCell.inColSpan && effCol >= 0);
    return prevCell.cell;
}

RenderTableCell* RenderTable::cellAfter(const RenderTableCell* cell) const
{
    int effCol = colToEffCol(cell->col() + cell->colSpan());
    if (effCol >= numEffCols())
        return 0;
    return cell->section()->cellAt(cell->row(), effCol).cell;
}

RenderBlock* RenderTable::firstLineBlock() const
{
    return 0;
}

void RenderTable::updateFirstLetter()
{
}

IntRect RenderTable::getOverflowClipRect(int tx, int ty)
{
    IntRect rect = RenderBlock::getOverflowClipRect(tx, ty);
    
    // If we have a caption, expand the clip to include the caption.
    // FIXME: Technically this is wrong, but it's virtually impossible to fix this
    // for real until captions have been re-written.
    // FIXME: This code assumes (like all our other caption code) that only top/bottom are
    // supported.  When we actually support left/right and stop mapping them to top/bottom,
    // we might have to hack this code first (depending on what order we do these bug fixes in).
    if (m_caption) {
        rect.setHeight(height());
        rect.setY(ty);
    }

    return rect;
}

#ifndef NDEBUG
void RenderTable::dump(TextStream* stream, DeprecatedString ind) const
{
    if (m_caption)
        *stream << " tCaption";
    if (m_head)
        *stream << " head";
    if (m_foot)
        *stream << " foot";

    *stream << endl << ind << "cspans:";
    for (unsigned i = 0; i < m_columns.size(); i++)
        *stream << " " << m_columns[i].span;
    *stream << endl << ind;

    RenderBlock::dump(stream, ind);
}
#endif

}
