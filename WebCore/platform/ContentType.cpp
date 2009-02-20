/*
 * Copyright (C) 2009 Apple Inc.  All rights reserved.
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or othematerials provided with the
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
#include "ContentType.h"

namespace WebCore {

ContentType::ContentType(const String& contentType)
    : m_type(contentType)
{
}

String ContentType::parameter(const String& parameterName) const
{
    String parameterValue;
    String strippedType = m_type.stripWhiteSpace();

    // a MIME type can have one or more "param=value" after a semi-colon, and separated from each other by semi-colons
    int semi = strippedType.find(';');
    if (semi != -1) {
        int start = strippedType.find(parameterName, semi + 1, false);
        if (start != -1) {
            start = strippedType.find('=', start + 6);
            if (start != -1) {
                int end = strippedType.find(';', start + 6);
                if (end == -1)
                    end = strippedType.length();
                parameterValue = strippedType.substring(start + 1, end - (start + 1)).stripWhiteSpace();
            }
        }
    }

    return parameterValue;
}

String ContentType::type() const
{
    String strippedType = m_type.stripWhiteSpace();

    // "type" can have parameters after a semi-colon, strip them
    int semi = strippedType.find(';');
    if (semi != -1)
        strippedType = strippedType.left(semi).stripWhiteSpace();

    return strippedType;
}

} // namespace WebCore
