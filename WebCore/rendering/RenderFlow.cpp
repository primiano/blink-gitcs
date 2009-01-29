/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.
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

#include "config.h"
#include "RenderFlow.h"

#include "Document.h"
#include "GraphicsContext.h"
#include "HTMLNames.h"
#include "InlineTextBox.h"
#include "RenderArena.h"
#include "RenderInline.h"
#include "RenderLayer.h"
#include "RenderView.h"

using namespace std;

namespace WebCore {

using namespace HTMLNames;

// FIXME: This is temporary and will go away once we finish pushing all the continuation code down into RenderBlock and RenderInline.
static RenderFlow* nextContinuation(RenderObject* renderer)
{
    if (renderer->isInline() && !renderer->isReplaced())
        return static_cast<RenderFlow*>(static_cast<RenderInline*>(renderer)->continuation());
    return static_cast<RenderBlock*>(renderer)->inlineContinuation();
}

RenderFlow* RenderFlow::continuationBefore(RenderObject* beforeChild)
{
    if (beforeChild && beforeChild->parent() == this)
        return this;

    RenderFlow* curr = nextContinuation(this);
    RenderFlow* nextToLast = this;
    RenderFlow* last = this;
    while (curr) {
        if (beforeChild && beforeChild->parent() == curr) {
            if (curr->firstChild() == beforeChild)
                return last;
            return curr;
        }

        nextToLast = last;
        last = curr;
        curr = nextContinuation(curr);
    }

    if (!beforeChild && !last->firstChild())
        return nextToLast;
    return last;
}

void RenderFlow::addChildWithContinuation(RenderObject* newChild, RenderObject* beforeChild)
{
    if (beforeChild && (beforeChild->parent()->isTableRow() || beforeChild->parent()->isTableSection() || beforeChild->parent()->isTable())) {
        RenderObject* anonymousTablePart = beforeChild->parent();
        ASSERT(anonymousTablePart->isAnonymous());
        while (!anonymousTablePart->isTable()) {
            anonymousTablePart = anonymousTablePart->parent();
            ASSERT(anonymousTablePart->isAnonymous());
        }
        return anonymousTablePart->addChild(newChild, beforeChild);
    }

    RenderFlow* flow = continuationBefore(beforeChild);
    ASSERT(!beforeChild || beforeChild->parent()->isRenderBlock() || beforeChild->parent()->isRenderInline());
    RenderFlow* beforeChildParent = 0;
    if (beforeChild)
        beforeChildParent = static_cast<RenderFlow*>(beforeChild->parent());
    else {
        RenderFlow* cont = nextContinuation(flow);
        if (cont)
            beforeChildParent = cont;
        else
            beforeChildParent = flow;
    }

    if (newChild->isFloatingOrPositioned())
        return beforeChildParent->addChildToFlow(newChild, beforeChild);

    // A continuation always consists of two potential candidates: an inline or an anonymous
    // block box holding block children.
    bool childInline = newChild->isInline();
    bool bcpInline = beforeChildParent->isInline();
    bool flowInline = flow->isInline();

    if (flow == beforeChildParent)
        return flow->addChildToFlow(newChild, beforeChild);
    else {
        // The goal here is to match up if we can, so that we can coalesce and create the
        // minimal # of continuations needed for the inline.
        if (childInline == bcpInline)
            return beforeChildParent->addChildToFlow(newChild, beforeChild);
        else if (flowInline == childInline)
            return flow->addChildToFlow(newChild, 0); // Just treat like an append.
        else
            return beforeChildParent->addChildToFlow(newChild, beforeChild);
    }
}

void RenderFlow::addChild(RenderObject* newChild, RenderObject* beforeChild)
{
    if (nextContinuation(this))
        return addChildWithContinuation(newChild, beforeChild);
    return addChildToFlow(newChild, beforeChild);
}

} // namespace WebCore
