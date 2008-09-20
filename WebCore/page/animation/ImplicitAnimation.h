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

#ifndef ImplicitAnimation_h
#define ImplicitAnimation_h

#include "AnimationBase.h"
#include "Document.h"

namespace WebCore {

// An ImplicitAnimation tracks the state of a transition of a specific CSS property
// for a single RenderObject.
class ImplicitAnimation : public AnimationBase {
public:
    ImplicitAnimation(const Animation*, int animatingProperty, RenderObject*, CompositeAnimation*);
    virtual ~ImplicitAnimation();
    
    int transitionProperty() const { return m_transitionProperty; }
    int animatingProperty() const { return m_animatingProperty; }

    virtual void onAnimationEnd(double elapsedTime);

    virtual void animate(CompositeAnimation*, RenderObject*, const RenderStyle* currentStyle, const RenderStyle* targetStyle, RenderStyle*& animatedStyle);
    virtual void reset(RenderObject*, const RenderStyle* from = 0, const RenderStyle* to = 0);

    void setOverridden(bool);
    virtual bool overridden() const { return m_overridden; }

    virtual bool shouldFireEvents() const { return true; }

    virtual bool affectsProperty(int) const;

    bool hasStyle() const { return m_fromStyle && m_toStyle; }

    bool isTargetPropertyEqual(int, const RenderStyle* targetStyle);

    void blendPropertyValueInStyle(int, RenderStyle* currentStyle);

protected:
    bool shouldSendEventForListener(Document::ListenerType);    
    bool sendTransitionEvent(const AtomicString&, double elapsedTime);

    void validateTransformFunctionList();

private:
    int m_transitionProperty;   // Transition property as specified in the RenderStyle. May be cAnimateAll
    int m_animatingProperty;    // Specific property for this ImplicitAnimation
    bool m_overridden;          // true when there is a keyframe animation that overrides the transitioning property

    // The two styles that we are blending.
    const RenderStyle* m_fromStyle;
    const RenderStyle* m_toStyle;
};

} // namespace WebCore

#endif // ImplicitAnimation_h
