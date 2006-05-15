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

#import "config.h"
#import "WebTextRendererFactory.h"
#import "FontData.h"
#import "FontDescription.h"
#import "FontFallbackList.h"
#import "WebCoreSystemInterface.h"

#import "FoundationExtras.h"
#import "KWQListBox.h"
#import "Page.h"
#import "WebCoreFrameBridge.h"
#import "WebCoreStringTruncator.h"
#import <wtf/Assertions.h>

#import <mach-o/dyld.h>

#define SYNTHESIZED_FONT_TRAITS (NSBoldFontMask | NSItalicFontMask)

#define IMPORTANT_FONT_TRAITS (0 \
    | NSBoldFontMask \
    | NSCompressedFontMask \
    | NSCondensedFontMask \
    | NSExpandedFontMask \
    | NSItalicFontMask \
    | NSNarrowFontMask \
    | NSPosterFontMask \
    | NSSmallCapsFontMask \
)

#define DESIRED_WEIGHT 5

using namespace WebCore;

static WebTextRendererFactory* sharedFactory;

@interface NSFont (WebPrivate)
- (ATSUFontID)_atsFontID;
@end

@interface NSFont (WebAppKitSecretAPI)
- (BOOL)_isFakeFixedPitch;
@end

@implementation WebTextRendererFactory

static bool getAppDefaultValue(CFStringRef key, int *v)
{
    CFPropertyListRef value;

    value = CFPreferencesCopyValue(key, kCFPreferencesCurrentApplication,
                                   kCFPreferencesAnyUser,
                                   kCFPreferencesAnyHost);
    if (value == 0) {
        value = CFPreferencesCopyValue(key, kCFPreferencesCurrentApplication,
                                       kCFPreferencesCurrentUser,
                                       kCFPreferencesAnyHost);
        if (value == 0)
            return false;
    }

    if (CFGetTypeID(value) == CFNumberGetTypeID()) {
        if (v != 0)
            CFNumberGetValue((const CFNumberRef)value, kCFNumberIntType, v);
    } else if (CFGetTypeID(value) == CFStringGetTypeID()) {
        if (v != 0)
            *v = CFStringGetIntValue((const CFStringRef)value);
    } else {
        CFRelease(value);
        return false;
    }

    CFRelease(value);
    return true;
}

static bool getUserDefaultValue(CFStringRef key, int *v)
{
    CFPropertyListRef value;

    value = CFPreferencesCopyValue(key, kCFPreferencesAnyApplication,
                                   kCFPreferencesCurrentUser,
                                   kCFPreferencesCurrentHost);
    if (value == 0)
        return false;

    if (CFGetTypeID(value) == CFNumberGetTypeID()) {
        if (v != 0)
            CFNumberGetValue((const CFNumberRef)value, kCFNumberIntType, v);
    } else if (CFGetTypeID(value) == CFStringGetTypeID()) {
        if (v != 0)
            *v = CFStringGetIntValue((const CFStringRef)value);
    } else {
        CFRelease(value);
        return false;
    }

    CFRelease(value);
    return true;
}

static int getLCDScaleParameters(void)
{
    int mode;
    CFStringRef key;

    key = CFSTR("AppleFontSmoothing");
    if (!getAppDefaultValue(key, &mode)) {
        if (!getUserDefaultValue(key, &mode))
            return 1;
    }

    if (wkFontSmoothingModeIsLCD(mode))
        return 4;
    return 1;
}

static CFMutableDictionaryRef fontCache;
static CFMutableDictionaryRef fixedPitchFonts;

- (void)clearCaches
{
    int i;
    for (i = 0; i < WEB_TEXT_RENDERER_FACTORY_NUM_CACHES; ++i) {
        deleteAllValues(*caches[i]);
        caches[i]->clear();
    }

    if (fontCache)
        CFRelease(fontCache);
    fontCache = 0;
    
    if (fixedPitchFonts) {
        CFRelease(fixedPitchFonts);
        fixedPitchFonts = 0;
    }
        
    QListBox::clearCachedTextRenderers();
    [WebCoreStringTruncator clear];
    Page::setNeedsReapplyStyles();
}

static void 
fontsChanged( ATSFontNotificationInfoRef info, void *_factory)
{
    WebTextRendererFactory *factory = (WebTextRendererFactory *)_factory;

    assert(factory);

    [factory clearCaches];
}

