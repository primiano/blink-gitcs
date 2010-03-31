/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "FrameLoaderClientImpl.h"

#include "Chrome.h"
#include "Document.h"
#include "DocumentLoader.h"
#include "FormState.h"
#include "FrameLoader.h"
#include "FrameLoadRequest.h"
#include "HTTPParsers.h"
#include "HistoryItem.h"
#include "HitTestResult.h"
#include "HTMLAppletElement.h"
#include "HTMLFormElement.h"  // needed by FormState.h
#include "HTMLNames.h"
#include "MIMETypeRegistry.h"
#include "MouseEvent.h"
#include "Page.h"
#include "PlatformString.h"
#include "PluginData.h"
#include "PluginDataChromium.h"
#include "StringExtras.h"
#include "WebDataSourceImpl.h"
#include "WebDevToolsAgentPrivate.h"
#include "WebFormElement.h"
#include "WebFrameClient.h"
#include "WebFrameImpl.h"
#include "WebKit.h"
#include "WebKitClient.h"
#include "WebMimeRegistry.h"
#include "WebNode.h"
#include "WebPlugin.h"
#include "WebPluginContainerImpl.h"
#include "WebPluginLoadObserver.h"
#include "WebPluginParams.h"
#include "WebSecurityOrigin.h"
#include "WebURL.h"
#include "WebURLError.h"
#include "WebVector.h"
#include "WebViewClient.h"
#include "WebViewImpl.h"
#include "WindowFeatures.h"
#include "WrappedResourceRequest.h"
#include "WrappedResourceResponse.h"
#include <wtf/text/CString.h>

using namespace WebCore;

