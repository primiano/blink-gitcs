/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
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
#include "DOMWindow.h"

#include "BarInfo.h"
#include "CSSComputedStyleDeclaration.h"
#include "CSSRuleList.h"
#include "Chrome.h"
#include "Document.h"
#include "Element.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "Page.h"
#include "PlatformScreen.h"
#include "Screen.h"
#include "cssstyleselector.h"

namespace WebCore {

DOMWindow::DOMWindow(Frame* frame)
    : m_frame(frame)
{
}

DOMWindow::~DOMWindow()
{
}

Frame* DOMWindow::frame()
{
    return m_frame;
}

void DOMWindow::disconnectFrame()
{
    m_frame = 0;
    if (m_screen)
        m_screen->disconnectFrame();
    if (m_locationbar)
        m_locationbar->disconnectFrame();
    if (m_menubar)
        m_menubar->disconnectFrame();
    if (m_personalbar)
        m_personalbar->disconnectFrame();
    if (m_scrollbars)
        m_scrollbars->disconnectFrame();
    if (m_statusbar)
        m_statusbar->disconnectFrame();
    if (m_toolbar)
        m_toolbar->disconnectFrame();
}

Screen* DOMWindow::screen() const
{
    if (!m_screen)
        m_screen = new Screen(m_frame);
    return m_screen.get();
}

BarInfo* DOMWindow::locationbar() const
{
    if (!m_locationbar)
        m_locationbar = new BarInfo(m_frame, BarInfo::Locationbar);
    return m_locationbar.get();
}

BarInfo* DOMWindow::menubar() const
{
    if (!m_menubar)
        m_menubar = new BarInfo(m_frame, BarInfo::Menubar);
    return m_menubar.get();
}

BarInfo* DOMWindow::personalbar() const
{
    if (!m_personalbar)
        m_personalbar = new BarInfo(m_frame, BarInfo::Personalbar);
    return m_personalbar.get();
}

BarInfo* DOMWindow::scrollbars() const
{
    if (!m_scrollbars)
        m_scrollbars = new BarInfo(m_frame, BarInfo::Scrollbars);
    return m_scrollbars.get();
}

BarInfo* DOMWindow::statusbar() const
{
    if (!m_statusbar)
        m_statusbar = new BarInfo(m_frame, BarInfo::Statusbar);
    return m_statusbar.get();
}

BarInfo* DOMWindow::toolbar() const
{
    if (!m_toolbar)
        m_toolbar = new BarInfo(m_frame, BarInfo::Toolbar);
    return m_toolbar.get();
}

Document* DOMWindow::document() const
{
    if (!m_frame)
        return 0;

    ASSERT(m_frame->document());
    return m_frame->document();
}

PassRefPtr<CSSStyleDeclaration> DOMWindow::getComputedStyle(Element* elt, const String&) const
{
    if (!elt)
        return 0;
    
    // FIXME: This needs to work with pseudo elements. 
    return new CSSComputedStyleDeclaration(elt);
}

PassRefPtr<CSSRuleList> DOMWindow::getMatchedCSSRules(Element* elt, const String& pseudoElt, bool authorOnly) const
{
    if (!m_frame || !m_frame->document())
        return 0;
    
    if (!pseudoElt.isEmpty())
        return m_frame->document()->styleSelector()->pseudoStyleRulesForElement(elt, pseudoElt.impl(), authorOnly);
    return m_frame->document()->styleSelector()->styleRulesForElement(elt, authorOnly);
}

double DOMWindow::devicePixelRatio() const
{
    return m_frame->page()->chrome()->scaleFactor();
}

} // namespace WebCore
