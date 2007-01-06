/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007 Apple Computer, Inc.
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

#ifndef RenderListItem_h
#define RenderListItem_h

#include "RenderBlock.h"

namespace WebCore {

class RenderListMarker;

class RenderListItem : public RenderBlock {
public:
    RenderListItem(Node*);

    virtual const char* renderName() const { return "RenderListItem"; }

    virtual bool isListItem() const { return true; }
    
    virtual void destroy();

    virtual void setStyle(RenderStyle*);

    int value() const { if (!m_isValueUpToDate) updateValueNow(); return m_value; }
    void updateValue();

    bool hasExplicitValue() const { return m_hasExplicitValue; }
    int explicitValue() const { return m_explicitValue; }
    void setExplicitValue(int value);
    void clearExplicitValue();

    virtual bool isEmpty() const;
    virtual void paint(PaintInfo&, int tx, int ty);

    virtual void layout();
    virtual void calcMinMaxWidth();

    virtual void positionListMarker();

    void setNotInList(bool notInList) { m_notInList = notInList; }
    bool notInList() const { return m_notInList; }

    const String& markerText() const;

private:
    void updateMarkerLocation();
    inline int calcValue() const;
    void updateValueNow() const;
    void explicitValueChanged();

    RenderListMarker* m_marker;
    int m_explicitValue;
    mutable int m_value;

    bool m_hasExplicitValue : 1;
    mutable bool m_isValueUpToDate : 1;
    bool m_notInList : 1;
};

} // namespace WebCore

#endif // RenderListItem_h
