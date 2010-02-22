/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
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

#ifndef WebFontRenderStyle_h
#define WebFontRenderStyle_h

#include "../WebCommon.h"

namespace WebCore { struct FontRenderStyle; }

namespace WebKit {

struct WebFontRenderStyle {
    // Each of the use* members below can take one of three values:
    //   0: off
    //   1: on
    //   2: no preference expressed
    char useBitmaps; // use embedded bitmap strike if possible
    char useAutoHint; // use 'auto' hinting (FreeType specific)
    char useHinting; // hint glyphs to the pixel grid
    char hintStyle; // level of hinting, 0..3
    char useAntiAlias; // antialias glyph shapes
    char useSubpixel; // use subpixel antialias

#ifdef WEBKIT_IMPLEMENTATION
    // Translates the members of this struct to a FontRenderStyle
    void toFontRenderStyle(WebCore::FontRenderStyle*);
#endif

    void setDefaults();
};

} // namespace WebKit

#endif // WebFontRenderStyle_h
