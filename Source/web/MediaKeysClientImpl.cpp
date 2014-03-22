// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"
#include "MediaKeysClientImpl.h"

#include "WebFrameClient.h"
#include "WebFrameImpl.h"
#include "core/dom/Document.h"
#include "core/dom/ExecutionContext.h"
#include "public/platform/WebContentDecryptionModule.h"

using namespace WebCore;

namespace blink {

MediaKeysClientImpl::MediaKeysClientImpl()
{
}

PassOwnPtr<WebContentDecryptionModule> MediaKeysClientImpl::createContentDecryptionModule(WebCore::ExecutionContext* executionContext, const String& keySystem)
{
    Document* document = toDocument(executionContext);
    WebFrameImpl* webFrame = WebFrameImpl::fromFrame(document->frame());
    WebSecurityOrigin securityOrigin(executionContext->securityOrigin());
    return adoptPtr(webFrame->client()->createContentDecryptionModule(webFrame, securityOrigin, keySystem));
}

} // namespace blink
