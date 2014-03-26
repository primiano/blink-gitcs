// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"
#include "public/web/win/WebFontRendering.h"

#include "platform/fonts/FontCache.h"

namespace blink {

// static
void WebFontRendering::setUseDirectWrite(bool useDirectWrite)
{
    WebCore::FontCache::setUseDirectWrite(useDirectWrite);
}

// static
void WebFontRendering::setUseSubpixelPositioning(bool useSubpixelPositioning)
{
    WebCore::FontCache::setUseSubpixelPositioning(useSubpixelPositioning);
}

} // namespace blink