#define MINIMUM_GLYPH_CACHE_SIZE 1536 * 1024

+ (void)createSharedFactory
{
    [[[self alloc] init] release];

    size_t s = MINIMUM_GLYPH_CACHE_SIZE*getLCDScaleParameters();

    wkSetUpFontCache(s);

    // Ignore errors returned from ATSFontNotificationSubscribe.  If we can't subscribe then we
    // won't be told about changes to fonts.
    ATSFontNotificationSubscribe( fontsChanged, kATSFontNotifyOptionDefault, (void *)[self sharedFactory], nil );
}

+ (WebTextRendererFactory *)sharedFactory
{
    if (!sharedFactory)
        [WebTextRendererFactory createSharedFactory];
    return sharedFactory;
}

- (BOOL)isFontFixedPitch:(FontPlatformData)font
{
    NSFont *f = font.font;

    if (!fixedPitchFonts)
        fixedPitchFonts = CFDictionaryCreateMutable(0, 0, &kCFTypeDictionaryKeyCallBacks, 0);
    CFBooleanRef val = (CFBooleanRef)CFDictionaryGetValue(fixedPitchFonts, f);

    BOOL ret;    
    if (val) {
        ret = (val == kCFBooleanTrue);
    } else {
        // Special case Osaka-Mono.
        // According to <rdar://problem/3999467>, we should treat Osaka-Mono as fixed pitch.
        // Note that the AppKit does not report Osaka-Mono as fixed pitch.

        // Special case MS-PGothic.
        // According to <rdar://problem/4032938, we should not treat MS-PGothic as fixed pitch.
        // Note that AppKit does report MS-PGothic as fixed pitch.

        NSString *name = [f fontName];
        ret = ([f isFixedPitch] || [f _isFakeFixedPitch] || [name caseInsensitiveCompare:@"Osaka-Mono"] == NSOrderedSame)
            && ![name caseInsensitiveCompare:@"MS-PGothic"] == NSOrderedSame;

        CFDictionarySetValue(fixedPitchFonts, f, ret ? kCFBooleanTrue : kCFBooleanFalse);
    }

    return ret;    
}

- init
{
    [super init];
    
    int i;
    for (i = 0; i < WEB_TEXT_RENDERER_FACTORY_NUM_CACHES; ++i)
        caches[i] = new HashMap<NSFont*, FontData*>;
    
    ASSERT(!sharedFactory);
    sharedFactory = KWQRetain(self);

    return self;
}

- (void)dealloc
{
    int i;
    for (i = 0; i < WEB_TEXT_RENDERER_FACTORY_NUM_CACHES; ++i)
        delete caches[i];

    [super dealloc];
}

- (FontData *)rendererWithFont:(FontPlatformData)font
{
    HashMap<NSFont*, FontData*>* cache = caches[(font.syntheticBold << 1) | (font.syntheticOblique)];
    FontData *renderer = cache->get(font.font);
    if (!renderer) {
        renderer = new FontData(font);
        cache->set(font.font, renderer);
    }
    return renderer;
}

- (NSFont *)fallbackFontWithTraits:(NSFontTraitMask)traits size:(float)size 
{
    // FIXME: Would be even better to somehow get the user's default font here.  For now we'll pick
    // the default that the user would get without changing any prefs.
    NSFont *font = [self cachedFontFromFamily:@"Times" traits:traits size:size];
    if (font == nil) {
        // The Times fallback will almost always work, but in the highly unusual case where
        // the user doesn't have it, we fall back on Lucida Grande because that's
        // guaranteed to be there, according to Nathan Taylor. This is good enough
        // to avoid a crash, at least. To reduce the possibility of failure even further,
        // we don't even bother with traits.
        font = [self cachedFontFromFamily:@"Lucida Grande" traits:0 size:size];
    }
    return font;
}

