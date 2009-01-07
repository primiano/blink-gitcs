/*
 * Copyright (C) 2007 Apple Inc. All rights reserved.
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

#ifndef AnimationController_h
#define AnimationController_h

#include <wtf/Forward.h>

namespace WebCore {

class AnimationControllerPrivate;
class AtomicString;
class Document;
class Element;
class Frame;
class Node;
class RenderObject;
class RenderStyle;
class String;

class AnimationController {
public:
    AnimationController(Frame*);
    ~AnimationController();

    void cancelAnimations(RenderObject*);
    PassRefPtr<RenderStyle> updateAnimations(RenderObject*, RenderStyle* newStyle);

    void setAnimationStartTime(RenderObject*, double t);
    void setTransitionStartTime(RenderObject*, int property, double t);

    bool pauseAnimationAtTime(RenderObject*, const String& name, double t); // To be used only for testing
    bool pauseTransitionAtTime(RenderObject*, const String& property, double t); // To be used only for testing
    unsigned numberOfActiveAnimations() const; // To be used only for testing
    
    bool isAnimatingPropertyOnRenderer(RenderObject*, int property, bool isRunningNow) const;

    void suspendAnimations(Document*);
    void resumeAnimations(Document*);

    void startUpdateRenderingDispatcher();
    void addEventToDispatch(PassRefPtr<Element>, const AtomicString& eventType, const String& name, double elapsedTime);

    void styleAvailable();

    void setWaitingForStyleAvailable(bool waiting)
    {
        if (waiting)
            m_numStyleAvailableWaiters++;
        else
            m_numStyleAvailableWaiters--;
    }
    
    double beginAnimationUpdateTime();
    
    void beginAnimationUpdate();
    void endAnimationUpdate();

private:
    AnimationControllerPrivate* m_data;
    unsigned m_numStyleAvailableWaiters;    
};

} // namespace WebCore

#endif // AnimationController_h
