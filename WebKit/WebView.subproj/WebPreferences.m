/*        
        WebPreferences.m
        Copyright 2001, 2002, Apple Computer, Inc. All rights reserved.
*/

#import <WebKit/WebPreferencesPrivate.h>
#import <WebKit/WebPreferenceKeysPrivate.h>

#import <WebKit/WebKitLogging.h>
#import <WebKit/WebKitNSStringExtras.h>
#import <WebKit/WebNSURLExtras.h>

#import <Foundation/NSDictionary_NSURLExtras.h>
#import <Foundation/NSString_NSURLExtras.h>

#import <WebCore/WebCoreSettings.h>

NSString *WebPreferencesChangedNotification = @"WebPreferencesChangedNotification";

#define KEY(x) (_private->identifier ? [_private->identifier stringByAppendingString:(x)] : (x))

enum { WebPreferencesVersion = 1 };

@interface WebPreferencesPrivate : NSObject
{
@public
    NSMutableDictionary *values;
    NSString *identifier;
    NSString *IBCreatorID;
    BOOL autosaves;
}
@end

@implementation WebPreferencesPrivate
- (void)dealloc
{
    [values release];
    [identifier release];
    [IBCreatorID release];
    [super dealloc];
}
@end

@interface WebPreferences (WebInternal)
+ (NSString *)_concatenateKeyWithIBCreatorID:(NSString *)key;
+ (NSString *)_IBCreatorID;
@end

@interface WebPreferences (WebForwardDeclarations)
// This pseudo-category is needed so these methods can be used from within other category implementations
// without being in the public header file.
- (BOOL)_boolValueForKey:(NSString *)key;
- (void)_setBoolValue:(BOOL)value forKey:(NSString *)key;
@end

@implementation WebPreferences

- init
{
    // Create fake identifier
    static int instanceCount = 1;
    NSString *fakeIdentifier;
    
    // At least ensure that identifier hasn't been already used.  
    fakeIdentifier = [NSString stringWithFormat:@"WebPreferences%d", instanceCount++];
    while ([[self class] _getInstanceForIdentifier:fakeIdentifier]){
        fakeIdentifier = [NSString stringWithFormat:@"WebPreferences%d", instanceCount++];
    }
    
    return [self initWithIdentifier:fakeIdentifier];
}

static WebPreferences *_standardPreferences = nil;

- (id)initWithIdentifier:(NSString *)anIdentifier
{
    [super init];

    _private = [[WebPreferencesPrivate alloc] init];
    _private->IBCreatorID = [[WebPreferences _IBCreatorID] retain];

    WebPreferences *instance = [[self class] _getInstanceForIdentifier:anIdentifier];
    if (instance){
        [self release];
        return [instance retain];
    }

    _private->values = [[NSMutableDictionary alloc] init];
    _private->identifier = [anIdentifier copy];
    
    [[self class] _setInstance:self forIdentifier:_private->identifier];

    [[NSNotificationCenter defaultCenter]
       postNotificationName:WebPreferencesChangedNotification object:self userInfo:nil];

    return self;
}

- (id)initWithCoder:(NSCoder *)decoder
{
    volatile id result = nil;

NS_DURING

    int version;

    _private = [[WebPreferencesPrivate alloc] init];
    _private->IBCreatorID = [[WebPreferences _IBCreatorID] retain];
    
    if ([decoder allowsKeyedCoding]){
        _private->identifier = [[decoder decodeObjectForKey:@"Identifier"] retain];
        _private->values = [[decoder decodeObjectForKey:@"Values"] retain];
        LOG (Encoding, "Identifier = %@, Values = %@\n", _private->identifier, _private->values);
    }
    else {
        [decoder decodeValueOfObjCType:@encode(int) at:&version];
        if (version == 1){
            _private->identifier = [[decoder decodeObject] retain];
            _private->values = [[decoder decodeObject] retain];
        }
    }
    
    // If we load a nib multiple times, or have instances in multiple
    // nibs with the same name, the first guy up wins.
    WebPreferences *instance = [[self class] _getInstanceForIdentifier:_private->identifier];
    if (instance){
        [self release];
        result = [instance retain];
    }
    else {
        [[self class] _setInstance:self forIdentifier:_private->identifier];
        result = self;
    }
    
NS_HANDLER

    result = nil;
    [self release];
    
NS_ENDHANDLER

    return result;
}

