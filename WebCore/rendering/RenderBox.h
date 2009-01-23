/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2006, 2007 Apple Inc. All rights reserved.
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

#ifndef RenderBox_h
#define RenderBox_h

#include "RenderObject.h"

namespace WebCore {

enum WidthType { Width, MinWidth, MaxWidth };

class RenderBox : public RenderObject {
public:
    RenderBox(Node*);
    virtual ~RenderBox();

    virtual const char* renderName() const { return "RenderBox"; }

    static RenderBox* toRenderBox(RenderObject* o) { ASSERT(!o || o->isBox()); return static_cast<RenderBox*>(o); }
    static const RenderBox* toConstRenderBox(const RenderObject* o) { ASSERT(!o || o->isBox()); return static_cast<const RenderBox*>(o);}
    
    int x() const { return m_frameRect.x(); }
    int y() const { return m_frameRect.y(); }
    int width() const { return m_frameRect.width(); }
    int height() const { return m_frameRect.height(); }
    
    void setX(int x) { m_frameRect.setX(x); }
    void setY(int y) { m_frameRect.setY(y); }
    void setWidth(int width) { m_frameRect.setWidth(width); }
    void setHeight(int height) { m_frameRect.setHeight(height); }
    
    IntPoint location() const { return m_frameRect.location(); }
    IntSize size() const { return m_frameRect.size(); }

    void setLocation(const IntPoint& location) { m_frameRect.setLocation(location); }
    void setLocation(int x, int y) { setLocation(IntPoint(x, y)); }
    
    void setSize(const IntSize& size) { m_frameRect.setSize(size); }
    void move(int dx, int dy) { m_frameRect.move(dx, dy); }

    IntRect frameRect() const { return m_frameRect; }
    void setFrameRect(const IntRect& rect) { m_frameRect = rect; }

    IntRect borderBoxRect() const { return IntRect(0, 0, width(), height()); }
    
    // The content area of the box (excludes padding and border).
    IntRect contentBoxRect() const { return IntRect(borderLeft() + paddingLeft(), borderTop() + paddingTop(), contentWidth(), contentHeight()); }
    // The content box in absolute coords. Ignores transforms.
    IntRect absoluteContentBox() const;
    // The content box converted to absolute coords (taking transforms into account).
    FloatQuad absoluteContentQuad() const;

    // Bounds of the outline box in absolute coords. Respects transforms
    virtual IntRect absoluteOutlineBounds() const;
    virtual void addFocusRingRects(GraphicsContext*, int tx, int ty);

    // Use this with caution! No type checking is done!
    RenderBox* previousSiblingBox() const { ASSERT(!previousSibling() || previousSibling()->isBox()); return toRenderBox(previousSibling()); }
    RenderBox* nextSiblingBox() const { ASSERT(!nextSibling() || nextSibling()->isBox()); return toRenderBox(nextSibling()); }
    RenderBox* parentBox() const { ASSERT(!parent() || parent()->isBox()); return toRenderBox(parent()); }

    // The height of a block when you include normal flow overflow spillage out of the bottom
    // of the block (e.g., a <div style="height:25px"> that has a 100px tall image inside
    // it would have an overflow height of borderTop() + paddingTop() + 100px.
    virtual int overflowHeight(bool /*includeInterior*/ = true) const { return height(); }
    virtual int overflowWidth(bool /*includeInterior*/ = true) const { return width(); }
    virtual void setOverflowHeight(int) { }
    virtual void setOverflowWidth(int) { }
    virtual int overflowLeft(bool /*includeInterior*/ = true) const { return 0; }
    virtual int overflowTop(bool /*includeInterior*/ = true) const { return 0; }
    virtual IntRect overflowRect(bool /*includeInterior*/ = true) const { return borderBoxRect(); }

    int contentWidth() const { return clientWidth() - paddingLeft() - paddingRight(); }
    int contentHeight() const { return clientHeight() - paddingTop() - paddingBottom(); }

    // IE extensions. Used to calculate offsetWidth/Height.  Overridden by inlines (RenderFlow)
    // to return the remaining width on a given line (and the height of a single line).
    virtual int offsetWidth() const { return width(); }
    virtual int offsetHeight() const { return height(); }
    virtual int offsetLeft() const;
    virtual int offsetTop() const;
    virtual RenderBox* offsetParent() const;

    // More IE extensions.  clientWidth and clientHeight represent the interior of an object
    // excluding border and scrollbar.  clientLeft/Top are just the borderLeftWidth and borderTopWidth.
    int clientLeft() const { return borderLeft(); }
    int clientTop() const { return borderTop(); }
    int clientWidth() const;
    int clientHeight() const;

    // scrollWidth/scrollHeight will be the same as clientWidth/clientHeight unless the
    // object has overflow:hidden/scroll/auto specified and also has overflow.
    // scrollLeft/Top return the current scroll position.  These methods are virtual so that objects like
    // textareas can scroll shadow content (but pretend that they are the objects that are
    // scrolling).
    virtual int scrollLeft() const;
    virtual int scrollTop() const;
    virtual int scrollWidth() const;
    virtual int scrollHeight() const;
    virtual void setScrollLeft(int);
    virtual void setScrollTop(int);

