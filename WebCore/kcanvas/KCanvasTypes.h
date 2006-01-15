/*
    Copyright (C) 2004, 2005 Nikolas Zimmermann <wildfox@kde.org>
                  2004, 2005 Rob Buis <buis@kde.org>

    This file is part of the KDE project

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    aint with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#ifndef KCanvasTypes_H
#define KCanvasTypes_H

#include <kxmlcore/RefPtr.h>

namespace khtml {
    class RenderStyle;
}

class KCanvasPath;
class RenderPath;
class KCanvasCommonArgs
{
public:
    KCanvasCommonArgs();
    ~KCanvasCommonArgs();

    KCanvasPath* path() const;
    void setPath(KCanvasPath* path);

    khtml::RenderStyle *renderStyle() const;
    void setRenderStyle(khtml::RenderStyle *style);
    
    const RenderPath* renderPath() const;
    void setRenderPath(const RenderPath* path);

private:
    RefPtr<KCanvasPath> m_path;
    khtml::RenderStyle *m_renderStyle;
    const RenderPath *m_renderPath;
};

#endif

// vim:ts=4:noet
