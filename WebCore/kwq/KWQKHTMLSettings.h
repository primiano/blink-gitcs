/*
 * Copyright (C) 2003 Apple Computer, Inc.  All rights reserved.
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

#ifndef KHTML_SETTINGS_H_
#define KHTML_SETTINGS_H_

#include "DeprecatedString.h"
#include "DeprecatedStringList.h"
#include "Font.h"

class KHTMLSettings
{
public:
    enum KAnimationAdvice {
        KAnimationDisabled,
        KAnimationLoopOnce,
        KAnimationEnabled
    };
    
    KHTMLSettings() { }
    
    const DeprecatedString &stdFontName() const { return _stdFontName; }
    const DeprecatedString &fixedFontName() const { return _fixedFontName; }
    const DeprecatedString &serifFontName() const { return _serifFontName; }
    const DeprecatedString &sansSerifFontName() const { return _sansSerifFontName; }
    const DeprecatedString &cursiveFontName() const { return _cursiveFontName; }
    const DeprecatedString &fantasyFontName() const { return _fantasyFontName; }

    int minFontSize() const { return _minimumFontSize; }
    int minLogicalFontSize() const { return _minimumLogicalFontSize; }
    int mediumFontSize() const { return _defaultFontSize; }
    int mediumFixedFontSize() const { return _defaultFixedFontSize; }

    static bool changeCursor() { return true; }

    static bool isFormCompletionEnabled() { return false; }
    static int maxFormCompletionItems() { return 0; }

    bool autoLoadImages() const { return _willLoadImagesAutomatically; }
    static KAnimationAdvice showAnimations() { return KAnimationEnabled; }

    bool isJavaScriptEnabled() const { return _JavaScriptEnabled; }
    bool JavaScriptCanOpenWindowsAutomatically() const { return _JavaScriptCanOpenWindowsAutomatically; }
    bool isJavaScriptEnabled(const DeprecatedString &host) const { return _JavaScriptEnabled; }
    bool isJavaEnabled() const { return _JavaEnabled; }
    bool isJavaEnabled(const DeprecatedString &host) const { return _JavaEnabled; }
    bool isPluginsEnabled() const { return _pluginsEnabled; }
    bool isPluginsEnabled(const DeprecatedString &host) const { return _pluginsEnabled; }
    
    const DeprecatedString &encoding() const { return _encoding; }

    const DeprecatedString &userStyleSheet() const { return _userStyleSheetLocation; }
    bool shouldPrintBackgrounds() const { return _shouldPrintBackgrounds; }
    bool textAreasAreResizable() const { return _textAreasAreResizable; }

    void setStdFontName(const DeprecatedString &s) { _stdFontName = s; }
    void setFixedFontName(const DeprecatedString &s) { _fixedFontName = s; }
    void setSerifFontName(const DeprecatedString &s) { _serifFontName = s; }
    void setSansSerifFontName(const DeprecatedString &s) { _sansSerifFontName = s; }
    void setCursiveFontName(const DeprecatedString &s) { _cursiveFontName = s; }
    void setFantasyFontName(const DeprecatedString &s) { _fantasyFontName = s; }
    
    void setMinFontSize(int s) { _minimumFontSize = s; }
    void setMinLogicalFontSize(int s) { _minimumLogicalFontSize = s; }
    void setMediumFontSize(int s) { _defaultFontSize = s; }
    void setMediumFixedFontSize(int s) { _defaultFixedFontSize = s; }
    
    void setAutoLoadImages(bool f) { _willLoadImagesAutomatically = f; }
    void setIsJavaScriptEnabled(bool f) { _JavaScriptEnabled = f; }
    void setIsJavaEnabled(bool f) { _JavaEnabled = f; }
    void setArePluginsEnabled(bool f) { _pluginsEnabled = f; }
    void setJavaScriptCanOpenWindowsAutomatically(bool f) { _JavaScriptCanOpenWindowsAutomatically = f; }

    void setEncoding(const DeprecatedString &s) { _encoding = s; }

    void setUserStyleSheet(const DeprecatedString &s) { _userStyleSheetLocation = s; }
    void setShouldPrintBackgrounds(bool f) { _shouldPrintBackgrounds = f; }
    void setTextAreasAreResizable(bool f) { _textAreasAreResizable = f; }
    
private:
    DeprecatedString _stdFontName;
    DeprecatedString _fixedFontName;
    DeprecatedString _serifFontName;
    DeprecatedString _sansSerifFontName;
    DeprecatedString _cursiveFontName;
    DeprecatedString _fantasyFontName;
    DeprecatedString _encoding;
    DeprecatedString _userStyleSheetLocation;
    
    int _minimumFontSize;
    int _minimumLogicalFontSize;
    int _defaultFontSize;
    int _defaultFixedFontSize;
    bool _JavaEnabled : 1;
    bool _willLoadImagesAutomatically : 1;
    bool _pluginsEnabled : 1;
    bool _JavaScriptEnabled : 1;
    bool _JavaScriptCanOpenWindowsAutomatically : 1;
    bool _shouldPrintBackgrounds : 1;
    bool _textAreasAreResizable : 1;
    
};

#endif
