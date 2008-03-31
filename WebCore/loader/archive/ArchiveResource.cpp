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

#include "config.h"
#include "ArchiveResource.h"

#include "SharedBuffer.h"

namespace WebCore {

PassRefPtr<ArchiveResource> ArchiveResource::create(PassRefPtr<SharedBuffer> data, const KURL& url, const ResourceResponse& response)
{
    return adoptRef(new ArchiveResource(data, url, response));
}

PassRefPtr<ArchiveResource> ArchiveResource::create(PassRefPtr<SharedBuffer> data, const KURL& url, const String& mimeType, const String& textEncoding, const String& frameName)
{
    return adoptRef(new ArchiveResource(data, url, mimeType, textEncoding, frameName));
}

PassRefPtr<ArchiveResource> ArchiveResource::create(PassRefPtr<SharedBuffer> data, const KURL& url, const String& mimeType, const String& textEncoding, const String& frameName, const ResourceResponse& resourceResponse)
{
    return adoptRef(new ArchiveResource(data, url, mimeType, textEncoding, frameName, resourceResponse));
}

ArchiveResource::ArchiveResource(PassRefPtr<SharedBuffer> data, const KURL& url, const ResourceResponse& response)
    : m_data(data)
    , m_url(url)
    , m_mimeType(response.mimeType())
    , m_textEncoding(response.textEncodingName())
    , m_response(response)
{
}

ArchiveResource::ArchiveResource(PassRefPtr<SharedBuffer> data, const KURL& url, const String& mimeType, const String& textEncoding, const String& frameName)
    : m_data(data)
    , m_url(url)
    , m_mimeType(mimeType)
    , m_textEncoding(textEncoding)
    , m_frameName(frameName)
    , m_shouldIgnoreWhenUnarchiving(false)
{
}

ArchiveResource::ArchiveResource(PassRefPtr<SharedBuffer> data, const KURL& url, const String& mimeType, const String& textEncoding, const String& frameName, const ResourceResponse& response)
    : m_data(data)
    , m_url(url)
    , m_mimeType(mimeType)
    , m_textEncoding(textEncoding)
    , m_frameName(frameName)
    , m_response(response)
    , m_shouldIgnoreWhenUnarchiving(false)
{
}

const ResourceResponse& ArchiveResource::response()
{
    if (!m_response.isNull())
        return m_response;
    
    m_response = ResourceResponse(m_url, m_mimeType, m_data ? m_data->size() : 0, m_textEncoding, String());
    return m_response;
}

}
