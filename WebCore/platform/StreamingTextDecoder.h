/*
 * Copyright (C) 2004, 2006 Apple Computer, Inc.  All rights reserved.
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

#ifndef StreamingTextDecoder_H
#define StreamingTextDecoder_H

#include "TextEncoding.h"
#include <wtf/Noncopyable.h>
#include <unicode/ucnv.h>
#include <unicode/utypes.h>

namespace WebCore {

    class StreamingTextDecoder : Noncopyable {
    public:
        StreamingTextDecoder(const TextEncoding&);
        ~StreamingTextDecoder();

        DeprecatedString toUnicode(const char* chs, int len, bool flush = false);

    private:
        DeprecatedString convert(const char* chs, int len, bool flush)
            { return convert(reinterpret_cast<const unsigned char*>(chs), len, flush); }
        DeprecatedString convert(const unsigned char* chs, int len, bool flush);

        bool convertIfASCII(const unsigned char*, int len, DeprecatedString&);
        DeprecatedString convertUTF16(const unsigned char*, int len);
        DeprecatedString convertUsingICU(const unsigned char*, int len, bool flush);

        void createICUConverter();

        static void appendOmittingUnwanted(DeprecatedString&, const UChar* characters, int byteCount);

        TextEncoding m_encoding;
        bool m_littleEndian;
        bool m_atStart;
        bool m_error;
        
        unsigned m_numBufferedBytes;
        unsigned char m_bufferedBytes[16]; // bigger than any single multi-byte character
        
        // ICU decoding.
        UConverter* m_converterICU;
    };
    
} // namespace WebCore

#endif // StreamingTextDecoder_H
