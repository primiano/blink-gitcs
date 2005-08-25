/*
 * Copyright (C) 2004, 2005 Apple Computer, Inc.  All rights reserved.
 *           (C) 2005 Rob Buis <buis@kde.org>
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "KCanvasTreeDebug.h"

#include <kcanvas/KCanvasMatrix.h>
#include <kcanvas/KCanvasItem.h>
#include <kcanvas/KCanvasContainer.h>
#include <kcanvas/device/KRenderingStyle.h>
#include <kcanvas/device/KRenderingStrokePainter.h>
#include <kcanvas/device/KRenderingFillPainter.h>
#include <kcanvas/KCanvasResources.h>
#include <kcanvas/KCanvasFilters.h>
#include <kcanvas/device/quartz/KRenderingDeviceQuartz.h>
#include <kcanvas/device/quartz/QuartzSupport.h>

#include <ksvg2/impl/SVGStyledElementImpl.h>

#include <kdom/DOMString.h>

#include <qtextstream.h>

static QTextStream &operator<<(QTextStream &ts, const QRect &r)
{
    return ts << "at (" << r.x() << "," << r.y() << ") size " << r.width() << "x" << r.height();
}

static void writeIndent(QTextStream &ts, int indent)
{
    for (int i = 0; i != indent; ++i) {
        ts << "  ";
    }
}

static QTextStream &operator<<(QTextStream &ts, const KCanvasMatrix &m)
{
    ts << "{m=((" << m.a() << "," << m.b() << ")(" << m.c() << "," << m.d() << "))";
    ts << " t=(" << m.e() << "," << m.f() << ")}";
    return ts;
}

static QTextStream &operator<<(QTextStream &ts, const KCanvasResource *f)
{
    ts << "UNIMPLEMENTED";
    return ts;
}

static QTextStream &operator<<(QTextStream &ts, const KRenderingPaintServer *f)
{
    ts << "UNIMPLEMENTED";
    return ts;
}

static QTextStream &operator<<(QTextStream &ts, const KCDashArray &a)
{
    ts << "{";
    for (KCDashArray::ConstIterator it = a.begin(); it != a.end(); ++it) {
        if (it != a.begin())
            ts << ", ";
        ts << *it;
    }
    ts << "}";
    return ts;
}

static QTextStream &operator<<(QTextStream &ts, KCCapStyle style)
{
    switch (style) {
    case CAP_BUTT:
        ts << "BUTT"; break;
    case CAP_ROUND:
        ts << "ROUND"; break;
    case CAP_SQUARE:
        ts << "SQUARE"; break;
    }
    return ts;
}

static QTextStream &operator<<(QTextStream &ts, KCJoinStyle style)
{
    switch (style) {
    case JOIN_MITER:
        ts << "MITER"; break;
    case JOIN_ROUND:
        ts << "ROUND"; break;
    case JOIN_BEVEL:
        ts << "BEVEL"; break;
    }
    return ts;
}

static QTextStream &operator<<(QTextStream &ts, const KRenderingStrokePainter *p)
{
    ts << "{";
    ts << "server=" << p->paintServer();
    if (p->opacity() != 1.0f)
        ts << ", " << "opacity=" << p->opacity();
    if (p->strokeWidth() != 1.0f)
        ts << ", " << "stroke width=" << p->strokeWidth();
    if (p->strokeMiterLimit() != 4)
        ts << ", " << "miter limit=" << p->strokeMiterLimit();
    if (p->strokeCapStyle() != 1)
        ts << ", " << "line cap=" << p->strokeCapStyle();
    if (p->strokeJoinStyle() != 1)
        ts << ", " << "line join=" << p->strokeJoinStyle();
    if (p->dashOffset() != 0.0f)
        ts << ", " << "dash offset=" << p->dashOffset();
    if (!p->dashArray().isEmpty())
        ts << ", " << "dash array=" << p->dashArray();
    ts << "}";
    return ts;
}



static QTextStream &operator<<(QTextStream &ts, KCWindRule rule)
{
    switch (rule) {
    case RULE_NONZERO:
        ts << "NON-ZERO"; break;
    case RULE_EVENODD:
        ts << "EVEN-ODD"; break;
    }
    return ts;
}

static QTextStream &operator<<(QTextStream &ts, const KRenderingFillPainter *p)
{
    ts << "{";
    ts << "server=" << p->paintServer();
    if (p->opacity() != 1.0f)
        ts << ", " << "opacity=" << p->opacity();
    if (p->fillRule() != RULE_NONZERO)
        ts << ", " << "fill rule=" << p->fillRule();
    ts << "}";
    return ts;
}

#define DIFFERS_FROM_PARENT(path) (o.parent() && (o.parent()->path != o.path))

static QTextStream &operator<<(QTextStream &ts, const KCanvasItem &o)
{
    ts << " " << o.bbox();
    
    if (DIFFERS_FROM_PARENT(style()->visible()) && !o.style()->visible())
        ts << " [HIDDEN]";
    if (DIFFERS_FROM_PARENT(style()->objectMatrix()))
        ts << " [transform=" << o.style()->objectMatrix() << "]";
    if (DIFFERS_FROM_PARENT(style()->colorInterpolation()))
        ts << " [color interpolation=" << o.style()->colorInterpolation() << "]";
    if (DIFFERS_FROM_PARENT(style()->imageRendering()))
        ts << " [image rendering=" << o.style()->imageRendering() << "]";
    if (DIFFERS_FROM_PARENT(style()->opacity()))
        ts << " [opacity=" << o.style()->opacity() << "]";
    if (o.style()->isStroked() && DIFFERS_FROM_PARENT(style()->strokePainter()) && o.style()->strokePainter())
        ts << " [stroke=" << o.style()->strokePainter() << "]";
    if (o.style()->isFilled() && DIFFERS_FROM_PARENT(style()->fillPainter()) && o.style()->fillPainter())
        ts << " [fill=" << o.style()->fillPainter() << "]";
    if (!o.style()->clipPaths().isEmpty())
        ts << " [clip paths=" << o.style()->clipPaths().join(", ") << "]";
    if (o.style()->hasMarkers()) {
        if (o.style()->startMarker())
            ts << " [start marker=" << o.style()->startMarker() << "]";
        if (o.style()->midMarker())
            ts << " [middle marker=" << o.style()->midMarker() << "]";
        if (o.style()->endMarker())
            ts << " [end marker=" << o.style()->endMarker() << "]";
    }
    if (DIFFERS_FROM_PARENT(style()->filter()) && o.style()->filter())
        ts << " [filter=" << o.style()->filter() << "]";
    
    // Print the actual path data
    if (o.path()) {
        CGMutablePathRef cgPath = static_cast<KCanvasQuartzPathData *>(o.path())->path;
        CFStringRef pathString = CFStringFromCGPath(cgPath);
        ts << " [data=\"" << QString::fromCFString(pathString) << "\"]";
        CFRelease(pathString);
    }
    
    return ts;
}

static QString getTagName(void *node)
{
    KSVG::SVGStyledElementImpl *elem = static_cast<KSVG::SVGStyledElementImpl *>(node);
    if (elem)
        return KDOM::DOMString(elem->nodeName()).string();
    return QString();
}

void write(QTextStream &ts, KCanvasItem *item, int indent = 0)
{
    if (item)
    {
        writeIndent(ts, indent);
        if(item->isContainer())
            ts << "KCanvasContainer";
        else
            ts << "KCanvasItem";
        
        if (item->userData()) {
            QString tagName = getTagName(item->userData());
            if (!tagName.isEmpty()) {
                ts << " {" << tagName << "}";
            }
        }
        
        ts << *item << endl;
        
        if(item->isContainer()) {
            KCanvasContainer *parent = static_cast<KCanvasContainer *>(item);
            for (KCanvasItem *child = parent->first(); child != NULL; child = child->next())
                write(ts, child, indent + 1);
        }
    }
}

QString externalRepresentation(KCanvasItem *item)
{
    QString s;
    {
        QTextStream ts(&s);
        write(ts, item);
    }
    return s;
}
