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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "RunLoop.h"
#import "WebProcess.h"
#import "WebSystemInterface.h"
#import <runtime/InitializeThreading.h>
#import <servers/bootstrap.h>
#import <signal.h>
#import <unistd.h>
#import <wtf/Threading.h>

// FIXME: We should be doing this another way.
extern "C" kern_return_t bootstrap_look_up2(mach_port_t, const name_t, mach_port_t*, pid_t, uint64_t);

#define SHOW_CRASH_REPORTER 0

using namespace WebKit;

int main(int argc, char** argv)
{
    mach_port_t serverPort;
    kern_return_t kr = bootstrap_look_up2(bootstrap_port, "com.apple.WebKit.WebProcess", &serverPort, getppid(), /* BOOTSTRAP_PER_PID_SERVICE */ 1);
    if (kr) {
        printf("bootstrap_look_up2 result: %x", kr);
        return 2;
    }
    
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

#if !SHOW_CRASH_REPORTER
    // Installs signal handlers that exit on a crash so that CrashReporter does not show up.
    signal(SIGILL, _exit);
    signal(SIGFPE, _exit);
    signal(SIGBUS, _exit);
    signal(SIGSEGV, _exit);
#endif

    InitWebCoreSystemInterface();
    JSC::initializeThreading();
    WTF::initializeMainThread();
    RunLoop::initializeMainRunLoop();

    // Create the connection.
    WebProcess::shared().initialize(serverPort, RunLoop::main());
    
    [pool drain];

     // Initialize AppKit.
    [NSApplication sharedApplication];

    RunLoop::run();

    // FIXME: Do more cleanup here.

    return 0;
}
