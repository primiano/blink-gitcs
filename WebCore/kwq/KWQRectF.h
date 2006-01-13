/*
 * Copyright (C) 2003, 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2005 Nokia.  All rights reserved.
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

#ifndef QRECTF_H_
#define QRECTF_H_

#include <math.h>
#include "FloatSize.h"
#include "KWQPointF.h"
#include "KWQRect.h"

#ifdef NSGEOMETRY_TYPES_SAME_AS_CGGEOMETRY_TYPES
typedef struct CGRect NSRect;
#else
typedef struct _NSRect NSRect;
#endif
typedef struct CGRect CGRect;
class QRect;

class QRectF {
public:
    QRectF();
    QRectF(QPointF p, FloatSize s);
    QRectF(float, float, float, float);
    QRectF(const QPointF&, const QPointF&);
    QRectF(const QRect&);
#ifndef NSGEOMETRY_TYPES_SAME_AS_CGGEOMETRY_TYPES
    explicit QRectF(const NSRect&);
#endif
    explicit QRectF(const CGRect&);

    bool isNull() const;
    bool isValid() const;
    bool isEmpty() const;

    float x() const { return xp; }
    float y() const { return yp; }
    float left() const { return xp; }
    float top() const { return yp; }
    float right() const;
    float bottom() const;
    float width() const { return w; }
    float height() const { return h; }

    QPointF topLeft() const;
    QPointF topRight() const;
    QPointF bottomRight() const;
    QPointF bottomLeft() const;

    FloatSize size() const;
    void setX(float x) { xp = x; }
    void setY(float y) { yp = y; }
    void setWidth(float width) { w = width; }
    void setHeight(float height) { h = height; }
    void setRect(float x, float y, float width, float height) { xp = x; yp = y; w = width; h = height; }
    QRectF intersect(const QRectF&) const;
    bool intersects(const QRectF&) const;
    QRectF unite(const QRectF&) const;
    QRectF normalize() const;

    bool contains(const QPointF& point) const { return contains(point.x(), point.y()); }

    bool contains(float x, float y, bool proper = false) const
    {
        if (proper)
            return x > xp && (x < (xp + w - 1)) && y > yp && y < (yp + h - 1);
        return x >= xp && x < (xp + w) && y >= yp && y < (yp + h);
    }

    bool contains(const QRectF& rect) const { return intersect(rect) == rect; }

    void inflate(float s);

    inline QRectF operator&(const QRectF& r) const { return intersect(r); }

#ifndef NSGEOMETRY_TYPES_SAME_AS_CGGEOMETRY_TYPES
    operator NSRect() const;
#endif
    operator CGRect() const;
    operator QRect() const;
private:
    float xp;
    float yp;
    float w;
    float h;

    friend bool operator==(const QRectF&, const QRectF&);
    friend bool operator!=(const QRectF&, const QRectF&);
};

inline QRect enclosingQRect(const QRectF& fr)
{
    int x = int(floor(fr.x()));
    int y = int(floor(fr.y()));
    return QRect(x, y, int(ceil(fr.x() + fr.width())) - x, int(ceil(fr.y() + fr.height())) - y);
}

#endif
