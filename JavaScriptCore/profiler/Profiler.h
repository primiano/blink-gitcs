/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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

#ifndef Profiler_h
#define Profiler_h

#include "Profile.h"
#include <wtf/RefPtr.h>

namespace KJS {

    class ExecState;
    class FunctionImp;
    class JSObject;

    class Profiler {
    public:
        static Profiler* profiler();
        static void debugLog(UString);

        void startProfiling(unsigned pageGroupIdentifier, const UString&);
        void stopProfiling();

        void willExecute(ExecState*, JSObject* calledFunction);
        void willExecute(ExecState*, const UString& sourceURL, int startingLineNumber);
        void didExecute(ExecState*, JSObject* calledFunction);
        void didExecute(ExecState*, const UString& sourceURL, int startingLineNumber);

        const Vector<RefPtr<Profile> >& allProfiles() { return m_allProfiles; };
        void clearProfiles() { if (!m_profiling) m_allProfiles.clear(); };

        Profile* currentProfile() const { return m_currentProfile.get(); }

    private:
        Profiler()
            : m_profiling(false)
            , m_pageGroupIdentifier(0)
        {
        }

        bool m_profiling;
        unsigned m_pageGroupIdentifier;

        RefPtr<Profile> m_currentProfile;
        Vector<RefPtr<Profile> > m_allProfiles;
    };

} // namespace KJS

#endif // Profiler_h
