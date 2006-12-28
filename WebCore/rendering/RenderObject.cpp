/**
 * This file is part of the html renderer for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 *           (C) 2004 Allan Sandfeld Jensen (kde@carewolf.com)
 * Copyright (C) 2004, 2005, 2006 Apple Computer, Inc.
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
 *
 */

#include "config.h"
#include "RenderObject.h"

#include "AXObjectCache.h"
#include "AffineTransform.h"
#include "CachedImage.h"
#include "Chrome.h"
#include "CounterNode.h"
#include "CounterResetNode.h"
#include "Document.h"
#include "Element.h"
#include "EventHandler.h"
#include "EventNames.h"
#include "FloatRect.h"
#include "Frame.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "HTMLNames.h"
#include "HTMLOListElement.h"
#include "HitTestRequest.h"
#include "HitTestResult.h"
#include "KURL.h"
#include "Page.h"
#include "Position.h"
#include "RenderArena.h"
#include "RenderFlexibleBox.h"
#include "RenderImage.h"
#include "RenderInline.h"
#include "RenderListItem.h"
#include "RenderTableCell.h"
#include "RenderTableCol.h"
#include "RenderTableRow.h"
#include "RenderText.h"
#include "RenderTheme.h"
#include "RenderView.h"
#include "Screen.h"
#include "TextResourceDecoder.h"
#include "TextStream.h"
#include "cssstyleselector.h"
#include <algorithm>

using namespace std;