- (FontPlatformData)fontWithDescription:(WebCore::FontDescription*)fontDescription familyIndex:(int*)index
{
    NSFont *font = nil;
    NSString *matchedFamily = nil;
    NSFontTraitMask traits = 0;
    if (fontDescription->italic())
        traits |= NSItalicFontMask;
    if (fontDescription->weight() >= WebCore::cBoldWeight)
        traits |= NSBoldFontMask;
    float size = fontDescription->computedPixelSize(); // We only do integral font sizes right now.
    
    int startIndex = *index;
    const FontFamily* startFamily = &fontDescription->family();
    for (int i = 0; startFamily && i < startIndex; i++)
        startFamily = startFamily->next();
    const FontFamily* currFamily = startFamily;
    while (currFamily && font == nil) {
        NSString *family = currFamily->getNSFamily();
        *index = *index + 1;
        if ([family length] != 0) {
            font = [self cachedFontFromFamily:family traits:traits size:size];
            if (font) {
                matchedFamily = family;
                break;
            }
        }
        currFamily = currFamily->next();
    }

    if (!currFamily)
        *index = cAllFamiliesScanned;

    if (font == nil) {
        // We didn't find a font. Use a fallback font.
        
        // First we'll attempt to find an appropriate font using a match based on 
        // the presence of keywords in the the requested names.  For example, we'll
        // match any name that contains "Arabic" to Geeza Pro.
        const FontFamily* currFamily = startFamily;
        while (currFamily && font == nil) {
            NSString *family = currFamily->getNSFamily();
            if ([family length] != 0) {
                static NSString * const matchWords[3] = { @"Arabic", @"Pashto", @"Urdu" };
                static NSString * const matchFamilies[3] = { @"Geeza Pro", @"Geeza Pro", @"Geeza Pro" };
                int j;
                for (j = 0; j < 3 && font == nil; ++j)
                    if ([family rangeOfString:matchWords[j] options:NSCaseInsensitiveSearch].location != NSNotFound)
                        font = [self cachedFontFromFamily:matchFamilies[j] traits:traits size:size];
            }
            currFamily = currFamily->next();
        }
        
        // Still nothing found, use the final fallback.  We only do the last resort fallback
        // when trying to find the primary font.  Otherwise our fallback will rely on the actual characters used.
        if (font == nil && startIndex == 0)
            font = [self fallbackFontWithTraits:traits size:size];
    }

    NSFontTraitMask actualTraits = 0;
    if (traits & (NSItalicFontMask | NSBoldFontMask))
        actualTraits = [[NSFontManager sharedFontManager] traitsOfFont:font];
    
    FontPlatformData result;
    result.font = font;
    result.syntheticBold = (traits & NSBoldFontMask) && !(actualTraits & NSBoldFontMask);
    result.syntheticOblique = (traits & NSItalicFontMask) && !(actualTraits & NSItalicFontMask);
    
    // There are some malformed fonts that will be correctly returned by -cachedFontForFamily:traits:size: as a match for a particular trait,
    // though -[NSFontManager traitsOfFont:] incorrectly claims the font does not have the specified trait. This could result in applying 
    // synthetic bold on top of an already-bold font, as reported in <http://bugzilla.opendarwin.org/show_bug.cgi?id=6146>. To work around this
    // problem, if we got an apparent exact match, but the requested traits aren't present in the matched font, we'll try to get a font from 
    // the same family without those traits (to apply the synthetic traits to later).
    if (matchedFamily) {
        NSFontTraitMask nonSyntheticTraits = traits;
        
        if (result.syntheticBold)
            nonSyntheticTraits &= ~NSBoldFontMask;
        
        if (result.syntheticOblique)
            nonSyntheticTraits &= ~NSItalicFontMask;
        
        if (nonSyntheticTraits != traits) {
            NSFont *fontWithoutSyntheticTraits = [self cachedFontFromFamily:matchedFamily traits:nonSyntheticTraits size:size];
            if (fontWithoutSyntheticTraits)
                result.font = fontWithoutSyntheticTraits;
        }
    }
    
    // Use the correct font for print vs. screen.
    result.font = fontDescription->usePrinterFont() ? [result.font printerFont] : [result.font screenFont];

    return result;
}

static BOOL acceptableChoice(NSFontTraitMask desiredTraits, int desiredWeight,
    NSFontTraitMask candidateTraits, int candidateWeight)
{
    desiredTraits &= ~SYNTHESIZED_FONT_TRAITS;
    return (candidateTraits & desiredTraits) == desiredTraits;
}