- (void)encodeWithCoder:(NSCoder *)encoder
{
    if ([encoder allowsKeyedCoding]){
        [encoder encodeObject:_private->identifier forKey:@"Identifier"];
        [encoder encodeObject:_private->values forKey:@"Values"];
        LOG (Encoding, "Identifier = %@, Values = %@\n", _private->identifier, _private->values);
    }
    else {
        int version = WebPreferencesVersion;
        [encoder encodeValueOfObjCType:@encode(int) at:&version];
        [encoder encodeObject:_private->identifier];
        [encoder encodeObject:_private->values];
    }
}

+ (WebPreferences *)standardPreferences
{
    if (_standardPreferences == nil) {
        _standardPreferences = [[WebPreferences alloc] initWithIdentifier:nil];
        [_standardPreferences setAutosaves:YES];
        [_standardPreferences _postPreferencesChangesNotification];
    }

    return _standardPreferences;
}

// if we ever have more than one WebPreferences object, this would move to init
+ (void)initialize
{
    NSDictionary *dict = [NSDictionary dictionaryWithObjectsAndKeys:
        @"0x0",                         WebKitLogLevelPreferenceKey,
        @"Times",                       WebKitStandardFontPreferenceKey,
        @"Courier",                     WebKitFixedFontPreferenceKey,
        @"Times",                       WebKitSerifFontPreferenceKey,
        @"Helvetica",                   WebKitSansSerifFontPreferenceKey,
        @"Apple Chancery",              WebKitCursiveFontPreferenceKey,
        @"Papyrus",                     WebKitFantasyFontPreferenceKey,
        @"1",                           WebKitMinimumFontSizePreferenceKey,
	@"9",                           WebKitMinimumLogicalFontSizePreferenceKey, 
        @"16",                          WebKitDefaultFontSizePreferenceKey,
        @"13",                          WebKitDefaultFixedFontSizePreferenceKey,
        @"ISO-8859-1",                  WebKitDefaultTextEncodingNamePreferenceKey,
        @"4",                           WebKitPageCacheSizePreferenceKey,
        @"8388608",                     WebKitObjectCacheSizePreferenceKey,
        [NSNumber numberWithBool:NO],   WebKitUserStyleSheetEnabledPreferenceKey,
        @"",                            WebKitUserStyleSheetLocationPreferenceKey,
        [NSNumber numberWithBool:NO],   WebKitShouldPrintBackgroundsPreferenceKey,
        [NSNumber numberWithBool:YES],  WebKitJavaEnabledPreferenceKey,
        [NSNumber numberWithBool:YES],  WebKitJavaScriptEnabledPreferenceKey,
        [NSNumber numberWithBool:YES],  WebKitJavaScriptCanOpenWindowsAutomaticallyPreferenceKey,
        [NSNumber numberWithBool:YES],  WebKitPluginsEnabledPreferenceKey,
        [NSNumber numberWithBool:YES],  WebKitAllowAnimatedImagesPreferenceKey,
        [NSNumber numberWithBool:YES],  WebKitAllowAnimatedImageLoopingPreferenceKey,
        [NSNumber numberWithBool:YES],  WebKitDisplayImagesKey,
        @"1800",                        WebKitBackForwardCacheExpirationIntervalKey,
        [NSNumber numberWithBool:NO],   WebKitTabToLinksPreferenceKey,
        [NSNumber numberWithBool:NO],   WebKitPrivateBrowsingEnabledPreferenceKey,
        [NSNumber numberWithBool:NO],   WebKitRespectStandardStyleKeyEquivalentsPreferenceKey,
        [NSNumber numberWithBool:NO],   WebKitShowsURLsInToolTipsPreferenceKey,
        nil];

    [[NSUserDefaults standardUserDefaults] registerDefaults:dict];
}

- (void)dealloc
{
    [_private release];
    [super dealloc];
}

- (NSString *)identifier
{
    return _private->identifier;
}

- (id)_valueForKey:(NSString *)key
{
    NSString *_key = KEY(key);
    id o = [_private->values objectForKey:_key];
    if (o)
        return o;
    o = [[NSUserDefaults standardUserDefaults] objectForKey:_key];
    if (!o && key != _key)
        o = [[NSUserDefaults standardUserDefaults] objectForKey:key];
    return o;
}

- (NSString *)_stringValueForKey:(NSString *)key
{
    id s = [self _valueForKey:key];
    return [s isKindOfClass:[NSString class]] ? (NSString *)s : nil;
}

- (void)_setStringValue:(NSString *)value forKey:(NSString *)key
{
    NSString *_key = KEY(key);
    if (_private->autosaves)
        [[NSUserDefaults standardUserDefaults] setObject:value forKey:_key];
    [_private->values setObject:value forKey:_key];
    [self _postPreferencesChangesNotification];
}

