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

#import <WebKit/WebAuthenticationPanel.h>

#import <Foundation/NSURLAuthenticationChallenge.h>
#import <Foundation/NSURLProtectionSpace.h>
#import <Foundation/NSURLCredential.h>
#import <WebKit/WebAssertions.h>
#import <WebKit/WebLocalizableStrings.h>
#import <WebKit/WebNSURLExtras.h>

#import <WebKit/WebNSControlExtras.h>

#define WebAuthenticationPanelNibName @"WebAuthenticationPanel"

@implementation WebAuthenticationPanel

-(id)initWithCallback:(id)cb selector:(SEL)sel
{
    self = [self init];
    if (self != nil) {
        callback = [cb retain];
        selector = sel;
    }
    return self;
}


- (void)dealloc
{
    [panel release];

    [callback release];
    
    [super dealloc];
}

// IB actions

- (IBAction)cancel:(id)sender
{
    // This is required because the body of this method is going to
    // remove all of the panel's remaining refs, which can cause a
    // crash later when finishing button hit tracking.  So we make
    // sure it lives on a bit longer.
    [[panel retain] autorelease];
    
    // This is required as a workaround for AppKit issue 4118422
    [[self retain] autorelease];

    [panel close];
    if (usingSheet) {
	[[NSApplication sharedApplication] endSheet:panel returnCode:1];
    } else {
	[[NSApplication sharedApplication] stopModalWithCode:1];
    }
}

- (IBAction)logIn:(id)sender
{
    // This is required because the body of this method is going to
    // remove all of the panel's remaining refs, which can cause a
    // crash later when finishing button hit tracking.  So we make
    // sure it lives on a bit longer.
    [[panel retain] autorelease];

    [panel close];
    if (usingSheet) {
	[[NSApplication sharedApplication] endSheet:panel returnCode:0];
    } else {
	[[NSApplication sharedApplication] stopModalWithCode:0];
    }
}

- (BOOL)loadNib
{
    if (!nibLoaded) {
        if ([NSBundle loadNibNamed:WebAuthenticationPanelNibName owner:self]) {
            nibLoaded = YES;
            [imageView setImage:[NSImage imageNamed:@"NSApplicationIcon"]];
        } else {
            ERROR("couldn't load nib named '%@'", WebAuthenticationPanelNibName);
            return FALSE;
        }
    }
    return TRUE;
}

// Methods related to displaying the panel

-(void)setUpForChallenge:(NSURLAuthenticationChallenge *)chall
{
    [self loadNib];

    NSURLProtectionSpace *space = [chall protectionSpace];

    NSString *host;
    if ([space port] == 0) {
	host = [space host];
    } else {
	host = [NSString stringWithFormat:@"%@:%u", [space host], [space port]];
    }

    NSString *realm = [space realm];
    NSString *message;

    if ([chall previousFailureCount] == 0) {
        if ([space isProxy]) {
            message = [NSString stringWithFormat:UI_STRING("To view this page, you need to log in to the %@ proxy server %@.",
                                                           "prompt string in authentication panel"),
                [space proxyType], host];
        } else {
            message = [NSString stringWithFormat:UI_STRING("To view this page, you need to log in to area “%@” on %@.",
                                                           "prompt string in authentication panel"),
                realm, host];
        }
    } else {
        if ([space isProxy]) {
            message = [NSString stringWithFormat:UI_STRING("The name or password entered for the %@ proxy server %@ was incorrect. Please try again.",
                                                           "prompt string in authentication panel"),
				[space proxyType], host];
        } else {
            message = [NSString stringWithFormat:UI_STRING("The name or password entered for area “%@” on %@ was incorrect. Please try again.",
                                                           "prompt string in authentication panel"),
                realm, host];
        }
    }

    [mainLabel setStringValue:message];
    [mainLabel sizeToFitAndAdjustWindowHeight];

    if ([space receivesCredentialSecurely]) {
        [smallLabel setStringValue:
            UI_STRING("Your log-in information will be sent securely.",
                "message in authentication panel")];
    } else {
        [smallLabel setStringValue:
            UI_STRING("Your password will be sent in the clear.",
                "message in authentication panel")];
    }

    if ([[chall proposedCredential] user] != nil) {
        [username setStringValue:[[chall proposedCredential] user]];
        [panel setInitialFirstResponder:password];
    } else {
        [username setStringValue:@""];
        [password setStringValue:@""];
        [panel setInitialFirstResponder:username];
    }
}

- (void)runAsModalDialogWithChallenge:(NSURLAuthenticationChallenge *)chall
{
    [self setUpForChallenge:chall];
    usingSheet = FALSE;
    NSURLCredential *credential = nil;

    if ([[NSApplication sharedApplication] runModalForWindow:panel] == 0) {
        credential = [NSURLCredential credentialWithUser:[username stringValue] password:[password stringValue] persistence:([remember state] == NSOnState) ? NSURLCredentialPersistencePermanent : NSURLCredentialPersistenceForSession];
    }

    [callback performSelector:selector withObject:chall withObject:credential];
}

- (void)runAsSheetOnWindow:(NSWindow *)window withChallenge:(NSURLAuthenticationChallenge *)chall
{
    ASSERT(!usingSheet);

    [self setUpForChallenge:chall];

    usingSheet = TRUE;
    challenge = [chall retain];
    
    [[NSApplication sharedApplication] beginSheet:panel modalForWindow:window modalDelegate:self didEndSelector:@selector(sheetDidEnd:returnCode:contextInfo:) contextInfo:NULL];
}

- (void)sheetDidEnd:(NSWindow *)sheet returnCode:(int)returnCode contextInfo:(void  *)contextInfo
{
    NSURLCredential *credential = nil;
    NSURLAuthenticationChallenge *chall;

    ASSERT(usingSheet);
    ASSERT(challenge != nil);

    if (returnCode == 0) {
        credential = [NSURLCredential credentialWithUser:[username stringValue] password:[password stringValue] persistence:([remember state] == NSOnState) ? NSURLCredentialPersistencePermanent : NSURLCredentialPersistenceForSession];
    }

    // We take this tricky approach to nilling out and releasing the challenge
    // because the callback below might remove our last ref.
    chall = challenge;
    challenge = nil;
    [callback performSelector:selector withObject:chall withObject:credential];
    [chall release];
}

@end

@implementation NonBlockingPanel

- (BOOL)_blocksActionWhenModal:(SEL)theAction
{
    // This override of a private AppKit method allows the user to quit when a login dialog
    // is onscreen, which is nice in general but in particular prevents pathological cases
    // like 3744583 from requiring a Force Quit.
    //
    // It would be nice to allow closing the individual window as well as quitting the app when
    // a login sheet is up, but this _blocksActionWhenModal: mechanism doesn't support that.
    // This override matches those in NSOpenPanel and NSToolbarConfigPanel.
    if (theAction == @selector(terminate:)) {
        return NO;
    }
    return YES;
}

@end
