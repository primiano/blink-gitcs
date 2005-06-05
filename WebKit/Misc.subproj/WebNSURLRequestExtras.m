/*
 * Copyright (C) 2005 Apple Computer, Inc.  All rights reserved.
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

#import <WebKit/WebNSURLRequestExtras.h>

#import <WebKit/WebAssertions.h>
#import <WebKit/WebNSURLExtras.h>

#define WebContentType	(@"Content-Type")
#define WebUserAgent	(@"User-Agent")
#define WebReferrer	(@"Referer")

@implementation NSURLRequest (WebNSURLRequestExtras)

- (NSString *)_web_HTTPReferrer
{
    return [self valueForHTTPHeaderField:WebReferrer];
}

- (NSString *)_web_HTTPContentType
{
    return [self valueForHTTPHeaderField:WebContentType];
}

@end


@implementation NSMutableURLRequest (WebNSURLRequestExtras)

- (void)_web_setHTTPContentType:(NSString *)contentType
{
    [self setValue:contentType forHTTPHeaderField:WebContentType];
}

- (void)_web_setHTTPReferrer:(NSString *)theReferrer
{
    // Do not set the referrer to a string that refers to a file URL.
    // That is a potential security hole.
    if ([theReferrer _webkit_isFileURL]) {
        return;
    }

    // Don't allow empty Referer: headers; some servers refuse them
    if( [theReferrer length] == 0 )
        theReferrer = nil;

    [self setValue:theReferrer forHTTPHeaderField:WebReferrer];
}

- (void)_web_setHTTPUserAgent:(NSString *)theUserAgent
{
    [self setValue:theUserAgent forHTTPHeaderField:WebUserAgent];
}

@end

