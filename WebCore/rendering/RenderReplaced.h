/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007 Apple Inc.
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

#ifndef RenderReplaced_h
#define RenderReplaced_h

#include "RenderBox.h"

namespace WebCore {

class RenderReplaced : public RenderBox {
public:
    RenderReplaced(Node*);
    RenderReplaced(Node*, const IntSize& intrinsicSize);

    virtual const char* renderName() const { return "RenderReplaced"; }

    virtual short lineHeight(bool firstLine, bool isRootLineBox = false) const;
    virtual short baselinePosition(bool firstLine, bool isRootLineBox = false) const;

    virtual void calcPrefWidths();

    virtual void paint(PaintInfo&, int tx, int ty) = 0;

    virtual IntSize intrinsicSize() const;

    virtual int caretMinOffset() const;
    virtual int caretMaxOffset() const;
    virtual unsigned caretMaxRenderedOffset() const;
    virtual VisiblePosition positionForCoordinates(int x, int y);
    
    virtual bool canBeSelectionLeaf() const { return true; }
    virtual SelectionState selectionState() const { return static_cast<SelectionState>(m_selectionState); }
    virtual void setSelectionState(SelectionState);
    virtual IntRect selectionRect();

    bool isSelected() const;

protected:
    void setIntrinsicSize(const IntSize&);

    bool shouldPaint(PaintInfo&, int& tx, int& ty);

private:
    IntSize m_intrinsicSize;
    
    unsigned m_selectionState : 3; // SelectionState
};

}

#endif