static BOOL betterChoice(NSFontTraitMask desiredTraits, int desiredWeight,
    NSFontTraitMask chosenTraits, int chosenWeight,
    NSFontTraitMask candidateTraits, int candidateWeight)
{
    if (!acceptableChoice(desiredTraits, desiredWeight, candidateTraits, candidateWeight)) {
        return NO;
    }
    
    // A list of the traits we care about.
    // The top item in the list is the worst trait to mismatch; if a font has this
    // and we didn't ask for it, we'd prefer any other font in the family.
    const NSFontTraitMask masks[] = {
        NSPosterFontMask,
        NSSmallCapsFontMask,
        NSItalicFontMask,
        NSCompressedFontMask,
        NSCondensedFontMask,
        NSExpandedFontMask,
        NSNarrowFontMask,
        NSBoldFontMask,
        0 };
    int i = 0;
    NSFontTraitMask mask;
    while ((mask = masks[i++])) {
        BOOL desired = (desiredTraits & mask) != 0;
        BOOL chosenHasUnwantedTrait = desired != ((chosenTraits & mask) != 0);
        BOOL candidateHasUnwantedTrait = desired != ((candidateTraits & mask) != 0);
        if (!candidateHasUnwantedTrait && chosenHasUnwantedTrait)
            return YES;
        if (!chosenHasUnwantedTrait && candidateHasUnwantedTrait)
            return NO;
    }
    
    int chosenWeightDelta = chosenWeight - desiredWeight;
    int candidateWeightDelta = candidateWeight - desiredWeight;
    
    int chosenWeightDeltaMagnitude = ABS(chosenWeightDelta);
    int candidateWeightDeltaMagnitude = ABS(candidateWeightDelta);
    
    // Smaller magnitude wins.
    // If both have same magnitude, tie breaker is that the smaller weight wins.
    // Otherwise, first font in the array wins (should almost never happen).
    if (candidateWeightDeltaMagnitude < chosenWeightDeltaMagnitude) {
        return YES;
    }
    if (candidateWeightDeltaMagnitude == chosenWeightDeltaMagnitude && candidateWeight < chosenWeight) {
        return YES;
    }
    
    return NO;
}

// Family name is somewhat of a misnomer here.  We first attempt to find an exact match
// comparing the desiredFamily to the PostScript name of the installed fonts.  If that fails
// we then do a search based on the family names of the installed fonts.
- (NSFont *)fontWithFamily:(NSString *)desiredFamily traits:(NSFontTraitMask)desiredTraits size:(float)size
{
    NSFontManager *fontManager = [NSFontManager sharedFontManager];
    NSFont *font= nil;
    
    // Look for an exact match first.
    NSEnumerator *availableFonts = [[fontManager availableFonts] objectEnumerator];
    NSString *availableFont;
    while ((availableFont = [availableFonts nextObject])) {
        if ([desiredFamily caseInsensitiveCompare:availableFont] == NSOrderedSame) {
            NSFont *nameMatchedFont = [NSFont fontWithName:availableFont size:size];
    
            // Special case Osaka-Mono.  According to <rdar://problem/3999467>, we need to 
            // treat Osaka-Mono as fixed pitch.
            if ([desiredFamily caseInsensitiveCompare:@"Osaka-Mono"] == NSOrderedSame && desiredTraits == 0)
               return nameMatchedFont;

            NSFontTraitMask traits = [fontManager traitsOfFont:nameMatchedFont];
            
            if ((traits & desiredTraits) == desiredTraits){
                font = [fontManager convertFont:nameMatchedFont toHaveTrait:desiredTraits];
                return font;
            }
            break;
        }
    }
    
    // Do a simple case insensitive search for a matching font family.
    // NSFontManager requires exact name matches.
    // This addresses the problem of matching arial to Arial, etc., but perhaps not all the issues.
    NSEnumerator *e = [[fontManager availableFontFamilies] objectEnumerator];
    NSString *availableFamily;
    while ((availableFamily = [e nextObject])) {
        if ([desiredFamily caseInsensitiveCompare:availableFamily] == NSOrderedSame) {
            break;
        }
    }
    
    // Found a family, now figure out what weight and traits to use.
    BOOL choseFont = false;
    int chosenWeight = 0;
    NSFontTraitMask chosenTraits = 0;

    NSArray *fonts = [fontManager availableMembersOfFontFamily:availableFamily];    
    unsigned n = [fonts count];
    unsigned i;
    for (i = 0; i < n; i++) {
        NSArray *fontInfo = [fonts objectAtIndex:i];
        
        // Array indices must be hard coded because of lame AppKit API.
        int fontWeight = [[fontInfo objectAtIndex:2] intValue];
        NSFontTraitMask fontTraits = [[fontInfo objectAtIndex:3] unsignedIntValue];
        
        BOOL newWinner;
        if (!choseFont) {
            newWinner = acceptableChoice(desiredTraits, DESIRED_WEIGHT, fontTraits, fontWeight);
        } else {
            newWinner = betterChoice(desiredTraits, DESIRED_WEIGHT, chosenTraits, chosenWeight, fontTraits, fontWeight);
        }

        if (newWinner) {
            choseFont = YES;
            chosenWeight = fontWeight;
            chosenTraits = fontTraits;
            
            if (chosenWeight == DESIRED_WEIGHT
                    && (chosenTraits & IMPORTANT_FONT_TRAITS) == (desiredTraits & IMPORTANT_FONT_TRAITS)) {
                break;
            }
        }
    }
    
    if (!choseFont)
        return nil;

    font = [fontManager fontWithFamily:availableFamily traits:chosenTraits weight:chosenWeight size:size];
    return font;
}

