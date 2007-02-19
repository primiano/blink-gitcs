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

#import "WebDefaultContextMenuDelegate.h"

#import "WebDOMOperations.h"
#import "WebDataSourcePrivate.h"
#import "WebDefaultUIDelegate.h"
#import "WebFrameBridge.h"
#import "WebFrameInternal.h"
#import "WebFrameView.h"
#import "WebHTMLViewPrivate.h"
#import "WebLocalizableStrings.h"
#import "WebNSPasteboardExtras.h"
#import "WebNSURLRequestExtras.h"
#import "WebPolicyDelegate.h"
#import "WebUIDelegate.h"
#import "WebUIDelegatePrivate.h"
#import "WebViewInternal.h"
#import <Foundation/NSURLConnection.h>
#import <Foundation/NSURLRequest.h>
#import <JavaScriptCore/Assertions.h>
#import <WebCore/Editor.h>
#import <WebCore/Frame.h>
#import <WebCore/FrameLoader.h>
#import <WebCore/KURL.h>
#import <WebCore/WebCoreFrameBridge.h>
#import <WebKit/DOM.h>
#import <WebKit/DOMPrivate.h>

@implementation WebDefaultUIDelegate (WebContextMenu)

static NSString *localizedMenuTitleFromAppKit(NSString *key, NSString *comment)
{
    NSBundle *appKitBundle = [NSBundle bundleWithIdentifier:@"com.apple.AppKit"];
    if (!appKitBundle) {
        return key;
    }
    NSString *result = NSLocalizedStringFromTableInBundle(key, @"MenuCommands", appKitBundle, comment);
    if (result == nil) {
        return key;
    }
    return result;
}

- (NSMenuItem *)menuItemWithTag:(int)tag target:(id)target representedObject:(id)representedObject
{
    NSMenuItem *menuItem = [[[NSMenuItem alloc] init] autorelease];
    [menuItem setTag:tag];
    [menuItem setTarget:target]; // can be nil
    [menuItem setRepresentedObject:representedObject];
    
    NSString *title = nil;
    SEL action = NULL;
    
    switch(tag) {
        case WebMenuItemTagCopy:
            title = UI_STRING("Copy", "Copy context menu item");
            action = @selector(copy:);
            break;
        case WebMenuItemTagGoBack:
            title = UI_STRING("Back", "Back context menu item");
            action = @selector(goBack:);
            break;
        case WebMenuItemTagGoForward:
            title = UI_STRING("Forward", "Forward context menu item");
            action = @selector(goForward:);
            break;
        case WebMenuItemTagStop:
            title = UI_STRING("Stop", "Stop context menu item");
            action = @selector(stopLoading:);
            break;
        case WebMenuItemTagReload:
            title = UI_STRING("Reload", "Reload context menu item");
            action = @selector(reload:);
            break;
        case WebMenuItemTagSearchInSpotlight:
            // FIXME: Perhaps move this string into WebKit directly when we're not in localization freeze
            title = localizedMenuTitleFromAppKit(@"Search in Spotlight", @"Search in Spotlight menu title.");
            action = @selector(_searchWithSpotlightFromMenu:);
            break;
        case WebMenuItemTagSearchWeb:
            // FIXME: Perhaps move this string into WebKit directly when we're not in localization freeze
            title = localizedMenuTitleFromAppKit(@"Search in Google", @"Search in Google menu title.");
            action = @selector(_searchWithGoogleFromMenu:);
            break;
        case WebMenuItemTagLookUpInDictionary:
            // FIXME: Perhaps move this string into WebKit directly when we're not in localization freeze
            title = localizedMenuTitleFromAppKit(@"Look Up in Dictionary", @"Look Up in Dictionary menu title.");
            action = @selector(_lookUpInDictionaryFromMenu:);
            break;
        default:
            return nil;
    }

    if (title != nil) {
        [menuItem setTitle:title];
    }
    [menuItem setAction:action];
    
    return menuItem;
}

- (void)appendDefaultItems:(NSArray *)defaultItems toArray:(NSMutableArray *)menuItems
{
    ASSERT_ARG(menuItems, menuItems != nil);
    if ([defaultItems count] > 0) {
        ASSERT(![[menuItems lastObject] isSeparatorItem]);
        if (![[defaultItems objectAtIndex:0] isSeparatorItem]) {
            [menuItems addObject:[NSMenuItem separatorItem]];
            
            NSEnumerator *e = [defaultItems objectEnumerator];
            NSMenuItem *item;
            while ((item = [e nextObject]) != nil) {
                [menuItems addObject:item];
            }
        }
    }
}

- (NSArray *)webView:(WebView *)wv contextMenuItemsForElement:(NSDictionary *)element  defaultMenuItems:(NSArray *)defaultMenuItems
{
    // The defaultMenuItems here are ones supplied by the WebDocumentView protocol implementation. WebPDFView is
    // one case that has non-nil default items here.
    NSMutableArray *menuItems = [NSMutableArray array];

    WebFrame *webFrame = [element objectForKey:WebElementFrameKey];
    
    if ([[element objectForKey:WebElementIsSelectedKey] boolValue]) {
        // The Spotlight and Google items are implemented in WebView, and require that the
        // current document view conforms to WebDocumentText
        ASSERT([[[webFrame frameView] documentView] conformsToProtocol:@protocol(WebDocumentText)]);
        [menuItems addObject:[self menuItemWithTag:WebMenuItemTagSearchInSpotlight target:nil representedObject:element]];
        [menuItems addObject:[self menuItemWithTag:WebMenuItemTagSearchWeb target:nil representedObject:element]];
        [menuItems addObject:[NSMenuItem separatorItem]];

        // FIXME 4184640: The Look Up in Dictionary item is only implemented in WebHTMLView, and so is present but
        // dimmed for other cases where WebElementIsSelectedKey is present. It would probably 
        // be better not to include it in the menu if the documentView isn't a WebHTMLView, but that could break 
        // existing clients that have code that relies on it being present (unlikely for clients outside of Apple, 
        // but Safari has such code).
        [menuItems addObject:[self menuItemWithTag:WebMenuItemTagLookUpInDictionary target:nil representedObject:element]];            
        [menuItems addObject:[NSMenuItem separatorItem]];
        [menuItems addObject:[self menuItemWithTag:WebMenuItemTagCopy target:nil representedObject:element]];
    } else {
        WebView *wv = [webFrame webView];
        if ([wv canGoBack]) {
            [menuItems addObject:[self menuItemWithTag:WebMenuItemTagGoBack target:wv representedObject:element]];
        }
        if ([wv canGoForward]) {
            [menuItems addObject:[self menuItemWithTag:WebMenuItemTagGoForward target:wv representedObject:element]];
        }
        if ([wv isLoading]) {
            [menuItems addObject:[self menuItemWithTag:WebMenuItemTagStop target:wv representedObject:element]];
        } else {
            [menuItems addObject:[self menuItemWithTag:WebMenuItemTagReload target:wv representedObject:element]];
        }

        if (webFrame != [wv mainFrame]) {
            [menuItems addObject:[self menuItemWithTag:WebMenuItemTagOpenFrameInNewWindow target:self representedObject:element]];
        }
    }
    
    // Add the default items at the end, if any, after a separator
    [self appendDefaultItems:defaultMenuItems toArray:menuItems];

    return menuItems;
}

@end