namespace WebCore {

using namespace EventNames;
using namespace HTMLNames;

#ifndef NDEBUG
static void* baseOfRenderObjectBeingDeleted;
#endif

typedef HashMap<String, CounterNode*> CounterNodeMap;
typedef HashMap<const RenderObject*, CounterNodeMap*> RenderObjectsToCounterNodeMaps;

static RenderObjectsToCounterNodeMaps* getRenderObjectsToCounterNodeMaps()
{
    static RenderObjectsToCounterNodeMaps objectsMap;
    return &objectsMap;
}

void* RenderObject::operator new(size_t sz, RenderArena* renderArena) throw()
{
    return renderArena->allocate(sz);
}

void RenderObject::operator delete(void* ptr, size_t sz)
{
    ASSERT(baseOfRenderObjectBeingDeleted == ptr);

    // Stash size where destroy can find it.
    *(size_t *)ptr = sz;
}

RenderObject* RenderObject::createObject(Node* node, RenderStyle* style)
{
    RenderObject* o = 0;
    RenderArena* arena = node->document()->renderArena();

    if (ContentData* contentData = style->contentData()) {
        RenderImage* contentImage = new (arena) RenderImage(node);
        if (contentImage) {
            contentImage->setStyle(style);
            contentImage->setContentObject(contentData->contentObject());
            contentImage->setIsAnonymousImage(true);
        }
        return contentImage;
    }

    switch (style->display()) {
        case NONE:
            break;
        case INLINE:
            o = new (arena) RenderInline(node);
            break;
        case BLOCK:
            o = new (arena) RenderBlock(node);
            break;
        case INLINE_BLOCK:
            o = new (arena) RenderBlock(node);
            break;
        case LIST_ITEM:
            o = new (arena) RenderListItem(node);
            break;
        case RUN_IN:
        case COMPACT:
            o = new (arena) RenderBlock(node);
            break;
        case TABLE:
        case INLINE_TABLE:
            o = new (arena) RenderTable(node);
            break;
        case TABLE_ROW_GROUP:
        case TABLE_HEADER_GROUP:
        case TABLE_FOOTER_GROUP:
            o = new (arena) RenderTableSection(node);
            break;
        case TABLE_ROW:
            o = new (arena) RenderTableRow(node);
            break;
        case TABLE_COLUMN_GROUP:
        case TABLE_COLUMN:
            o = new (arena) RenderTableCol(node);
            break;
        case TABLE_CELL:
            o = new (arena) RenderTableCell(node);
            break;
        case TABLE_CAPTION:
            o = new (arena) RenderBlock(node);
            break;
        case BOX:
        case INLINE_BOX:
            o = new (arena) RenderFlexibleBox(node);
            break;
    }
    return o;
}

#ifndef NDEBUG
struct RenderObjectCounter {
    static int count;
    ~RenderObjectCounter() { if (count != 0) fprintf(stderr, "LEAK: %d RenderObject\n", count); }
};
int RenderObjectCounter::count;
static RenderObjectCounter renderObjectCounter;
#endif

RenderObject::RenderObject(Node* node)
    : CachedResourceClient()
    , m_style(0)
    , m_node(node)
    , m_parent(0)
    , m_previous(0)
    , m_next(0)
    , m_verticalPosition(PositionUndefined)
    , m_needsLayout(false)
    , m_normalChildNeedsLayout(false)
    , m_posChildNeedsLayout(false)
    , m_minMaxKnown(false)
    , m_floating(false)
    , m_positioned(false)
    , m_relPositioned(false)
    , m_paintBackground(false)
    , m_isAnonymous(node == node->document())
    , m_recalcMinMax(false)
    , m_isText(false)
    , m_inline(true)
    , m_replaced(false)
    , m_isDragging(false)
    , m_hasOverflowClip(false)
    , m_hasCounterNodeMap(false)
{
#ifndef NDEBUG
    ++RenderObjectCounter::count;
#endif
}

RenderObject::~RenderObject()
{
#ifndef NDEBUG
    --RenderObjectCounter::count;
#endif
}

bool RenderObject::isDescendantOf(const RenderObject* obj) const
{
    for (const RenderObject* r = this; r; r = r->m_parent) {
        if (r == obj)
            return true;
    }
    return false;
}

bool RenderObject::isRoot() const
{
    return element() && element()->renderer() == this && element()->document()->documentElement() == element();
}

bool RenderObject::isBody() const
{
    return element() && element()->renderer() == this && element()->hasTagName(bodyTag);
}

bool RenderObject::isHR() const
{
    return element() && element()->hasTagName(hrTag);
}

bool RenderObject::isHTMLMarquee() const
{
    return element() && element()->renderer() == this && element()->hasTagName(marqueeTag);
}

bool RenderObject::canHaveChildren() const
{
    return false;
}

RenderFlow* RenderObject::continuation() const
{
    return 0;
}

bool RenderObject::isInlineContinuation() const
{
    return false;
}

void RenderObject::addChild(RenderObject*, RenderObject*)
{
    ASSERT_NOT_REACHED();
}

RenderObject* RenderObject::removeChildNode(RenderObject*)
{
    ASSERT_NOT_REACHED();
    return 0;
}

void RenderObject::removeChild(RenderObject*)
{
    ASSERT_NOT_REACHED();
}

void RenderObject::appendChildNode(RenderObject*)
{
    ASSERT_NOT_REACHED();
}

void RenderObject::insertChildNode(RenderObject*, RenderObject*)
{
    ASSERT_NOT_REACHED();
}

RenderObject* RenderObject::nextInPreOrder() const
{
    if (RenderObject* o = firstChild())
        return o;

    return nextInPreOrderAfterChildren();
}

RenderObject* RenderObject::nextInPreOrderAfterChildren() const
{
    RenderObject* o;
    if (!(o = nextSibling())) {
        o = parent();
        while (o && !o->nextSibling())
            o = o->parent();
        if (o)
            o = o->nextSibling();
    }

    return o;
}

RenderObject* RenderObject::previousInPreOrder() const
{
    if (RenderObject* o = previousSibling()) {
        while (o->lastChild())
            o = o->lastChild();
        return o;
    }

    return parent();
}

RenderObject* RenderObject::childAt(unsigned index) const
{
    RenderObject* child = firstChild();
    for (unsigned i = 0; child && i < index; i++)
        child = child->nextSibling();
    return child;
}

bool RenderObject::isEditable() const
{
    RenderText* textRenderer = 0;
    if (isText())
        textRenderer = static_cast<RenderText*>(const_cast<RenderObject*>(this));

    return style()->visibility() == VISIBLE &&
        element() && element()->isContentEditable() &&
        ((isBlockFlow() && !firstChild()) ||
        isReplaced() ||
        isBR() ||
        (textRenderer && textRenderer->firstTextBox()));
}

RenderObject* RenderObject::nextEditable() const
{
    RenderObject* r = const_cast<RenderObject*>(this);
    RenderObject* n = firstChild();
    if (n) {
        while (n) {
            r = n;
            n = n->firstChild();
        }
        if (r->isEditable())
            return r;
        else
            return r->nextEditable();
    }
    n = r->nextSibling();
    if (n) {
        r = n;
        while (n) {
            r = n;
            n = n->firstChild();
        }
        if (r->isEditable())
            return r;
        else
            return r->nextEditable();
    }
    n = r->parent();
    while (n) {
        r = n;
        n = r->nextSibling();
        if (n) {
            r = n;
            n = r->firstChild();
            while (n) {
                r = n;
                n = n->firstChild();
            }
            if (r->isEditable())
                return r;
            else
                return r->nextEditable();
        }
        n = r->parent();
    }

    return 0;
}

RenderObject* RenderObject::previousEditable() const
{
    RenderObject* r = const_cast<RenderObject*>(this);
    RenderObject* n = firstChild();
    if (n) {
        while (n) {
            r = n;
            n = n->lastChild();
        }
        if (r->isEditable())
            return r;
        else
            return r->previousEditable();
    }
    n = r->previousSibling();
    if (n) {
        r = n;
        while (n) {
            r = n;
            n = n->lastChild();
        }
        if (r->isEditable())
            return r;
        else
            return r->previousEditable();
    }
    n = r->parent();
    while (n) {
        r = n;
        n = r->previousSibling();
        if (n) {
            r = n;
            n = r->lastChild();
            while (n) {
                r = n;
                n = n->lastChild();
            }
            if (r->isEditable())
                return r;
            else
                return r->previousEditable();
        }
        n = r->parent();
    }

    return 0;
}

RenderObject* RenderObject::firstLeafChild() const
{
    RenderObject* r = firstChild();
    while (r) {
        RenderObject* n = 0;
        n = r->firstChild();
        if (!n)
            break;
        r = n;
    }
    return r;
}

RenderObject* RenderObject::lastLeafChild() const
{
    RenderObject* r = lastChild();
    while (r) {
        RenderObject* n = 0;
        n = r->lastChild();
        if (!n)
            break;
        r = n;
    }
    return r;
}

static void addLayers(RenderObject* obj, RenderLayer* parentLayer, RenderObject*& newObject,
                      RenderLayer*& beforeChild)
{
    if (obj->layer()) {
        if (!beforeChild && newObject) {
            // We need to figure out the layer that follows newObject.  We only do
            // this the first time we find a child layer, and then we update the
            // pointer values for newObject and beforeChild used by everyone else.
            beforeChild = newObject->parent()->findNextLayer(parentLayer, newObject);
            newObject = 0;
        }
        parentLayer->addChild(obj->layer(), beforeChild);
        return;
    }

    for (RenderObject* curr = obj->firstChild(); curr; curr = curr->nextSibling())
        addLayers(curr, parentLayer, newObject, beforeChild);
}

void RenderObject::addLayers(RenderLayer* parentLayer, RenderObject* newObject)
{
    if (!parentLayer)
        return;

    RenderObject* object = newObject;
    RenderLayer* beforeChild = 0;
    WebCore::addLayers(this, parentLayer, object, beforeChild);
}

void RenderObject::removeLayers(RenderLayer* parentLayer)
{
    if (!parentLayer)
        return;

    if (layer()) {
        parentLayer->removeChild(layer());
        return;
    }

    for (RenderObject* curr = firstChild(); curr; curr = curr->nextSibling())
        curr->removeLayers(parentLayer);
}

void RenderObject::moveLayers(RenderLayer* oldParent, RenderLayer* newParent)
{
    if (!newParent)
        return;

    if (layer()) {
        if (oldParent)
            oldParent->removeChild(layer());
        newParent->addChild(layer());
        return;
    }

    for (RenderObject* curr = firstChild(); curr; curr = curr->nextSibling())
        curr->moveLayers(oldParent, newParent);
}

RenderLayer* RenderObject::findNextLayer(RenderLayer* parentLayer, RenderObject* startPoint,
                                         bool checkParent)
{
    // Error check the parent layer passed in.  If it's null, we can't find anything.
    if (!parentLayer)
        return 0;

    // Step 1: If our layer is a child of the desired parent, then return our layer.
    RenderLayer* ourLayer = layer();
    if (ourLayer && ourLayer->parent() == parentLayer)
        return ourLayer;

    // Step 2: If we don't have a layer, or our layer is the desired parent, then descend
    // into our siblings trying to find the next layer whose parent is the desired parent.
    if (!ourLayer || ourLayer == parentLayer) {
        for (RenderObject* curr = startPoint ? startPoint->nextSibling() : firstChild();
             curr; curr = curr->nextSibling()) {
            RenderLayer* nextLayer = curr->findNextLayer(parentLayer, 0, false);
            if (nextLayer)
                return nextLayer;
        }
    }

    // Step 3: If our layer is the desired parent layer, then we're finished.  We didn't
    // find anything.
    if (parentLayer == ourLayer)
        return 0;

    // Step 4: If |checkParent| is set, climb up to our parent and check its siblings that
    // follow us to see if we can locate a layer.
    if (checkParent && parent())
        return parent()->findNextLayer(parentLayer, this, true);

    return 0;
}

RenderLayer* RenderObject::enclosingLayer() const
{
    const RenderObject* curr = this;
    while (curr) {
        RenderLayer* layer = curr->layer();
        if (layer)
            return layer;
        curr = curr->parent();
    }
    return 0;
}

bool RenderObject::requiresLayer()
{
    return isRoot() || isPositioned() || isRelPositioned() || isTransparent() || hasOverflowClip();
}

RenderBlock* RenderObject::firstLineBlock() const
{
    return 0;
}

void RenderObject::updateFirstLetter()
{
}

int RenderObject::offsetLeft() const
{
    RenderObject* offsetPar = offsetParent();
    if (!offsetPar)
        return 0;
    int x = xPos();
    if (!isPositioned()) {
        if (isRelPositioned())
            x += static_cast<const RenderBox*>(this)->relativePositionOffsetX();
        RenderObject* curr = parent();
        while (curr && curr != offsetPar) {
            x += curr->xPos();
            curr = curr->parent();
        }
    }
    return x;
}

int RenderObject::offsetTop() const
{
    RenderObject* offsetPar = offsetParent();
    if (!offsetPar)
        return 0;
    int y = yPos();
    if (!isPositioned()) {
        if (isRelPositioned())
            y += static_cast<const RenderBox*>(this)->relativePositionOffsetY();
        RenderObject* curr = parent();
        while (curr && curr != offsetPar) {
            if (!curr->isTableRow())
                y += curr->yPos();
            curr = curr->parent();
        }
    }
    return y;
}

RenderObject* RenderObject::offsetParent() const
{
    // FIXME: It feels like this function could almost be written using containing blocks.
    if (isBody())
        return 0;

    bool skipTables = isPositioned() || isRelPositioned();
    RenderObject* curr = parent();
    while (curr && (!curr->element() ||
                    (!curr->isPositioned() && !curr->isRelPositioned() &&
                        !(!style()->htmlHacks() && skipTables ? curr->isRoot() : curr->isBody())))) {
        if (!skipTables && curr->element() && (curr->isTableCell() || curr->isTable()))
            break;
        curr = curr->parent();
    }
    return curr;
}

// More IE extensions.  clientWidth and clientHeight represent the interior of an object
// excluding border and scrollbar.
int RenderObject::clientWidth() const
{
    return width() - borderLeft() - borderRight() -
        (includeVerticalScrollbarSize() ? layer()->verticalScrollbarWidth() : 0);
}

int RenderObject::clientHeight() const
{
    return height() - borderTop() - borderBottom() -
      (includeHorizontalScrollbarSize() ? layer()->horizontalScrollbarHeight() : 0);
}

// scrollWidth/scrollHeight will be the same as clientWidth/clientHeight unless the
// object has overflow:hidden/scroll/auto specified and also has overflow.
int RenderObject::scrollWidth() const
{
    return hasOverflowClip() ? layer()->scrollWidth() : overflowWidth();
}

int RenderObject::scrollHeight() const
{
    return hasOverflowClip() ? layer()->scrollHeight() : overflowHeight();
}

int RenderObject::scrollLeft() const
{
    return hasOverflowClip() ? layer()->scrollXOffset() : 0;
}

int RenderObject::scrollTop() const
{
    return hasOverflowClip() ? layer()->scrollYOffset() : 0;
}

void RenderObject::setScrollLeft(int newLeft)
{
    if (hasOverflowClip())
        layer()->scrollToXOffset(newLeft);
}

void RenderObject::setScrollTop(int newTop)
{
    if (hasOverflowClip())
        layer()->scrollToYOffset(newTop);
}

bool RenderObject::scroll(ScrollDirection direction, ScrollGranularity granularity, float multiplier)
{
    RenderLayer* l = layer();
    if (l && l->scroll(direction, granularity, multiplier))
        return true;
    RenderBlock* b = containingBlock();
    if (b && !b->isRenderView())
        return b->scroll(direction, granularity, multiplier);
    return false;
}

bool RenderObject::shouldAutoscroll() const
{
    return ((isRoot()) || (hasOverflowClip() && (scrollsOverflow() || (node() && node()->isContentEditable()))));
}

void RenderObject::autoscroll()
{
    if (RenderLayer* l = layer())
        l->autoscroll();
}

bool RenderObject::hasStaticX() const
{
    return (style()->left().isAuto() && style()->right().isAuto()) || style()->left().isStatic() || style()->right().isStatic();
}

bool RenderObject::hasStaticY() const
{
    return (style()->top().isAuto() && style()->bottom().isAuto()) || style()->top().isStatic();
}

void RenderObject::markAllDescendantsWithFloatsForLayout(RenderObject*)
{
}

void RenderObject::setNeedsLayout(bool b, bool markParents)
{
    bool alreadyNeededLayout = m_needsLayout;
    m_needsLayout = b;
    if (b) {
        if (!alreadyNeededLayout && markParents)
            markContainingBlocksForLayout();
    } else {
        m_posChildNeedsLayout = false;
        m_normalChildNeedsLayout = false;
    }
}

void RenderObject::setChildNeedsLayout(bool b, bool markParents)
{
    bool alreadyNeededLayout = m_normalChildNeedsLayout;
    m_normalChildNeedsLayout = b;
    if (b) {
        if (!alreadyNeededLayout && markParents)
            markContainingBlocksForLayout();
    } else {
        m_posChildNeedsLayout = false;
        m_normalChildNeedsLayout = false;
    }
}

void RenderObject::markContainingBlocksForLayout(bool scheduleRelayout)
{
    RenderObject* o = container();
    RenderObject* last = this;

    while (o) {
        if (!last->isText() && (last->style()->position() == FixedPosition || last->style()->position() == AbsolutePosition)) {
            if (o->m_posChildNeedsLayout)
                return;
            o->m_posChildNeedsLayout = true;
        } else {
            if (o->m_normalChildNeedsLayout)
                return;
            o->m_normalChildNeedsLayout = true;
        }

        last = o;
        if (scheduleRelayout && (last->isTextField() || last->isTextArea()))
            break;
        o = o->container();
    }

    if (scheduleRelayout)
        last->scheduleRelayout();
}

RenderBlock* RenderObject::containingBlock() const
{
    if (isTableCell())
        return static_cast<const RenderTableCell*>(this)->table();
    if (isRenderView())
        return (RenderBlock*)this;

    RenderObject* o = parent();
    if (!isText() && m_style->position() == FixedPosition) {
        while ( o && !o->isRenderView() )
            o = o->parent();
    } else if (!isText() && m_style->position() == AbsolutePosition) {
        while (o && (o->style()->position() == StaticPosition || (o->isInline() && !o->isReplaced())) && !o->isRenderView()) {
            // For relpositioned inlines, we return the nearest enclosing block.  We don't try
            // to return the inline itself.  This allows us to avoid having a positioned objects
            // list in all RenderInlines and lets us return a strongly-typed RenderBlock* result
            // from this method.  The container() method can actually be used to obtain the
            // inline directly.
            if (o->style()->position() == RelativePosition && o->isInline() && !o->isReplaced())
                return o->containingBlock();
            o = o->parent();
        }
    } else {
        while (o && ((o->isInline() && !o->isReplaced()) || o->isTableRow() || o->isTableSection()
                     || o->isTableCol() || o->isFrameSet()
#ifdef SVG_SUPPORT
                     || o->isKCanvasContainer()
#endif
                     ))
            o = o->parent();
    }

    if (!o || !o->isRenderBlock())
        return 0; // Probably doesn't happen any more, but leave just in case. -dwh

    return static_cast<RenderBlock*>(o);
}

int RenderObject::containingBlockWidth() const
{
    // FIXME ?
    return containingBlock()->contentWidth();
}

int RenderObject::containingBlockHeight() const
{
    // FIXME ?
    return containingBlock()->contentHeight();
}

bool RenderObject::mustRepaintBackgroundOrBorder() const
{
    // If we don't have a background/border, then nothing to do.
    if (!shouldPaintBackgroundOrBorder())
        return false;

    // Ok, let's check the background first.
    const BackgroundLayer* bgLayer = style()->backgroundLayers();

    // Nobody will use multiple background layers without wanting fancy positioning.
    if (bgLayer->next())
        return true;

    // Make sure we have a valid background image.
    CachedImage* bg = bgLayer->backgroundImage();
    bool shouldPaintBackgroundImage = bg && bg->canRender();

    // These are always percents or auto.
    if (shouldPaintBackgroundImage &&
            (bgLayer->backgroundXPosition().value() != 0 || bgLayer->backgroundYPosition().value() != 0 ||
             bgLayer->backgroundSize().width.isPercent() || bgLayer->backgroundSize().height.isPercent()))
        // The background image will shift unpredictably if the size changes.
        return true;

    // Background is ok.  Let's check border.
    if (style()->hasBorder()) {
        // Border images are not ok.
        CachedImage* borderImage = style()->borderImage().image();
        bool shouldPaintBorderImage = borderImage && borderImage->canRender();

        // If the image hasn't loaded, we're still using the normal border style.
        if (shouldPaintBorderImage && borderImage->isLoaded())
            return true;
    }

    return false;
}

void RenderObject::drawBorderArc(GraphicsContext* graphicsContext, int x, int y, float thickness, IntSize radius,
                                 int angleStart, int angleSpan, BorderSide s, Color c, const Color& textColor,
                                 EBorderStyle style, bool firstCorner)
{
    if ((style == DOUBLE && thickness / 2 < 3) || ((style == RIDGE || style == GROOVE) && thickness / 2 < 2))
        style = SOLID;

    if (!c.isValid()) {
        if (style == INSET || style == OUTSET || style == RIDGE || style == GROOVE)
            c.setRGB(238, 238, 238);
        else
            c = textColor;
    }

    switch (style) {
        case BNONE:
        case BHIDDEN:
            return;
        case DOTTED:
        case DASHED:
            graphicsContext->setStrokeColor(c);
            graphicsContext->setStrokeStyle(style == DOTTED ? DottedStroke : DashedStroke);
            graphicsContext->setStrokeThickness(thickness);
            graphicsContext->strokeArc(IntRect(x, y, radius.width() * 2, radius.height() * 2), angleStart, angleSpan);
            break;
        case DOUBLE: {
            float third = thickness / 3.0f;
            float innerThird = (thickness + 1.0f) / 6.0f;
            int shiftForInner = static_cast<int>(innerThird * 2.5f);

            int outerY = y;
            int outerHeight = radius.height() * 2;
            int innerX = x + shiftForInner;
            int innerY = y + shiftForInner;
            int innerWidth = (radius.width() - shiftForInner) * 2;
            int innerHeight = (radius.height() - shiftForInner) * 2;
            if (innerThird > 1 && (s == BSTop || (firstCorner && (s == BSLeft || s == BSRight)))) {
                outerHeight += 2;
                innerHeight += 2;
            }

            graphicsContext->setStrokeStyle(SolidStroke);
            graphicsContext->setStrokeColor(c);
            graphicsContext->setStrokeThickness(third);
            graphicsContext->strokeArc(IntRect(x, outerY, radius.width() * 2, outerHeight), angleStart, angleSpan);
            graphicsContext->setStrokeThickness(innerThird > 2 ? innerThird - 1 : innerThird);
            graphicsContext->strokeArc(IntRect(innerX, innerY, innerWidth, innerHeight), angleStart, angleSpan);
            break;
        }
        case GROOVE:
        case RIDGE: {
            Color c2;
            if ((style == RIDGE && (s == BSTop || s == BSLeft)) ||
                    (style == GROOVE && (s == BSBottom || s == BSRight)))
                c2 = c.dark();
            else {
                c2 = c;
                c = c.dark();
            }

            graphicsContext->setStrokeStyle(SolidStroke);
            graphicsContext->setStrokeColor(c);
            graphicsContext->setStrokeThickness(thickness);
            graphicsContext->strokeArc(IntRect(x, y, radius.width() * 2, radius.height() * 2), angleStart, angleSpan);

            float halfThickness = (thickness + 1.0f) / 4.0f;
            int shiftForInner = static_cast<int>(halfThickness * 1.5f);
            graphicsContext->setStrokeColor(c2);
            graphicsContext->setStrokeThickness(halfThickness > 2 ? halfThickness - 1 : halfThickness);
            graphicsContext->strokeArc(IntRect(x + shiftForInner, y + shiftForInner, (radius.width() - shiftForInner) * 2,
                                       (radius.height() - shiftForInner) * 2), angleStart, angleSpan);
            break;
        }
        case INSET:
            if (s == BSTop || s == BSLeft)
                c = c.dark();
        case OUTSET:
            if (style == OUTSET && (s == BSBottom || s == BSRight))
                c = c.dark();
        case SOLID:
            graphicsContext->setStrokeStyle(SolidStroke);
            graphicsContext->setStrokeColor(c);
            graphicsContext->setStrokeThickness(thickness);
            graphicsContext->strokeArc(IntRect(x, y, radius.width() * 2, radius.height() * 2), angleStart, angleSpan);
            break;
    }
}

void RenderObject::drawBorder(GraphicsContext* graphicsContext, int x1, int y1, int x2, int y2,
                              BorderSide s, Color c, const Color& textcolor, EBorderStyle style,
                              int adjbw1, int adjbw2)
{
    int width = (s == BSTop || s == BSBottom ? y2 - y1 : x2 - x1);

    if (style == DOUBLE && width < 3)
        style = SOLID;

    if (!c.isValid()) {
        if (style == INSET || style == OUTSET || style == RIDGE || style == GROOVE)
            c.setRGB(238, 238, 238);
        else
            c = textcolor;
    }

    switch (style) {
        case BNONE:
        case BHIDDEN:
            return;
        case DOTTED:
        case DASHED:
            graphicsContext->setStrokeColor(c);
            graphicsContext->setStrokeThickness(width);
            graphicsContext->setStrokeStyle(style == DASHED ? DashedStroke : DottedStroke);

            if (width > 0)
                switch (s) {
                    case BSBottom:
                    case BSTop:
                        graphicsContext->drawLine(IntPoint(x1, (y1 + y2) / 2), IntPoint(x2, (y1 + y2) / 2));
                        break;
                    case BSRight:
                    case BSLeft:
                        graphicsContext->drawLine(IntPoint((x1 + x2) / 2, y1), IntPoint((x1 + x2) / 2, y2));
                        break;
                }
            break;
        case DOUBLE: {
            int third = (width + 1) / 3;

            if (adjbw1 == 0 && adjbw2 == 0) {
                graphicsContext->setStrokeStyle(NoStroke);
                graphicsContext->setFillColor(c);
                switch (s) {
                    case BSTop:
                    case BSBottom:
                        graphicsContext->drawRect(IntRect(x1, y1, x2 - x1, third));
                        graphicsContext->drawRect(IntRect(x1, y2 - third, x2 - x1, third));
                        break;
                    case BSLeft:
                        graphicsContext->drawRect(IntRect(x1, y1 + 1, third, y2 - y1 - 1));
                        graphicsContext->drawRect(IntRect(x2 - third, y1 + 1, third, y2 - y1 - 1));
                        break;
                    case BSRight:
                        graphicsContext->drawRect(IntRect(x1, y1 + 1, third, y2 - y1 - 1));
                        graphicsContext->drawRect(IntRect(x2 - third, y1 + 1, third, y2 - y1 - 1));
                        break;
                }
            } else {
                int adjbw1bigthird = ((adjbw1 > 0) ? adjbw1 + 1 : adjbw1 - 1) / 3;
                int adjbw2bigthird = ((adjbw2 > 0) ? adjbw2 + 1 : adjbw2 - 1) / 3;

                switch (s) {
                    case BSTop:
                        drawBorder(graphicsContext, x1 + max((-adjbw1 * 2 + 1) / 3, 0),
                                   y1, x2 - max((-adjbw2 * 2 + 1) / 3, 0), y1 + third,
                                   s, c, textcolor, SOLID, adjbw1bigthird, adjbw2bigthird);
                        drawBorder(graphicsContext, x1 + max((adjbw1 * 2 + 1) / 3, 0),
                                   y2 - third, x2 - max((adjbw2 * 2 + 1) / 3, 0), y2,
                                   s, c, textcolor, SOLID, adjbw1bigthird, adjbw2bigthird);
                        break;
                    case BSLeft:
                        drawBorder(graphicsContext, x1, y1 + max((-adjbw1 * 2 + 1) / 3, 0),
                                   x1 + third, y2 - max((-adjbw2 * 2 + 1) / 3, 0),
                                   s, c, textcolor, SOLID, adjbw1bigthird, adjbw2bigthird);
                        drawBorder(graphicsContext, x2 - third, y1 + max((adjbw1 * 2 + 1) / 3, 0),
                                   x2, y2 - max((adjbw2 * 2 + 1) / 3, 0),
                                   s, c, textcolor, SOLID, adjbw1bigthird, adjbw2bigthird);
                        break;
                    case BSBottom:
                        drawBorder(graphicsContext, x1 + max((adjbw1 * 2 + 1) / 3, 0),
                                   y1, x2 - max((adjbw2 * 2 + 1) / 3, 0), y1 + third,
                                   s, c, textcolor, SOLID, adjbw1bigthird, adjbw2bigthird);
                        drawBorder(graphicsContext, x1 + max((-adjbw1 * 2 + 1) / 3, 0),
                                   y2 - third, x2 - max((-adjbw2 * 2 + 1) / 3, 0), y2,
                                   s, c, textcolor, SOLID, adjbw1bigthird, adjbw2bigthird);
                        break;
                    case BSRight:
                        drawBorder(graphicsContext, x1, y1 + max((adjbw1 * 2 + 1) / 3, 0),
                                   x1 + third, y2 - max(( adjbw2 * 2 + 1) / 3, 0),
                                   s, c, textcolor, SOLID, adjbw1bigthird, adjbw2bigthird);
                        drawBorder(graphicsContext, x2 - third, y1 + max((-adjbw1 * 2 + 1) / 3, 0),
                                   x2, y2 - max((-adjbw2 * 2 + 1) / 3, 0),
                                   s, c, textcolor, SOLID, adjbw1bigthird, adjbw2bigthird);
                        break;
                    default:
                        break;
                }
            }
            break;
        }
        case RIDGE:
        case GROOVE:
        {
            EBorderStyle s1;
            EBorderStyle s2;
            if (style == GROOVE) {
                s1 = INSET;
                s2 = OUTSET;
            } else {
                s1 = OUTSET;
                s2 = INSET;
            }

            int adjbw1bighalf = ((adjbw1 > 0) ? adjbw1 + 1 : adjbw1 - 1) / 2;
            int adjbw2bighalf = ((adjbw2 > 0) ? adjbw2 + 1 : adjbw2 - 1) / 2;

            switch (s) {
                case BSTop:
                    drawBorder(graphicsContext, x1 + max(-adjbw1, 0) / 2, y1, x2 - max(-adjbw2, 0) / 2, (y1 + y2 + 1) / 2,
                               s, c, textcolor, s1, adjbw1bighalf, adjbw2bighalf);
                    drawBorder(graphicsContext, x1 + max(adjbw1 + 1, 0) / 2, (y1 + y2 + 1) / 2, x2 - max(adjbw2 + 1, 0) / 2, y2,
                               s, c, textcolor, s2, adjbw1 / 2, adjbw2 / 2);
                    break;
                case BSLeft:
                    drawBorder(graphicsContext, x1, y1 + max(-adjbw1, 0) / 2, (x1 + x2 + 1) / 2, y2 - max(-adjbw2, 0) / 2,
                               s, c, textcolor, s1, adjbw1bighalf, adjbw2bighalf);
                    drawBorder(graphicsContext, (x1 + x2 + 1) / 2, y1 + max(adjbw1 + 1, 0) / 2, x2, y2 - max(adjbw2 + 1, 0) / 2,
                               s, c, textcolor, s2, adjbw1 / 2, adjbw2 / 2);
                    break;
                case BSBottom:
                    drawBorder(graphicsContext, x1 + max(adjbw1, 0) / 2, y1, x2 - max(adjbw2, 0) / 2, (y1 + y2 + 1) / 2,
                               s, c, textcolor, s2, adjbw1bighalf, adjbw2bighalf);
                    drawBorder(graphicsContext, x1 + max(-adjbw1 + 1, 0) / 2, (y1 + y2 + 1) / 2, x2 - max(-adjbw2 + 1, 0) / 2, y2,
                               s, c, textcolor, s1, adjbw1/2, adjbw2/2);
                    break;
                case BSRight:
                    drawBorder(graphicsContext, x1, y1 + max(adjbw1, 0) / 2, (x1 + x2 + 1) / 2, y2 - max(adjbw2, 0) / 2,
                               s, c, textcolor, s2, adjbw1bighalf, adjbw2bighalf);
                    drawBorder(graphicsContext, (x1 + x2 + 1) / 2, y1 + max(-adjbw1 + 1, 0) / 2, x2, y2 - max(-adjbw2 + 1, 0) / 2,
                               s, c, textcolor, s1, adjbw1/2, adjbw2/2);
                    break;
            }
            break;
        }
        case INSET:
            if (s == BSTop || s == BSLeft)
                c = c.dark();
            // fall through
        case OUTSET:
            if (style == OUTSET && (s == BSBottom || s == BSRight))
                c = c.dark();
            // fall through
        case SOLID: {
            graphicsContext->setStrokeStyle(NoStroke);
            graphicsContext->setFillColor(c);
            ASSERT(x2 >= x1);
            ASSERT(y2 >= y1);
            if (!adjbw1 && !adjbw2) {
                graphicsContext->drawRect(IntRect(x1, y1, x2 - x1, y2 - y1));
                return;
            }
            FloatPoint quad[4];
            switch (s) {
                case BSTop:
                    quad[0] = FloatPoint(x1 + max(-adjbw1, 0), y1);
                    quad[1] = FloatPoint(x1 + max(adjbw1, 0), y2);
                    quad[2] = FloatPoint(x2 - max(adjbw2, 0), y2);
                    quad[3] = FloatPoint(x2 - max(-adjbw2, 0), y1);
                    break;
                case BSBottom:
                    quad[0] = FloatPoint(x1 + max(adjbw1, 0), y1);
                    quad[1] = FloatPoint(x1 + max(-adjbw1, 0), y2);
                    quad[2] = FloatPoint(x2 - max(-adjbw2, 0), y2);
                    quad[3] = FloatPoint(x2 - max(adjbw2, 0), y1);
                    break;
                case BSLeft:
                    quad[0] = FloatPoint(x1, y1 + max(-adjbw1, 0));
                    quad[1] = FloatPoint(x1, y2 - max(-adjbw2, 0));
                    quad[2] = FloatPoint(x2, y2 - max(adjbw2, 0));
                    quad[3] = FloatPoint(x2, y1 + max(adjbw1, 0));
                    break;
                case BSRight:
                    quad[0] = FloatPoint(x1, y1 + max(adjbw1, 0));
                    quad[1] = FloatPoint(x1, y2 - max(adjbw2, 0));
                    quad[2] = FloatPoint(x2, y2 - max(-adjbw2, 0));
                    quad[3] = FloatPoint(x2, y1 + max(-adjbw1, 0));
                    break;
            }
            graphicsContext->drawConvexPolygon(4, quad);
            break;
        }
    }
}

bool RenderObject::paintBorderImage(GraphicsContext* graphicsContext, int tx, int ty, int w, int h, const RenderStyle* style)
{
    CachedImage* borderImage = style->borderImage().image();
    if (!borderImage->isLoaded())
        return true; // Never paint a border image incrementally, but don't paint the fallback borders either.

    // If we have a border radius, the border image gets clipped to the rounded rect.
    bool clipped = false;
    if (style->hasBorderRadius()) {
        IntRect clipRect(tx, ty, w, h);
        graphicsContext->save();
        graphicsContext->addRoundedRectClip(clipRect, style->borderTopLeftRadius(), style->borderTopRightRadius(),
                                            style->borderBottomLeftRadius(), style->borderBottomRightRadius());
        clipped = true;
    }

    int imageWidth = borderImage->image()->width();
    int imageHeight = borderImage->image()->height();

    int topSlice = min(imageHeight, style->borderImage().m_slices.top.calcValue(borderImage->image()->height()));
    int bottomSlice = min(imageHeight, style->borderImage().m_slices.bottom.calcValue(borderImage->image()->height()));
    int leftSlice = min(imageWidth, style->borderImage().m_slices.left.calcValue(borderImage->image()->width()));
    int rightSlice = min(imageWidth, style->borderImage().m_slices.right.calcValue(borderImage->image()->width()));

    EBorderImageRule hRule = style->borderImage().horizontalRule();
    EBorderImageRule vRule = style->borderImage().verticalRule();

    bool drawLeft = leftSlice > 0 && style->borderLeftWidth() > 0;
    bool drawTop = topSlice > 0 && style->borderTopWidth() > 0;
    bool drawRight = rightSlice > 0 && style->borderRightWidth() > 0;
    bool drawBottom = bottomSlice > 0 && style->borderBottomWidth() > 0;
    bool drawMiddle = (imageWidth - leftSlice - rightSlice) > 0 && (w - style->borderLeftWidth() - style->borderRightWidth()) > 0 &&
                      (imageHeight - topSlice - bottomSlice) > 0 && (h - style->borderTopWidth() - style->borderBottomWidth()) > 0;

    if (drawLeft) {
        // Paint the top and bottom left corners.

        // The top left corner rect is (tx, ty, leftWidth, topWidth)
        // The rect to use from within the image is obtained from our slice, and is (0, 0, leftSlice, topSlice)
        if (drawTop)
            graphicsContext->drawImage(borderImage->image(), IntRect(tx, ty, style->borderLeftWidth(), style->borderTopWidth()),
                                       IntRect(0, 0, leftSlice, topSlice));

        // The bottom left corner rect is (tx, ty + h - bottomWidth, leftWidth, bottomWidth)
        // The rect to use from within the image is (0, imageHeight - bottomSlice, leftSlice, botomSlice)
        if (drawBottom)
            graphicsContext->drawImage(borderImage->image(), IntRect(tx, ty + h - style->borderBottomWidth(), style->borderLeftWidth(), style->borderBottomWidth()),
                                       IntRect(0, imageHeight - bottomSlice, leftSlice, bottomSlice));

        // Paint the left edge.
        // Have to scale and tile into the border rect.
        graphicsContext->drawTiledImage(borderImage->image(), IntRect(tx, ty + style->borderTopWidth(), style->borderLeftWidth(),
                                        h - style->borderTopWidth() - style->borderBottomWidth()),
                                        IntRect(0, topSlice, leftSlice, imageHeight - topSlice - bottomSlice),
                                        Image::StretchTile, (Image::TileRule)vRule);
    }

    if (drawRight) {
        // Paint the top and bottom right corners
        // The top right corner rect is (tx + w - rightWidth, ty, rightWidth, topWidth)
        // The rect to use from within the image is obtained from our slice, and is (imageWidth - rightSlice, 0, rightSlice, topSlice)
        if (drawTop)
            graphicsContext->drawImage(borderImage->image(), IntRect(tx + w - style->borderRightWidth(), ty, style->borderRightWidth(), style->borderTopWidth()),
                                       IntRect(imageWidth - rightSlice, 0, rightSlice, topSlice));

        // The bottom right corner rect is (tx + w - rightWidth, ty + h - bottomWidth, rightWidth, bottomWidth)
        // The rect to use from within the image is (imageWidth - rightSlice, imageHeight - bottomSlice, rightSlice, botomSlice)
        if (drawBottom)
            graphicsContext->drawImage(borderImage->image(), IntRect(tx + w - style->borderRightWidth(), ty + h - style->borderBottomWidth(), style->borderRightWidth(), style->borderBottomWidth()),
                                       IntRect(imageWidth - rightSlice, imageHeight - bottomSlice, rightSlice, bottomSlice));

        // Paint the right edge.
        graphicsContext->drawTiledImage(borderImage->image(), IntRect(tx + w - style->borderRightWidth(), ty + style->borderTopWidth(), style->borderRightWidth(),
                                        h - style->borderTopWidth() - style->borderBottomWidth()),
                                        IntRect(imageWidth - rightSlice, topSlice, rightSlice, imageHeight - topSlice - bottomSlice),
                                        Image::StretchTile, (Image::TileRule)vRule);
    }

    // Paint the top edge.
    if (drawTop)
        graphicsContext->drawTiledImage(borderImage->image(), IntRect(tx + style->borderLeftWidth(), ty, w - style->borderLeftWidth() - style->borderRightWidth(), style->borderTopWidth()),
                                        IntRect(leftSlice, 0, imageWidth - rightSlice - leftSlice, topSlice),
                                        (Image::TileRule)hRule, Image::StretchTile);

    // Paint the bottom edge.
    if (drawBottom)
        graphicsContext->drawTiledImage(borderImage->image(), IntRect(tx + style->borderLeftWidth(), ty + h - style->borderBottomWidth(),
                                        w - style->borderLeftWidth() - style->borderRightWidth(), style->borderBottomWidth()),
                                        IntRect(leftSlice, imageHeight - bottomSlice, imageWidth - rightSlice - leftSlice, bottomSlice),
                                        (Image::TileRule)hRule, Image::StretchTile);

    // Paint the middle.
    if (drawMiddle)
        graphicsContext->drawTiledImage(borderImage->image(), IntRect(tx + style->borderLeftWidth(), ty + style->borderTopWidth(), w - style->borderLeftWidth() - style->borderRightWidth(),
                                        h - style->borderTopWidth() - style->borderBottomWidth()),
                                        IntRect(leftSlice, topSlice, imageWidth - rightSlice - leftSlice, imageHeight - topSlice - bottomSlice),
                                        (Image::TileRule)hRule, (Image::TileRule)vRule);

    // Clear the clip for the border radius.
    if (clipped)
        graphicsContext->restore();

    return true;
}

void RenderObject::paintBorder(GraphicsContext* graphicsContext, int tx, int ty, int w, int h,
                               const RenderStyle* style, bool begin, bool end)
{
    CachedImage* borderImage = style->borderImage().image();
    bool shouldPaintBackgroundImage = borderImage && borderImage->canRender();
    if (shouldPaintBackgroundImage)
        shouldPaintBackgroundImage = paintBorderImage(graphicsContext, tx, ty, w, h, style);

    if (shouldPaintBackgroundImage)
        return;

    const Color& tc = style->borderTopColor();
    const Color& bc = style->borderBottomColor();
    const Color& lc = style->borderLeftColor();
    const Color& rc = style->borderRightColor();

    bool tt = style->borderTopIsTransparent();
    bool bt = style->borderBottomIsTransparent();
    bool rt = style->borderRightIsTransparent();
    bool lt = style->borderLeftIsTransparent();

    EBorderStyle ts = style->borderTopStyle();
    EBorderStyle bs = style->borderBottomStyle();
    EBorderStyle ls = style->borderLeftStyle();
    EBorderStyle rs = style->borderRightStyle();

    bool renderTop = ts > BHIDDEN && !tt;
    bool renderLeft = ls > BHIDDEN && begin && !lt;
    bool renderRight = rs > BHIDDEN && end && !rt;
    bool renderBottom = bs > BHIDDEN && !bt;

    // Need sufficient width and height to contain border radius curves.  Sanity check our top/bottom
    // values and our width/height values to make sure the curves can all fit. If not, then we won't paint
    // any border radii.
    bool renderRadii = false;
    IntSize topLeft = style->borderTopLeftRadius();
    IntSize topRight = style->borderTopRightRadius();
    IntSize bottomLeft = style->borderBottomLeftRadius();
    IntSize bottomRight = style->borderBottomRightRadius();

    if (style->hasBorderRadius()) {
        int requiredWidth = max(topLeft.width() + topRight.width(), bottomLeft.width() + bottomRight.width());
        int requiredHeight = max(topLeft.height() + bottomLeft.height(), topRight.height() + bottomRight.height());
        renderRadii = (requiredWidth <= w && requiredHeight <= h);
    }

    // Clip to the rounded rectangle.
    if (renderRadii) {
        graphicsContext->save();
        graphicsContext->addRoundedRectClip(IntRect(tx, ty, w, h), topLeft, topRight, bottomLeft, bottomRight);
    }

    int firstAngleStart, secondAngleStart, firstAngleSpan, secondAngleSpan;
    float thickness;
    bool upperLeftBorderStylesMatch = renderLeft && (ts == ls) && (tc == lc);
    bool upperRightBorderStylesMatch = renderRight && (ts == rs) && (tc == rc) && (ts != OUTSET) && (ts != RIDGE) && (ts != INSET) && (ts != GROOVE);
    bool lowerLeftBorderStylesMatch = renderLeft && (bs == ls) && (bc == lc) && (bs != OUTSET) && (bs != RIDGE) && (bs != INSET) && (bs != GROOVE);
    bool lowerRightBorderStylesMatch = renderRight && (bs == rs) && (bc == rc);

    if (renderTop) {
        bool ignore_left = (renderRadii && topLeft.width() > 0) ||
            (tc == lc && tt == lt && ts >= OUTSET &&
             (ls == DOTTED || ls == DASHED || ls == SOLID || ls == OUTSET));

        bool ignore_right = (renderRadii && topRight.width() > 0) ||
            (tc == rc && tt == rt && ts >= OUTSET &&
             (rs == DOTTED || rs == DASHED || rs == SOLID || rs == INSET));

        int x = tx;
        int x2 = tx + w;
        if (renderRadii) {
            x += topLeft.width();
            x2 -= topRight.width();
        }

        drawBorder(graphicsContext, x, ty, x2, ty + style->borderTopWidth(), BSTop, tc, style->color(), ts,
                   ignore_left ? 0 : style->borderLeftWidth(), ignore_right ? 0 : style->borderRightWidth());

        if (renderRadii) {
            int leftY = ty;

            // We make the arc double thick and let the clip rect take care of clipping the extra off.
            // We're doing this because it doesn't seem possible to match the curve of the clip exactly
            // with the arc-drawing function.
            thickness = style->borderTopWidth() * 2;

            if (topLeft.width()) {
                int leftX = tx;
                // The inner clip clips inside the arc. This is especially important for 1px borders.
                bool applyLeftInnerClip = (style->borderLeftWidth() < topLeft.width())
                    && (style->borderTopWidth() < topLeft.height())
                    && (ts != DOUBLE || style->borderTopWidth() > 6);
                if (applyLeftInnerClip) {
                    graphicsContext->save();
                    graphicsContext->addInnerRoundedRectClip(IntRect(leftX, leftY, topLeft.width() * 2, topLeft.height() * 2),
                                                             style->borderTopWidth());
                }

                firstAngleStart = 90;
                firstAngleSpan = upperLeftBorderStylesMatch ? 90 : 45;

                // Draw upper left arc
                drawBorderArc(graphicsContext, leftX, leftY, thickness, topLeft, firstAngleStart, firstAngleSpan,
                              BSTop, tc, style->color(), ts, true);
                if (applyLeftInnerClip)
                    graphicsContext->restore();
            }

            if (topRight.width()) {
                int rightX = tx + w - topRight.width() * 2;
                bool applyRightInnerClip = (style->borderRightWidth() < topRight.width())
                    && (style->borderTopWidth() < topRight.height())
                    && (ts != DOUBLE || style->borderTopWidth() > 6);
                if (applyRightInnerClip) {
                    graphicsContext->save();
                    graphicsContext->addInnerRoundedRectClip(IntRect(rightX, leftY, topRight.width() * 2, topRight.height() * 2),
                                                             style->borderTopWidth());
                }

                if (upperRightBorderStylesMatch) {
                    secondAngleStart = 0;
                    secondAngleSpan = 90;
                } else {
                    secondAngleStart = 45;
                    secondAngleSpan = 45;
                }

                // Draw upper right arc
                drawBorderArc(graphicsContext, rightX, leftY, thickness, topRight, secondAngleStart, secondAngleSpan,
                              BSTop, tc, style->color(), ts, false);
                if (applyRightInnerClip)
                    graphicsContext->restore();
            }
        }
    }

    if (renderBottom) {
        bool ignore_left = (renderRadii && bottomLeft.width() > 0) ||
            (bc == lc && bt == lt && bs >= OUTSET &&
             (ls == DOTTED || ls == DASHED || ls == SOLID || ls == OUTSET));

        bool ignore_right = (renderRadii && bottomRight.width() > 0) ||
            (bc == rc && bt == rt && bs >= OUTSET &&
             (rs == DOTTED || rs == DASHED || rs == SOLID || rs == INSET));

        int x = tx;
        int x2 = tx + w;
        if (renderRadii) {
            x += bottomLeft.width();
            x2 -= bottomRight.width();
        }

        drawBorder(graphicsContext, x, ty + h - style->borderBottomWidth(), x2, ty + h, BSBottom, bc, style->color(), bs,
                   ignore_left ? 0 : style->borderLeftWidth(), ignore_right ? 0 : style->borderRightWidth());

        if (renderRadii) {
            thickness = style->borderBottomWidth() * 2;

            if (bottomLeft.width()) {
                int leftX = tx;
                int leftY = ty + h - bottomLeft.height() * 2;
                bool applyLeftInnerClip = (style->borderLeftWidth() < bottomLeft.width())
                    && (style->borderBottomWidth() < bottomLeft.height())
                    && (bs != DOUBLE || style->borderBottomWidth() > 6);
                if (applyLeftInnerClip) {
                    graphicsContext->save();
                    graphicsContext->addInnerRoundedRectClip(IntRect(leftX, leftY, bottomLeft.width() * 2, bottomLeft.height() * 2),
                                                             style->borderBottomWidth());
                }

                if (lowerLeftBorderStylesMatch) {
                    firstAngleStart = 180;
                    firstAngleSpan = 90;
                } else {
                    firstAngleStart = 225;
                    firstAngleSpan = 45;
                }

                // Draw lower left arc
                drawBorderArc(graphicsContext, leftX, leftY, thickness, bottomLeft, firstAngleStart, firstAngleSpan,
                              BSBottom, bc, style->color(), bs, true);
                if (applyLeftInnerClip)
                    graphicsContext->restore();
            }

            if (bottomRight.width()) {
                int rightY = ty + h - bottomRight.height() * 2;
                int rightX = tx + w - bottomRight.width() * 2;
                bool applyRightInnerClip = (style->borderRightWidth() < bottomRight.width())
                    && (style->borderBottomWidth() < bottomRight.height())
                    && (bs != DOUBLE || style->borderBottomWidth() > 6);
                if (applyRightInnerClip) {
                    graphicsContext->save();
                    graphicsContext->addInnerRoundedRectClip(IntRect(rightX, rightY, bottomRight.width() * 2, bottomRight.height() * 2),
                                                             style->borderBottomWidth());
                }

                secondAngleStart = 270;
                secondAngleSpan = lowerRightBorderStylesMatch ? 90 : 45;

                // Draw lower right arc
                drawBorderArc(graphicsContext, rightX, rightY, thickness, bottomRight, secondAngleStart, secondAngleSpan,
                              BSBottom, bc, style->color(), bs, false);
                if (applyRightInnerClip)
                    graphicsContext->restore();
            }
        }
    }

    if (renderLeft) {
        bool ignore_top = (renderRadii && topLeft.height() > 0) ||
            (tc == lc && tt == lt && ls >= OUTSET &&
             (ts == DOTTED || ts == DASHED || ts == SOLID || ts == OUTSET));

        bool ignore_bottom = (renderRadii && bottomLeft.height() > 0) ||
            (bc == lc && bt == lt && ls >= OUTSET &&
             (bs == DOTTED || bs == DASHED || bs == SOLID || bs == INSET));

        int y = ty;
        int y2 = ty + h;
        if (renderRadii) {
            y += topLeft.height();
            y2 -= bottomLeft.height();
        }

        drawBorder(graphicsContext, tx, y, tx + style->borderLeftWidth(), y2, BSLeft, lc, style->color(), ls,
                   ignore_top ? 0 : style->borderTopWidth(), ignore_bottom ? 0 : style->borderBottomWidth());

        if (renderRadii && (!upperLeftBorderStylesMatch || !lowerLeftBorderStylesMatch)) {
            int topX = tx;
            thickness = style->borderLeftWidth() * 2;

            if (!upperLeftBorderStylesMatch && topLeft.width()) {
                int topY = ty;
                bool applyTopInnerClip = (style->borderLeftWidth() < topLeft.width())
                    && (style->borderTopWidth() < topLeft.height())
                    && (ls != DOUBLE || style->borderLeftWidth() > 6);
                if (applyTopInnerClip) {
                    graphicsContext->save();
                    graphicsContext->addInnerRoundedRectClip(IntRect(topX, topY, topLeft.width() * 2, topLeft.height() * 2),
                                                             style->borderLeftWidth());
                }

                firstAngleStart = 135;
                firstAngleSpan = 45;

                // Draw top left arc
                drawBorderArc(graphicsContext, topX, topY, thickness, topLeft, firstAngleStart, firstAngleSpan,
                              BSLeft, lc, style->color(), ls, true);
                if (applyTopInnerClip)
                    graphicsContext->restore();
            }

            if (!lowerLeftBorderStylesMatch && bottomLeft.width()) {
                int bottomY = ty + h - bottomLeft.height() * 2;
                bool applyBottomInnerClip = (style->borderLeftWidth() < bottomLeft.width())
                    && (style->borderBottomWidth() < bottomLeft.height())
                    && (ls != DOUBLE || style->borderLeftWidth() > 6);
                if (applyBottomInnerClip) {
                    graphicsContext->save();
                    graphicsContext->addInnerRoundedRectClip(IntRect(topX, bottomY, bottomLeft.width() * 2, bottomLeft.height() * 2),
                                                             style->borderLeftWidth());
                }

                secondAngleStart = 180;
                secondAngleSpan = 45;

                // Draw bottom left arc
                drawBorderArc(graphicsContext, topX, bottomY, thickness, bottomLeft, secondAngleStart, secondAngleSpan,
                              BSLeft, lc, style->color(), ls, false);
                if (applyBottomInnerClip)
                    graphicsContext->restore();
            }
        }
    }

    if (renderRight) {
        bool ignore_top = (renderRadii && topRight.height() > 0) ||
            ((tc == rc) && (tt == rt) &&
            (rs >= DOTTED || rs == INSET) &&
            (ts == DOTTED || ts == DASHED || ts == SOLID || ts == OUTSET));

        bool ignore_bottom = (renderRadii && bottomRight.height() > 0) ||
            ((bc == rc) && (bt == rt) &&
            (rs >= DOTTED || rs == INSET) &&
            (bs == DOTTED || bs == DASHED || bs == SOLID || bs == INSET));

        int y = ty;
        int y2 = ty + h;
        if (renderRadii) {
            y += topRight.height();
            y2 -= bottomRight.height();
        }

        drawBorder(graphicsContext, tx + w - style->borderRightWidth(), y, tx + w, y2, BSRight, rc, style->color(), rs,
                   ignore_top ? 0 : style->borderTopWidth(), ignore_bottom ? 0 : style->borderBottomWidth());

        if (renderRadii && (!upperRightBorderStylesMatch || !lowerRightBorderStylesMatch)) {
            thickness = style->borderRightWidth() * 2;

            if (!upperRightBorderStylesMatch && topRight.width()) {
                int topX = tx + w - topRight.width() * 2;
                int topY = ty;
                bool applyTopInnerClip = (style->borderRightWidth() < topRight.width())
                    && (style->borderTopWidth() < topRight.height())
                    && (rs != DOUBLE || style->borderRightWidth() > 6);
                if (applyTopInnerClip) {
                    graphicsContext->save();
                    graphicsContext->addInnerRoundedRectClip(IntRect(topX, topY, topRight.width() * 2, topRight.height() * 2),
                                                             style->borderRightWidth());
                }

                firstAngleStart = 0;
                firstAngleSpan = 45;

                // Draw top right arc
                drawBorderArc(graphicsContext, topX, topY, thickness, topRight, firstAngleStart, firstAngleSpan,
                              BSRight, rc, style->color(), rs, true);
                if (applyTopInnerClip)
                    graphicsContext->restore();
            }

            if (!lowerRightBorderStylesMatch && bottomRight.width()) {
                int bottomX = tx + w - bottomRight.width() * 2;
                int bottomY = ty + h - bottomRight.height() * 2;
                bool applyBottomInnerClip = (style->borderRightWidth() < bottomRight.width())
                    && (style->borderBottomWidth() < bottomRight.height())
                    && (rs != DOUBLE || style->borderRightWidth() > 6);
                if (applyBottomInnerClip) {
                    graphicsContext->save();
                    graphicsContext->addInnerRoundedRectClip(IntRect(bottomX, bottomY, bottomRight.width() * 2, bottomRight.height() * 2),
                                                             style->borderRightWidth());
                }

                secondAngleStart = 315;
                secondAngleSpan = 45;

                // Draw bottom right arc
                drawBorderArc(graphicsContext, bottomX, bottomY, thickness, bottomRight, secondAngleStart, secondAngleSpan,
                              BSRight, rc, style->color(), rs, false);
                if (applyBottomInnerClip)
                    graphicsContext->restore();
            }
        }
    }

    if (renderRadii)
        graphicsContext->restore();
}

void RenderObject::addLineBoxRects(Vector<IntRect>&, unsigned startOffset, unsigned endOffset)
{
}

void RenderObject::absoluteRects(Vector<IntRect>& rects, int tx, int ty)
{
    // For blocks inside inlines, we go ahead and include margins so that we run right up to the
    // inline boxes above and below us (thus getting merged with them to form a single irregular
    // shape).
    if (continuation()) {
        rects.append(IntRect(tx, ty - collapsedMarginTop(),
                             width(), height() + collapsedMarginTop() + collapsedMarginBottom()));
        continuation()->absoluteRects(rects,
                                      tx - xPos() + continuation()->containingBlock()->xPos(),
                                      ty - yPos() + continuation()->containingBlock()->yPos());
    } else
        rects.append(IntRect(tx, ty, width(), height() + borderTopExtra() + borderBottomExtra()));
}

IntRect RenderObject::absoluteBoundingBoxRect()
{
    int x, y;
    absolutePosition(x, y);
    Vector<IntRect> rects;
    absoluteRects(rects, x, y);

    size_t n = rects.size();
    if (!n)
        return IntRect();

    IntRect result = rects[0];
    for (size_t i = 1; i < n; ++i)
        result.unite(rects[i]);
    return result;
}

void RenderObject::addAbsoluteRectForLayer(IntRect& result)
{
    if (layer())
        result.unite(absoluteBoundingBoxRect());
    for (RenderObject* current = firstChild(); current; current = current->nextSibling())
        current->addAbsoluteRectForLayer(result);
}

IntRect RenderObject::paintingRootRect(IntRect& topLevelRect)
{
    IntRect result = absoluteBoundingBoxRect();
    topLevelRect = result;
    for (RenderObject* current = firstChild(); current; current = current->nextSibling())
        current->addAbsoluteRectForLayer(result);
    return result;
}

void RenderObject::addPDFURLRect(GraphicsContext* graphicsContext, IntRect rect)
{
    Node* node = element();
    if (node) {
        if (graphicsContext) {
            if (rect.width() > 0 && rect.height() > 0) {
                Element* element = static_cast<Element*>(node);
                String href;
                if (element->isLink())
                    href = element->getAttribute(hrefAttr);

                if (!href.isNull()) {
                    KURL link = element->document()->completeURL(href.deprecatedString());
                    if (link.isValid())
                        graphicsContext->setURLForRect(link, rect);
                }
            }
        }
    }
}


void RenderObject::addFocusRingRects(GraphicsContext* graphicsContext, int tx, int ty)
{
    // For blocks inside inlines, we go ahead and include margins so that we run right up to the
    // inline boxes above and below us (thus getting merged with them to form a single irregular
    // shape).
    if (continuation()) {
        graphicsContext->addFocusRingRect(IntRect(tx, ty - collapsedMarginTop(), width(), height() + collapsedMarginTop() + collapsedMarginBottom()));
        continuation()->addFocusRingRects(graphicsContext,
                                          tx - xPos() + continuation()->containingBlock()->xPos(),
                                          ty - yPos() + continuation()->containingBlock()->yPos());
    } else
        graphicsContext->addFocusRingRect(IntRect(tx, ty, width(), height()));
}

void RenderObject::paintOutline(GraphicsContext* graphicsContext, int tx, int ty, int w, int h, const RenderStyle* style)
{
    if (!hasOutline())
        return;

    int ow = style->outlineWidth();

    EBorderStyle os = style->outlineStyle();

    Color oc = style->outlineColor();
    if (!oc.isValid())
        oc = style->color();

    int offset = style->outlineOffset();

    if (style->outlineStyleIsAuto() || hasOutlineAnnotation()) {
        if (!theme()->supportsFocusRing(style)) {
            // Only paint the focus ring by hand if the theme isn't able to draw the focus ring.
            graphicsContext->initFocusRing(ow, offset);
            if (style->outlineStyleIsAuto())
                addFocusRingRects(graphicsContext, tx, ty);
            else
                addPDFURLRect(graphicsContext, graphicsContext->focusRingBoundingRect());
            graphicsContext->drawFocusRing(oc);
            graphicsContext->clearFocusRing();
        }
    }

    if (style->outlineStyleIsAuto() || style->outlineStyle() <= BHIDDEN)
        return;

    tx -= offset;
    ty -= offset;
    w += 2 * offset;
    h += 2 * offset;

    drawBorder(graphicsContext, tx - ow, ty - ow, tx, ty + h + ow,
               BSLeft, Color(oc), style->color(), os, ow, ow);

    drawBorder(graphicsContext, tx - ow, ty - ow, tx + w + ow, ty,
               BSTop, Color(oc), style->color(), os, ow, ow);

    drawBorder(graphicsContext, tx + w, ty - ow, tx + w + ow, ty + h + ow,
               BSRight, Color(oc), style->color(), os, ow, ow);

    drawBorder(graphicsContext, tx - ow, ty + h, tx + w + ow, ty + h + ow,
               BSBottom, Color(oc), style->color(), os, ow, ow);
}

void RenderObject::paint(PaintInfo& /*paintInfo*/, int /*tx*/, int /*ty*/)
{
}

void RenderObject::repaint(bool immediate)
{
    // Can't use view(), since we might be unrooted.
    RenderObject* o = this;
    while (o->parent())
        o = o->parent();
    if (!o->isRenderView())
        return;
    RenderView* view = static_cast<RenderView*>(o);
    if (view->printingMode())
        return; // Don't repaint if we're printing.
    view->repaintViewRectangle(getAbsoluteRepaintRect(), immediate);
}

void RenderObject::repaintRectangle(const IntRect& r, bool immediate)
{
    // Can't use view(), since we might be unrooted.
    RenderObject* o = this;
    while (o->parent())
        o = o->parent();
    if (!o->isRenderView())
        return;
    RenderView* view = static_cast<RenderView*>(o);
    if (view->printingMode())
        return; // Don't repaint if we're printing.
    IntRect absRect(r);
    computeAbsoluteRepaintRect(absRect);
    view->repaintViewRectangle(absRect, immediate);
}

bool RenderObject::repaintAfterLayoutIfNeeded(const IntRect& oldBounds, const IntRect& oldFullBounds)
{
    RenderView* v = view();
    if (v->printingMode())
        return false; // Don't repaint if we're printing.

    IntRect newBounds, newFullBounds;
    getAbsoluteRepaintRectIncludingFloats(newBounds, newFullBounds);
    if (newBounds == oldBounds && !selfNeedsLayout())
        return false;

    bool fullRepaint = selfNeedsLayout() || newBounds.location() != oldBounds.location() || mustRepaintBackgroundOrBorder();
    if (fullRepaint) {
        v->repaintViewRectangle(oldFullBounds);
        if (newBounds != oldBounds)
            v->repaintViewRectangle(newFullBounds);
        return true;
    }

    // We didn't move, but we did change size.  Invalidate the delta, which will consist of possibly
    // two rectangles (but typically only one).
    int ow = style() ? style()->outlineSize() : 0;
    int width = abs(newBounds.width() - oldBounds.width());
    if (width) {
        int borderWidth = max(borderRight(), max(style()->borderTopRightRadius().width(), style()->borderBottomRightRadius().width())) + ow;
        v->repaintViewRectangle(IntRect(newBounds.x() + min(newBounds.width(), oldBounds.width()) - borderWidth,
            newBounds.y(),
            width + borderWidth,
            max(newBounds.height(), oldBounds.height())));
    }
    int height = abs(newBounds.height() - oldBounds.height());
    if (height) {
        int borderHeight = max(borderBottom(), max(style()->borderBottomLeftRadius().height(), style()->borderBottomRightRadius().height())) + ow;
        v->repaintViewRectangle(IntRect(newBounds.x(),
            min(newBounds.bottom(), oldBounds.bottom()) - borderHeight,
            max(newBounds.width(), oldBounds.width()),
            height + borderHeight));
    }
    return false;
}

void RenderObject::repaintDuringLayoutIfMoved(const IntRect& rect)
{
}

void RenderObject::repaintOverhangingFloats(bool paintAllDescendants)
{
}

bool RenderObject::checkForRepaintDuringLayout() const
{
    return !document()->view()->needsFullRepaint() && !layer();
}

void RenderObject::repaintObjectsBeforeLayout()
{
    if (!needsLayout() || isText())
        return;

    bool blockWithInlineChildren = (isRenderBlock() && !isTable() && normalChildNeedsLayout() && childrenInline());
    if (selfNeedsLayout()) {
        repaint();
        if (blockWithInlineChildren)
            return;
    }

    for (RenderObject* current = firstChild(); current; current = current->nextSibling()) {
        if (!current->isPositioned()) // RenderBlock subclass method handles walking the positioned objects.
            current->repaintObjectsBeforeLayout();
    }
}

IntRect RenderObject::getAbsoluteRepaintRectWithOutline(int ow)
{
    IntRect r(getAbsoluteRepaintRect());
    r.inflate(ow);

    if (continuation() && !isInline())
        r.inflateY(collapsedMarginTop());

    if (isInlineFlow()) {
        for (RenderObject* curr = firstChild(); curr; curr = curr->nextSibling()) {
            if (!curr->isText())
                r.unite(curr->getAbsoluteRepaintRectWithOutline(ow));
        }
    }

    return r;
}

IntRect RenderObject::getAbsoluteRepaintRect()
{
    if (parent())
        return parent()->getAbsoluteRepaintRect();
    return IntRect();
}

void RenderObject::getAbsoluteRepaintRectIncludingFloats(IntRect& bounds, IntRect& fullBounds)
{
    bounds = fullBounds = getAbsoluteRepaintRect();
}

void RenderObject::computeAbsoluteRepaintRect(IntRect& r, bool f)
{
    if (parent())
        return parent()->computeAbsoluteRepaintRect(r, f);
}

void RenderObject::dirtyLinesFromChangedChild(RenderObject* child)
{
}

#ifndef NDEBUG

DeprecatedString RenderObject::information() const
{
    DeprecatedString str;
    TextStream ts(&str);
    ts << renderName()
       << "(" << (style() ? style()->refCount() : 0) << ")"
       << ": " << (void*)this << "  ";
    if (isInline())
        ts << "il ";
    if (childrenInline())
        ts << "ci ";
    if (isFloating())
        ts << "fl ";
    if (isAnonymous())
        ts << "an ";
    if (isRelPositioned())
        ts << "rp ";
    if (isPositioned())
        ts << "ps ";
    if (needsLayout())
        ts << "nl ";
    if (m_recalcMinMax)
        ts << "rmm ";
    if (style() && style()->zIndex())
        ts << "zI: " << style()->zIndex();
    if (element()) {
        if (element()->active())
            ts << "act ";
        if (element()->isLink())
            ts << "anchor ";
        if (element()->focused())
            ts << "focus ";
        ts << " <" << element()->localName().deprecatedString() << ">";
        ts << " (" << xPos() << "," << yPos() << "," << width() << "," << height() << ")";
        if (isTableCell()) {
            const RenderTableCell* cell = static_cast<const RenderTableCell*>(this);
            ts << " [r=" << cell->row() << " c=" << cell->col() << " rs=" << cell->rowSpan() << " cs=" << cell->colSpan() << "]";
        }
    }
    return str;
}

void RenderObject::dump(TextStream* stream, DeprecatedString ind) const
{
    if (isAnonymous())
        *stream << " anonymous";
    if (isFloating())
        *stream << " floating";
    if (isPositioned())
        *stream << " positioned";
    if (isRelPositioned())
        *stream << " relPositioned";
    if (isText())
        *stream << " text";
    if (isInline())
        *stream << " inline";
    if (isReplaced())
        *stream << " replaced";
    if (shouldPaintBackgroundOrBorder())
        *stream << " paintBackground";
    if (needsLayout())
        *stream << " needsLayout";
    if (minMaxKnown())
        *stream << " minMaxKnown";
    *stream << endl;

    RenderObject* child = firstChild();
    while (child) {
        *stream << ind << child->renderName() << ": ";
        child->dump(stream, ind + "  ");
        child = child->nextSibling();
    }
}

void RenderObject::showTreeForThis() const
{
    if (element())
        element()->showTreeForThis();
}

#endif // NDEBUG

static Node* selectStartNode(const RenderObject* object)
{
    Node* node = 0;
    bool forcedOn = false;

    for (const RenderObject* curr = object; curr; curr = curr->parent()) {
        if (curr->style()->userSelect() == SELECT_TEXT)
            forcedOn = true;
        if (!forcedOn && curr->style()->userSelect() == SELECT_NONE)
            return 0;

        if (!node)
            node = curr->element();
    }

    // somewhere up the render tree there must be an element!
    ASSERT(node);

    return node;
}

bool RenderObject::canSelect() const
{
    return selectStartNode(this) != 0;
}

bool RenderObject::shouldSelect() const
{
    if (Node* node = selectStartNode(this))
        return EventTargetNodeCast(node)->dispatchHTMLEvent(selectstartEvent, true, true);

    return false;
}

Color RenderObject::selectionBackgroundColor() const
{
    Color color;
    if (style()->userSelect() != SELECT_NONE) {
        RenderStyle* pseudoStyle = getPseudoStyle(RenderStyle::SELECTION);
        if (pseudoStyle && pseudoStyle->backgroundColor().isValid())
            color = pseudoStyle->backgroundColor().blendWithWhite();
        else
            color = document()->frame()->isActive() ?
                    theme()->activeSelectionBackgroundColor() :
                    theme()->inactiveSelectionBackgroundColor();
    }

    return color;
}

Color RenderObject::selectionForegroundColor() const
{
    Color color;
    if (style()->userSelect() != SELECT_NONE) {
        RenderStyle* pseudoStyle = getPseudoStyle(RenderStyle::SELECTION);
        if (pseudoStyle) {
            color = pseudoStyle->textFillColor();
            if (!color.isValid())
                color = pseudoStyle->color();
        } else
            color = document()->frame()->isActive() ?
                    theme()->platformActiveSelectionForegroundColor() :
                    theme()->platformInactiveSelectionForegroundColor();
    }

    return color;
}

Node* RenderObject::draggableNode(bool dhtmlOK, bool uaOK, int x, int y, bool& dhtmlWillDrag) const
{
    if (!dhtmlOK && !uaOK)
        return 0;

    for (const RenderObject* curr = this; curr; curr = curr->parent()) {
        Node* elt = curr->element();
        if (elt && elt->nodeType() == Node::TEXT_NODE) {
            // Since there's no way for the author to address the -webkit-user-drag style for a text node,
            // we use our own judgement.
            if (uaOK && view()->frameView()->frame()->eventHandler()->shouldDragAutoNode(curr->node(), IntPoint(x, y))) {
                dhtmlWillDrag = false;
                return curr->node();
            }
            if (curr->shouldSelect())
                // In this case we have a click in the unselected portion of text.  If this text is
                // selectable, we want to start the selection process instead of looking for a parent
                // to try to drag.
                return 0;
        } else {
            EUserDrag dragMode = curr->style()->userDrag();
            if (dhtmlOK && dragMode == DRAG_ELEMENT) {
                dhtmlWillDrag = true;
                return curr->node();
            }
            if (uaOK && dragMode == DRAG_AUTO
                    && view()->frameView()->frame()->eventHandler()->shouldDragAutoNode(curr->node(), IntPoint(x, y))) {
                dhtmlWillDrag = false;
                return curr->node();
            }
        }
    }
    return 0;
}

void RenderObject::selectionStartEnd(int& spos, int& epos)
{
    view()->selectionStartEnd(spos, epos);
}

RenderBlock* RenderObject::createAnonymousBlock()
{
    RenderStyle* newStyle = new (renderArena()) RenderStyle();
    newStyle->inheritFrom(m_style);
    newStyle->setDisplay(BLOCK);

    RenderBlock* newBox = new (renderArena()) RenderBlock(document() /* anonymous box */);
    newBox->setStyle(newStyle);
    return newBox;
}

void RenderObject::handleDynamicFloatPositionChange()
{
    // We have gone from not affecting the inline status of the parent flow to suddenly
    // having an impact.  See if there is a mismatch between the parent flow's
    // childrenInline() state and our state.
    setInline(style()->isDisplayInlineType());
    if (isInline() != parent()->childrenInline()) {
        if (!isInline()) {
            if (parent()->isRenderInline()) {
                // We have to split the parent flow.
                RenderInline* parentInline = static_cast<RenderInline*>(parent());
                RenderBlock* newBox = parentInline->createAnonymousBlock();

                RenderFlow* oldContinuation = parent()->continuation();
                parentInline->setContinuation(newBox);

                RenderObject* beforeChild = nextSibling();
                parent()->removeChildNode(this);
                parentInline->splitFlow(beforeChild, newBox, this, oldContinuation);
            } else if (parent()->isRenderBlock())
                static_cast<RenderBlock*>(parent())->makeChildrenNonInline();
        } else {
            // An anonymous block must be made to wrap this inline.
            RenderBlock* box = createAnonymousBlock();
            parent()->insertChildNode(box, this);
            box->appendChildNode(parent()->removeChildNode(this));
        }
    }
}

void RenderObject::setStyle(RenderStyle* style)
{
    if (m_style == style)
        return;

    bool affectsParentBlock = false;
    RenderStyle::Diff d = RenderStyle::Equal;
    if (m_style) {
        // If our z-index changes value or our visibility changes,
        // we need to dirty our stacking context's z-order list.
        if (style) {
#if PLATFORM(MAC)
            if (m_style->visibility() != style->visibility() ||
                    m_style->zIndex() != style->zIndex() ||
                    m_style->hasAutoZIndex() != style->hasAutoZIndex())
                document()->setDashboardRegionsDirty(true);
#endif

            if ((m_style->hasAutoZIndex() != style->hasAutoZIndex() ||
                    m_style->zIndex() != style->zIndex() ||
                    m_style->visibility() != style->visibility()) && layer()) {
                layer()->stackingContext()->dirtyZOrderLists();
                if (m_style->hasAutoZIndex() != style->hasAutoZIndex() ||
                        m_style->visibility() != style->visibility())
                    layer()->dirtyZOrderLists();
            }
            // keep layer hierarchy visibility bits up to date if visibility changes
            if (m_style->visibility() != style->visibility()) {
                RenderLayer* l = enclosingLayer();
                if (style->visibility() == VISIBLE && l)
                    l->setHasVisibleContent(true);
                else if (l && l->hasVisibleContent() &&
                            (this == l->renderer() || l->renderer()->style()->visibility() != VISIBLE))
                    l->dirtyVisibleContentStatus();
            }
        }

        d = m_style->diff(style);

        // If we have no layer(), just treat a RepaintLayer hint as a normal Repaint.
        if (d == RenderStyle::RepaintLayer && !layer())
            d = RenderStyle::Repaint;

        // The background of the root element or the body element could propagate up to
        // the canvas.  Just dirty the entire canvas when our style changes substantially.
        if (d >= RenderStyle::Repaint && element() &&
                (element()->hasTagName(htmlTag) || element()->hasTagName(bodyTag)))
            view()->repaint();
        else if (m_parent && !isText()) {
            // Do a repaint with the old style first, e.g., for example if we go from
            // having an outline to not having an outline.
            if (d == RenderStyle::RepaintLayer) {
                layer()->repaintIncludingDescendants();
                if (!(m_style->clip() == style->clip()))
                    layer()->clearClipRects();
            } else if (d == RenderStyle::Repaint || style->outlineSize() < m_style->outlineSize())
                repaint();
        }

        // When a layout hint happens, we go ahead and do a repaint of the layer, since the layer could
        // end up being destroyed.
        if (d == RenderStyle::Layout && layer() &&
                (m_style->position() != style->position() ||
                 m_style->zIndex() != style->zIndex() ||
                 m_style->hasAutoZIndex() != style->hasAutoZIndex() ||
                 !(m_style->clip() == style->clip()) ||
                 m_style->hasClip() != style->hasClip() ||
                 m_style->opacity() != style->opacity()))
            layer()->repaintIncludingDescendants();

        // When a layout hint happens and an object's position style changes, we have to do a layout
        // to dirty the render tree using the old position value now.
        if (d == RenderStyle::Layout && m_parent && m_style->position() != style->position()) {
            markContainingBlocksForLayout();
            if (m_style->position() == StaticPosition)
                repaint();
            if (isRenderBlock()) {
                if (style->position() == StaticPosition)
                    // Clear our positioned objects list. Our absolutely positioned descendants will be
                    // inserted into our containing block's positioned objects list during layout.
                    removePositionedObjects(0);
                else if (m_style->position() == StaticPosition) {
                    // Remove our absolutely positioned descendants from their current containing block.
                    // They will be inserted into our positioned objects list during layout.
                    RenderObject* cb = parent();
                    while (cb && (cb->style()->position() == StaticPosition || (cb->isInline() && !cb->isReplaced())) && !cb->isRenderView()) {
                        if (cb->style()->position() == RelativePosition && cb->isInline() && !cb->isReplaced()) {
                            cb = cb->containingBlock();
                            break;
                        }
                        cb = cb->parent();
                    }
                    cb->removePositionedObjects(static_cast<RenderBlock*>(this));
                }
            }
        }

        if (isFloating() && (m_style->floating() != style->floating()))
            // For changes in float styles, we need to conceivably remove ourselves
            // from the floating objects list.
            removeFromObjectLists();
        else if (isPositioned() && (style->position() != AbsolutePosition && style->position() != FixedPosition))
            // For changes in positioning styles, we need to conceivably remove ourselves
            // from the positioned objects list.
            removeFromObjectLists();

        affectsParentBlock = m_style && isFloatingOrPositioned() &&
            (!style->isFloating() && style->position() != AbsolutePosition && style->position() != FixedPosition)
            && parent() && (parent()->isBlockFlow() || parent()->isInlineFlow());

        // reset style flags
        m_floating = false;
        m_positioned = false;
        m_relPositioned = false;
        m_paintBackground = false;
        m_hasOverflowClip = false;
    }

    if (view()->frameView()) {
        // FIXME: A better solution would be to only invalidate the fixed regions when scrolling.  It's overkill to
        // prevent the entire view from blitting on a scroll.
        bool oldStyleSlowScroll = style && (style->position() == FixedPosition || style->hasFixedBackgroundImage());
        bool newStyleSlowScroll = m_style && (m_style->position() == FixedPosition || m_style->hasFixedBackgroundImage());
        if (oldStyleSlowScroll != newStyleSlowScroll) {
            if (oldStyleSlowScroll)
                view()->frameView()->removeSlowRepaintObject();
            if (newStyleSlowScroll)
                view()->frameView()->addSlowRepaintObject();
        }
    }

    RenderStyle* oldStyle = m_style;
    m_style = style;

    updateBackgroundImages(oldStyle);

    if (m_style)
        m_style->ref();

    if (oldStyle)
        oldStyle->deref(renderArena());

    setShouldPaintBackgroundOrBorder(m_style->hasBorder() || m_style->hasBackground() || m_style->hasAppearance());

    if (affectsParentBlock)
        handleDynamicFloatPositionChange();

    // No need to ever schedule repaints from a style change of a text run, since
    // we already did this for the parent of the text run.
    if (d == RenderStyle::Layout && m_parent)
        setNeedsLayoutAndMinMaxRecalc();
    else if (m_parent && !isText() && (d == RenderStyle::RepaintLayer || d == RenderStyle::Repaint))
        // Do a repaint with the new style now, e.g., for example if we go from
        // not having an outline to having an outline.
        repaint();
}

void RenderObject::setStyleInternal(RenderStyle* style)
{
    if (m_style == style)
        return;
    if (m_style)
        m_style->deref(renderArena());
    m_style = style;
    if (m_style)
        m_style->ref();
}

void RenderObject::updateBackgroundImages(RenderStyle* oldStyle)
{
    // FIXME: This will be slow when a large number of images is used.  Fix by using a dict.
    const BackgroundLayer* oldLayers = oldStyle ? oldStyle->backgroundLayers() : 0;
    const BackgroundLayer* newLayers = m_style ? m_style->backgroundLayers() : 0;
    for (const BackgroundLayer* currOld = oldLayers; currOld; currOld = currOld->next()) {
        if (currOld->backgroundImage() && (!newLayers || !newLayers->containsImage(currOld->backgroundImage())))
            currOld->backgroundImage()->deref(this);
    }
    for (const BackgroundLayer* currNew = newLayers; currNew; currNew = currNew->next()) {
        if (currNew->backgroundImage() && (!oldLayers || !oldLayers->containsImage(currNew->backgroundImage())))
            currNew->backgroundImage()->ref(this);
    }

    CachedImage* oldBorderImage = oldStyle ? oldStyle->borderImage().image() : 0;
    CachedImage* newBorderImage = m_style ? m_style->borderImage().image() : 0;
    if (oldBorderImage != newBorderImage) {
        if (oldBorderImage)
            oldBorderImage->deref(this);
        if (newBorderImage)
            newBorderImage->ref(this);
    }
}

IntRect RenderObject::viewRect() const
{
    return view()->viewRect();
}

bool RenderObject::absolutePosition(int& xPos, int& yPos, bool f)
{
    RenderObject* o = parent();
    if (o) {
        o->absolutePosition(xPos, yPos, f);
        yPos += o->borderTopExtra();
        if (o->hasOverflowClip())
            o->layer()->subtractScrollOffset(xPos, yPos);
        return true;
    } else {
        xPos = yPos = 0;
        return false;
    }
}

IntRect RenderObject::caretRect(int offset, EAffinity affinity, int* extraWidthToEndOfLine)
{
   if (extraWidthToEndOfLine)
       *extraWidthToEndOfLine = 0;

    return IntRect();
}

int RenderObject::paddingTop() const
{
    int w = 0;
    Length padding = m_style->paddingTop();
    if (padding.isPercent())
        w = containingBlock()->contentWidth();
    w = padding.calcMinValue(w);
    if (isTableCell() && padding.isAuto())
        w = static_cast<const RenderTableCell*>(this)->table()->cellPadding();
    return w;
}

int RenderObject::paddingBottom() const
{
    int w = 0;
    Length padding = style()->paddingBottom();
    if (padding.isPercent())
        w = containingBlock()->contentWidth();
    w = padding.calcMinValue(w);
    if (isTableCell() && padding.isAuto())
        w = static_cast<const RenderTableCell*>(this)->table()->cellPadding();
    return w;
}

int RenderObject::paddingLeft() const
{
    int w = 0;
    Length padding = style()->paddingLeft();
    if (padding.isPercent())
        w = containingBlock()->contentWidth();
    w = padding.calcMinValue(w);
    if (isTableCell() && padding.isAuto())
        w = static_cast<const RenderTableCell*>(this)->table()->cellPadding();
    return w;
}

int RenderObject::paddingRight() const
{
    int w = 0;
    Length padding = style()->paddingRight();
    if (padding.isPercent())
        w = containingBlock()->contentWidth();
    w = padding.calcMinValue(w);
    if (isTableCell() && padding.isAuto())
        w = static_cast<const RenderTableCell*>(this)->table()->cellPadding();
    return w;
}

int RenderObject::tabWidth() const
{
    if (style()->collapseWhiteSpace())
        return 0;

    return containingBlock()->tabWidth(true);
}

RenderView* RenderObject::view() const
{
    return static_cast<RenderView*>(document()->renderer());
}

bool RenderObject::hasOutlineAnnotation() const
{
    return element() && element()->isLink() && document()->printing();
}

RenderObject* RenderObject::container() const
{
    // This method is extremely similar to containingBlock(), but with a few notable
    // exceptions.
    // (1) It can be used on orphaned subtrees, i.e., it can be called safely even when
    // the object is not part of the primary document subtree yet.
    // (2) For normal flow elements, it just returns the parent.
    // (3) For absolute positioned elements, it will return a relative positioned inline.
    // containingBlock() simply skips relpositioned inlines and lets an enclosing block handle
    // the layout of the positioned object.  This does mean that calcAbsoluteHorizontal and
    // calcAbsoluteVertical have to use container().
    EPosition pos = m_style->position();
    RenderObject* o = parent();
    if (!isText() && pos == FixedPosition) {
        // container() can be called on an object that is not in the
        // tree yet.  We don't call view() since it will assert if it
        // can't get back to the canvas.  Instead we just walk as high up
        // as we can.  If we're in the tree, we'll get the root.  If we
        // aren't we'll get the root of our little subtree (most likely
        // we'll just return 0).
        while (o && o->parent())
            o = o->parent();
    } else if (!isText() && pos == AbsolutePosition) {
        // Same goes here.  We technically just want our containing block, but
        // we may not have one if we're part of an uninstalled subtree.  We'll
        // climb as high as we can though.
        while (o && o->style()->position() == StaticPosition && !o->isRenderView())
            o = o->parent();
    }

    return o;
}

// This code has been written to anticipate the addition of CSS3-::outside and ::inside generated
// content (and perhaps XBL).  That's why it uses the render tree and not the DOM tree.
RenderObject* RenderObject::hoverAncestor() const
{
    return (!isInline() && continuation()) ? continuation() : parent();
}

bool RenderObject::isSelectionBorder() const
{
    SelectionState st = selectionState();
    return st == SelectionStart || st == SelectionEnd || st == SelectionBoth;
}

void RenderObject::removeFromObjectLists()
{
    if (documentBeingDestroyed())
        return;

    if (isFloating()) {
        RenderBlock* outermostBlock = containingBlock();
        for (RenderBlock* p = outermostBlock; p && !p->isRenderView(); p = p->containingBlock()) {
            if (p->containsFloat(this))
                outermostBlock = p;
        }

        if (outermostBlock)
            outermostBlock->markAllDescendantsWithFloatsForLayout(this);
    }

    if (isPositioned()) {
        RenderObject* p;
        for (p = parent(); p; p = p->parent()) {
            if (p->isRenderBlock())
                static_cast<RenderBlock*>(p)->removePositionedObject(this);
        }
    }
}

RenderArena* RenderObject::renderArena() const
{
    Document* doc = document();
    return doc ? doc->renderArena() : 0;
}

bool RenderObject::documentBeingDestroyed() const
{
    return !document()->renderer();
}

void RenderObject::destroy()
{
    // If this renderer is being autoscrolled, stop the autoscroll timer
    if (document() && document()->frame() && document()->frame()->eventHandler()->autoscrollRenderer() == this)
        document()->frame()->eventHandler()->stopAutoscrollTimer(true);

    if (m_hasCounterNodeMap) {
        RenderObjectsToCounterNodeMaps* objectsMap = getRenderObjectsToCounterNodeMaps();
        if (CounterNodeMap* counterNodesMap = objectsMap->get(this)) {
            CounterNodeMap::const_iterator end = counterNodesMap->end();
            for (CounterNodeMap::const_iterator it = counterNodesMap->begin(); it != end; ++it) {
                CounterNode* counterNode = it->second;
                counterNode->remove();
                delete counterNode;
                counterNode = 0;
            }
            objectsMap->remove(this);
            delete counterNodesMap;
        }
    }

    document()->axObjectCache()->remove(this);

    // By default no ref-counting. RenderWidget::destroy() doesn't call
    // this function because it needs to do ref-counting. If anything
    // in this function changes, be sure to fix RenderWidget::destroy() as well.

    remove();

    arenaDelete(document()->renderArena(), this);
}

void RenderObject::arenaDelete(RenderArena* arena, void* base)
{
    if (m_style->backgroundImage())
        m_style->backgroundImage()->deref(this);
    if (m_style)
        m_style->deref(arena);

#ifndef NDEBUG
    void* savedBase = baseOfRenderObjectBeingDeleted;
    baseOfRenderObjectBeingDeleted = base;
#endif
    delete this;
#ifndef NDEBUG
    baseOfRenderObjectBeingDeleted = savedBase;
#endif

    // Recover the size left there for us by operator delete and free the memory.
    arena->free(*(size_t*)base, base);
}

VisiblePosition RenderObject::positionForCoordinates(int x, int y)
{
    return VisiblePosition(element(), caretMinOffset(), DOWNSTREAM);
}

void RenderObject::updateDragState(bool dragOn)
{
    bool valueChanged = (dragOn != m_isDragging);
    m_isDragging = dragOn;
    if (valueChanged && style()->affectedByDragRules())
        element()->setChanged();
    for (RenderObject* curr = firstChild(); curr; curr = curr->nextSibling())
        curr->updateDragState(dragOn);
    if (continuation())
        continuation()->updateDragState(dragOn);
}

bool RenderObject::hitTest(const HitTestRequest& request, HitTestResult& result, int x, int y, int tx, int ty, HitTestFilter hitTestFilter)
{
    bool inside = false;
    if (hitTestFilter != HitTestSelf) {
        // First test the foreground layer (lines and inlines).
        inside = nodeAtPoint(request, result, x, y, tx, ty, HitTestForeground);

        // Test floats next.
        if (!inside)
            inside = nodeAtPoint(request, result, x, y, tx, ty, HitTestFloat);

        // Finally test to see if the mouse is in the background (within a child block's background).
        if (!inside)
            inside = nodeAtPoint(request, result, x, y, tx, ty, HitTestChildBlockBackgrounds);
    }

    // See if the mouse is inside us but not any of our descendants
    if (hitTestFilter != HitTestDescendants && !inside)
        inside = nodeAtPoint(request, result, x, y, tx, ty, HitTestBlockBackground);

    return inside;
}

void RenderObject::setInnerNode(HitTestResult& result)
{
    if (result.innerNode())
        return;

    Node* node = element();
    if (isRenderView())
        node = document()->documentElement();
    else if (!isInline() && continuation())
        // We are in the margins of block elements that are part of a continuation.  In
        // this case we're actually still inside the enclosing inline element that was
        // split.  Go ahead and set our inner node accordingly.
        node = continuation()->element();

    if (node) {
        result.setInnerNode(node);
        if (!result.innerNonSharedNode())
            result.setInnerNonSharedNode(node);
    }
}

bool RenderObject::nodeAtPoint(const HitTestRequest&, HitTestResult&, int /*x*/, int /*y*/, int /*tx*/, int /*ty*/, HitTestAction)
{
    return false;
}

short RenderObject::verticalPositionHint(bool firstLine) const
{
    short vpos = m_verticalPosition;
    if (m_verticalPosition == PositionUndefined || firstLine) {
        vpos = getVerticalPosition(firstLine);
        if (!firstLine)
            m_verticalPosition = vpos;
    }

    return vpos;
}

short RenderObject::getVerticalPosition(bool firstLine) const
{
    if (!isInline())
        return 0;

    // This method determines the vertical position for inline elements.
    int vpos = 0;
    EVerticalAlign va = style()->verticalAlign();
    if (va == TOP)
        vpos = PositionTop;
    else if (va == BOTTOM)
        vpos = PositionBottom;
    else if (va == LENGTH)
        vpos = -style()->verticalAlignLength().calcValue(lineHeight(firstLine));
    else {
        bool checkParent = parent()->isInline() && !parent()->isInlineBlockOrInlineTable() && parent()->style()->verticalAlign() != TOP && parent()->style()->verticalAlign() != BOTTOM;
        vpos = checkParent ? parent()->verticalPositionHint(firstLine) : 0;
        // don't allow elements nested inside text-top to have a different valignment.
        if (va == BASELINE)
            return vpos;

        const Font& f = parent()->font(firstLine);
        int fontsize = f.pixelSize();

        if (va == SUB)
            vpos += fontsize / 5 + 1;
        else if (va == SUPER)
            vpos -= fontsize / 3 + 1;
        else if (va == TEXT_TOP)
            vpos += baselinePosition(firstLine) - f.ascent();
        else if (va == MIDDLE)
            vpos += -static_cast<int>(f.xHeight() / 2) - lineHeight(firstLine) / 2 + baselinePosition(firstLine);
        else if (va == TEXT_BOTTOM) {
            vpos += f.descent();
            if (!isReplaced())
                vpos -= font(firstLine).descent();
        } else if (va == BASELINE_MIDDLE)
            vpos += -lineHeight(firstLine) / 2 + baselinePosition(firstLine);
    }

    return vpos;
}

short RenderObject::lineHeight(bool firstLine, bool /*isRootLineBox*/) const
{
    RenderStyle* s = style(firstLine);

    Length lh = s->lineHeight();

    // its "unset", choose nice default
    if (lh.value() < 0)
        return s->font().lineSpacing();

    if (lh.isPercent())
        return lh.calcMinValue(s->fontSize());

    // its fixed
    return lh.value();
}

short RenderObject::baselinePosition(bool firstLine, bool isRootLineBox) const
{
    const Font& f = font(firstLine);
    return f.ascent() + (lineHeight(firstLine, isRootLineBox) - f.height()) / 2;
}

void RenderObject::invalidateVerticalPositions()
{
    m_verticalPosition = PositionUndefined;
    RenderObject* child = firstChild();
    while(child) {
        child->invalidateVerticalPositions();
        child = child->nextSibling();
    }
}

void RenderObject::recalcMinMaxWidths()
{
    ASSERT(m_recalcMinMax);

    if (m_recalcMinMax)
        updateFirstLetter();

    RenderObject* child = firstChild();
    while (child) {
        int cmin = 0;
        int cmax = 0;
        bool test = false;
        if ((m_minMaxKnown && child->m_recalcMinMax) || !child->m_minMaxKnown) {
            cmin = child->minWidth();
            cmax = child->maxWidth();
            test = true;
        }
        if (child->m_recalcMinMax)
            child->recalcMinMaxWidths();
        if (!child->m_minMaxKnown)
            child->calcMinMaxWidth();
        if (m_minMaxKnown && test && (cmin != child->minWidth() || cmax != child->maxWidth()))
            m_minMaxKnown = false;
        child = child->nextSibling();
    }

    // we need to recalculate, if the contains inline children, as the change could have
    // happened somewhere deep inside the child tree. Also do this for blocks or tables that
    // are inline (i.e., inline-block and inline-table).
    if ((!isInline() || isInlineBlockOrInlineTable()) && childrenInline())
        m_minMaxKnown = false;

    if (!m_minMaxKnown)
        calcMinMaxWidth();
    m_recalcMinMax = false;
}

void RenderObject::scheduleRelayout()
{
    if (isRenderView()) {
        FrameView* view = static_cast<RenderView*>(this)->frameView();
        if (view)
            view->scheduleRelayout();
    } else {
        FrameView* v = view() ? view()->frameView() : 0;
        if (v)
            v->scheduleRelayoutOfSubtree(node());
    }
}

void RenderObject::removeLeftoverAnonymousBoxes()
{
}

InlineBox* RenderObject::createInlineBox(bool, bool isRootLineBox, bool)
{
    ASSERT(!isRootLineBox);
    return new (renderArena()) InlineBox(this);
}

void RenderObject::dirtyLineBoxes(bool, bool)
{
}

InlineBox* RenderObject::inlineBoxWrapper() const
{
    return 0;
}

void RenderObject::setInlineBoxWrapper(InlineBox*)
{
}

void RenderObject::deleteLineBoxWrapper()
{
}

RenderStyle* RenderObject::firstLineStyle() const
{
    RenderStyle* s = m_style;
    const RenderObject* obj = isText() ? parent() : this;
    if (obj->isBlockFlow()) {
        RenderBlock* firstLineBlock = obj->firstLineBlock();
        if (firstLineBlock)
            s = firstLineBlock->getPseudoStyle(RenderStyle::FIRST_LINE, style());
    } else if (!obj->isAnonymous() && obj->isInlineFlow()) {
        RenderStyle* parentStyle = obj->parent()->firstLineStyle();
        if (parentStyle != obj->parent()->style()) {
            // A first-line style is in effect. We need to cache a first-line style
            // for ourselves.
            style()->setHasPseudoStyle(RenderStyle::FIRST_LINE_INHERITED);
            s = obj->getPseudoStyle(RenderStyle::FIRST_LINE_INHERITED, parentStyle);
        }
    }
    return s;
}

RenderStyle* RenderObject::getPseudoStyle(RenderStyle::PseudoId pseudo, RenderStyle* parentStyle) const
{
    if (!style()->hasPseudoStyle(pseudo))
        return 0;

    if (!parentStyle)
        parentStyle = style();

    RenderStyle* result = style()->getPseudoStyle(pseudo);
    if (result)
        return result;

    Node* node = element();
    if (isText())
        node = element()->parentNode();
    if (!node)
        return 0;

    if (pseudo == RenderStyle::FIRST_LINE_INHERITED) {
        result = document()->styleSelector()->styleForElement(static_cast<Element*>(node), parentStyle, false);
        result->setStyleType(RenderStyle::FIRST_LINE_INHERITED);
    } else
        result = document()->styleSelector()->pseudoStyleForElement(pseudo, static_cast<Element*>(node), parentStyle);
    if (result) {
        style()->addPseudoStyle(result);
        result->deref(document()->renderArena());
    }
    return result;
}

static Color decorationColor(RenderStyle* style)
{
    Color result;
    if (style->textStrokeWidth() > 0) {
        // Prefer stroke color if possible but not if it's fully transparent.
        result = style->textStrokeColor();
        if (!result.isValid())
            result = style->color();
        if (result.alpha())
            return result;
    }
    
    result = style->textFillColor();
    if (!result.isValid())
        result = style->color();
    return result;
}

void RenderObject::getTextDecorationColors(int decorations, Color& underline, Color& overline,
                                           Color& linethrough, bool quirksMode)
{
    RenderObject* curr = this;
    do {
        int currDecs = curr->style()->textDecoration();
        if (currDecs) {
            if (currDecs & UNDERLINE) {
                decorations &= ~UNDERLINE;
                underline = decorationColor(curr->style());
            }
            if (currDecs & OVERLINE) {
                decorations &= ~OVERLINE;
                overline = decorationColor(curr->style());
            }
            if (currDecs & LINE_THROUGH) {
                decorations &= ~LINE_THROUGH;
                linethrough = decorationColor(curr->style());
            }
        }
        curr = curr->parent();
        if (curr && curr->isRenderBlock() && curr->continuation())
            curr = curr->continuation();
    } while (curr && decorations && (!quirksMode || !curr->element() ||
                                     (!curr->element()->hasTagName(aTag) && !curr->element()->hasTagName(fontTag))));

    // If we bailed out, use the element we bailed out at (typically a <font> or <a> element).
    if (decorations && curr) {
        if (decorations & UNDERLINE)
            underline = decorationColor(curr->style());
        if (decorations & OVERLINE)
            overline = decorationColor(curr->style());
        if (decorations & LINE_THROUGH)
            linethrough = decorationColor(curr->style());
    }
}

void RenderObject::updateWidgetPosition()
{
}

void RenderObject::addDashboardRegions(Vector<DashboardRegionValue>& regions)
{
    // Convert the style regions to absolute coordinates.
    if (style()->visibility() != VISIBLE)
        return;

    const Vector<StyleDashboardRegion>& styleRegions = style()->dashboardRegions();
    unsigned i, count = styleRegions.size();
    for (i = 0; i < count; i++) {
        StyleDashboardRegion styleRegion = styleRegions[i];

        int w = width();
        int h = height();

        DashboardRegionValue region;
        region.label = styleRegion.label;
        region.bounds = IntRect(styleRegion.offset.left.value(),
                                styleRegion.offset.top.value(),
                                w - styleRegion.offset.left.value() - styleRegion.offset.right.value(),
                                h - styleRegion.offset.top.value() - styleRegion.offset.bottom.value());
        region.type = styleRegion.type;

        region.clip = region.bounds;
        computeAbsoluteRepaintRect(region.clip);
        if (region.clip.height() < 0) {
            region.clip.setHeight(0);
            region.clip.setWidth(0);
        }

        int x, y;
        absolutePosition(x, y);
        region.bounds.setX(x + styleRegion.offset.left.value());
        region.bounds.setY(y + styleRegion.offset.top.value());

        if (document()->frame()) {
            float pageScaleFactor = document()->frame()->page()->chrome()->scaleFactor();
            if (pageScaleFactor != 1.0f) {
                region.bounds.scale(pageScaleFactor);
                region.clip.scale(pageScaleFactor);
            }
        }

        regions.append(region);
    }
}

void RenderObject::collectDashboardRegions(Vector<DashboardRegionValue>& regions)
{
    // RenderTexts don't have their own style, they just use their parent's style,
    // so we don't want to include them.
    if (isText())
        return;

    addDashboardRegions(regions);
    for (RenderObject* curr = firstChild(); curr; curr = curr->nextSibling())
        curr->collectDashboardRegions(regions);
}

void RenderObject::collectBorders(DeprecatedValueList<CollapsedBorderValue>& borderStyles)
{
    for (RenderObject* curr = firstChild(); curr; curr = curr->nextSibling())
        curr->collectBorders(borderStyles);
}

bool RenderObject::avoidsFloats() const
{
    return isReplaced() || hasOverflowClip() || isHR();
}

bool RenderObject::usesLineWidth() const
{
    // 1. All auto-width objects that avoid floats should always use lineWidth
    // 2. For objects with a specified width, we match WinIE's behavior:
    // (a) tables use contentWidth
    // (b) <hr>s use lineWidth
    // (c) all other objects use lineWidth in quirks mode and contentWidth in strict mode.
    return (avoidsFloats() && (style()->width().isAuto() || isHR() || (style()->htmlHacks() && !isTable())));
}

CounterNode* RenderObject::findCounter(const String& counterName, bool willNeedLayout,
                                       bool usesSeparator, bool createIfNotFound)
{
    if (!style())
        return 0;

    RenderObjectsToCounterNodeMaps* objectsMap = getRenderObjectsToCounterNodeMaps();
    CounterNode* newNode = 0;
    if (CounterNodeMap* counterNodesMap = objectsMap->get(this)) {
        if (counterNodesMap)
            newNode = counterNodesMap->get(counterName);
    }

    if (newNode)
        return newNode;

    int val = 0;
    if (style()->hasCounterReset(counterName) || isRoot()) {
        newNode = new CounterResetNode(this);
        val = style()->counterReset(counterName);
        if (style()->hasCounterIncrement(counterName))
            val += style()->counterIncrement(counterName);
        newNode->setValue(val);
    } else if (style()->hasCounterIncrement(counterName)) {
        newNode = new CounterNode(this);
        newNode->setValue(style()->counterIncrement(counterName));
    } else if (counterName == "list-item") {
        if (isListItem()) {
            if (element()) {
                String v = static_cast<Element*>(element())->getAttribute("value");
                if (!v.isEmpty()) {
                    newNode = new CounterResetNode(this);
                    val = v.toInt();
                }
            }

            if (!newNode) {
                newNode = new CounterNode(this);
                val = 1;
            }

            newNode->setValue(val);
        } else if (element() && element()->hasTagName(olTag)) {
            newNode = new CounterResetNode(this);
            newNode->setValue(static_cast<HTMLOListElement*>(element())->start());
        } else if (element() &&
            (element()->hasTagName(ulTag) ||
             element()->hasTagName(menuTag) ||
             element()->hasTagName(dirTag))) {
            newNode = new CounterResetNode(this);
            newNode->setValue(0);
        }
    }

    if (!newNode && !createIfNotFound)
        return 0;
    else if (!newNode) {
        newNode = new CounterNode(this);
        newNode->setValue(0);
    }

    if (willNeedLayout)
        newNode->setWillNeedLayout();
    if (usesSeparator)
        newNode->setUsesSeparator();

    CounterNodeMap* nodeMap;
    if (m_hasCounterNodeMap)
        nodeMap = objectsMap->get(this);
    else {
        nodeMap = new CounterNodeMap;
        objectsMap->set(this, nodeMap);
        m_hasCounterNodeMap = true;
    }

    nodeMap->set(counterName, newNode);

    if (!isRoot()) {
        RenderObject* n = !isListItem() && previousSibling()
            ? previousSibling()->previousSibling() : previousSibling();

        CounterNode* current = 0;
        for (; n; n = n->previousSibling()) {
            current = n->findCounter(counterName, false, false, false);
            if (current)
                break;
        }

        CounterNode* last = current;
        CounterNode* sibling = current;
        if (last && !newNode->isReset()) {
            // Found render-sibling, now search for later counter-siblings among its render-children
            n = n->lastChild();
            while (n) {
                current = n->findCounter(counterName, false, false, false);
                if (current && (last->parent() == current->parent() || sibling == current->parent())) {
                    last = current;
                    // If the current counter is not the last, search deeper
                    if (current->nextSibling()) {
                        n = n->lastChild();
                        continue;
                    } else
                        break;
                }
                n = n->previousSibling();
            }

            if (sibling->isReset()) {
                if (last != sibling)
                    sibling->insertAfter(newNode, last);
                else
                    sibling->insertAfter(newNode, 0);
            } else
                last->parent()->insertAfter(newNode, last);
        } else {
            // Nothing found among siblings, let our parent search
            last = parent()->findCounter(counterName, false);
            if (last->isReset())
                last->insertAfter(newNode, 0);
            else
                last->parent()->insertAfter(newNode, last);
        }
    }

    return newNode;
}

UChar RenderObject::backslashAsCurrencySymbol() const
{
    if (Node *node = element()) {
        if (TextResourceDecoder* decoder = node->document()->decoder())
            return decoder->encoding().backslashAsCurrencySymbol();
    }
    return '\\';
}

void RenderObject::imageChanged(CachedImage* image)
{
    // Repaint when the background image or border image finishes loading.
    // This is needed for RenderBox objects, and also for table objects that hold
    // backgrounds that are then respected by the table cells (which are RenderBox
    // subclasses). It would be even better to find a more elegant way of doing this that
    // would avoid putting this function and the CachedResourceClient base class into RenderObject.
    if (image && image->canRender() && parent()) {
        if (view() && element() && (element()->hasTagName(htmlTag) || element()->hasTagName(bodyTag)))
            // repaint the entire canvas since the background gets propagated up
            view()->repaint();
        else
            // repaint object, which is a box or a container with boxes inside it
            repaint();
    }
}

bool RenderObject::willRenderImage(CachedImage*)
{
    // Without visibility we won't render (and therefore don't care about animation).
    if (style()->visibility() != VISIBLE)
        return false;

    // If we're not in a window (i.e., we're dormant from being put in the b/f cache or in a background tab)
    // then we don't want to render either.
    return !document()->inPageCache() && document()->view()->inWindow();
}

int RenderObject::maximalOutlineSize(PaintPhase p) const
{
    if (p != PaintPhaseOutline && p != PaintPhaseSelfOutline && p != PaintPhaseChildOutlines)
        return 0;
    return static_cast<RenderView*>(document()->renderer())->maximalOutlineSize();
}

int RenderObject::caretMinOffset() const
{
    return 0;
}

int RenderObject::caretMaxOffset() const
{
    return isReplaced() ? 1 : 0;
}

unsigned RenderObject::caretMaxRenderedOffset() const
{
    return 0;
}

int RenderObject::previousOffset(int current) const
{
    return current - 1;
}

int RenderObject::nextOffset(int current) const
{
    return current + 1;
}

InlineBox* RenderObject::inlineBox(int offset, EAffinity affinity)
{
    return inlineBoxWrapper();
}

int RenderObject::maxTopMargin(bool positive) const
{
    return positive ? max(0, marginTop()) : -min(0, marginTop());
}

int RenderObject::maxBottomMargin(bool positive) const
{
    return positive ? max(0, marginBottom()) : -min(0, marginBottom());
}

#ifdef SVG_SUPPORT

FloatRect RenderObject::relativeBBox(bool) const
{
    return FloatRect();
}

AffineTransform RenderObject::localTransform() const
{
    return AffineTransform(1, 0, 0, 1, xPos(), yPos());
}

void RenderObject::setLocalTransform(const AffineTransform&)
{
    ASSERT(false);
}

AffineTransform RenderObject::absoluteTransform() const
{
    if (parent())
        return localTransform() * parent()->absoluteTransform();
    return localTransform();
}

#endif // SVG_SUPPORT

} // namespace WebCore

#ifndef NDEBUG

void showTree(const WebCore::RenderObject* ro)
{
    if (ro)
        ro->showTreeForThis();
}

#endif
