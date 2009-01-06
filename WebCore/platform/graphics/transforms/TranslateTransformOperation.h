/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Graham Dennis (graham.dennis@gmail.com)
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
 *
 */

#ifndef TranslateTransformOperation_h
#define TranslateTransformOperation_h

#include "Length.h"
#include "TransformOperation.h"

namespace WebCore {

class TranslateTransformOperation : public TransformOperation {
public:
    static PassRefPtr<TranslateTransformOperation> create(const Length& tx, const Length& ty, OperationType type)
    {
        return adoptRef(new TranslateTransformOperation(tx, ty, type));
    }

    virtual bool isIdentity() const { return m_x.calcFloatValue(1) == 0 && m_y.calcFloatValue(1) == 0; }
    virtual OperationType getOperationType() const { return m_type; }
    virtual bool isSameType(const TransformOperation& o) const { return o.getOperationType() == m_type; }

    virtual bool operator==(const TransformOperation& o) const
    {
        if (!isSameType(o))
            return false;
        const TranslateTransformOperation* t = static_cast<const TranslateTransformOperation*>(&o);
        return m_x == t->m_x && m_y == t->m_y;
    }

    virtual bool apply(TransformationMatrix& transform, const IntSize& borderBoxSize) const
    {
        transform.translate(m_x.calcFloatValue(borderBoxSize.width()), m_y.calcFloatValue(borderBoxSize.height()));
        return m_x.type() == Percent || m_y.type() == Percent;
    }

    virtual PassRefPtr<TransformOperation> blend(const TransformOperation* from, double progress, bool blendToIdentity = false);

private:
    TranslateTransformOperation(const Length& tx, const Length& ty, OperationType type)
        : m_x(tx)
        , m_y(ty)
        , m_type(type)
    {
    }

    Length m_x;
    Length m_y;
    OperationType m_type;
};

} // namespace WebCore

#endif // TranslateTransformOperation_h
