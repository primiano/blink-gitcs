/*
 * Copyright (C) 2004 Apple Computer, Inc.  All rights reserved.
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

#ifndef QTextStream_H
#define QTextStream_H

#include <wtf/Vector.h>

class DeprecatedCString;
class DeprecatedString;
class QChar;
class QTextStream;

namespace WebCore {
    class String;
}

typedef QTextStream &(*QTextStreamManipulator)(QTextStream&);

QTextStream &endl(QTextStream& stream);

class QTextStream {
public:
    QTextStream(DeprecatedString*);

    QTextStream& operator<<(char);
    QTextStream& operator<<(const QChar&);
    QTextStream& operator<<(short);
    QTextStream& operator<<(unsigned short);
    QTextStream& operator<<(int);
    QTextStream& operator<<(unsigned);
    QTextStream& operator<<(long);
    QTextStream& operator<<(unsigned long);
    QTextStream& operator<<(float);
    QTextStream& operator<<(double);
    QTextStream& operator<<(const char*);
    QTextStream& operator<<(const WebCore::String&);
    QTextStream& operator<<(const DeprecatedString&);
    QTextStream& operator<<(const DeprecatedCString&);
    QTextStream& operator<<(void*);

    QTextStream& operator<<(const QTextStreamManipulator&);
    int precision(int);
private:
    QTextStream(const QTextStream&);
    QTextStream& operator=(const QTextStream&);

    bool m_hasByteArray;
    Vector<char> m_byteArray;
    DeprecatedString* m_string;
    int m_precision;
};

#endif