typedef struct {
    NSString *family;
    NSFontTraitMask traits;
    float size;
} FontCacheKey;

static const void *FontCacheKeyCopy(CFAllocatorRef allocator, const void *value)
{
    const FontCacheKey *key = (const FontCacheKey *)value;
    FontCacheKey *result = (FontCacheKey*)malloc(sizeof(FontCacheKey));
    result->family = [key->family copy];
    result->traits = key->traits;
    result->size = key->size;
    return result;
}

static void FontCacheKeyFree(CFAllocatorRef allocator, const void *value)
{
    const FontCacheKey *key = (const FontCacheKey *)value;
    [key->family release];
    free((void *)key);
}

static Boolean FontCacheKeyEqual(const void *value1, const void *value2)
{
    const FontCacheKey *key1 = (const FontCacheKey *)value1;
    const FontCacheKey *key2 = (const FontCacheKey *)value2;
    return key1->size == key2->size && key1->traits == key2->traits && [key1->family isEqualToString:key2->family];
}

static CFHashCode FontCacheKeyHash(const void *value)
{
    const FontCacheKey *key = (const FontCacheKey *)value;
    return [key->family hash] ^ key->traits ^ (int)key->size;
}

static const void *FontCacheValueRetain(CFAllocatorRef allocator, const void *value)
{
    if (value != 0) {
        CFRetain(value);
    }
    return value;
}

static void FontCacheValueRelease(CFAllocatorRef allocator, const void *value)
{
    if (value != 0) {
        CFRelease(value);
    }
}

- (NSFont *)cachedFontFromFamily:(NSString *)family traits:(NSFontTraitMask)traits size:(float)size
{
    assert(family);
    
    if (!fontCache) {
        static const CFDictionaryKeyCallBacks fontCacheKeyCallBacks = { 0, FontCacheKeyCopy, FontCacheKeyFree, 0, FontCacheKeyEqual, FontCacheKeyHash };
        static const CFDictionaryValueCallBacks fontCacheValueCallBacks = { 0, FontCacheValueRetain, FontCacheValueRelease, 0, 0 };
        fontCache = CFDictionaryCreateMutable(0, 0, &fontCacheKeyCallBacks, &fontCacheValueCallBacks);
    }

    const FontCacheKey fontKey = { family, traits, size };
    const void *value;
    NSFont *font;
    if (CFDictionaryGetValueIfPresent(fontCache, &fontKey, &value)) {
        font = (NSFont *)value;
    } else {
        if ([family length] == 0) {
            return nil;
        }
        font = [self fontWithFamily:family traits:traits size:size];
        CFDictionaryAddValue(fontCache, &fontKey, font);
    }
    
#ifdef DEBUG_MISSING_FONT
    static int unableToFindFontCount = 0;
    if (font == nil) {
        unableToFindFontCount++;
        NSLog(@"unableToFindFontCount %@, traits 0x%08x, size %f, %d\n", family, traits, size, unableToFindFontCount);
    }
#endif
    
    return font;
}

@end
