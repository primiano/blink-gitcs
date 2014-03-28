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

#include "config.h"
#include "core/animation/AnimationPlayer.h"

#include "core/animation/Animation.h"
#include "core/animation/DocumentTimeline.h"

namespace WebCore {

namespace {

static unsigned nextSequenceNumber()
{
    static unsigned next = 0;
    return ++next;
}

}

PassRefPtr<AnimationPlayer> AnimationPlayer::create(DocumentTimeline& timeline, TimedItem* content)
{
    return adoptRef(new AnimationPlayer(timeline, content));
}

AnimationPlayer::AnimationPlayer(DocumentTimeline& timeline, TimedItem* content)
    : m_playbackRate(1)
    , m_startTime(nullValue())
    , m_holdTime(nullValue())
    , m_storedTimeLag(0)
    , m_sortInfo(nextSequenceNumber(), timeline.effectiveTime())
    , m_content(content)
    , m_timeline(&timeline)
    , m_paused(false)
    , m_held(false)
    , m_isPausedForTesting(false)
    , m_outdated(false)
{
    if (m_content) {
        if (m_content->player())
            m_content->player()->cancel();
        m_content->attach(this);
    }
}

AnimationPlayer::~AnimationPlayer()
{
    if (m_content)
        m_content->detach();
    if (m_timeline)
        m_timeline->playerDestroyed(this);
}

double AnimationPlayer::sourceEnd() const
{
    return m_content ? m_content->endTime() : 0;
}

bool AnimationPlayer::limited(double currentTime) const
{
    return (m_playbackRate < 0 && currentTime <= 0) || (m_playbackRate > 0 && currentTime >= sourceEnd());
}

double AnimationPlayer::currentTimeWithoutLag() const
{
    if (isNull(m_startTime) || !m_timeline)
        return 0;
    return (m_timeline->effectiveTime() - m_startTime) * m_playbackRate;
}

double AnimationPlayer::currentTimeWithLag() const
{
    ASSERT(!m_held);
    double time = currentTimeWithoutLag();
    return std::isinf(time) ? time : time - m_storedTimeLag;
}

void AnimationPlayer::updateTimingState(double newCurrentTime)
{
    ASSERT(!isNull(newCurrentTime));
    bool oldHeld = m_held;
    m_held = m_paused || !m_playbackRate || limited(newCurrentTime);
    if (m_held) {
        if (!oldHeld || m_holdTime != newCurrentTime)
            setOutdated();
        m_holdTime = newCurrentTime;
        m_storedTimeLag = nullValue();
    } else {
        m_holdTime = nullValue();
        m_storedTimeLag = currentTimeWithoutLag() - newCurrentTime;
        setOutdated();
    }
}

void AnimationPlayer::updateCurrentTimingState()
{
    if (m_held) {
        updateTimingState(m_holdTime);
        return;
    }
    if (!limited(currentTimeWithLag()))
        return;
    m_held = true;
    m_holdTime = m_playbackRate < 0 ? 0 : sourceEnd();
    m_storedTimeLag = nullValue();
}

double AnimationPlayer::currentTime()
{
    updateCurrentTimingState();
    if (m_held)
        return m_holdTime;
    return currentTimeWithLag();
}

void AnimationPlayer::setCurrentTime(double newCurrentTime)
{
    if (!std::isfinite(newCurrentTime))
        return;
    updateTimingState(newCurrentTime);
}

void AnimationPlayer::setStartTime(double newStartTime)
{
    if (!std::isfinite(newStartTime))
        return;
    updateCurrentTimingState(); // Update the value of held
    m_startTime = newStartTime;
    m_sortInfo.m_startTime = newStartTime;
    if (m_held)
        return;
    updateCurrentTimingState();
    setOutdated();
}

void AnimationPlayer::setSource(TimedItem* newSource)
{
    if (m_content == newSource)
        return;
    double storedCurrentTime = currentTime();
    if (m_content)
        m_content->detach();
    m_content = newSource;
    if (newSource) {
        // FIXME: This logic needs to be updated once groups are implemented
        if (newSource->player())
            newSource->player()->cancel();
        newSource->attach(this);
    }
    updateTimingState(storedCurrentTime);
}

void AnimationPlayer::pause()
{
    if (m_paused)
        return;
    m_paused = true;
    updateTimingState(currentTime());
    // FIXME: resume compositor animation rather than pull back to main-thread
    cancelAnimationOnCompositor();
}

void AnimationPlayer::unpause()
{
    if (!m_paused)
        return;
    m_paused = false;
    updateTimingState(currentTime());
}

void AnimationPlayer::play()
{
    unpause();
    if (!m_content)
        return;
    double currentTime = this->currentTime();
    if (m_playbackRate > 0 && (currentTime < 0 || currentTime >= sourceEnd()))
        setCurrentTime(0);
    else if (m_playbackRate < 0 && (currentTime <= 0 || currentTime > sourceEnd()))
        setCurrentTime(sourceEnd());
}

void AnimationPlayer::reverse()
{
    if (!m_playbackRate)
        return;
    if (m_content) {
        if (m_playbackRate > 0 && currentTime() > sourceEnd())
            setCurrentTime(sourceEnd());
        else if (m_playbackRate < 0 && currentTime() < 0)
            setCurrentTime(0);
    }
    setPlaybackRate(-m_playbackRate);
    unpause();
}

void AnimationPlayer::finish(ExceptionState& exceptionState)
{
    if (!m_playbackRate)
        return;
    if (m_playbackRate < 0) {
        setCurrentTime(0);
    } else {
        if (sourceEnd() == std::numeric_limits<double>::infinity()) {
            exceptionState.throwDOMException(InvalidStateError, "AnimationPlayer has source content whose end time is infinity.");
            return;
        }
        setCurrentTime(sourceEnd());
    }
    ASSERT(finished());
}

void AnimationPlayer::setPlaybackRate(double playbackRate)
{
    if (!std::isfinite(playbackRate))
        return;
    double storedCurrentTime = currentTime();
    m_playbackRate = playbackRate;
    updateTimingState(storedCurrentTime);
}

void AnimationPlayer::setOutdated()
{
    m_outdated = true;
    if (m_timeline)
        m_timeline->setOutdatedAnimationPlayer(this);
}

bool AnimationPlayer::maybeStartAnimationOnCompositor()
{
    // FIXME: Support starting compositor animations that have a fixed
    // start time.
    ASSERT(!hasStartTime());
    if (!m_content || !m_content->isAnimation() || paused())
        return false;

    return toAnimation(m_content.get())->maybeStartAnimationOnCompositor();
}

bool AnimationPlayer::hasActiveAnimationsOnCompositor()
{
    if (!m_content || !m_content->isAnimation())
        return false;
    return toAnimation(m_content.get())->hasActiveAnimationsOnCompositor();
}

void AnimationPlayer::cancelAnimationOnCompositor()
{
    if (hasActiveAnimationsOnCompositor())
        toAnimation(m_content.get())->cancelAnimationOnCompositor();
}

bool AnimationPlayer::update()
{
    m_outdated = false;

    if (!m_timeline || !m_content)
        return false;

    double inheritedTime = isNull(m_timeline->currentTime()) ? nullValue() : currentTime();
    m_content->updateInheritedTime(inheritedTime);

    ASSERT(!m_outdated);
    return m_content->isCurrent() || m_content->isInEffect();
}

double AnimationPlayer::timeToEffectChange()
{
    ASSERT(!m_outdated);
    if (!m_content || !m_playbackRate)
        return std::numeric_limits<double>::infinity();
    if (m_playbackRate > 0)
        return m_content->timeToForwardsEffectChange() / m_playbackRate;
    return m_content->timeToReverseEffectChange() / std::abs(m_playbackRate);
}

void AnimationPlayer::cancel()
{
    if (!m_content)
        return;

    ASSERT(m_content->player() == this);
    m_content->detach();
    m_content = nullptr;
}

bool AnimationPlayer::SortInfo::operator<(const SortInfo& other) const
{
    ASSERT(!std::isnan(m_startTime) && !std::isnan(other.m_startTime));
    if (m_startTime < other.m_startTime)
        return true;
    if (m_startTime > other.m_startTime)
        return false;
    return m_sequenceNumber < other.m_sequenceNumber;
}

void AnimationPlayer::pauseForTesting(double pauseTime)
{
    RELEASE_ASSERT(!paused());
    updateTimingState(pauseTime);
    if (!m_isPausedForTesting && hasActiveAnimationsOnCompositor())
        toAnimation(m_content.get())->pauseAnimationForTestingOnCompositor(currentTime());
    m_isPausedForTesting = true;
    pause();
}

} // namespace
