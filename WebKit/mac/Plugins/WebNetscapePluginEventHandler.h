/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef WebNetscapePluginEventHandler_h
#define WebNetscapePluginEventHandler_h

#if ENABLE(NETSCAPE_PLUGIN_API)

@class NSEvent;
@class WebBaseNetscapePluginView;

struct CGRect;

class WebNetscapePluginEventHandler {
public:
    static WebNetscapePluginEventHandler* create(WebBaseNetscapePluginView*);
    virtual ~WebNetscapePluginEventHandler() { }
    
    virtual void drawRect(const NSRect&) = 0;
    
    virtual void mouseDown(NSEvent*) = 0;
    virtual void mouseDragged(NSEvent*) = 0;
    virtual void mouseEntered(NSEvent*) = 0;
    virtual void mouseExited(NSEvent*) = 0;
    virtual void mouseMoved(NSEvent*) = 0;
    virtual void mouseUp(NSEvent*) = 0;
    
    virtual void keyDown(NSEvent*) = 0;
    virtual void keyUp(NSEvent*) = 0;
    virtual void focusChanged(bool hasFocus) = 0;
    virtual void windowFocusChanged(bool hasFocus) = 0;
    
    virtual void startTimers(bool throttleTimers) = 0;
    virtual void stopTimers() = 0;
    
    bool currentEventIsUserGesture() const { return m_currentEventIsUserGesture; }
protected:
    WebNetscapePluginEventHandler(WebBaseNetscapePluginView* pluginView)
        : m_pluginView(pluginView)
        , m_currentEventIsUserGesture(false)
    {
    }
    
    WebBaseNetscapePluginView* m_pluginView;
    bool m_currentEventIsUserGesture;
};

#endif // ENABLE(NETSCAPE_PLUGIN_API)

#endif // WebNetscapePluginEventHandler_h


