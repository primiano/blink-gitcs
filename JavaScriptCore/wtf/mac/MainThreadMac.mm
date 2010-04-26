/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
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
 
#import "config.h"
#import "MainThread.h"

#import <CoreFoundation/CoreFoundation.h>
#import <Foundation/NSThread.h>
#import <stdio.h>
#import <wtf/Assertions.h>
#import <wtf/Threading.h>

@interface WTFMainThreadCaller : NSObject {
}
- (void)call;
@end

@implementation WTFMainThreadCaller

- (void)call
{
    WTF::dispatchFunctionsFromMainThread();
}

@end // implementation WTFMainThreadCaller

namespace WTF {

static WTFMainThreadCaller* staticMainThreadCaller;
static bool isTimerPosted; // This is only accessed on the 'main' thread.
static bool mainThreadEstablishedAsPthreadMain;
static pthread_t mainThreadPthread;
static NSThread* mainThreadNSThread;

void initializeMainThreadPlatform()
{
#if !defined(BUILDING_ON_TIGER)
    ASSERT(!staticMainThreadCaller);
    staticMainThreadCaller = [[WTFMainThreadCaller alloc] init];

    mainThreadEstablishedAsPthreadMain = false;
    mainThreadPthread = pthread_self();
    mainThreadNSThread = [[NSThread currentThread] retain];
#else
    ASSERT_NOT_REACHED();
#endif
}

void initializeMainThreadToProcessMainThreadPlatform()
{
    if (!pthread_main_np())
        NSLog(@"WebKit Threading Violation - initial use of WebKit from a secondary thread.");

    ASSERT(!staticMainThreadCaller);
    staticMainThreadCaller = [[WTFMainThreadCaller alloc] init];

    mainThreadEstablishedAsPthreadMain = true;
    mainThreadPthread = 0;
    mainThreadNSThread = nil;
}

static void timerFired(CFRunLoopTimerRef timer, void*)
{
    CFRelease(timer);
    isTimerPosted = false;
    WTF::dispatchFunctionsFromMainThread();
}

static void postTimer()
{
    ASSERT(isMainThread());

    if (isTimerPosted)
        return;

    isTimerPosted = true;
    CFRunLoopAddTimer(CFRunLoopGetCurrent(), CFRunLoopTimerCreate(0, 0, 0, 0, 0, timerFired, 0), kCFRunLoopCommonModes);
}

void scheduleDispatchFunctionsOnMainThread()
{
    ASSERT(staticMainThreadCaller);

    if (isMainThread()) {
        postTimer();
        return;
    }

    if (mainThreadEstablishedAsPthreadMain) {
        ASSERT(!mainThreadNSThread);
        [staticMainThreadCaller performSelectorOnMainThread:@selector(call) withObject:nil waitUntilDone:NO];
        return;
    }

#if !defined(BUILDING_ON_TIGER)
    ASSERT(mainThreadNSThread);
    [staticMainThreadCaller performSelector:@selector(call) onThread:mainThreadNSThread withObject:nil waitUntilDone:NO];
#else
    ASSERT_NOT_REACHED();
#endif
}

bool isMainThread()
{
    if (mainThreadEstablishedAsPthreadMain) {
        ASSERT(!mainThreadPthread);
        return pthread_main_np();
    }

#if !defined(BUILDING_ON_TIGER)
    ASSERT(mainThreadPthread);
    return pthread_equal(pthread_self(), mainThreadPthread);
#else
    ASSERT_NOT_REACHED();
#endif
}

} // namespace WTF
