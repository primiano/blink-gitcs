/*      
    WebKitSystemInterface.h
    Copyright (C) 2005 Apple Computer, Inc. All rights reserved.    
    
    Public header file.
*/

#import <Cocoa/Cocoa.h>

typedef enum {
    WKCertificateParseResultSucceeded  = 0,
    WKCertificateParseResultFailed     = 1,
    WKCertificateParseResultPKCS7      = 2,
} WKCertificateParseResult;

CFStringRef WKCopyCFLocalizationPreferredName(CFStringRef localization);
CFStringRef WKSignedPublicKeyAndChallengeString(unsigned keySize, CFStringRef challenge, CFStringRef keyDescription);
WKCertificateParseResult WKAddCertificatesToKeychainFromData(const void *bytes, unsigned length);

NSString *WKGetPreferredExtensionForMIMEType(NSString *type);
NSArray *WKGetExtensionsForMIMEType(NSString *type);
NSString *WKGetMIMETypeForExtension(NSString *extension);

NSDate *WKGetNSURLResponseLastModifiedDate(NSURLResponse *response);
NSTimeInterval WKGetNSURLResponseFreshnessLifetime(NSURLResponse *response);
NSTimeInterval WKGetNSURLResponseCalculatedExpiration(NSURLResponse *response);
BOOL WKGetNSURLResponseMustRevalidate(NSURLResponse *response);

CFStringEncoding WKGetWebDefaultCFStringEncoding(void);

float WKSecondsSinceLastInputEvent();
CGColorSpaceRef WKCreateUncorrectedRGBColorSpace();
CGColorSpaceRef WKCreateUncorrectedGrayColorSpace();
CFStringRef WKPreferRGB32Key();

void WKSetNSURLConnectionDefersCallbacks(NSURLConnection *connection, BOOL defers);
float WKSecondsSinceLastInputEvent(void);

void WKShowKeyAndMain(void);
OSStatus WKSyncWindowWithCGAfterMove(WindowRef);
unsigned WKCarbonWindowMask(void);
void *WKGetNativeWindowFromWindowRef(WindowRef);
OSType WKCarbonWindowPropertyCreator(void);
OSType WKCarbonWindowPropertyTag(void);

typedef id WKNSURLConnectionDelegateProxyPtr;

WKNSURLConnectionDelegateProxyPtr WKCreateNSURLConnectionDelegateProxy();

void WKDisableCGDeferredUpdates();

Class WKNSURLProtocolClassForReqest(NSURLRequest *request);

unsigned WKGetNSAutoreleasePoolCount();

NSString *WKMouseMovedNotification();
BOOL WKMouseIsDown();
void WKSetNSWindowShouldPostEventNotifications(NSWindow *window, BOOL post);

CFTypeID WKGetAXTextMarkerTypeID();
CFTypeID WKGetAXTextMarkerRangeTypeID();
CFTypeRef WKCreateAXTextMarker(const void *bytes, size_t len);
BOOL WKGetBytesFromAXTextMarker(CFTypeRef textMarker, void *bytes, size_t length);
CFTypeRef WKCreateAXTextMarkerRange(CFTypeRef start, CFTypeRef end);
CFTypeRef WKCopyAXTextMarkerRangeStart(CFTypeRef range);
CFTypeRef WKCopyAXTextMarkerRangeEnd(CFTypeRef range);
void WKAccessibilityHandleFocusChanged();
AXUIElementRef WKCreateAXUIElementRef(id element);
void WKUnregisterUniqueIdForElement(id element);

BOOL WKFontSmoothingModeIsLCD(int mode);
void WKSetUpFontCache(size_t s);

void WKSignalCFReadStreamEnd(CFReadStreamRef stream);
void WKSignalCFReadStreamHasBytes(CFReadStreamRef stream);
void WKSignalCFReadStreamError(CFReadStreamRef stream, CFStreamError *error);

CFReadStreamRef WKCreateCustomCFReadStream(void *(*formCreate)(CFReadStreamRef, void *), 
										   void (*formFinalize)(CFReadStreamRef, void *), 
										   Boolean (*formOpen)(CFReadStreamRef, CFStreamError *, Boolean *, void *), 
										   CFIndex (*formRead)(CFReadStreamRef, UInt8 *, CFIndex, CFStreamError *, Boolean *, void *), 
										   Boolean (*formCanRead)(CFReadStreamRef, void *), 
										   void (*formClose)(CFReadStreamRef, void *), 
										   void (*formSchedule)(CFReadStreamRef, CFRunLoopRef, CFStringRef, void *), 
										   void (*formUnschedule)(CFReadStreamRef, CFRunLoopRef, CFStringRef, void *),
										   void *context);

void WKSetFocusRingStyle(NSFocusRingPlacement placement, int radius, NSColor *color);
void WKSetDragImage(NSImage *image, NSPoint offset);