- (int)_integerValueForKey:(NSString *)key
{
    id o = [self _valueForKey:key];
    // 
    return [o respondsToSelector:@selector(intValue)] ? [o intValue] : 0;
}

- (void)_setIntegerValue:(int)value forKey:(NSString *)key
{
    NSString *_key = KEY(key);
    if (_private->autosaves)
        [[NSUserDefaults standardUserDefaults] setInteger:value forKey:_key];
    [_private->values _web_setInt:value forKey:_key];
    [self _postPreferencesChangesNotification];
}

- (BOOL)_boolValueForKey:(NSString *)key
{
    return [self _integerValueForKey:key] != 0;
}

- (void)_setBoolValue:(BOOL)value forKey:(NSString *)key
{
    NSString *_key = KEY(key);
    if (_private->autosaves)
        [[NSUserDefaults standardUserDefaults] setBool:value forKey:_key];
    [_private->values _web_setBool:value forKey:_key];
    [self _postPreferencesChangesNotification];
}

- (NSString *)standardFontFamily
{
    return [self _stringValueForKey: WebKitStandardFontPreferenceKey];
}

- (void)setStandardFontFamily:(NSString *)family
{
    [self _setStringValue: family forKey: WebKitStandardFontPreferenceKey];
}

- (NSString *)fixedFontFamily
{
    return [self _stringValueForKey: WebKitFixedFontPreferenceKey];
}

- (void)setFixedFontFamily:(NSString *)family
{
    [self _setStringValue: family forKey: WebKitFixedFontPreferenceKey];
}

- (NSString *)serifFontFamily
{
    return [self _stringValueForKey: WebKitSerifFontPreferenceKey];
}

- (void)setSerifFontFamily:(NSString *)family 
{
    [self _setStringValue: family forKey: WebKitSerifFontPreferenceKey];
}

- (NSString *)sansSerifFontFamily
{
    return [self _stringValueForKey: WebKitSansSerifFontPreferenceKey];
}

- (void)setSansSerifFontFamily:(NSString *)family
{
    [self _setStringValue: family forKey: WebKitSansSerifFontPreferenceKey];
}

- (NSString *)cursiveFontFamily
{
    return [self _stringValueForKey: WebKitCursiveFontPreferenceKey];
}

- (void)setCursiveFontFamily:(NSString *)family
{
    [self _setStringValue: family forKey: WebKitCursiveFontPreferenceKey];
}

- (NSString *)fantasyFontFamily
{
    return [self _stringValueForKey: WebKitFantasyFontPreferenceKey];
}

- (void)setFantasyFontFamily:(NSString *)family
{
    [self _setStringValue: family forKey: WebKitFantasyFontPreferenceKey];
}

- (int)defaultFontSize
{
    return [self _integerValueForKey: WebKitDefaultFontSizePreferenceKey];
}

- (void)setDefaultFontSize:(int)size
{
    [self _setIntegerValue: size forKey: WebKitDefaultFontSizePreferenceKey];
}

- (int)defaultFixedFontSize
{
    return [self _integerValueForKey: WebKitDefaultFixedFontSizePreferenceKey];
}

- (void)setDefaultFixedFontSize:(int)size
{
    [self _setIntegerValue: size forKey: WebKitDefaultFixedFontSizePreferenceKey];
}

- (int)minimumFontSize
{
    return [self _integerValueForKey: WebKitMinimumFontSizePreferenceKey];
}

- (void)setMinimumFontSize:(int)size
{
    [self _setIntegerValue: size forKey: WebKitMinimumFontSizePreferenceKey];
}

- (int)minimumLogicalFontSize
{
  return [self _integerValueForKey: WebKitMinimumLogicalFontSizePreferenceKey];
}

- (void)setMinimumLogicalFontSize:(int)size
{
  [self _setIntegerValue: size forKey: WebKitMinimumLogicalFontSizePreferenceKey];
}

- (NSString *)defaultTextEncodingName
{
    return [self _stringValueForKey: WebKitDefaultTextEncodingNamePreferenceKey];
}

- (void)setDefaultTextEncodingName:(NSString *)encoding
{
    [self _setStringValue: encoding forKey: WebKitDefaultTextEncodingNamePreferenceKey];
}

- (BOOL)userStyleSheetEnabled
{
    return [self _boolValueForKey: WebKitUserStyleSheetEnabledPreferenceKey];
}

