/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
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
#import "config.h"
#import "ThreadCheck.h"

namespace WebCore {

void _WebCoreThreadViolationCheck(const char* function)
{
    static bool fetchDefault = true;
    static bool performThreadCheck = true;
    static bool threadViolationIsException = true;
    if (fetchDefault) {
        NSString *threadCheckLevel = [[NSUserDefaults standardUserDefaults] objectForKey:@"WebCoreThreadCheck"];
        if ([threadCheckLevel isEqualToString:@"None"])
            performThreadCheck = false;
        else if ([threadCheckLevel isEqualToString:@"Exception"]) {
            performThreadCheck = true;
            threadViolationIsException = true;
        } else if ([threadCheckLevel isEqualToString:@"Log"]) {
            performThreadCheck = true;
            threadViolationIsException = false;
        }
        fetchDefault = false;
    }
    
    if (!performThreadCheck)
        return;
        
    if (pthread_main_np())
        return;
        
    WebCoreReportThreadViolation(function, threadViolationIsException);
}

} // namespace WebCore

// Split out the actual reporting of the thread violation to make it easier to set a breakpoint
void WebCoreReportThreadViolation(const char* function, bool threadViolationIsException)
{
    if (threadViolationIsException)
        [NSException raise:@"WebKitThreadingException" format:@"%s was called from a secondary thread", function];
    else
        NSLog(@"WebKit Threading Violation - %s called from secondary thread", function);
}
