/*
 * Copyright (C) 2003, 2006 Apple Computer, Inc.  All rights reserved.
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
#include "IntRect.h"

namespace WebCore {

IntRect::operator CGRect() const
{
    return CGRectMake(x(), y(), width(), height());
}

IntRect enclosingIntRect(const CGRect& rect)
{
    int l = static_cast<int>(floorf(rect.origin.x));
    int t = static_cast<int>(floorf(rect.origin.y));
    int r = static_cast<int>(ceilf(CGRectGetMaxX(rect)));
    int b = static_cast<int>(ceilf(CGRectGetMaxY(rect)));
    return IntRect(l, t, r - l, b - t);
}

#if !NSGEOMETRY_TYPES_SAME_AS_CGGEOMETRY_TYPES

IntRect::operator NSRect() const
{
    return NSMakeRect(x(), y(), width(), height());
}

IntRect enclosingIntRect(const NSRect& rect)
{
    int l = static_cast<int>(floorf(rect.origin.x));
    int t = static_cast<int>(floorf(rect.origin.y));
    int r = static_cast<int>(ceilf(NSMaxX(rect)));
    int b = static_cast<int>(ceilf(NSMaxY(rect)));
    return IntRect(l, t, r - l, b - t);
}

#endif

}
