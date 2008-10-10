/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "RenderScrollbar.h"
#include "RenderScrollbarPart.h"
#include "RenderScrollbarTheme.h"

namespace WebCore {

PassRefPtr<Scrollbar> RenderScrollbar::createCustomScrollbar(ScrollbarClient* client, ScrollbarOrientation orientation, RenderStyle* style, RenderObject* renderer)
{
    return adoptRef(new RenderScrollbar(client, orientation, style, renderer));
}

RenderScrollbar::RenderScrollbar(ScrollbarClient* client, ScrollbarOrientation orientation, RenderStyle* style, RenderObject* renderer)
    : Scrollbar(client, orientation, RegularScrollbar, RenderScrollbarTheme::renderScrollbarTheme())
    , m_owner(renderer)
{
    updateScrollbarParts(style);
}

RenderScrollbar::~RenderScrollbar()
{
    ASSERT(m_parts.isEmpty());
}

void RenderScrollbar::setParent(ScrollView* parent)
{
    Scrollbar::setParent(parent);
    if (!parent) {
        // Destroy all of the scrollbar's RenderObjects.
        updateScrollbarParts(0, true);
    }
}

void RenderScrollbar::setEnabled(bool e)
{
    bool wasEnabled = enabled();
    Scrollbar::setEnabled(e);
    if (wasEnabled != e)
        updateScrollbarParts();
}

void RenderScrollbar::paint(GraphicsContext* context, const IntRect& damageRect)
{
    if (context->updatingControlTints())
        updateScrollbarParts();
    Scrollbar::paint(context, damageRect);
}

void RenderScrollbar::setHoveredPart(ScrollbarPart part)
{
    if (part == m_hoveredPart)
        return;

    ScrollbarPart oldPart = m_hoveredPart;
    Scrollbar::setHoveredPart(part);

    updateScrollbarPart(oldPart);
    updateScrollbarPart(m_hoveredPart);

    // FIXME: Shouldn't always update the hover state of the scrollbar and track bg.
    updateScrollbarPart(ScrollbarBGPart);
    updateScrollbarPart(TrackBGPart);
    invalidate();
}

void RenderScrollbar::setPressedPart(ScrollbarPart part)
{
    ScrollbarPart oldPart = m_pressedPart;
    Scrollbar::setPressedPart(part);
    
    updateScrollbarPart(oldPart);
    updateScrollbarPart(part);
    
    // FIXME: Shouldn't always update the active state of the scrollbar and track bg.
    updateScrollbarPart(ScrollbarBGPart);
    updateScrollbarPart(TrackBGPart);
    invalidate();
}

static ScrollbarPart s_styleResolvePart;
static RenderScrollbar* s_styleResolveScrollbar;

RenderScrollbar* RenderScrollbar::scrollbarForStyleResolve()
{
    return s_styleResolveScrollbar;
}

ScrollbarPart RenderScrollbar::partForStyleResolve()
{
    return s_styleResolvePart;
}

RenderStyle* RenderScrollbar::getScrollbarPseudoStyle(ScrollbarPart partType, RenderStyle::PseudoId pseudoId)
{
    s_styleResolvePart = partType;
    s_styleResolveScrollbar = this;
    RenderStyle* result = m_owner->getPseudoStyle(pseudoId, m_owner->style(), false);
    s_styleResolvePart = NoPart;
    s_styleResolveScrollbar = 0;
    return result;
}

void RenderScrollbar::updateScrollbarParts(RenderStyle* scrollbarStyle, bool destroy)
{
    updateScrollbarPart(ScrollbarBGPart, scrollbarStyle, destroy);
    updateScrollbarPart(BackButtonStartPart, 0, destroy);
    updateScrollbarPart(ForwardButtonStartPart, 0, destroy);
    updateScrollbarPart(BackTrackPart, 0, destroy);
    updateScrollbarPart(ThumbPart, 0, destroy);
    updateScrollbarPart(ForwardTrackPart, 0, destroy);
    updateScrollbarPart(BackButtonEndPart, 0, destroy);
    updateScrollbarPart(ForwardButtonEndPart, 0, destroy);
    updateScrollbarPart(TrackBGPart, 0, destroy);
    
    if (destroy)
        return;

    // See if the scrollbar's thickness changed.  If so, we need to mark our owning object as needing a layout.
    bool isHorizontal = orientation() == HorizontalScrollbar;    
    int oldThickness = isHorizontal ? height() : width();
    int newThickness = 0;
    RenderScrollbarPart* part = m_parts.get(ScrollbarBGPart);
    if (part) {
        part->layout();
        newThickness = isHorizontal ? part->height() : part->width();
    }
    
    if (newThickness != oldThickness) {
        setFrameRect(IntRect(x(), y(), isHorizontal ? width() : newThickness, isHorizontal ? newThickness : height()));
        m_owner->setChildNeedsLayout(true);
    }
}

static RenderStyle::PseudoId pseudoForScrollbarPart(ScrollbarPart part)
{
    switch (part) {
        case BackButtonStartPart:
        case ForwardButtonStartPart:
        case BackButtonEndPart:
        case ForwardButtonEndPart:
            return RenderStyle::SCROLLBAR_BUTTON;
        case BackTrackPart:
        case ForwardTrackPart:
            return RenderStyle::SCROLLBAR_TRACK_PIECE;
        case ThumbPart:
            return RenderStyle::SCROLLBAR_THUMB;
        case TrackBGPart:
            return RenderStyle::SCROLLBAR_TRACK;
        default:
            return RenderStyle::SCROLLBAR;
    }
}

void RenderScrollbar::updateScrollbarPart(ScrollbarPart partType, RenderStyle* partStyle, bool destroy)
{
    if (partType == NoPart)
        return;

    if (!partStyle && !destroy)
        partStyle = getScrollbarPseudoStyle(partType, pseudoForScrollbarPart(partType));
    
    bool needRenderer = !destroy && partStyle && partStyle->display() != NONE && partStyle->visibility() == VISIBLE;
    
    RenderScrollbarPart* partRenderer = m_parts.get(partType);
    if (!partRenderer && needRenderer) {
        partRenderer = new (m_owner->renderArena()) RenderScrollbarPart(this, partType, m_owner->document());
        m_parts.set(partType, partRenderer);
    } else if (partRenderer && !needRenderer) {
        m_parts.remove(partType);
        partRenderer->destroy();
        partRenderer = 0;
    }
    
    if (partRenderer)
        partRenderer->setStyle(partStyle);
}

void RenderScrollbar::paintPart(GraphicsContext* graphicsContext, ScrollbarPart partType, const IntRect& rect)
{
    RenderScrollbarPart* partRenderer = m_parts.get(partType);
    if (!partRenderer)
        return;

    // Make sure our dimensions match the rect.
    partRenderer->setPos(rect.x() - x(), rect.y() - y());
    partRenderer->setWidth(rect.width());
    partRenderer->setHeight(rect.height());
    partRenderer->setOverflowWidth(max(rect.width(), partRenderer->overflowWidth()));
    partRenderer->setOverflowHeight(max(rect.height(), partRenderer->overflowHeight()));

    // Now do the paint.
    RenderObject::PaintInfo paintInfo(graphicsContext, rect, PaintPhaseBlockBackground, false, 0, 0);
    partRenderer->paint(paintInfo, x(), y());
    paintInfo.phase = PaintPhaseChildBlockBackgrounds;
    partRenderer->paint(paintInfo, x(), y());
    paintInfo.phase = PaintPhaseFloat;
    partRenderer->paint(paintInfo, x(), y());
    paintInfo.phase = PaintPhaseForeground;
    partRenderer->paint(paintInfo, x(), y());
    paintInfo.phase = PaintPhaseOutline;
    partRenderer->paint(paintInfo, x(), y());
}

IntRect RenderScrollbar::buttonRect(ScrollbarPart partType)
{
    RenderScrollbarPart* partRenderer = m_parts.get(partType);
    if (!partRenderer)
        return IntRect();
        
    partRenderer->layout();
    
    bool isHorizontal = orientation() == HorizontalScrollbar;
    if (partType == BackButtonStartPart)
        return IntRect(x(), y(), isHorizontal ? partRenderer->width() : width(), isHorizontal ? height() : partRenderer->height());
    if (partType == ForwardButtonEndPart)
        return IntRect(isHorizontal ? x() + width() - partRenderer->width() : x(),
        
                       isHorizontal ? y() : y() + height() - partRenderer->height(),
                       isHorizontal ? partRenderer->width() : width(),
                       isHorizontal ? height() : partRenderer->height());
    
    if (partType == ForwardButtonStartPart) {
        IntRect previousButton = buttonRect(BackButtonStartPart);
        return IntRect(isHorizontal ? x() + previousButton.width() : x(),
                       isHorizontal ? y() : y() + previousButton.height(),
                       isHorizontal ? partRenderer->width() : width(),
                       isHorizontal ? height() : partRenderer->height());
    }
    
    IntRect followingButton = buttonRect(ForwardButtonEndPart);
    return IntRect(isHorizontal ? x() + width() - followingButton.width() - partRenderer->width() : x(),
                   isHorizontal ? y() : y() + height() - followingButton.height() - partRenderer->height(),
                   isHorizontal ? partRenderer->width() : width(),
                   isHorizontal ? height() : partRenderer->height());
}

IntRect RenderScrollbar::trackRect(int startLength, int endLength)
{
    
    RenderScrollbarPart* part = m_parts.get(TrackBGPart);
    if (part)
        part->layout();

    if (orientation() == HorizontalScrollbar) {
        int marginLeft = part ? part->marginLeft() : 0;
        int marginRight = part ? part->marginRight() : 0;
        startLength += marginLeft;
        endLength += marginRight;
        int totalLength = startLength + endLength;
        return IntRect(x() + startLength, y(), width() - totalLength, height());
    }
    
    int marginTop = part ? part->marginTop() : 0;
    int marginBottom = part ? part->marginBottom() : 0;
    startLength += marginTop;
    endLength += marginBottom;
    int totalLength = startLength + endLength;

    return IntRect(x(), y() + startLength, width(), height() - totalLength);
}

int RenderScrollbar::minimumThumbLength()
{
    RenderScrollbarPart* partRenderer = m_parts.get(ThumbPart);
    if (!partRenderer)
        return 0;    
    partRenderer->layout();
    return orientation() == HorizontalScrollbar ? partRenderer->width() : partRenderer->height();
}

}
