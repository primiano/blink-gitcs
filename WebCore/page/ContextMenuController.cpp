/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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
#include "ContextMenuController.h"

#include "Chrome.h"
#include "ContextMenu.h"
#include "ContextMenuClient.h"
#include "Document.h"
#include "DocumentFragment.h"
#include "DocumentLoader.h"
#include "Editor.h"
#include "EditorClient.h"
#include "Event.h"
#include "EventNames.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoadRequest.h"
#include "HitTestRequest.h"
#include "HitTestResult.h"
#include "KURL.h"
#include "MouseEvent.h"
#include "Node.h"
#include "Page.h"
#include "RenderLayer.h"
#include "RenderObject.h"
#include "ReplaceSelectionCommand.h"
#include "ResourceRequest.h"
#include "SelectionController.h"
#include "markup.h"

namespace WebCore {

using namespace EventNames;

ContextMenuController::ContextMenuController(Page* page, ContextMenuClient* client)
    : m_page(page)
    , m_client(client)
    , m_contextMenu(0)
{
}

ContextMenuController::~ContextMenuController()
{
    m_client->contextMenuDestroyed();
}

void ContextMenuController::handleContextMenuEvent(Event* event)
{
    ASSERT(event->type() == contextmenuEvent);
    MouseEvent* mouseEvent = static_cast<MouseEvent*>(event);
    HitTestResult result(IntPoint(mouseEvent->pageX(), mouseEvent->pageY()));

    if (RenderObject* renderer = event->target()->toNode()->renderer())
        if (RenderLayer* layer = renderer->enclosingLayer())
            layer->hitTest(HitTestRequest(false, true), result);

    if (!result.innerNonSharedNode()) {
        Frame* mainFrame = m_page->mainFrame();
        if (!mainFrame)
            return;
        Document* document = mainFrame->document();
        if (!document)
            return;
        result.setInnerNonSharedNode(document);
        result.setInnerNode(document);
    }

    m_contextMenu.set(new ContextMenu(result));
    m_contextMenu->populate();
    PlatformMenuDescription customMenu = m_client->getCustomMenuFromDefaultItems(m_contextMenu.get());
    m_contextMenu->setPlatformDescription(customMenu);
    m_contextMenu->show();

    event->setDefaultHandled();
}

static String makeGoogleSearchURL(String searchString)
{
    searchString.stripWhiteSpace();
    DeprecatedString encoded = KURL::encode_string(searchString.deprecatedString());
    encoded.replace(DeprecatedString("%20"), DeprecatedString("+"));
    
    String url("http://www.google.com/search?client=safari&q=");
    url.append(String(encoded));
    url.append("&ie=UTF-8&oe=UTF-8");
    return url;
}

static void openNewWindow(const KURL& urlToLoad, const Frame* frame)
{
    Page* newPage = frame->page()->chrome()->createWindow(FrameLoadRequest(ResourceRequest(urlToLoad, 
        frame->loader()->outgoingReferrer())));
    if (newPage)
        newPage->chrome()->show();
}

void ContextMenuController::contextMenuItemSelected(ContextMenuItem* item)
{
    ASSERT(item->type() == ActionType);

    if (item->action() >= ContextMenuItemBaseApplicationTag) {
        m_client->contextMenuItemSelected(item, m_contextMenu.get());
        return;
    }

    HitTestResult result = m_contextMenu->hitTestResult();
    Frame* frame = result.innerNonSharedNode()->document()->frame();
    if (!frame)
        return;
    ASSERT(m_page == frame->page());
    
    switch (item->action()) {
        case ContextMenuItemTagOpenLinkInNewWindow: 
            openNewWindow(result.absoluteLinkURL(), frame);
            break;
        case ContextMenuItemTagDownloadLinkToDisk:
            // FIXME: Some day we should be able to do this from within WebCore.
            m_client->downloadURL(result.absoluteLinkURL());
            break;
        case ContextMenuItemTagCopyLinkToClipboard:
            frame->editor()->copyURL(result.absoluteLinkURL(), result.textContent());
            break;
        case ContextMenuItemTagOpenImageInNewWindow:
            openNewWindow(result.absoluteImageURL(), frame);
            break;
        case ContextMenuItemTagDownloadImageToDisk:
            // FIXME: Some day we should be able to do this from within WebCore.
            m_client->downloadURL(result.absoluteImageURL());
            break;
        case ContextMenuItemTagCopyImageToClipboard:
            // FIXME: The Pasteboard class is not written yet
            // For now, call into the client. This is temporary!
            m_client->copyImageToClipboard(result);
            break;
        case ContextMenuItemTagOpenFrameInNewWindow: {
            // FIXME: The DocumentLoader is all-Mac right now
#if PLATFORM(MAC)
            KURL unreachableURL = frame->loader()->documentLoader()->unreachableURL();
            if (frame && unreachableURL.isEmpty())
                unreachableURL = frame->loader()->documentLoader()->URL();
            openNewWindow(unreachableURL, frame);
#endif
            break;
        }
        case ContextMenuItemTagCopy:
            frame->editor()->copy();
            break;
        case ContextMenuItemTagGoBack:
            frame->loader()->goBackOrForward(-1);
            break;
        case ContextMenuItemTagGoForward:
            frame->loader()->goBackOrForward(1);
            break;
        case ContextMenuItemTagStop:
            frame->loader()->stop();
            break;
        case ContextMenuItemTagReload:
            frame->loader()->reload();
            break;
        case ContextMenuItemTagCut:
            frame->editor()->cut();
            break;
        case ContextMenuItemTagPaste:
            frame->editor()->paste();
            break;
        case ContextMenuItemTagSpellingGuess:
            ASSERT(frame->selectedText().length());
            if (frame->editor()->shouldInsertText(item->title(), frame->selectionController()->toRange().get(),
                EditorInsertActionPasted)) {
                Document* document = frame->document();
                applyCommand(new ReplaceSelectionCommand(document, createFragmentFromMarkup(document, item->title(), ""),
                    true, false, true));
                frame->revealSelection(RenderLayer::gAlignToEdgeIfNeeded);
            }
            break;
        case ContextMenuItemTagIgnoreSpelling:
            frame->editor()->ignoreSpelling();
            break;
        case ContextMenuItemTagLearnSpelling:
            frame->editor()->learnSpelling();
            break;
        case ContextMenuItemTagSearchWeb: {
            String url = makeGoogleSearchURL(frame->selectedText());
            ResourceRequest request = ResourceRequest(url);
            frame->loader()->urlSelected(FrameLoadRequest(request), new Event());
            break;
        }
        case ContextMenuItemTagLookUpInDictionary:
            // FIXME: Some day we may be able to do this from within WebCore.
            m_client->lookUpInDictionary(frame);
            break;
        case ContextMenuItemTagOpenLink: {
            if (Frame* targetFrame = result.targetFrame())
                targetFrame->loader()->load(FrameLoadRequest(ResourceRequest(result.absoluteLinkURL(), 
                    frame->loader()->outgoingReferrer())), true, new Event(), 0, HashMap<String, String>());
            else
                openNewWindow(result.absoluteLinkURL(), frame);
        }
        case ContextMenuItemTagBold:
            frame->editor()->execCommand("ToggleBold");
            break;
        case ContextMenuItemTagItalic:
            frame->editor()->execCommand("ToggleItalic");
            break;
        case ContextMenuItemTagUnderline:
            frame->editor()->toggleUnderline();
            break;
        case ContextMenuItemTagOutline:
            // We actually never enable this because CSS does not have a way to specify an outline font,
            // which may make this difficult to implement. Maybe a special case of text-shadow?
            break;
        case ContextMenuItemTagStartSpeaking: {
            ExceptionCode ec;
            RefPtr<Range> selectedRange = frame->selectionController()->toRange();
            if (!selectedRange || selectedRange->collapsed(ec)) {
                Document* document = result.innerNonSharedNode()->document();
                selectedRange = document->createRange();
                selectedRange->selectNode(document->documentElement(), ec);
            }
            m_client->speak(selectedRange->toString(true, ec));
            break;
        }
        case ContextMenuItemTagStopSpeaking:
            m_client->stopSpeaking();
            break;
        case ContextMenuItemTagDefaultDirection:
            frame->editor()->setBaseWritingDirection("inherit");
            break;
        case ContextMenuItemTagLeftToRight:
            frame->editor()->setBaseWritingDirection("ltr");
            break;
        case ContextMenuItemTagRightToLeft:
            frame->editor()->setBaseWritingDirection("rtl");
            break;
#if PLATFORM(MAC)
        case ContextMenuItemTagSearchInSpotlight:
            m_client->searchWithSpotlight();
            break;
        case ContextMenuItemTagShowSpellingPanel:
            frame->editor()->showSpellingGuessPanel();
            break;
        case ContextMenuItemTagCheckSpelling:
            frame->editor()->advanceToNextMisspelling();
            break;
        case ContextMenuItemTagCheckSpellingWhileTyping:
            frame->editor()->toggleContinuousSpellChecking();
            break;
#ifndef BUILDING_ON_TIGER
        case ContextMenuItemTagCheckGrammarWithSpelling:
            frame->editor()->toggleGrammarChecking();
            break;
#endif
        case ContextMenuItemTagShowFonts:
            frame->editor()->showFontPanel();
            break;
        case ContextMenuItemTagStyles:
            frame->editor()->showStylesPanel();
            break;
        case ContextMenuItemTagShowColors:
            frame->editor()->showColorPanel();
            break;
#endif
        default:
            break;
    }
}

} // namespace WebCore

