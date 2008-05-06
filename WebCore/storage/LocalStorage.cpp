/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "LocalStorage.h"

#include "EventNames.h"
#include "Frame.h"
#include "FrameTree.h"
#include "LocalStorageArea.h"
#include "Page.h"
#include "PageGroup.h"
#include "StorageArea.h"

namespace WebCore {


LocalStorage::LocalStorage(PageGroup* group, const String& path)
    : m_group(group)
    , m_path(path.copy())
{
    ASSERT(m_group);
}

PassRefPtr<StorageArea> LocalStorage::storageArea(Frame* sourceFrame, SecurityOrigin* origin)
{
    // FIXME: If the security origin in question has never had a storage area established,
    // we need to ask a client call if establishing it is okay.  If the client denies the request,
    // this method will return null.
    // The sourceFrame argument exists for the purpose of asking a client.

    RefPtr<StorageArea> storageArea;
    if (storageArea = m_storageAreaMap.get(origin))
        return storageArea.release();
        
    storageArea = LocalStorageArea::create(origin, this);
    m_storageAreaMap.set(origin, storageArea);
    return storageArea.release();
}

void LocalStorage::close()
{
    // FIXME: Make sure all pending writes complete and terminate the background thread
}

} // namespace WebCore
