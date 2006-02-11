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
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#ifndef KSVG_SVGPointImpl_H
#define KSVG_SVGPointImpl_H
#if SVG_SUPPORT

#include "Shared.h"

namespace WebCore {
class IntPoint;
}

namespace WebCore
{
    class SVGMatrixImpl;
    class SVGStyledElementImpl;
    class SVGPointImpl : public Shared<SVGPointImpl>
    {
    public:
        SVGPointImpl(const SVGStyledElementImpl *context = 0);
        SVGPointImpl(float x, float y, const SVGStyledElementImpl *context = 0);
        SVGPointImpl(const WebCore::IntPoint &p, const SVGStyledElementImpl *context = 0);
        ~SVGPointImpl();

        // 'SVGPoint' functions
        void setX(float x);
        float x() const;

        void setY(float y);
        float y() const;

        SVGPointImpl *matrixTransform(SVGMatrixImpl *matrix);

    private:
        float m_x;
        float m_y;

        const SVGStyledElementImpl *m_context;
    };
};

#endif // SVG_SUPPORT
#endif

// vim:ts=4:noet
