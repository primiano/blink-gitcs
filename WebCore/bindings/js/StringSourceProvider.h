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

#ifndef StringSourceProvider_h
#define StringSourceProvider_h

#include <kjs/SourceCode.h>

namespace WebCore {

    class StringSourceProvider : public JSC::SourceProvider {
    public:
        static PassRefPtr<StringSourceProvider> create(const String& source, const String& url) { return adoptRef(new StringSourceProvider(source, url)); }

        JSC::UString getRange(int start, int end) const { return JSC::UString(m_source.characters() + start, end - start); }
        const UChar* data() const { return m_source.characters(); }
        int length() const { return m_source.length(); }

    private:
        StringSourceProvider(const String& source, const String& url)
            : SourceProvider(url)
            , m_source(source)
        {
        }
        
        String m_source;
    };

    inline JSC::SourceCode makeSource(const String& source, const JSC::UString& url = JSC::UString(), int firstLine = 1)
    {
        return JSC::SourceCode(StringSourceProvider::create(source, url), firstLine);
    }
}

#endif
