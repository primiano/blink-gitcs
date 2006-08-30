/*
    Copyright (C) 2004, 2005, 2006 Nikolas Zimmermann <wildfox@kde.org>
                  2004, 2005, 2006 Rob Buis <buis@kde.org>
                  2005 Eric Seidel <eric.seidel@kdemail.net>

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

#include "config.h"
#include "RenderPathQt.h"
#include "KCanvasRenderingStyle.h"
#include "KRenderingStrokePainter.h"

#include <QDebug>
#include <QPainterPathStroker>

namespace WebCore {
    
RenderPathQt::RenderPathQt(RenderStyle* style, SVGStyledElement* node)
    : RenderPath(style, node)
{
}

void RenderPathQt::drawMarkersIfNeeded(GraphicsContext*, const FloatRect&, const Path&) const
{
    qDebug("RenderPathQt::drawMarkersIfNeeded() TODO!");
}

bool RenderPathQt::strokeContains(const FloatPoint& point, bool requiresStroke) const
{
    if (path().isEmpty())
        return false;

    if (requiresStroke && !KSVGPainterFactory::strokePaintServer(style(), this))
        return false;

    return false;
}

static QPainterPath getPathStroke(const QPainterPath &path, const KRenderingStrokePainter &strokePainter)
{
    QPainterPathStroker s;
    s.setWidth(strokePainter.strokeWidth());
    if(strokePainter.strokeCapStyle() == CAP_BUTT)
        s.setCapStyle(Qt::FlatCap);
    else if(strokePainter.strokeCapStyle() == CAP_ROUND)
        s.setCapStyle(Qt::RoundCap);

    if(strokePainter.strokeJoinStyle() == JOIN_MITER) {
        s.setJoinStyle(Qt::MiterJoin);
        s.setMiterLimit((qreal)strokePainter.strokeMiterLimit());
    } else if(strokePainter.strokeJoinStyle() == JOIN_ROUND)
        s.setJoinStyle(Qt::RoundJoin);

    KCDashArray dashes = strokePainter.dashArray();
    unsigned int dashLength = !dashes.isEmpty() ? dashes.count() : 0;
    if(dashLength) {
        QVector<qreal> pattern;
        unsigned int count = (dashLength % 2) == 0 ? dashLength : dashLength * 2;

        for(unsigned int i = 0; i < count; i++)
            pattern.append(dashes[i % dashLength] / (float)s.width());

        s.setDashPattern(pattern);
        // TODO: dash-offset, does/will qt4 API allow it? (Rob)
    }

    return s.createStroke(path);
}

FloatRect RenderPathQt::strokeBBox() const
{
    KRenderingStrokePainter strokePainter = KSVGPainterFactory::strokePainter(style(), this);
    QPainterPath outline = getPathStroke(*(path().platformPath()), strokePainter);
    return outline.boundingRect();
}

}

// vim:ts=4:noet