- (void)setUserStyleSheetEnabled:(BOOL)flag
{
    [self _setBoolValue: flag forKey: WebKitUserStyleSheetEnabledPreferenceKey];
}

- (NSURL *)userStyleSheetLocation
{
    NSString *locationString = [self _stringValueForKey: WebKitUserStyleSheetLocationPreferenceKey];
    
    if ([locationString _web_looksLikeAbsoluteURL]) {
        return [NSURL _web_URLWithDataAsString:locationString];
    } else {
        locationString = [locationString stringByExpandingTildeInPath];
        return [NSURL fileURLWithPath:locationString];
    }
}

- (void)setUserStyleSheetLocation:(NSURL *)URL
{
    NSString *locationString;
    
    if ([URL isFileURL]) {
        locationString = [[URL path] _web_stringByAbbreviatingWithTildeInPath];
    } else {
        locationString = [URL _web_originalDataAsString];
    }
    
    [self _setStringValue:locationString forKey: WebKitUserStyleSheetLocationPreferenceKey];
}

- (BOOL)shouldPrintBackgrounds
{
    return [self _boolValueForKey: WebKitShouldPrintBackgroundsPreferenceKey];
}

- (void)setShouldPrintBackgrounds:(BOOL)flag
{
    [self _setBoolValue: flag forKey: WebKitShouldPrintBackgroundsPreferenceKey];
}

- (BOOL)isJavaEnabled
{
    return [self _boolValueForKey: WebKitJavaEnabledPreferenceKey];
}

- (void)setJavaEnabled:(BOOL)flag
{
    [self _setBoolValue: flag forKey: WebKitJavaEnabledPreferenceKey];
}

- (BOOL)isJavaScriptEnabled
{
    return [self _boolValueForKey: WebKitJavaScriptEnabledPreferenceKey];
}

- (void)setJavaScriptEnabled:(BOOL)flag
{
    [self _setBoolValue: flag forKey: WebKitJavaScriptEnabledPreferenceKey];
}

- (BOOL)javaScriptCanOpenWindowsAutomatically
{
    return [self _boolValueForKey: WebKitJavaScriptCanOpenWindowsAutomaticallyPreferenceKey];
}

- (void)setJavaScriptCanOpenWindowsAutomatically:(BOOL)flag
{
    [self _setBoolValue: flag forKey: WebKitJavaScriptCanOpenWindowsAutomaticallyPreferenceKey];
}

- (BOOL)arePlugInsEnabled
{
    return [self _boolValueForKey: WebKitPluginsEnabledPreferenceKey];
}

- (void)setPlugInsEnabled:(BOOL)flag
{
    [self _setBoolValue: flag forKey: WebKitPluginsEnabledPreferenceKey];
}

- (BOOL)allowsAnimatedImages
{
    return [self _boolValueForKey: WebKitAllowAnimatedImagesPreferenceKey];
}

- (void)setAllowsAnimatedImages:(BOOL)flag;
{
    [self _setBoolValue: flag forKey: WebKitAllowAnimatedImagesPreferenceKey];
}

- (BOOL)allowsAnimatedImageLooping
{
    return [self _boolValueForKey: WebKitAllowAnimatedImageLoopingPreferenceKey];
}

- (void)setAllowsAnimatedImageLooping: (BOOL)flag
{
    [self _setBoolValue: flag forKey: WebKitAllowAnimatedImageLoopingPreferenceKey];
}

- (void)setLoadsImagesAutomatically: (BOOL)flag
{
    [self _setBoolValue: flag forKey: WebKitDisplayImagesKey];
}

- (BOOL)loadsImagesAutomatically
{
    return [self _boolValueForKey: WebKitDisplayImagesKey];
}

- (void)setAutosaves:(BOOL)flag;
{
    _private->autosaves = flag;
}

- (BOOL)autosaves
{
    return _private->autosaves;
}

- (void)setTabsToLinks:(BOOL)flag
{
    [self _setBoolValue: flag forKey: WebKitTabToLinksPreferenceKey];
}

- (BOOL)tabsToLinks
{
    return [self _boolValueForKey:WebKitTabToLinksPreferenceKey];
}

- (void)setPrivateBrowsingEnabled:(BOOL)flag
{
    [self _setBoolValue:flag forKey:WebKitPrivateBrowsingEnabledPreferenceKey];
}

- (BOOL)privateBrowsingEnabled
{
    return [self _boolValueForKey:WebKitPrivateBrowsingEnabledPreferenceKey];
}

@end

@implementation WebPreferences (WebPrivate)

