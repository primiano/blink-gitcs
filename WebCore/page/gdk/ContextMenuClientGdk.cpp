/*
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
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
#include "ContextMenuClientGdk.h"

#include "HitTestResult.h"
#include "KURL.h"

#include <stdio.h>

#define notImplemented() do { fprintf(stderr, "FIXME: UNIMPLEMENTED %s %s:%d\n", __PRETTY_FUNCTION__, __FILE__, __LINE__); } while(0)

namespace WebCore {
    
void ContextMenuClientGdk::contextMenuDestroyed()
{
    notImplemented();
}

PlatformMenuDescription ContextMenuClientGdk::getCustomMenuFromDefaultItems(ContextMenu*)
{
    notImplemented();
    return PlatformMenuDescription();
}

void ContextMenuClientGdk::contextMenuItemSelected(ContextMenuItem*, const ContextMenu*)
{
    notImplemented();
}

void ContextMenuClientGdk::downloadURL(const KURL& url)
{
    notImplemented();
}

void ContextMenuClientGdk::copyImageToClipboard(const HitTestResult&)
{
    notImplemented();
}

void ContextMenuClientGdk::searchWithGoogle(const Frame*)
{
    notImplemented();
}

void ContextMenuClientGdk::lookUpInDictionary(Frame*)
{
    notImplemented();
}

void ContextMenuClientGdk::speak(const String&)
{
    notImplemented();
}

void ContextMenuClientGdk::stopSpeaking()
{
    notImplemented();
}

}

