/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#ifndef RenderFrameBase_h
#define RenderFrameBase_h

#include "RenderPartObject.h"

namespace WebCore {

// Base class for RenderFrame and RenderIFrame
class RenderFrameBase : public RenderPart {
protected:
    RenderFrameBase(Element*);

public:
    void layoutWithFlattening(bool fixedWidth, bool fixedHeight);

private:
    virtual bool isRenderFrameBase() const { return true; }
};

inline RenderFrameBase* toRenderFrameBase(RenderObject* object)
{
    ASSERT(!object || object->isRenderFrameBase());
    return static_cast<RenderFrameBase*>(object);
}

inline const RenderFrameBase* toRenderFrameBase(const RenderObject* object)
{
    ASSERT(!object || object->isRenderFrameBase());
    return static_cast<const RenderFrameBase*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderFrameBase(const RenderFrameBase*);


} // namespace WebCore

#endif // RenderFrameBase_h