- (BOOL)respectStandardStyleKeyEquivalents
{
    return [self _boolValueForKey:WebKitRespectStandardStyleKeyEquivalentsPreferenceKey];
}

- (void)setRespectStandardStyleKeyEquivalents:(BOOL)flag
{
    [self _setBoolValue:flag forKey:WebKitRespectStandardStyleKeyEquivalentsPreferenceKey];
}

- (BOOL)showsURLsInToolTips
{
    return [self _boolValueForKey:WebKitShowsURLsInToolTipsPreferenceKey];
}

- (void)setShowsURLsInToolTips:(BOOL)flag
{
    [self _setBoolValue:flag forKey:WebKitShowsURLsInToolTipsPreferenceKey];
}

- (int)_pageCacheSize
{
    return [[NSUserDefaults standardUserDefaults] integerForKey:WebKitPageCacheSizePreferenceKey];
}

- (int)_objectCacheSize
{
    return [[NSUserDefaults standardUserDefaults] integerForKey:WebKitObjectCacheSizePreferenceKey];
}

- (NSTimeInterval)_backForwardCacheExpirationInterval
{
    return (NSTimeInterval)[[NSUserDefaults standardUserDefaults] floatForKey:WebKitBackForwardCacheExpirationIntervalKey];
}

static NSMutableDictionary *webPreferencesInstances = nil;

+ (WebPreferences *)_getInstanceForIdentifier:(NSString *)ident
{
        LOG (Encoding, "requesting for %@\n", ident);

    if (!ident){
        if(_standardPreferences)
            return _standardPreferences;
        return nil;
    }    
    
    WebPreferences *instance = [webPreferencesInstances objectForKey:[self _concatenateKeyWithIBCreatorID:ident]];

    return instance;
}

+ (void)_setInstance:(WebPreferences *)instance forIdentifier:(NSString *)ident
{
    if (!webPreferencesInstances)
        webPreferencesInstances = [[NSMutableDictionary alloc] init];
    if (ident) {
        [webPreferencesInstances setObject:instance forKey:[self _concatenateKeyWithIBCreatorID:ident]];
        LOG (Encoding, "recording %p for %@\n", instance, [self _concatenateKeyWithIBCreatorID:ident]);
    }
}

+ (void)_removeReferenceForIdentifier:(NSString *)ident
{
    if (ident != nil) {
        [webPreferencesInstances performSelector:@selector(_web_checkLastReferenceForIdentifier:) withObject: [self _concatenateKeyWithIBCreatorID:ident] afterDelay:.1];
    }
}

- (void)_postPreferencesChangesNotification
{
    [[NSNotificationCenter defaultCenter]
        postNotificationName:WebPreferencesChangedNotification object:self
                    userInfo:nil];
}

- (void)_setInitialDefaultTextEncodingToSystemEncoding
{
    CFStringEncoding encoding = CFStringGetSystemEncoding();
    
    // MacRoman is a special case; maybe we will learn that there should be other special cases later.
    if (encoding == kCFStringEncodingMacRoman) {
        encoding = kCFStringEncodingISOLatin1;
    }
    
    NSString *name = (NSString *)CFStringConvertEncodingToIANACharSetName(encoding);
    
    // fall back to latin1 if necessary
    if (name == nil) {
        name = (NSString *)CFStringConvertEncodingToIANACharSetName(kCFStringEncodingISOLatin1);
    }
    
    [[NSUserDefaults standardUserDefaults] registerDefaults:
        [NSDictionary dictionaryWithObject:name
                                    forKey:WebKitDefaultTextEncodingNamePreferenceKey]];
}

static NSString *classIBCreatorID = nil;

+ (void)_setIBCreatorID:(NSString *)string
{
    NSString *old = classIBCreatorID;
    classIBCreatorID = [string copy];
    [old release];
}

@end

@implementation WebPreferences (WebInternal)

+ (NSString *)_IBCreatorID
{
    return classIBCreatorID;
}

+ (NSString *)_concatenateKeyWithIBCreatorID:(NSString *)key
{
    NSString *IBCreatorID = [WebPreferences _IBCreatorID];
    if (!IBCreatorID)
        return key;
    return [IBCreatorID stringByAppendingString:key];
}

@end

@implementation NSMutableDictionary (WebInternal)

- (void)_web_checkLastReferenceForIdentifier:(NSString *)identifier
{
    WebPreferences *instance = [webPreferencesInstances objectForKey:identifier];
    if ([instance retainCount] == 1)
        [webPreferencesInstances removeObjectForKey:identifier];
}

@end
