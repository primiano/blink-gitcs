/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
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
#include "Screen.h"

#include "FloatRect.h"
#include "Frame.h"
#include "FrameView.h"
#include "PlatformScreen.h"
#include "Widget.h"

namespace WebCore {

Screen::Screen(Frame* frame)
    : m_frame(frame)
{
}

void Screen::disconnectFrame()
{
    m_frame = 0;
}

unsigned Screen::height() const
{
    if (!m_frame)
        return 0;
    return screenRect(m_frame->view()).height();
}

unsigned Screen::width() const
{
    if (!m_frame)
        return 0;
    return screenRect(m_frame->view()).width();
}

unsigned Screen::colorDepth() const
{
    if (!m_frame)
        return 0;
    return screenDepth(m_frame->view());
}

unsigned Screen::pixelDepth() const
{
    if (!m_frame)
        return 0;
    return screenDepth(m_frame->view());
}

unsigned Screen::availLeft() const
{
    if (!m_frame)
        return 0;
    return screenAvailableRect(m_frame->view()).x();
}

unsigned Screen::availTop() const
{
    if (!m_frame)
        return 0;
    return screenAvailableRect(m_frame->view()).y();
}

unsigned Screen::availHeight() const
{
    if (!m_frame)
        return 0;
    return screenAvailableRect(m_frame->view()).height();
}

unsigned Screen::availWidth() const
{
    if (!m_frame)
        return 0;
    return screenAvailableRect(m_frame->view()).width();
}

} // namespace WebCore
