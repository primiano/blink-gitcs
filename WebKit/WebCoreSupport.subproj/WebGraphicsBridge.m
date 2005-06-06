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

#import "WebGraphicsBridge.h"

#import "WebAssertions.h"

#import "WebImageRenderer.h"
#import <WebKitSystemInterface.h>

@implementation WebGraphicsBridge

+ (void)createSharedBridge
{
    if (![self sharedBridge]) {
        [[[self alloc] init] release];
    }
    ASSERT([[self sharedBridge] isKindOfClass:self]);
}

+ (WebGraphicsBridge *)sharedBridge;
{
    return (WebGraphicsBridge *)[super sharedBridge];
}

- (void)setFocusRingStyle:(NSFocusRingPlacement)placement radius:(int)radius color:(NSColor *)color
{
	WKSetFocusRingStyle(placement, radius, color);
}

// Dashboard wants to set the drag image during dragging, but Cocoa does not allow this.  Instead we drop
// down to the CG API.  Converting an NSImage to a CGImageSpec is copied from NSDragManager.
- (void)setDraggingImage:(NSImage *)image at:(NSPoint)offset
{
	WKSetDragImage(image, offset);
	
    // Hack:  We must post an event to wake up the NSDragManager, which is sitting in a nextEvent call
    // up the stack from us because the CF drag manager is too lame to use the RunLoop by itself.  This
    // is the most innocuous event, per Kristen.
    
    NSEvent *ev = [NSEvent mouseEventWithType:NSMouseMoved location:NSZeroPoint modifierFlags:0 timestamp:0 windowNumber:0 context:nil eventNumber:0 clickCount:0 pressure:0];
    [NSApp postEvent:ev atStart:YES];
}

- (void)setAdditionalPatternPhase:(NSPoint)phase
{
    _phase = phase;
}

- (NSPoint)additionalPatternPhase {
    return _phase;
}


- (CGColorSpaceRef)createRGBColorSpace
{
    return WebCGColorSpaceCreateRGB();
}

- (CGColorSpaceRef)createGrayColorSpace
{
    return WebCGColorSpaceCreateGray();
}

- (CGColorSpaceRef)createCMYKColorSpace
{
    return WebCGColorSpaceCreateCMYK();
}

@end
