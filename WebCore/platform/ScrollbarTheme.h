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

#ifndef ScrollbarTheme_h
#define ScrollbarTheme_h

#include "IntRect.h"
#include "ScrollTypes.h"

namespace WebCore {

class GraphicsContext;
class PlatformMouseEvent;
class Scrollbar;

class ScrollbarTheme {
public:
    virtual ~ScrollbarTheme() {};

    virtual bool paint(Scrollbar*, GraphicsContext* context, const IntRect& damageRect) { return false; }
    virtual ScrollbarPart hitTest(Scrollbar*, const PlatformMouseEvent&) { return NoPart; }
    
    virtual int scrollbarThickness(ScrollbarControlSize = RegularScrollbar) { return 0; }

    virtual bool supportsControlTints() const { return false; }

    virtual void themeChanged() {}
    
    virtual bool invalidateOnMouseEnterExit() { return false; }
    virtual void invalidatePart(Scrollbar*, ScrollbarPart) {}

    virtual bool shouldCenterOnThumb(Scrollbar*, const PlatformMouseEvent&) { return false; }
    virtual void centerOnThumb(Scrollbar*, const PlatformMouseEvent&) {}
    virtual int thumbPosition(Scrollbar*) { return 0; }
    virtual int thumbLength(Scrollbar*) { return 0; }
    virtual int trackLength(Scrollbar*) { return 0; }
    
    virtual double initialAutoscrollTimerDelay() { return 0.25; }
    virtual double autoscrollTimerDelay() { return 0.05; }

    static ScrollbarTheme* nativeTheme(); // Must be implemented to return the correct theme subclass.
};

}
#endif
