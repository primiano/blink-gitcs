/*
 * Copyright (C) 2009 Apple Inc.  All rights reserved.
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

#include "config.h"
#include "TransformState.h"

namespace WebCore {

void TransformState::move(int x, int y, bool accumulateTransform)
{
    if (m_accumulatingTransform && m_accumulatedTransform) {
        // If we're accumulating into an existing transform, apply the translation.
        if (m_direction == ApplyTransformDirection)
            m_accumulatedTransform->translateRight(x, y);
        else
            m_accumulatedTransform->translate(-x, -y);  // We're unapplying, so negate
        
        // Then flatten if necessary.
        if (!accumulateTransform)
            flatten();
    } else {
        // Just move the point and, optionally, the quad.
        m_lastPlanarPoint.move(x, y);
        if (m_mapQuad)
            m_lastPlanarQuad.move(x, y);
    }
    m_accumulatingTransform = accumulateTransform;
}

void TransformState::applyTransform(const TransformationMatrix& transformFromContainer, bool accumulateTransform)
{
    // If we have an accumulated transform from last time, multiply in this transform
    if (m_accumulatedTransform) {
        if (m_direction == ApplyTransformDirection)
            m_accumulatedTransform->multiply(transformFromContainer);
        else
            m_accumulatedTransform->multLeft(transformFromContainer);
    } else if (accumulateTransform) {
        // Make one if we started to accumulate
        m_accumulatedTransform.set(new TransformationMatrix(transformFromContainer));
    }
    
    if (!accumulateTransform) {
        const TransformationMatrix* finalTransform = m_accumulatedTransform ? m_accumulatedTransform.get() : &transformFromContainer;
        flattenWithTransform(*finalTransform);
    }
    m_accumulatingTransform = accumulateTransform;
}

void TransformState::flatten()
{
    if (!m_accumulatedTransform) {
        m_accumulatingTransform = false;
        return;
    }
    
    flattenWithTransform(*m_accumulatedTransform);
}

FloatPoint TransformState::mappedPoint() const
{
    if (!m_accumulatedTransform)
        return m_lastPlanarPoint;

    if (m_direction == ApplyTransformDirection)
        return m_accumulatedTransform->mapPoint(m_lastPlanarPoint);

    return m_accumulatedTransform->inverse().projectPoint(m_lastPlanarPoint);
}

FloatQuad TransformState::mappedQuad() const
{
    if (!m_accumulatedTransform)
        return m_lastPlanarQuad;

    if (m_direction == ApplyTransformDirection)
        return m_accumulatedTransform->mapQuad(m_lastPlanarQuad);

    return m_accumulatedTransform->inverse().projectQuad(m_lastPlanarQuad);
}

void TransformState::flattenWithTransform(const TransformationMatrix& t)
{
    if (m_direction == ApplyTransformDirection) {
        m_lastPlanarPoint = t.mapPoint(m_lastPlanarPoint);
        if (m_mapQuad)
            m_lastPlanarQuad = t.mapQuad(m_lastPlanarQuad);
    } else {
        TransformationMatrix inverseTransform = t.inverse();
        m_lastPlanarPoint = inverseTransform.projectPoint(m_lastPlanarPoint);
        if (m_mapQuad)
            m_lastPlanarQuad = inverseTransform.projectQuad(m_lastPlanarQuad);
    }

    // We could throw away m_accumulatedTransform if we wanted to here, but that
    // would cause thrash when traversing hierarachies with alternating
    // preserve-3d and flat elements.
    if (m_accumulatedTransform)
        m_accumulatedTransform->makeIdentity();
    m_accumulatingTransform = false;
}

// HitTestingTransformState methods
void HitTestingTransformState::move(int x, int y)
{
    if (m_accumulatingTransform)
        flatten();

    m_lastPlanarPoint.move(x, y);
    m_lastPlanarQuad.move(x, y);
}

void HitTestingTransformState::applyTransform(const TransformationMatrix& transformFromContainer, bool accumulateTransform)
{
    m_accumulatedTransform.multLeft(transformFromContainer);    
    if (!accumulateTransform)
        flatten();

    m_accumulatingTransform = accumulateTransform;
}

void HitTestingTransformState::flatten()
{
    m_lastPlanarPoint = mappedPoint();
    m_lastPlanarQuad = mappedQuad();
    m_accumulatedTransform.makeIdentity();
    m_accumulatingTransform = false;
}

FloatPoint HitTestingTransformState::mappedPoint() const
{
    return m_accumulatedTransform.inverse().projectPoint(m_lastPlanarPoint);
}

FloatQuad HitTestingTransformState::mappedQuad() const
{
    return m_accumulatedTransform.inverse().projectQuad(m_lastPlanarQuad);
}

} // namespace WebCore