    virtual void absoluteRects(Vector<IntRect>&, int tx, int ty, bool topLevel = true);
    virtual void absoluteQuads(Vector<FloatQuad>&, bool topLevel = true);
    
    IntRect reflectionBox() const;
    int reflectionOffset() const;
    // Given a rect in the object's coordinate space, returns the corresponding rect in the reflection.
    IntRect reflectedRect(const IntRect&) const;

    virtual void paint(PaintInfo&, int tx, int ty);
    virtual bool nodeAtPoint(const HitTestRequest&, HitTestResult&, int x, int y, int tx, int ty, HitTestAction);

    virtual void destroy();

    virtual int minPrefWidth() const;
    virtual int maxPrefWidth() const;

    virtual int overrideSize() const;
    virtual int overrideWidth() const;
    virtual int overrideHeight() const;
    virtual void setOverrideSize(int);

    virtual FloatPoint localToAbsolute(FloatPoint localPoint = FloatPoint(), bool fixed = false, bool useTransforms = false) const;
    virtual FloatPoint absoluteToLocal(FloatPoint containerPoint, bool fixed = false, bool useTransforms = false) const;
    virtual FloatQuad localToAbsoluteQuad(const FloatQuad&, bool fixed = false) const;

    virtual IntSize offsetFromContainer(RenderObject*) const;

    virtual int marginTop() const { return m_marginTop; }
    virtual int marginBottom() const { return m_marginBottom; }
    virtual int marginLeft() const { return m_marginLeft; }
    virtual int marginRight() const { return m_marginRight; }

    int calcBorderBoxWidth(int width) const;
    int calcBorderBoxHeight(int height) const;
    int calcContentBoxWidth(int width) const;
    int calcContentBoxHeight(int height) const;

    virtual void borderFitAdjust(int& /*x*/, int& /*w*/) const { } // Shrink the box in which the border paints if border-fit is set.

    // This method is now public so that centered objects like tables that are
    // shifted right by left-aligned floats can recompute their left and
    // right margins (so that they can remain centered after being
    // shifted. -dwh
    void calcHorizontalMargins(const Length& marginLeft, const Length& marginRight, int containerWidth);

    virtual void position(InlineBox*);

    virtual void dirtyLineBoxes(bool fullLayout, bool isRootLineBox = false);

    // For inline replaced elements, this function returns the inline box that owns us.  Enables
    // the replaced RenderObject to quickly determine what line it is contained on and to easily
    // iterate over structures on the line.
    virtual InlineBox* inlineBoxWrapper() const { return m_inlineBoxWrapper; }
    virtual void setInlineBoxWrapper(InlineBox* boxWrapper) { m_inlineBoxWrapper = boxWrapper; }
    virtual void deleteLineBoxWrapper();

    virtual int lowestPosition(bool includeOverflowInterior = true, bool includeSelf = true) const;
    virtual int rightmostPosition(bool includeOverflowInterior = true, bool includeSelf = true) const;
    virtual int leftmostPosition(bool includeOverflowInterior = true, bool includeSelf = true) const;

    virtual IntRect absoluteClippedOverflowRect();
    virtual void computeAbsoluteRepaintRect(IntRect&, bool fixed = false);
    IntSize offsetForPositionedInContainer(RenderObject*) const;

    virtual void repaintDuringLayoutIfMoved(const IntRect&);

    virtual int containingBlockWidth() const;

    virtual void calcWidth();
    virtual void calcHeight();

    bool stretchesToViewHeight() const
    {
        return style()->htmlHacks() && style()->height().isAuto() && !isFloatingOrPositioned() && (isRoot() || isBody());
    }

    virtual IntSize intrinsicSize() const { return IntSize(); }

    // Whether or not the element shrinks to its intrinsic width (rather than filling the width
    // of a containing block).  HTML4 buttons, <select>s, <input>s, legends, and floating/compact elements do this.
    bool sizesToIntrinsicWidth(WidthType) const;
    virtual bool stretchesToMinIntrinsicWidth() const { return false; }

    int calcWidthUsing(WidthType, int containerWidth);
    int calcHeightUsing(const Length& height);
    int calcReplacedWidthUsing(Length width) const;
    int calcReplacedHeightUsing(Length height) const;

    virtual int calcReplacedWidth(bool includeMaxWidth = true) const;
    virtual int calcReplacedHeight() const;

    int calcPercentageHeight(const Length& height);

    // Block flows subclass availableWidth to handle multi column layout (shrinking the width available to children when laying out.)
    virtual int availableWidth() const { return contentWidth(); }
    virtual int availableHeight() const;
    int availableHeightUsing(const Length&) const;

    void calcVerticalMargins();

    int relativePositionOffsetX() const;
    int relativePositionOffsetY() const;
    IntSize relativePositionOffset() const { return IntSize(relativePositionOffsetX(), relativePositionOffsetY()); }
    
