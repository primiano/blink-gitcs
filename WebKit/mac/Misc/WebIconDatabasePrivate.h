/*
 * Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007, 2009 Apple Inc. All rights reserved.
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

#import <WebKit/WebIconDatabase.h>

// FIXME: Some of the following is not API and should be moved
// either inside WebIconDatabase.mm, or to WebIconDatabaseInternal.h.

// Sent when all icons are removed from the database. The object of the notification is 
// the icon database. There is no userInfo. Clients should react by removing any cached
// icon images from the user interface. Clients need not and should not call 
// releaseIconForURL: in response to this notification.
extern NSString *WebIconDatabaseDidRemoveAllIconsNotification;

// Key to store the path to look for old style icons in to convert to the new icon db
extern NSString *WebIconDatabaseImportDirectoryDefaultsKey;

@interface WebIconDatabase (WebPendingPublic)

/*!
   @method isEnabled
   @discussion Returns true if the icon database is currently enabled, or false if it
   is disabled.
*/
- (BOOL)isEnabled;

/*!
   @method setEnabled:
   @discussion Enables or disables the icon database based on the flag passed in.
   @param flag Pass true to enable the icon database, or false to disable it.
*/
- (void)setEnabled:(BOOL)flag;

/*!
   @method removeAllIcons
   @discussion Causes the icon database to delete all of the images that it has stored,
   and to send out the notification WebIconDatabaseDidRemoveAllIconsNotification.
*/
- (void)removeAllIcons;

@end

@interface WebIconDatabase (WebPrivate)

+ (void)_checkIntegrityBeforeOpening;

@end

