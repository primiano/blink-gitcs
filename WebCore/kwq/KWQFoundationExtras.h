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

#import <Foundation/Foundation.h>

// nil-checked CFRetain/CFRelease covers for Objective-C ids

// Use CFRetain, CFRelease, KWQRetain, or KWQRelease instead of
// -[NSObject retain] and -[NSObject release] if you want to store
// a pointer to an Objective-C object into memory that won't
// be scanned for GC, like a C++ object.

static inline id KWQRetain(id obj)
{
    if (obj) CFRetain(obj);
    return obj;
}

static inline void KWQRelease(id obj)
{
    if (obj) CFRelease(obj);
}

// As if CF and Foundation had logically separate reference counts,
// this function first increments the CF retain count, and then
// decrements the NS retain count. This is needed to handle cases where
// -retain/-release aren't equivalent to CFRetain/KWQRelease, such as
// when GC is used.

// Use KWQRetainNSRelease after allocating and initializing a NSObject
// if you want to store a pointer to that object into memory that won't
// be scanned for GC, like a C++ object.

static inline id KWQRetainNSRelease(id obj)
{
    KWQRetain(obj);
    [obj release];
    return obj;
}

// Use KWQCFAutorelease to return an object made by a CoreFoundation
// "create" or "copy" function as an autoreleased and garbage collected
// object. CF objects need to be "made collectable" for autorelease to work
// properly under GC.

static inline id KWQCFAutorelease(CFTypeRef obj)
{
#if !BUILDING_ON_PANTHER
    CFMakeCollectable(obj);
#endif
    [(id)obj autorelease];
    return (id)obj;
}

// Definitions for GC-specific methods for Panther.
// The finalize method simply won't be called.

#if BUILDING_ON_PANTHER

@interface NSObject (KWQFoundationExtras)
- (void)finalize;
@end

#endif
