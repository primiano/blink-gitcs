// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"
#include "core/dom/DOMMatrix.h"

namespace blink {

bool DOMMatrixReadOnly::is2D() const
{
    return m_is2D;
}

bool DOMMatrixReadOnly::isIdentity() const
{
    return m_matrix.isIdentity();
}

DOMMatrix* DOMMatrixReadOnly::translate(double tx, double ty, double tz)
{
    return DOMMatrix::create(this)->translateSelf(tx, ty, tz);
}

DOMMatrix* DOMMatrixReadOnly::scale(double scale, double ox, double oy)
{
    return DOMMatrix::create(this)->scaleSelf(scale, ox, oy);
}

DOMMatrix* DOMMatrixReadOnly::scale3d(double scale, double ox, double oy, double oz)
{
    return DOMMatrix::create(this)->scale3dSelf(scale, ox, oy, oz);
}

DOMMatrix* DOMMatrixReadOnly::scaleNonUniform(double sx, double sy, double sz,
    double ox, double oy, double oz)
{
    return DOMMatrix::create(this)->scaleNonUniformSelf(sx, sy, sz, ox, oy, oz);
}

} // namespace blink