namespace WebKit {

// Domain for internal error codes.
static const char internalErrorDomain[] = "WebKit";

// An internal error code.  Used to note a policy change error resulting from
// dispatchDecidePolicyForMIMEType not passing the PolicyUse option.
enum {
    PolicyChangeError = -10000,
};

FrameLoaderClientImpl::FrameLoaderClientImpl(WebFrameImpl* frame)
    : m_webFrame(frame)
    , m_hasRepresentation(false)
    , m_sentInitialResponseToPlugin(false)
    , m_nextNavigationPolicy(WebNavigationPolicyIgnore)
{
}

FrameLoaderClientImpl::~FrameLoaderClientImpl()
{
}

void FrameLoaderClientImpl::frameLoaderDestroyed()
{
    // When the WebFrame was created, it had an extra reference given to it on
    // behalf of the Frame.  Since the WebFrame owns us, this extra ref also
    // serves to keep us alive until the FrameLoader is done with us.  The
    // FrameLoader calls this method when it's going away.  Therefore, we balance
    // out that extra reference, which may cause 'this' to be deleted.
    m_webFrame->closing();
    m_webFrame->deref();
}

void FrameLoaderClientImpl::dispatchDidClearWindowObjectInWorld(DOMWrapperWorld*)
{
    if (m_webFrame->client())
        m_webFrame->client()->didClearWindowObject(m_webFrame);

    WebViewImpl* webview = m_webFrame->viewImpl();
    if (webview->devToolsAgentPrivate())
        webview->devToolsAgentPrivate()->didClearWindowObject(m_webFrame);
}

void FrameLoaderClientImpl::documentElementAvailable()
{
    if (m_webFrame->client())
        m_webFrame->client()->didCreateDocumentElement(m_webFrame);
}

void FrameLoaderClientImpl::didCreateScriptContextForFrame()
{
    if (m_webFrame->client())
        m_webFrame->client()->didCreateScriptContext(m_webFrame);
}

void FrameLoaderClientImpl::didDestroyScriptContextForFrame()
{
    if (m_webFrame->client())
        m_webFrame->client()->didDestroyScriptContext(m_webFrame);
}

void FrameLoaderClientImpl::didCreateIsolatedScriptContext()
{
    if (m_webFrame->client())
        m_webFrame->client()->didCreateIsolatedScriptContext(m_webFrame);
}

void FrameLoaderClientImpl::didPerformFirstNavigation() const
{
}

void FrameLoaderClientImpl::registerForIconNotification(bool)
{
}

void FrameLoaderClientImpl::didChangeScrollOffset()
{
    if (m_webFrame->client())
        m_webFrame->client()->didChangeScrollOffset(m_webFrame);
}

bool FrameLoaderClientImpl::allowJavaScript(bool enabledPerSettings)
{
    if (m_webFrame->client())
        return m_webFrame->client()->allowScript(m_webFrame, enabledPerSettings);

    return enabledPerSettings;
}

bool FrameLoaderClientImpl::allowPlugins(bool enabledPerSettings)
{
    if (m_webFrame->client())
        return m_webFrame->client()->allowPlugins(m_webFrame, enabledPerSettings);

    return enabledPerSettings;
}

bool FrameLoaderClientImpl::allowImages(bool enabledPerSettings)
{
    if (m_webFrame->client())
        return m_webFrame->client()->allowImages(m_webFrame, enabledPerSettings);

    return enabledPerSettings;
}

void FrameLoaderClientImpl::didNotAllowScript()
{
    if (m_webFrame->client())
        m_webFrame->client()->didNotAllowScript(m_webFrame);
}

void FrameLoaderClientImpl::didNotAllowPlugins()
{
    if (m_webFrame->client())
        m_webFrame->client()->didNotAllowPlugins(m_webFrame);
}

bool FrameLoaderClientImpl::hasWebView() const
{
    return m_webFrame->viewImpl();
}

bool FrameLoaderClientImpl::hasFrameView() const
{
    // The Mac port has this notion of a WebFrameView, which seems to be
    // some wrapper around an NSView.  Since our equivalent is HWND, I guess
    // we have a "frameview" whenever we have the toplevel HWND.
    return m_webFrame->viewImpl();
}

void FrameLoaderClientImpl::makeDocumentView()
{
    m_webFrame->createFrameView();
}

void FrameLoaderClientImpl::makeRepresentation(DocumentLoader*)
{
    m_hasRepresentation = true;
}

void FrameLoaderClientImpl::forceLayout()
{
    // FIXME
}

void FrameLoaderClientImpl::forceLayoutForNonHTML()
{
    // FIXME
}

void FrameLoaderClientImpl::setCopiesOnScroll()
{
    // FIXME
}

void FrameLoaderClientImpl::detachedFromParent2()
{
    // Nothing to do here.
}

void FrameLoaderClientImpl::detachedFromParent3()
{
    // Close down the proxy.  The purpose of this change is to make the
    // call to ScriptController::clearWindowShell a no-op when called from
    // Frame::pageDestroyed.  Without this change, this call to clearWindowShell
    // will cause a crash.  If you remove/modify this, just ensure that you can
    // go to a page and then navigate to a new page without getting any asserts
    // or crashes.
    m_webFrame->frame()->script()->proxy()->clearForClose();
    
    // Stop communicating with the WebFrameClient at this point since we are no
    // longer associated with the Page.
    m_webFrame->setClient(0);
}

// This function is responsible for associating the |identifier| with a given
// subresource load.  The following functions that accept an |identifier| are
// called for each subresource, so they should not be dispatched to the
// WebFrame.
void FrameLoaderClientImpl::assignIdentifierToInitialRequest(
    unsigned long identifier, DocumentLoader* loader,
    const ResourceRequest& request)
{
    if (m_webFrame->client()) {
        WrappedResourceRequest webreq(request);
        m_webFrame->client()->assignIdentifierToRequest(
            m_webFrame, identifier, webreq);
    }
}

// If the request being loaded by |loader| is a frame, update the ResourceType.
// A subresource in this context is anything other than a frame --
// this includes images and xmlhttp requests.  It is important to note that a
// subresource is NOT limited to stuff loaded through the frame's subresource
// loader. Synchronous xmlhttp requests for example, do not go through the
// subresource loader, but we still label them as TargetIsSubResource.
//
// The important edge cases to consider when modifying this function are
// how synchronous resource loads are treated during load/unload threshold.
static void setTargetTypeFromLoader(ResourceRequest& request, DocumentLoader* loader)
{
    if (loader == loader->frameLoader()->provisionalDocumentLoader()) {
        ResourceRequest::TargetType type;
        if (loader->frameLoader()->isLoadingMainFrame())
            type = ResourceRequest::TargetIsMainFrame;
        else
            type = ResourceRequest::TargetIsSubframe;
        request.setTargetType(type);
    }
}

void FrameLoaderClientImpl::dispatchWillSendRequest(
    DocumentLoader* loader, unsigned long identifier, ResourceRequest& request,
    const ResourceResponse& redirectResponse)
{
    if (loader) {
        // We want to distinguish between a request for a document to be loaded into
        // the main frame, a sub-frame, or the sub-objects in that document.
        setTargetTypeFromLoader(request, loader);

        // Avoid repeating a form submission when navigating back or forward.
        if (loader == loader->frameLoader()->provisionalDocumentLoader()
            && request.httpMethod() == "POST"
            && isBackForwardLoadType(loader->frameLoader()->loadType()))
            request.setCachePolicy(ReturnCacheDataDontLoad);
    }

    // FrameLoader::loadEmptyDocumentSynchronously() creates an empty document
    // with no URL.  We don't like that, so we'll rename it to about:blank.
    if (request.url().isEmpty())
        request.setURL(KURL(ParsedURLString, "about:blank"));
    if (request.firstPartyForCookies().isEmpty())
        request.setFirstPartyForCookies(KURL(ParsedURLString, "about:blank"));

    // Give the WebFrameClient a crack at the request.
    if (m_webFrame->client()) {
        WrappedResourceRequest webreq(request);
        WrappedResourceResponse webresp(redirectResponse);
        m_webFrame->client()->willSendRequest(
            m_webFrame, identifier, webreq, webresp);
    }
}

bool FrameLoaderClientImpl::shouldUseCredentialStorage(
    DocumentLoader*, unsigned long identifier)
{
    // FIXME
    // Intended to pass through to a method on the resource load delegate.
    // If implemented, that method controls whether the browser should ask the
    // networking layer for a stored default credential for the page (say from
    // the Mac OS keychain). If the method returns false, the user should be
    // presented with an authentication challenge whether or not the networking
    // layer has a credential stored.
    // This returns true for backward compatibility: the ability to override the
    // system credential store is new. (Actually, not yet fully implemented in
    // WebKit, as of this writing.)
    return true;
}

void FrameLoaderClientImpl::dispatchDidReceiveAuthenticationChallenge(
    DocumentLoader*, unsigned long identifier, const AuthenticationChallenge&)
{
    // FIXME
}

void FrameLoaderClientImpl::dispatchDidCancelAuthenticationChallenge(
    DocumentLoader*, unsigned long identifier, const AuthenticationChallenge&)
{
    // FIXME
}

void FrameLoaderClientImpl::dispatchDidReceiveResponse(DocumentLoader* loader,
                                                       unsigned long identifier,
                                                       const ResourceResponse& response)
{
    if (m_webFrame->client()) {
        WrappedResourceResponse webresp(response);
        m_webFrame->client()->didReceiveResponse(m_webFrame, identifier, webresp);
    }
}

void FrameLoaderClientImpl::dispatchDidReceiveContentLength(
    DocumentLoader* loader,
    unsigned long identifier,
    int lengthReceived)
{
}

// Called when a particular resource load completes
void FrameLoaderClientImpl::dispatchDidFinishLoading(DocumentLoader* loader,
                                                    unsigned long identifier)
{
    if (m_webFrame->client())
        m_webFrame->client()->didFinishResourceLoad(m_webFrame, identifier);
}

void FrameLoaderClientImpl::dispatchDidFailLoading(DocumentLoader* loader,
                                                  unsigned long identifier,
                                                  const ResourceError& error)
{
    if (m_webFrame->client())
        m_webFrame->client()->didFailResourceLoad(m_webFrame, identifier, error);
}

void FrameLoaderClientImpl::dispatchDidFinishDocumentLoad()
{
    // A frame may be reused.  This call ensures we don't hold on to our password
    // listeners and their associated HTMLInputElements.
    m_webFrame->clearPasswordListeners();

    if (m_webFrame->client())
        m_webFrame->client()->didFinishDocumentLoad(m_webFrame);
}

bool FrameLoaderClientImpl::dispatchDidLoadResourceFromMemoryCache(
    DocumentLoader* loader,
    const ResourceRequest& request,
    const ResourceResponse& response,
    int length)
{
    if (m_webFrame->client()) {
        WrappedResourceRequest webreq(request);
        WrappedResourceResponse webresp(response);
        m_webFrame->client()->didLoadResourceFromMemoryCache(
            m_webFrame, webreq, webresp);
    }
    return false;  // Do not suppress remaining notifications
}

void FrameLoaderClientImpl::dispatchDidLoadResourceByXMLHttpRequest(
    unsigned long identifier,
    const ScriptString& source)
{
}

void FrameLoaderClientImpl::dispatchDidHandleOnloadEvents()
{
    if (m_webFrame->client())
        m_webFrame->client()->didHandleOnloadEvents(m_webFrame);
}

// Redirect Tracking
// =================
// We want to keep track of the chain of redirects that occur during page
// loading. There are two types of redirects, server redirects which are HTTP
// response codes, and client redirects which are document.location= and meta
// refreshes.
//
// This outlines the callbacks that we get in different redirect situations,
// and how each call modifies the redirect chain.
//
// Normal page load
// ----------------
//   dispatchDidStartProvisionalLoad() -> adds URL to the redirect list
//   dispatchDidCommitLoad()           -> DISPATCHES & clears list
//
// Server redirect (success)
// -------------------------
//   dispatchDidStartProvisionalLoad()                    -> adds source URL
//   dispatchDidReceiveServerRedirectForProvisionalLoad() -> adds dest URL
//   dispatchDidCommitLoad()                              -> DISPATCHES
//
// Client redirect (success)
// -------------------------
//   (on page)
//   dispatchWillPerformClientRedirect() -> saves expected redirect
//   dispatchDidStartProvisionalLoad()   -> appends redirect source (since
//                                          it matches the expected redirect)
//                                          and the current page as the dest)
//   dispatchDidCancelClientRedirect()   -> clears expected redirect
//   dispatchDidCommitLoad()             -> DISPATCHES
//
// Client redirect (cancelled)
// (e.g meta-refresh trumped by manual doc.location change, or just cancelled
// because a link was clicked that requires the meta refresh to be rescheduled
// (the SOURCE URL may have changed).
// ---------------------------
//   dispatchDidCancelClientRedirect()                 -> clears expected redirect
//   dispatchDidStartProvisionalLoad()                 -> adds only URL to redirect list
//   dispatchDidCommitLoad()                           -> DISPATCHES & clears list
//   rescheduled ? dispatchWillPerformClientRedirect() -> saves expected redirect
//               : nothing

// Client redirect (failure)
// -------------------------
//   (on page)
//   dispatchWillPerformClientRedirect() -> saves expected redirect
//   dispatchDidStartProvisionalLoad()   -> appends redirect source (since
//                                          it matches the expected redirect)
//                                          and the current page as the dest)
//   dispatchDidCancelClientRedirect()
//   dispatchDidFailProvisionalLoad()
//
// Load 1 -> Server redirect to 2 -> client redirect to 3 -> server redirect to 4
// ------------------------------------------------------------------------------
//   dispatchDidStartProvisionalLoad()                    -> adds source URL 1
//   dispatchDidReceiveServerRedirectForProvisionalLoad() -> adds dest URL 2
//   dispatchDidCommitLoad()                              -> DISPATCHES 1+2
//    -- begin client redirect and NEW DATA SOURCE
//   dispatchWillPerformClientRedirect()                  -> saves expected redirect
//   dispatchDidStartProvisionalLoad()                    -> appends URL 2 and URL 3
//   dispatchDidReceiveServerRedirectForProvisionalLoad() -> appends destination URL 4
//   dispatchDidCancelClientRedirect()                    -> clears expected redirect
//   dispatchDidCommitLoad()                              -> DISPATCHES
//
// Interesting case with multiple location changes involving anchors.
// Load page 1 containing future client-redirect (back to 1, e.g meta refresh) > Click
// on a link back to the same page (i.e an anchor href) >
// client-redirect finally fires (with new source, set to 1#anchor)
// -----------------------------------------------------------------------------
//   dispatchWillPerformClientRedirect(non-zero 'interval' param) -> saves expected redirect
//   -- click on anchor href
//   dispatchDidCancelClientRedirect()                            -> clears expected redirect
//   dispatchDidStartProvisionalLoad()                            -> adds 1#anchor source
//   dispatchDidCommitLoad()                                      -> DISPATCHES 1#anchor
//   dispatchWillPerformClientRedirect()                          -> saves exp. source (1#anchor)
//   -- redirect timer fires
//   dispatchDidStartProvisionalLoad()                            -> appends 1#anchor (src) and 1 (dest)
//   dispatchDidCancelClientRedirect()                            -> clears expected redirect
//   dispatchDidCommitLoad()                                      -> DISPATCHES 1#anchor + 1
//
void FrameLoaderClientImpl::dispatchDidReceiveServerRedirectForProvisionalLoad()
{
    WebDataSourceImpl* ds = m_webFrame->provisionalDataSourceImpl();
    if (!ds) {
        // Got a server redirect when there is no provisional DS!
        ASSERT_NOT_REACHED();
        return;
    }

    // The server redirect may have been blocked.
    if (ds->request().isNull())
        return;

    // A provisional load should have started already, which should have put an
    // entry in our redirect chain.
    ASSERT(ds->hasRedirectChain());

    // The URL of the destination is on the provisional data source. We also need
    // to update the redirect chain to account for this addition (we do this
    // before the callback so the callback can look at the redirect chain to see
    // what happened).
    ds->appendRedirect(ds->request().url());

    if (m_webFrame->client())
        m_webFrame->client()->didReceiveServerRedirectForProvisionalLoad(m_webFrame);
}

// Called on both success and failure of a client redirect.
void FrameLoaderClientImpl::dispatchDidCancelClientRedirect()
{
    // No longer expecting a client redirect.
    if (m_webFrame->client()) {
        m_expectedClientRedirectSrc = KURL();
        m_expectedClientRedirectDest = KURL();
        m_webFrame->client()->didCancelClientRedirect(m_webFrame);
    }

    // No need to clear the redirect chain, since that data source has already
    // been deleted by the time this function is called.
}

void FrameLoaderClientImpl::dispatchWillPerformClientRedirect(
    const KURL& url,
    double interval,
    double fireDate)
{
    // Tells dispatchDidStartProvisionalLoad that if it sees this item it is a
    // redirect and the source item should be added as the start of the chain.
    m_expectedClientRedirectSrc = m_webFrame->url();
    m_expectedClientRedirectDest = url;

    // FIXME: bug 1135512. Webkit does not properly notify us of cancelling
    // http > file client redirects. Since the FrameLoader's policy is to never
    // carry out such a navigation anyway, the best thing we can do for now to
    // not get confused is ignore this notification.
    if (m_expectedClientRedirectDest.isLocalFile()
        && m_expectedClientRedirectSrc.protocolInHTTPFamily()) {
        m_expectedClientRedirectSrc = KURL();
        m_expectedClientRedirectDest = KURL();
        return;
    }

    if (m_webFrame->client()) {
        m_webFrame->client()->willPerformClientRedirect(
            m_webFrame,
            m_expectedClientRedirectSrc,
            m_expectedClientRedirectDest,
            static_cast<unsigned int>(interval),
            static_cast<unsigned int>(fireDate));
    }
}

void FrameLoaderClientImpl::dispatchDidNavigateWithinPage()
{
    // Anchor fragment navigations are not normal loads, so we need to synthesize
    // some events for our delegate.
    WebViewImpl* webView = m_webFrame->viewImpl();

    // Flag of whether frame loader is completed. Generate didStartLoading and
    // didStopLoading only when loader is completed so that we don't fire
    // them for fragment redirection that happens in window.onload handler.
    // See https://bugs.webkit.org/show_bug.cgi?id=31838
    bool loaderCompleted =
        !webView->page()->mainFrame()->loader()->activeDocumentLoader()->isLoadingInAPISense();

    // Generate didStartLoading if loader is completed.
    if (webView->client() && loaderCompleted)
        webView->client()->didStartLoading();

    // We need to classify some hash changes as client redirects.
    // FIXME: It seems wrong that the currentItem can sometimes be null.
    HistoryItem* currentItem = m_webFrame->frame()->loader()->history()->currentItem();
    bool isHashChange = !currentItem || !currentItem->stateObject();

    WebDataSourceImpl* ds = m_webFrame->dataSourceImpl();
    ASSERT(ds);  // Should not be null when navigating to a reference fragment!
    if (ds) {
        KURL url = ds->request().url();
        KURL chainEnd;
        if (ds->hasRedirectChain()) {
            chainEnd = ds->endOfRedirectChain();
            ds->clearRedirectChain();
        }

        if (isHashChange) {
            // Figure out if this location change is because of a JS-initiated
            // client redirect (e.g onload/setTimeout document.location.href=).
            // FIXME: (b/1085325, b/1046841) We don't get proper redirect
            // performed/cancelled notifications across anchor navigations, so the
            // other redirect-tracking code in this class (see
            // dispatch*ClientRedirect() and dispatchDidStartProvisionalLoad) is
            // insufficient to catch and properly flag these transitions. Once a
            // proper fix for this bug is identified and applied the following
            // block may no longer be required.
            bool wasClientRedirect =
                (url == m_expectedClientRedirectDest && chainEnd == m_expectedClientRedirectSrc)
                || !m_webFrame->isProcessingUserGesture();

            if (wasClientRedirect) {
                if (m_webFrame->client())
                    m_webFrame->client()->didCompleteClientRedirect(m_webFrame, chainEnd);
                ds->appendRedirect(chainEnd);
                // Make sure we clear the expected redirect since we just effectively
                // completed it.
                m_expectedClientRedirectSrc = KURL();
                m_expectedClientRedirectDest = KURL();
            }
        }

        // Regardless of how we got here, we are navigating to a URL so we need to
        // add it to the redirect chain.
        ds->appendRedirect(url);
    }

    bool isNewNavigation;
    webView->didCommitLoad(&isNewNavigation);
    if (m_webFrame->client()) {
        m_webFrame->client()->didNavigateWithinPage(m_webFrame, isNewNavigation);

        // FIXME: Remove this notification once it is no longer consumed downstream.
        if (isHashChange)
            m_webFrame->client()->didChangeLocationWithinPage(m_webFrame, isNewNavigation);
    }

    // Generate didStopLoading if loader is completed.
    if (webView->client() && loaderCompleted)
        webView->client()->didStopLoading();
}

void FrameLoaderClientImpl::dispatchDidChangeLocationWithinPage()
{
    if (m_webFrame)
        m_webFrame->client()->didChangeLocationWithinPage(m_webFrame);
}

void FrameLoaderClientImpl::dispatchDidPushStateWithinPage()
{
    dispatchDidNavigateWithinPage();
}

void FrameLoaderClientImpl::dispatchDidReplaceStateWithinPage()
{
    dispatchDidNavigateWithinPage();
}

void FrameLoaderClientImpl::dispatchDidPopStateWithinPage()
{
    // Ignored since dispatchDidNavigateWithinPage was already called.
}

void FrameLoaderClientImpl::dispatchWillClose()
{
    if (m_webFrame->client())
        m_webFrame->client()->willClose(m_webFrame);
}

void FrameLoaderClientImpl::dispatchDidReceiveIcon()
{
    // The icon database is disabled, so this should never be called.
    ASSERT_NOT_REACHED();
}

void FrameLoaderClientImpl::dispatchDidStartProvisionalLoad()
{
    // In case a redirect occurs, we need this to be set so that the redirect
    // handling code can tell where the redirect came from. Server redirects
    // will occur on the provisional load, so we need to keep track of the most
    // recent provisional load URL.
    // See dispatchDidReceiveServerRedirectForProvisionalLoad.
    WebDataSourceImpl* ds = m_webFrame->provisionalDataSourceImpl();
    if (!ds) {
        ASSERT_NOT_REACHED();
        return;
    }
    KURL url = ds->request().url();

    // Since the provisional load just started, we should have not gotten
    // any redirects yet.
    ASSERT(!ds->hasRedirectChain());

    // If this load is what we expected from a client redirect, treat it as a
    // redirect from that original page. The expected redirect urls will be
    // cleared by DidCancelClientRedirect.
    bool completingClientRedirect = false;
    if (m_expectedClientRedirectSrc.isValid()) {
        // m_expectedClientRedirectDest could be something like
        // "javascript:history.go(-1)" thus we need to exclude url starts with
        // "javascript:". See bug: 1080873
        ASSERT(m_expectedClientRedirectDest.protocolIs("javascript")
            || m_expectedClientRedirectDest == url);
        ds->appendRedirect(m_expectedClientRedirectSrc);
        completingClientRedirect = true;
    }
    ds->appendRedirect(url);

    if (m_webFrame->client()) {
        // Whatever information didCompleteClientRedirect contains should only
        // be considered relevant until the next provisional load has started.
        // So we first tell the client that the load started, and then tell it
        // about the client redirect the load is responsible for completing.
        m_webFrame->client()->didStartProvisionalLoad(m_webFrame);
        if (completingClientRedirect) {
            m_webFrame->client()->didCompleteClientRedirect(
                m_webFrame, m_expectedClientRedirectSrc);
        }
    }
}

void FrameLoaderClientImpl::dispatchDidReceiveTitle(const String& title)
{
    if (m_webFrame->client())
        m_webFrame->client()->didReceiveTitle(m_webFrame, title);
}

void FrameLoaderClientImpl::dispatchDidCommitLoad()
{
    WebViewImpl* webview = m_webFrame->viewImpl();
    bool isNewNavigation;
    webview->didCommitLoad(&isNewNavigation);

    if (m_webFrame->client())
        m_webFrame->client()->didCommitProvisionalLoad(m_webFrame, isNewNavigation);

    if (webview->devToolsAgentPrivate())
        webview->devToolsAgentPrivate()->didCommitProvisionalLoad(m_webFrame, isNewNavigation);
}

void FrameLoaderClientImpl::dispatchDidFailProvisionalLoad(
    const ResourceError& error)
{

    // If a policy change occured, then we do not want to inform the plugin
    // delegate.  See http://b/907789 for details.  FIXME: This means the
    // plugin won't receive NPP_URLNotify, which seems like it could result in
    // a memory leak in the plugin!!
    if (error.domain() == internalErrorDomain
        && error.errorCode() == PolicyChangeError) {
        m_webFrame->didFail(cancelledError(error.failingURL()), true);
        return;
    }

    OwnPtr<WebPluginLoadObserver> observer = pluginLoadObserver();
    m_webFrame->didFail(error, true);
    if (observer)
        observer->didFailLoading(error);
}

void FrameLoaderClientImpl::dispatchDidFailLoad(const ResourceError& error)
{
    OwnPtr<WebPluginLoadObserver> observer = pluginLoadObserver();
    m_webFrame->didFail(error, false);
    if (observer)
        observer->didFailLoading(error);

    // Don't clear the redirect chain, this will happen in the middle of client
    // redirects, and we need the context. The chain will be cleared when the
    // provisional load succeeds or fails, not the "real" one.
}

void FrameLoaderClientImpl::dispatchDidFinishLoad()
{
    OwnPtr<WebPluginLoadObserver> observer = pluginLoadObserver();

    if (m_webFrame->client())
        m_webFrame->client()->didFinishLoad(m_webFrame);

    if (observer)
        observer->didFinishLoading();

    // Don't clear the redirect chain, this will happen in the middle of client
    // redirects, and we need the context. The chain will be cleared when the
    // provisional load succeeds or fails, not the "real" one.
}

void FrameLoaderClientImpl::dispatchDidFirstLayout()
{
}

void FrameLoaderClientImpl::dispatchDidFirstVisuallyNonEmptyLayout()
{
    // FIXME: called when webkit finished layout of a page that was visually non-empty.
    // All resources have not necessarily finished loading.
}

Frame* FrameLoaderClientImpl::dispatchCreatePage()
{
    struct WindowFeatures features;
    Page* newPage = m_webFrame->frame()->page()->chrome()->createWindow(
        m_webFrame->frame(), FrameLoadRequest(), features);

    // Make sure that we have a valid disposition.  This should have been set in
    // the preceeding call to dispatchDecidePolicyForNewWindowAction.
    ASSERT(m_nextNavigationPolicy != WebNavigationPolicyIgnore);
    WebNavigationPolicy policy = m_nextNavigationPolicy;
    m_nextNavigationPolicy = WebNavigationPolicyIgnore;

    // createWindow can return null (e.g., popup blocker denies the window).
    if (!newPage)
        return 0;

    WebViewImpl::fromPage(newPage)->setInitialNavigationPolicy(policy);
    return newPage->mainFrame();
}

void FrameLoaderClientImpl::dispatchShow()
{
    WebViewImpl* webView = m_webFrame->viewImpl();
    if (webView && webView->client())
        webView->client()->show(webView->initialNavigationPolicy());
}

void FrameLoaderClientImpl::dispatchDecidePolicyForMIMEType(
     FramePolicyFunction function,
     const String& mimeType,
     const ResourceRequest&)
{
    const ResourceResponse& response =
        m_webFrame->frame()->loader()->activeDocumentLoader()->response();

    PolicyAction action;

    int statusCode = response.httpStatusCode();
    if (statusCode == 204 || statusCode == 205) {
        // The server does not want us to replace the page contents.
        action = PolicyIgnore;
    } else if (WebCore::shouldTreatAsAttachment(response)) {
        // The server wants us to download instead of replacing the page contents.
        // Downloading is handled by the embedder, but we still get the initial
        // response so that we can ignore it and clean up properly.
        action = PolicyIgnore;
    } else if (!canShowMIMEType(mimeType)) {
        // Make sure that we can actually handle this type internally.
        action = PolicyIgnore;
    } else {
        // OK, we will render this page.
        action = PolicyUse;
    }

    // NOTE: PolicyChangeError will be generated when action is not PolicyUse.
    (m_webFrame->frame()->loader()->policyChecker()->*function)(action);
}

void FrameLoaderClientImpl::dispatchDecidePolicyForNewWindowAction(
    FramePolicyFunction function,
    const NavigationAction& action,
    const ResourceRequest& request,
    PassRefPtr<FormState> formState,
    const String& frameName)
{
    WebNavigationPolicy navigationPolicy;
    if (!actionSpecifiesNavigationPolicy(action, &navigationPolicy))
        navigationPolicy = WebNavigationPolicyNewForegroundTab;

    PolicyAction policyAction;
    if (navigationPolicy == WebNavigationPolicyDownload)
        policyAction = PolicyDownload;
    else {
        policyAction = PolicyUse;

        // Remember the disposition for when dispatchCreatePage is called.  It is
        // unfortunate that WebCore does not provide us with any context when
        // creating or showing the new window that would allow us to avoid having
        // to keep this state.
        m_nextNavigationPolicy = navigationPolicy;
    }
    (m_webFrame->frame()->loader()->policyChecker()->*function)(policyAction);
}

void FrameLoaderClientImpl::dispatchDecidePolicyForNavigationAction(
    FramePolicyFunction function,
    const NavigationAction& action,
    const ResourceRequest& request,
    PassRefPtr<FormState> formState) {
    PolicyAction policyAction = PolicyIgnore;

    // It is valid for this function to be invoked in code paths where the
    // the webview is closed.
    // The null check here is to fix a crash that seems strange
    // (see - https://bugs.webkit.org/show_bug.cgi?id=23554).
    if (m_webFrame->client() && !request.url().isNull()) {
        WebNavigationPolicy navigationPolicy = WebNavigationPolicyCurrentTab;
        actionSpecifiesNavigationPolicy(action, &navigationPolicy);

        // Give the delegate a chance to change the navigation policy.
        const WebDataSourceImpl* ds = m_webFrame->provisionalDataSourceImpl();
        if (ds) {
            KURL url = ds->request().url();
            ASSERT(!url.protocolIs(backForwardNavigationScheme));

            bool isRedirect = ds->hasRedirectChain();

            WebNavigationType webnavType =
                WebDataSourceImpl::toWebNavigationType(action.type());

            RefPtr<Node> node;
            for (const Event* event = action.event(); event; event = event->underlyingEvent()) {
                if (event->isMouseEvent()) {
                    const MouseEvent* mouseEvent =
                        static_cast<const MouseEvent*>(event);
                    node = m_webFrame->frame()->eventHandler()->hitTestResultAtPoint(
                        mouseEvent->absoluteLocation(), false).innerNonSharedNode();
                    break;
                }
            }
            WebNode originatingNode(node);

            navigationPolicy = m_webFrame->client()->decidePolicyForNavigation(
                m_webFrame, ds->request(), webnavType, originatingNode,
                navigationPolicy, isRedirect);
        }

        if (navigationPolicy == WebNavigationPolicyCurrentTab)
            policyAction = PolicyUse;
        else if (navigationPolicy == WebNavigationPolicyDownload)
            policyAction = PolicyDownload;
        else {
            if (navigationPolicy != WebNavigationPolicyIgnore) {
                WrappedResourceRequest webreq(request);
                m_webFrame->client()->loadURLExternally(m_webFrame, webreq, navigationPolicy);
            }
            policyAction = PolicyIgnore;
        }
    }

    (m_webFrame->frame()->loader()->policyChecker()->*function)(policyAction);
}

void FrameLoaderClientImpl::cancelPolicyCheck()
{
    // FIXME
}

void FrameLoaderClientImpl::dispatchUnableToImplementPolicy(const ResourceError& error)
{
    m_webFrame->client()->unableToImplementPolicyWithError(m_webFrame, error);
}

void FrameLoaderClientImpl::dispatchWillSubmitForm(FramePolicyFunction function,
    PassRefPtr<FormState> formState)
{
    if (m_webFrame->client())
        m_webFrame->client()->willSubmitForm(m_webFrame, WebFormElement(formState->form()));
    (m_webFrame->frame()->loader()->policyChecker()->*function)(PolicyUse);
}

void FrameLoaderClientImpl::dispatchDidLoadMainResource(DocumentLoader*)
{
    // FIXME
}

void FrameLoaderClientImpl::revertToProvisionalState(DocumentLoader*)
{
    m_hasRepresentation = true;
}

void FrameLoaderClientImpl::setMainDocumentError(DocumentLoader*,
                                                 const ResourceError& error)
{
    if (m_pluginWidget.get()) {
        if (m_sentInitialResponseToPlugin) {
            m_pluginWidget->didFailLoading(error);
            m_sentInitialResponseToPlugin = false;
        }
        m_pluginWidget = 0;
    }
}

void FrameLoaderClientImpl::postProgressStartedNotification()
{
    WebViewImpl* webview = m_webFrame->viewImpl();
    if (webview && webview->client())
        webview->client()->didStartLoading();
}

void FrameLoaderClientImpl::postProgressEstimateChangedNotification()
{
    // FIXME
}

void FrameLoaderClientImpl::postProgressFinishedNotification()
{
    // FIXME: why might the webview be null?  http://b/1234461
    WebViewImpl* webview = m_webFrame->viewImpl();
    if (webview && webview->client())
        webview->client()->didStopLoading();
}

void FrameLoaderClientImpl::setMainFrameDocumentReady(bool ready)
{
    // FIXME
}

// Creates a new connection and begins downloading from that (contrast this
// with |download|).
void FrameLoaderClientImpl::startDownload(const ResourceRequest& request)
{
    if (m_webFrame->client()) {
        WrappedResourceRequest webreq(request);
        m_webFrame->client()->loadURLExternally(
            m_webFrame, webreq, WebNavigationPolicyDownload);
    }
}

void FrameLoaderClientImpl::willChangeTitle(DocumentLoader*)
{
    // FIXME
}

void FrameLoaderClientImpl::didChangeTitle(DocumentLoader*)
{
    // FIXME
}

// Called whenever data is received.
void FrameLoaderClientImpl::committedLoad(DocumentLoader* loader, const char* data, int length)
{
    if (!m_pluginWidget.get()) {
        if (m_webFrame->client()) {
            bool preventDefault = false;
            m_webFrame->client()->didReceiveDocumentData(m_webFrame, data, length, preventDefault);
            if (!preventDefault)
                m_webFrame->commitDocumentData(data, length);
        }
    }

    // If we are sending data to MediaDocument, we should stop here
    // and cancel the request.
    if (m_webFrame->frame()->document()
        && m_webFrame->frame()->document()->isMediaDocument())
        loader->cancelMainResourceLoad(pluginWillHandleLoadError(loader->response()));

    // The plugin widget could have been created in the m_webFrame->DidReceiveData
    // function.
    if (m_pluginWidget.get()) {
        if (!m_sentInitialResponseToPlugin) {
            m_sentInitialResponseToPlugin = true;
            m_pluginWidget->didReceiveResponse(
                m_webFrame->frame()->loader()->activeDocumentLoader()->response());
        }
        m_pluginWidget->didReceiveData(data, length);
    }
}

void FrameLoaderClientImpl::finishedLoading(DocumentLoader* dl)
{
    if (m_pluginWidget.get()) {
        m_pluginWidget->didFinishLoading();
        m_pluginWidget = 0;
        m_sentInitialResponseToPlugin = false;
    } else {
        // This is necessary to create an empty document. See bug 634004.
        // However, we only want to do this if makeRepresentation has been called, to
        // match the behavior on the Mac.
        if (m_hasRepresentation)
            dl->frameLoader()->setEncoding("", false);
    }
}

void FrameLoaderClientImpl::updateGlobalHistory()
{
}

void FrameLoaderClientImpl::updateGlobalHistoryRedirectLinks()
{
}

bool FrameLoaderClientImpl::shouldGoToHistoryItem(HistoryItem* item) const
{
    const KURL& url = item->url();
    if (!url.protocolIs(backForwardNavigationScheme))
        return true;

    // Else, we'll punt this history navigation to the embedder.  It is
    // necessary that we intercept this here, well before the FrameLoader
    // has made any state changes for this history traversal.

    bool ok;
    int offset = url.lastPathComponent().toIntStrict(&ok);
    if (!ok) {
        ASSERT_NOT_REACHED();
        return false;
    }

    WebViewImpl* webview = m_webFrame->viewImpl();
    if (webview->client())
        webview->client()->navigateBackForwardSoon(offset);

    return false;
}

void FrameLoaderClientImpl::dispatchDidAddBackForwardItem(HistoryItem*) const
{
}

void FrameLoaderClientImpl::dispatchDidRemoveBackForwardItem(HistoryItem*) const
{
}

void FrameLoaderClientImpl::dispatchDidChangeBackForwardIndex() const
{
}

void FrameLoaderClientImpl::didDisplayInsecureContent()
{
    if (m_webFrame->client())
        m_webFrame->client()->didDisplayInsecureContent(m_webFrame);
}

void FrameLoaderClientImpl::didRunInsecureContent(SecurityOrigin* origin)
{
    if (m_webFrame->client())
        m_webFrame->client()->didRunInsecureContent(m_webFrame, WebSecurityOrigin(origin));
}

ResourceError FrameLoaderClientImpl::blockedError(const ResourceRequest&)
{
    // FIXME
    return ResourceError();
}

ResourceError FrameLoaderClientImpl::cancelledError(const ResourceRequest& request)
{
    if (!m_webFrame->client())
        return ResourceError();

    return m_webFrame->client()->cancelledError(
        m_webFrame, WrappedResourceRequest(request));
}

ResourceError FrameLoaderClientImpl::cannotShowURLError(const ResourceRequest& request)
{
    if (!m_webFrame->client())
        return ResourceError();

    return m_webFrame->client()->cannotHandleRequestError(
        m_webFrame, WrappedResourceRequest(request));
}

ResourceError FrameLoaderClientImpl::interruptForPolicyChangeError(
    const ResourceRequest& request)
{
    return ResourceError(internalErrorDomain, PolicyChangeError,
                         request.url().string(), String());
}

ResourceError FrameLoaderClientImpl::cannotShowMIMETypeError(const ResourceResponse&)
{
    // FIXME
    return ResourceError();
}

ResourceError FrameLoaderClientImpl::fileDoesNotExistError(const ResourceResponse&)
{
    // FIXME
    return ResourceError();
}

ResourceError FrameLoaderClientImpl::pluginWillHandleLoadError(const ResourceResponse&)
{
    // FIXME
    return ResourceError();
}

bool FrameLoaderClientImpl::shouldFallBack(const ResourceError& error)
{
    // This method is called when we fail to load the URL for an <object> tag
    // that has fallback content (child elements) and is being loaded as a frame.
    // The error parameter indicates the reason for the load failure.
    // We should let the fallback content load only if this wasn't a cancelled
    // request.
    // Note: The mac version also has a case for "WebKitErrorPluginWillHandleLoad"
    ResourceError c = cancelledError(ResourceRequest());
    return error.errorCode() != c.errorCode() || error.domain() != c.domain();
}

bool FrameLoaderClientImpl::canHandleRequest(const ResourceRequest& request) const
{
    return m_webFrame->client()->canHandleRequest(
        m_webFrame, WrappedResourceRequest(request));
}

bool FrameLoaderClientImpl::canShowMIMEType(const String& mimeType) const
{
    // This method is called to determine if the media type can be shown
    // "internally" (i.e. inside the browser) regardless of whether or not the
    // browser or a plugin is doing the rendering.

    // mimeType strings are supposed to be ASCII, but if they are not for some
    // reason, then it just means that the mime type will fail all of these "is
    // supported" checks and go down the path of an unhandled mime type.
    if (webKitClient()->mimeRegistry()->supportsMIMEType(mimeType) == WebMimeRegistry::IsSupported)
        return true;

    // If Chrome is started with the --disable-plugins switch, pluginData is null.
    PluginData* pluginData = m_webFrame->frame()->page()->pluginData();

    // See if the type is handled by an installed plugin, if so, we can show it.
    // FIXME: (http://b/1085524) This is the place to stick a preference to
    //        disable full page plugins (optionally for certain types!)
    return !mimeType.isEmpty() && pluginData && pluginData->supportsMimeType(mimeType);
}

bool FrameLoaderClientImpl::representationExistsForURLScheme(const String&) const
{
    // FIXME
    return false;
}

String FrameLoaderClientImpl::generatedMIMETypeForURLScheme(const String& scheme) const
{
    // This appears to generate MIME types for protocol handlers that are handled
    // internally. The only place I can find in the WebKit code that uses this
    // function is WebView::registerViewClass, where it is used as part of the
    // process by which custom view classes for certain document representations
    // are registered.
    String mimeType("x-apple-web-kit/");
    mimeType.append(scheme.lower());
    return mimeType;
}

void FrameLoaderClientImpl::frameLoadCompleted()
{
    // FIXME: the mac port also conditionally calls setDrawsBackground:YES on
    // it's ScrollView here.

    // This comment from the Mac port:
    // Note: Can be called multiple times.
    // Even if already complete, we might have set a previous item on a frame that
    // didn't do any data loading on the past transaction. Make sure to clear these out.

    // FIXME: setPreviousHistoryItem() no longer exists. http://crbug.com/8566
    // m_webFrame->frame()->loader()->setPreviousHistoryItem(0);
}

void FrameLoaderClientImpl::saveViewStateToItem(HistoryItem*)
{
    // FIXME
}

void FrameLoaderClientImpl::restoreViewState()
{
    // FIXME: probably scrolls to last position when you go back or forward
}

void FrameLoaderClientImpl::provisionalLoadStarted()
{
    // FIXME: On mac, this does various caching stuff
}

void FrameLoaderClientImpl::didFinishLoad()
{
    OwnPtr<WebPluginLoadObserver> observer = pluginLoadObserver();
    if (observer)
        observer->didFinishLoading();
}

void FrameLoaderClientImpl::prepareForDataSourceReplacement()
{
    // FIXME
}

PassRefPtr<DocumentLoader> FrameLoaderClientImpl::createDocumentLoader(
    const ResourceRequest& request,
    const SubstituteData& data)
{
    RefPtr<WebDataSourceImpl> ds = WebDataSourceImpl::create(request, data);
    if (m_webFrame->client())
        m_webFrame->client()->didCreateDataSource(m_webFrame, ds.get());
    return ds.release();
}

void FrameLoaderClientImpl::setTitle(const String& title, const KURL& url)
{
    // FIXME: inform consumer of changes to the title.
}

String FrameLoaderClientImpl::userAgent(const KURL& url)
{
    return webKitClient()->userAgent(url);
}

void FrameLoaderClientImpl::savePlatformDataToCachedFrame(CachedFrame*)
{
    // The page cache should be disabled.
    ASSERT_NOT_REACHED();
}

void FrameLoaderClientImpl::transitionToCommittedFromCachedFrame(CachedFrame*)
{
    ASSERT_NOT_REACHED();
}

// Called when the FrameLoader goes into a state in which a new page load
// will occur.
void FrameLoaderClientImpl::transitionToCommittedForNewPage()
{
    makeDocumentView();
}

bool FrameLoaderClientImpl::canCachePage() const
{
    // Since we manage the cache, always report this page as non-cacheable to
    // FrameLoader.
    return false;
}

// Downloading is handled in the browser process, not WebKit. If we get to this
// point, our download detection code in the ResourceDispatcherHost is broken!
void FrameLoaderClientImpl::download(ResourceHandle* handle,
                                     const ResourceRequest& request,
                                     const ResourceRequest& initialRequest,
                                     const ResourceResponse& response)
{
    ASSERT_NOT_REACHED();
}

PassRefPtr<Frame> FrameLoaderClientImpl::createFrame(
    const KURL& url,
    const String& name,
    HTMLFrameOwnerElement* ownerElement,
    const String& referrer,
    bool allowsScrolling,
    int marginWidth,
    int marginHeight)
{
    FrameLoadRequest frameRequest(ResourceRequest(url, referrer), name);
    return m_webFrame->createChildFrame(frameRequest, ownerElement);
}

void FrameLoaderClientImpl::didTransferChildFrameToNewDocument()
{
    ASSERT(m_webFrame->frame()->ownerElement());

    WebFrameImpl* newParent = static_cast<WebFrameImpl*>(m_webFrame->parent());
    if (!newParent || !newParent->client())
        return;

    // Replace the client since the old client may be destroyed when the
    // previous page is closed.
    m_webFrame->setClient(newParent->client());
}

PassRefPtr<Widget> FrameLoaderClientImpl::createPlugin(
    const IntSize& size, // FIXME: how do we use this?
    HTMLPlugInElement* element,
    const KURL& url,
    const Vector<String>& paramNames,
    const Vector<String>& paramValues,
    const String& mimeType,
    bool loadManually)
{
#if !OS(WINDOWS)
    // WebCore asks us to make a plugin even if we don't have a
    // registered handler, with a comment saying it's so we can display
    // the broken plugin icon.  In Chromium, we normally register a
    // fallback plugin handler that allows you to install a missing
    // plugin.  Since we don't yet have a default plugin handler, we
    // need to return null here rather than going through all the
    // plugin-creation IPCs only to discover we don't have a plugin
    // registered, which causes a crash.
    // FIXME: remove me once we have a default plugin.
    if (objectContentType(url, mimeType) != ObjectContentNetscapePlugin)
        return 0;
#endif

    if (!m_webFrame->client())
        return 0;

    WebPluginParams params;
    params.url = url;
    params.mimeType = mimeType;
    params.attributeNames = paramNames;
    params.attributeValues = paramValues;
    params.loadManually = loadManually;

    WebPlugin* webPlugin = m_webFrame->client()->createPlugin(m_webFrame, params);
    if (!webPlugin)
        return 0;

    // The container takes ownership of the WebPlugin.
    RefPtr<WebPluginContainerImpl> container =
        WebPluginContainerImpl::create(element, webPlugin);

    if (!webPlugin->initialize(container.get()))
        return 0;

    // The element might have been removed during plugin initialization!
    if (!element->renderer())
        return 0;

    return container;
}

// This method gets called when a plugin is put in place of html content
// (e.g., acrobat reader).
void FrameLoaderClientImpl::redirectDataToPlugin(Widget* pluginWidget)
{
    m_pluginWidget = static_cast<WebPluginContainerImpl*>(pluginWidget);
    ASSERT(m_pluginWidget.get());
}

PassRefPtr<Widget> FrameLoaderClientImpl::createJavaAppletWidget(
    const IntSize& size,
    HTMLAppletElement* element,
    const KURL& /* baseURL */,
    const Vector<String>& paramNames,
    const Vector<String>& paramValues)
{
    return createPlugin(size, element, KURL(), paramNames, paramValues,
        "application/x-java-applet", false);
}

ObjectContentType FrameLoaderClientImpl::objectContentType(
    const KURL& url,
    const String& explicitMimeType)
{
    // This code is based on Apple's implementation from
    // WebCoreSupport/WebFrameBridge.mm.

    String mimeType = explicitMimeType;
    if (mimeType.isEmpty()) {
        // Try to guess the MIME type based off the extension.
        String filename = url.lastPathComponent();
        int extensionPos = filename.reverseFind('.');
        if (extensionPos >= 0) {
            String extension = filename.substring(extensionPos + 1);
            mimeType = MIMETypeRegistry::getMIMETypeForExtension(extension);
            if (mimeType.isEmpty()) {
                // If there's no mimetype registered for the extension, check to see
                // if a plugin can handle the extension.
                mimeType = getPluginMimeTypeFromExtension(extension);
            }
        }

        if (mimeType.isEmpty())
            return ObjectContentFrame;
    }

    if (MIMETypeRegistry::isSupportedImageMIMEType(mimeType))
        return ObjectContentImage;

    // If Chrome is started with the --disable-plugins switch, pluginData is 0.
    PluginData* pluginData = m_webFrame->frame()->page()->pluginData();
    if (pluginData && pluginData->supportsMimeType(mimeType))
        return ObjectContentNetscapePlugin;

    if (MIMETypeRegistry::isSupportedNonImageMIMEType(mimeType))
        return ObjectContentFrame;

    return ObjectContentNone;
}

String FrameLoaderClientImpl::overrideMediaType() const
{
    // FIXME
    return String();
}

bool FrameLoaderClientImpl::actionSpecifiesNavigationPolicy(
    const NavigationAction& action,
    WebNavigationPolicy* policy)
{
    const MouseEvent* event = 0;
    if (action.type() == NavigationTypeLinkClicked
        && action.event()->isMouseEvent())
        event = static_cast<const MouseEvent*>(action.event());
    else if (action.type() == NavigationTypeFormSubmitted
             && action.event()
             && action.event()->underlyingEvent()
             && action.event()->underlyingEvent()->isMouseEvent())
        event = static_cast<const MouseEvent*>(action.event()->underlyingEvent());

    if (!event)
        return false;

    return WebViewImpl::navigationPolicyFromMouseEvent(
        event->button(), event->ctrlKey(), event->shiftKey(), event->altKey(),
        event->metaKey(), policy);
}

PassOwnPtr<WebPluginLoadObserver> FrameLoaderClientImpl::pluginLoadObserver()
{
    WebDataSourceImpl* ds = WebDataSourceImpl::fromDocumentLoader(
        m_webFrame->frame()->loader()->activeDocumentLoader());
    return ds->releasePluginLoadObserver();
}

} // namespace WebKit
