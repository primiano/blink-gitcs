/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

#if ENABLE(3D_CANVAS)

#include "WebGLArrayBuffer.h"
#include "WebGLUnsignedByteArray.h"

namespace WebCore {

PassRefPtr<WebGLUnsignedByteArray> WebGLUnsignedByteArray::create(unsigned length)
{
    RefPtr<WebGLArrayBuffer> buffer = WebGLArrayBuffer::create(length, sizeof(unsigned char));
    return create(buffer, 0, length);
}

PassRefPtr<WebGLUnsignedByteArray> WebGLUnsignedByteArray::create(unsigned char* array, unsigned length)
{
    RefPtr<WebGLUnsignedByteArray> a = WebGLUnsignedByteArray::create(length);
    for (unsigned i = 0; i < length; ++i)
        a->set(i, array[i]);
    return a;
}

PassRefPtr<WebGLUnsignedByteArray> WebGLUnsignedByteArray::create(PassRefPtr<WebGLArrayBuffer> buffer,
                                                                  unsigned byteOffset,
                                                                  unsigned length)
{
    RefPtr<WebGLArrayBuffer> buf(buffer);
    if (!verifySubRange<unsigned char>(buf, byteOffset, length))
        return 0;

    return adoptRef(new WebGLUnsignedByteArray(buf, byteOffset, length));
}

WebGLUnsignedByteArray::WebGLUnsignedByteArray(PassRefPtr<WebGLArrayBuffer> buffer, unsigned byteOffset, unsigned length)
        : WebGLArray(buffer, byteOffset)
        , m_size(length)
{
}

unsigned WebGLUnsignedByteArray::length() const {
    return m_size;
}

unsigned WebGLUnsignedByteArray::byteLength() const {
    return m_size * sizeof(unsigned char);
}

PassRefPtr<WebGLArray> WebGLUnsignedByteArray::slice(int start, int end)
{
    unsigned offset, length;
    calculateOffsetAndLength(start, end, m_size, &offset, &length);
    clampOffsetAndNumElements<unsigned char>(buffer(), m_byteOffset, &offset, &length);
    return create(buffer(), offset, length);
}

void WebGLUnsignedByteArray::set(WebGLUnsignedByteArray* array, unsigned offset, ExceptionCode& ec) {
    setImpl(array, offset * sizeof(unsigned char), ec);
}

}

#endif // ENABLE(3D_CANVAS)
