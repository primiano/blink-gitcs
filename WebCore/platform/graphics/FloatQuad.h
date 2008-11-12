/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef FloatQuad_h
#define FloatQuad_h

#include "FloatPoint.h"
#include "FloatRect.h"
#include "IntRect.h"

namespace WebCore {

// A FloatQuad is a collection of 4 points, often representing the result of
// mapping a rectangle through transforms. When initialized from a rect, the
// points are in clockwise order from top left.
class FloatQuad {
public:
    FloatQuad()
    {
    }

    FloatQuad(const FloatPoint& p1, const FloatPoint& p2, const FloatPoint& p3, const FloatPoint& p4)
        : m_p1(p1)
        , m_p2(p2)
        , m_p3(p3)
        , m_p4(p4)
    {
    }

    FloatQuad(const FloatRect& inRect)
        : m_p1(inRect.location())
        , m_p2(inRect.right(), inRect.y())
        , m_p3(inRect.right(), inRect.bottom())
        , m_p4(inRect.x(), inRect.bottom())
    {
    }

    FloatPoint p1() const { return m_p1; }
    FloatPoint p2() const { return m_p2; }
    FloatPoint p3() const { return m_p3; }
    FloatPoint p4() const { return m_p4; }

    void setP1(const FloatPoint& p) { m_p1 = p; }
    void setP2(const FloatPoint& p) { m_p2 = p; }
    void setP3(const FloatPoint& p) { m_p3 = p; }
    void setP4(const FloatPoint& p) { m_p4 = p; }

    // isEmpty tests that the bounding box is empty. This will not identify
    // "slanted" empty quads.
    bool isEmpty() const { return boundingBox().isEmpty(); }

    FloatRect boundingBox() const;
    IntRect enclosingBoundingBox() const
    {
        return enclosingIntRect(boundingBox());
    }

    void move(const FloatSize& offset)
    {
        m_p1 += offset;
        m_p2 += offset;
        m_p3 += offset;
        m_p4 += offset;
    }

    void move(float dx, float dy)
    {
        m_p1.move(dx, dy);
        m_p2.move(dx, dy);
        m_p3.move(dx, dy);
        m_p4.move(dx, dy);
    }

private:
    FloatPoint m_p1;
    FloatPoint m_p2;
    FloatPoint m_p3;
    FloatPoint m_p4;
};

inline FloatQuad& operator+=(FloatQuad& a, const FloatSize& b)
{
    a.move(b);
    return a;
}

inline FloatQuad& operator-=(FloatQuad& a, const FloatSize& b)
{
    a.move(-b.width(), -b.height());
    return a;
}

inline bool operator==(const FloatQuad& a, const FloatQuad& b)
{
    return a.p1() == b.p1() &&
           a.p2() == b.p2() && 
           a.p3() == b.p3() &&
           a.p4() == b.p4();
}

inline bool operator!=(const FloatQuad& a, const FloatQuad& b)
{
    return a.p1() != b.p1() ||
           a.p2() != b.p2() || 
           a.p3() != b.p3() ||
           a.p4() != b.p4();
}

}   // namespace WebCore


#endif // FloatQuad_h

