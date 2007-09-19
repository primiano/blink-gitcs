/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com 
 * All rights reserved.
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

#include "config.h"

#include "AXObjectCache.h"
#include "CString.h"
#include "CachedPage.h"
#include "CachedResource.h"
#include "Clipboard.h"
#include "CookieJar.h"
#include "Cursor.h"
#include "DocumentFragment.h"
#include "DocumentLoader.h"
#include "EditCommand.h"
#include "Editor.h"
#include "Font.h"
#include "FrameLoadRequest.h"
#include "FrameLoader.h"
#include "FrameView.h"
#include "FTPDirectoryDocument.h"
#include "GlobalHistory.h"
#include "GraphicsContext.h"
#include "HTMLFrameOwnerElement.h"
#include "Icon.h"
#include "IconDatabase.h"
#include "IconLoader.h"
#include "IntPoint.h"
#include "KURL.h"
#include "Language.h"
#include "MainResourceLoader.h"
#include "Node.h"
#include "NotImplemented.h"
#include "Pasteboard.h"
#include "PlatformMouseEvent.h"
#include "PlugInInfoStore.h"
#include "ResourceError.h"
#include "ResourceHandle.h"
#include "ResourceLoader.h"
#include "Screen.h"
#include "ScrollBar.h"
#include "TextBoundaries.h"
#include "TextBreakIteratorInternalICU.h"
#include "loader.h"
#include <gtk/gtk.h>
#include <stdio.h>

using namespace WebCore;

// This function loads resources from WebKit
// This does not belong here and I'm not sure where
// it should go
// I don't know what the plans or design is
// for none code resources
Vector<char> loadResourceIntoArray(const char* resourceName)
{
    Vector<char> resource;
    //if (strcmp(resourceName,"missingImage") == 0) {
    //}
    return resource;
}

namespace WebCore {
    class Page;
}

void FrameView::updateBorder() { notImplemented(); }

int WebCore::findNextWordFromIndex(UChar const*, int, int, bool) { notImplemented(); return 0; }
void WebCore::findWordBoundary(UChar const* str, int len, int position, int* start, int* end) {*start = position; *end = position; }
const char* WebCore::currentTextBreakLocaleID() { notImplemented(); return "en_us"; }


/********************************************************/
/* Completely empty stubs (mostly to allow DRT to run): */
/********************************************************/
bool AXObjectCache::gAccessibilityEnabled = false;

bool WebCore::historyContains(DeprecatedString const&) { return false; }

String WebCore::defaultLanguage() { return "en"; }

PluginInfo* PlugInInfoStore::createPluginInfoForPluginAtIndex(unsigned) { notImplemented(); return 0;}
unsigned PlugInInfoStore::pluginCount() const { notImplemented(); return 0; }
bool WebCore::PlugInInfoStore::supportsMIMEType(const WebCore::String&) { notImplemented(); return false; }
void WebCore::refreshPlugins(bool) { notImplemented(); }


Color WebCore::focusRingColor() { return 0xFF0000FF; }
void WebCore::setFocusRingColorChangeFunction(void (*)()) { }

Icon::Icon() { notImplemented(); }
Icon::~Icon() { notImplemented(); }
PassRefPtr<Icon> Icon::newIconForFile(const String& filename) { notImplemented(); return PassRefPtr<Icon>(new Icon()); }
void Icon::paint(GraphicsContext*, const IntRect&) { notImplemented(); }

FloatRect Font::selectionRectForComplexText(const TextRun&, const TextStyle&, const IntPoint&, int, int, int) const { return FloatRect(); notImplemented(); }
void Font::drawComplexText(GraphicsContext*, const TextRun&, const TextStyle&, const FloatPoint&, int from, int to) const { notImplemented(); }
float Font::floatWidthForComplexText(const TextRun&, const TextStyle&) const { notImplemented(); return 0; }
int Font::offsetForPositionForComplexText(const TextRun&, const TextStyle&, int, bool) const { notImplemented(); return 0; }

void CachedPage::close() { notImplemented(); }

PassRefPtr<Clipboard> Editor::newGeneralClipboard(ClipboardAccessPolicy) { notImplemented(); return 0; }

Pasteboard* Pasteboard::generalPasteboard() { notImplemented(); return 0; }
void Pasteboard::writeSelection(Range*, bool, Frame*) { notImplemented(); }
void Pasteboard::writeURL(const KURL&, const String&, Frame*) { notImplemented(); }
void Pasteboard::writeImage(Node*, const KURL&, const String&) { notImplemented(); }
void Pasteboard::clear() { notImplemented(); }
bool Pasteboard::canSmartReplace() { notImplemented(); return false; }
PassRefPtr<DocumentFragment> Pasteboard::documentFragment(Frame*, PassRefPtr<Range>, bool allowPlainText, bool& chosePlainText) { notImplemented(); return 0; }
String Pasteboard::plainText(Frame* frame) { notImplemented(); return String(); }
Pasteboard::Pasteboard() { notImplemented(); }
Pasteboard::~Pasteboard() { notImplemented(); }


FTPDirectoryDocument::FTPDirectoryDocument(WebCore::DOMImplementation* i, WebCore::Frame* f) : HTMLDocument(i, f) { notImplemented(); }
Tokenizer* FTPDirectoryDocument::createTokenizer() { notImplemented(); return 0; }

namespace WebCore {
Vector<String> supportedKeySizes() { notImplemented(); return Vector<String>(); }
String signedPublicKeyAndChallengeString(unsigned keySizeIndex, const String &challengeString, const KURL &url) { return String(); }
float userIdleTime() { notImplemented(); return 0.0; }
void callOnMainThread(void (*)()) { notImplemented(); }
}

