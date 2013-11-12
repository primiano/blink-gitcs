/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CompositorAnimations_h
#define CompositorAnimations_h

#include "core/animation/AnimationEffect.h"
#include "core/animation/KeyframeAnimationEffect.h"
#include "core/animation/Timing.h"
#include "core/platform/animation/TimingFunction.h"
#include "public/platform/WebAnimation.h"
#include "public/platform/WebAnimationCurve.h"
#include "public/platform/WebCompositorSupport.h"
#include "public/platform/WebFilterAnimationCurve.h"
#include "public/platform/WebFilterKeyframe.h"
#include "public/platform/WebFilterOperations.h"
#include "public/platform/WebFloatAnimationCurve.h"
#include "public/platform/WebFloatKeyframe.h"
#include "public/platform/WebTransformAnimationCurve.h"
#include "public/platform/WebTransformKeyframe.h"
#include "public/platform/WebTransformOperations.h"
#include "wtf/Vector.h"

namespace WebCore {

class Element;
class AnimationEffect;

// Given an input timing function between keyframe at 0 and keyframe at 1.0, we
// need a timing function such that the behavior with the keyframes swapped is
// equivalent to reversing time with the input timing function and keyframes.
// This means flipping the timing function about x=0.5 and about y=0.5.
// FIXME: Remove once the Compositor natively understands reversing time.
class CompositorAnimationsTimingFunctionReverser {
public:
    static PassRefPtr<TimingFunction> reverse(const LinearTimingFunction* timefunc);
    static PassRefPtr<TimingFunction> reverse(const CubicBezierTimingFunction* timefunc);
    static PassRefPtr<TimingFunction> reverse(const ChainedTimingFunction* timefunc);
    static PassRefPtr<TimingFunction> reverse(const TimingFunction* timefunc);
};

class CompositorAnimations {
public:
    static CompositorAnimations* instance() { return instance(0); }
    static void setInstanceForTesting(CompositorAnimations* newInstance) { instance(newInstance); }

    virtual bool isCandidateForCompositorAnimation(const Timing&, const AnimationEffect&);
    virtual bool canStartCompositorAnimation(const Element&);
    virtual void startCompositorAnimation(const Element&, const Timing&, const AnimationEffect&, Vector<int>& startedAnimationIds);
    virtual void cancelCompositorAnimation(const Element&, int id);

protected:
    CompositorAnimations() { }
    static CompositorAnimations* instance(CompositorAnimations* newInstance)
    {
        static CompositorAnimations* instance = new CompositorAnimations();
        if (newInstance) {
            instance = newInstance;
        }
        return instance;
    }
};

} // namespace WebCore

#endif
