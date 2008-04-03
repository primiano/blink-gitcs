/*
 * Copyright (C) 2006, 2007, 2008 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
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
#include "Gradient.h"

#include "CSSParser.h"

namespace WebCore {

void Gradient::platformDestroy()
{
    cairo_pattern_destroy(m_shading);
    m_shading = 0;
}

cairo_pattern_t* Gradient::platformGradient()
{
    if (m_gradient)
        return m_gradient;

    if (m_radial)
        m_gradient = cairo_pattern_create_radial(m_p0.x(), m_p0.y(), m_r0, m_p1.x(), m_p1.y(), m_r1);
    else
        m_gradient = cairo_pattern_create_linear(m_p0.x(), m_p0.y(), m_p1.x(), m_p1.y());

    Vector<ColorStop>::iterator stopIterator = m_stops.begin();
    while (stopIterator != m_stops.end()) {
        cairo_pattern_add_color_stop_rgba(m_gradient, stopIterator->stop, stopIterator->red, stopIterator->green, stopIterator->blue, stopIterator->alpha);
        ++stopIterator;
    }

    return m_gradient;
}

} //namespace
