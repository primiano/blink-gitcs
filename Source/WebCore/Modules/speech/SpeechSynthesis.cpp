/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
#include "SpeechSynthesis.h"

#if ENABLE(SPEECH_SYNTHESIS)

#include "PlatformSpeechSynthesis.h"
#include "PlatformSpeechSynthesisVoice.h"
#include "SpeechSynthesisEvent.h"
#include "SpeechSynthesisUtterance.h"
#include <WTF/CurrentTime.h>

namespace WebCore {
    
PassRefPtr<SpeechSynthesis> SpeechSynthesis::create()
{
    return adoptRef(new SpeechSynthesis());
}
    
SpeechSynthesis::SpeechSynthesis()
    : m_platformSpeechSynthesizer(PlatformSpeechSynthesizer::create(this))
    , m_currentSpeechUtterance(0)
{
}
    
void SpeechSynthesis::setPlatformSynthesizer(PassOwnPtr<PlatformSpeechSynthesizer> synthesizer)
{
    m_platformSpeechSynthesizer = synthesizer;
}
    
void SpeechSynthesis::voicesDidChange()
{
    m_voiceList.clear();
}
    
const Vector<RefPtr<SpeechSynthesisVoice> >& SpeechSynthesis::getVoices()
{
    if (m_voiceList.size())
        return m_voiceList;
    
    // If the voiceList is empty, that's the cue to get the voices from the platform again.
    const Vector<RefPtr<PlatformSpeechSynthesisVoice> >& platformVoices = m_platformSpeechSynthesizer->voiceList();
    size_t voiceCount = platformVoices.size();
    for (size_t k = 0; k < voiceCount; k++)
        m_voiceList.append(SpeechSynthesisVoice::create(platformVoices[k]));

    return m_voiceList;
}

bool SpeechSynthesis::speaking() const
{
    // If we have a current speech utterance, then that means we're assumed to be in a speaking state.
    // This state is independent of whether the utterance happens to be paused.
    return m_currentSpeechUtterance;
}

bool SpeechSynthesis::pending() const
{
    return false;
}

bool SpeechSynthesis::paused() const
{
    return false;
}

void SpeechSynthesis::startSpeakingImmediately(SpeechSynthesisUtterance* utterance)
{
    ASSERT(!m_currentSpeechUtterance);
    utterance->setStartTime(monotonicallyIncreasingTime());
    m_currentSpeechUtterance = utterance;
    m_platformSpeechSynthesizer->speak(utterance->platformUtterance());
}

void SpeechSynthesis::speak(SpeechSynthesisUtterance* utterance)
{
    m_utteranceQueue.append(utterance);
    
    // If the queue was empty, speak this immediately and add it to the queue.
    if (m_utteranceQueue.size() == 1)
        startSpeakingImmediately(utterance);
}

void SpeechSynthesis::cancel()
{
}

void SpeechSynthesis::pause()
{
}

void SpeechSynthesis::resume()
{
}

void SpeechSynthesis::fireEvent(const AtomicString& type, SpeechSynthesisUtterance* utterance, unsigned long charIndex, const String& name)
{
    utterance->dispatchEvent(SpeechSynthesisEvent::create(type, charIndex, (currentTime() - utterance->startTime()), name));
}
    
void SpeechSynthesis::handleSpeakingCompleted(SpeechSynthesisUtterance* utterance, bool errorOccurred)
{
    ASSERT(utterance);
    ASSERT(m_currentSpeechUtterance);
    m_currentSpeechUtterance = 0;

    fireEvent(errorOccurred ? eventNames().errorEvent : eventNames().endEvent, utterance, 0, String());

    RefPtr<SpeechSynthesisUtterance> firstUtterance = m_utteranceQueue.first();
    ASSERT(firstUtterance == utterance);
    if (firstUtterance == utterance)
        m_utteranceQueue.removeFirst();

    // Start the next job if there is one pending.
    if (!m_utteranceQueue.isEmpty())
        startSpeakingImmediately(m_utteranceQueue.first().get());
}

void SpeechSynthesis::didStartSpeaking(const PlatformSpeechSynthesisUtterance* utterance)
{
    fireEvent(eventNames().startEvent, static_cast<SpeechSynthesisUtterance*>(utterance->client()), 0, String());
}

void SpeechSynthesis::didFinishSpeaking(const PlatformSpeechSynthesisUtterance* utterance)
{
    handleSpeakingCompleted(static_cast<SpeechSynthesisUtterance*>(utterance->client()), false);
}
    
void SpeechSynthesis::speakingErrorOccurred(const PlatformSpeechSynthesisUtterance* utterance)
{
    handleSpeakingCompleted(static_cast<SpeechSynthesisUtterance*>(utterance->client()), true);
}
    
} // namespace WebCore

#endif // ENABLE(INPUT_SPEECH)