    virtual RenderLayer* layer() const { return m_layer; }

    virtual IntRect localCaretRect(InlineBox*, int caretOffset, int* extraWidthToEndOfLine = 0);

    virtual void paintFillLayerExtended(const PaintInfo&, const Color&, const FillLayer*, int clipY, int clipHeight,
                                        int tx, int ty, int width, int height, InlineFlowBox* = 0, CompositeOperator = CompositeSourceOver);
    IntSize calculateBackgroundSize(const FillLayer*, int scaledWidth, int scaledHeight) const;

    virtual int staticX() const;
    virtual int staticY() const;
    virtual void setStaticX(int staticX);
    virtual void setStaticY(int staticY);

    virtual IntRect getOverflowClipRect(int tx, int ty);
    virtual IntRect getClipRect(int tx, int ty);

    virtual void paintBoxDecorations(PaintInfo&, int tx, int ty);
    virtual void paintMask(PaintInfo& paintInfo, int tx, int ty);
    virtual void imageChanged(WrappedImagePtr, const IntRect* = 0);

    // Called when a positioned object moves but doesn't change size.  A simplified layout is done
    // that just updates the object's position.
    virtual void tryLayoutDoingPositionedMovementOnly()
    {
        int oldWidth = width();
        calcWidth();
        // If we shrink to fit our width may have changed, so we still need full layout.
        if (oldWidth != width())
            return;
        calcHeight();
        setNeedsLayout(false);
    }

    IntRect maskClipRect();
    
#if ENABLE(SVG)
    virtual TransformationMatrix localTransform() const;
#endif

protected:
    virtual void styleWillChange(RenderStyle::Diff, const RenderStyle* newStyle);
    virtual void styleDidChange(RenderStyle::Diff, const RenderStyle* oldStyle);

    void paintFillLayer(const PaintInfo&, const Color&, const FillLayer*, int clipY, int clipHeight, int tx, int ty, int width, int height, CompositeOperator = CompositeSourceOver);
    void paintFillLayers(const PaintInfo&, const Color&, const FillLayer*, int clipY, int clipHeight, int tx, int ty, int width, int height, CompositeOperator = CompositeSourceOver);

    void paintMaskImages(const PaintInfo&, int clipY, int clipHeight, int tx, int ty, int width, int height);

#if PLATFORM(MAC)
    void paintCustomHighlight(int tx, int ty, const AtomicString& type, bool behindText);
#endif

    void calcAbsoluteHorizontal();
    
    virtual bool shouldCalculateSizeAsReplaced() const { return isReplaced() && !isInlineBlockOrInlineTable(); }

private:
    void paintRootBoxDecorations(PaintInfo&, int tx, int ty);
    // Returns true if we did a full repaint
    bool repaintLayerRectsForImage(WrappedImagePtr image, const FillLayer* layers, bool drawingBackground);

    void calculateBackgroundImageGeometry(const FillLayer*, int tx, int ty, int w, int h, IntRect& destRect, IntPoint& phase, IntSize& tileSize);
    
    int containingBlockWidthForPositioned(const RenderObject* containingBlock) const;
    int containingBlockHeightForPositioned(const RenderObject* containingBlock) const;

    void calcAbsoluteVertical();
    void calcAbsoluteHorizontalValues(Length width, const RenderObject* cb, TextDirection containerDirection,
                                      int containerWidth, int bordersPlusPadding,
                                      Length left, Length right, Length marginLeft, Length marginRight,
                                      int& widthValue, int& marginLeftValue, int& marginRightValue, int& xPos);
    void calcAbsoluteVerticalValues(Length height, const RenderObject* cb,
                                    int containerHeight, int bordersPlusPadding,
                                    Length top, Length bottom, Length marginTop, Length marginBottom,
                                    int& heightValue, int& marginTopValue, int& marginBottomValue, int& yPos);

    void calcAbsoluteVerticalReplaced();
    void calcAbsoluteHorizontalReplaced();

    // This function calculates the minimum and maximum preferred widths for an object.
    // These values are used in shrink-to-fit layout systems.
    // These include tables, positioned objects, floats and flexible boxes.
    virtual void calcPrefWidths() = 0;

protected:
    // The width/height of the contents + borders + padding.  The x/y location is relative to our container (which is not always our parent).
    IntRect m_frameRect;

    int m_marginLeft;
    int m_marginRight;
    int m_marginTop;
    int m_marginBottom;

    // The preferred width of the element if it were to break its lines at every possible opportunity.
    int m_minPrefWidth;
    
    // The preferred width of the element if it never breaks any lines at all.
    int m_maxPrefWidth;

    // A pointer to our layer if we have one.
    RenderLayer* m_layer;

    // For inline replaced elements, the inline box that owns us.
    InlineBox* m_inlineBoxWrapper;

private:
    // Used to store state between styleWillChange and styleDidChange
    static bool s_wasFloating;
    static bool s_hadOverflowClip;
};

} // namespace WebCore

#endif // RenderBox_h
