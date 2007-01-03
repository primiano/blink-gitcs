/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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
#import "FrameLoader.h"

#import "BlockExceptions.h"
#import "Cache.h"
#import "Chrome.h"
#import "DOMElementInternal.h"
#import "Document.h"
#import "DocumentLoader.h"
#import "Event.h"
#import "FloatRect.h"
#import "FormDataStreamMac.h"
#import "FormState.h"
#import "FrameLoadRequest.h"
#import "FrameLoaderClient.h"
#import "FrameMac.h"
#import "FramePrivate.h"
#import "FrameTree.h"
#import "FrameView.h"
#import "HistoryItem.h"
#import "HTMLFormElement.h"
#import "HTMLFrameElement.h"
#import "HTMLNames.h"
#import "IconDatabase.h"
#import "LoaderNSURLExtras.h"
#import "LoaderNSURLRequestExtras.h"
#import "MainResourceLoader.h"
#import "NavigationAction.h"
#import "Page.h"
#import "PageState.h"
#import "Plugin.h"
#import "ResourceError.h"
#import "ResourceHandle.h"
#import "ResourceRequest.h"
#import "ResourceResponse.h"
#import "Settings.h"
#import "SubresourceLoader.h"
#import "SystemTime.h"
#import "TextResourceDecoder.h"
#import "WebCoreFrameBridge.h"
#import "WebCoreSystemInterface.h"
#import "WebDataProtocol.h"
#import "Widget.h"
#import "WindowFeatures.h"
#import <kjs/JSLock.h>
#import <wtf/Assertions.h>

namespace WebCore {

using namespace HTMLNames;

void FrameLoader::load(const FrameLoadRequest& request, bool userGesture, Event* event,
    HTMLFormElement* submitForm, const HashMap<String, String>& formValues)
{
    String referrer;
    String argsReferrer = request.resourceRequest().httpReferrer();
    if (!argsReferrer.isEmpty())
        referrer = argsReferrer;
    else
        referrer = m_outgoingReferrer;
 
    bool hideReferrer;
    if (!canLoad(request.resourceRequest().url().getNSURL(), referrer, hideReferrer))
        return;
    if (hideReferrer)
        referrer = String();
    
    Frame* targetFrame = m_frame->tree()->find(request.frameName());
    if (!canTarget(targetFrame))
        return;
        
    if (request.resourceRequest().httpMethod() != "POST") {
        FrameLoadType loadType;
        if (request.resourceRequest().cachePolicy() == ReloadIgnoringCacheData)
            loadType = FrameLoadTypeReload;
        else if (!userGesture)
            loadType = FrameLoadTypeInternal;
        else
            loadType = FrameLoadTypeStandard;    
    
        load(request.resourceRequest().url(), referrer, loadType, 
            request.frameName(), event, submitForm, formValues);
    } else
        post(request.resourceRequest().url(), referrer, request.frameName(), 
            request.resourceRequest().httpBody(), request.resourceRequest().httpContentType(), event, submitForm, formValues);

    if (targetFrame && targetFrame != m_frame)
        targetFrame->page()->chrome()->focus();
}

void FrameLoader::load(const KURL& URL, const String& referrer, FrameLoadType newLoadType,
    const String& frameName, Event* event, HTMLFormElement* form, const HashMap<String, String>& values)
{
    bool isFormSubmission = !values.isEmpty();
    
    ResourceRequest request(URL);
    if (!referrer.isEmpty())
        request.setHTTPReferrer(referrer);
    addExtraFieldsToRequest(request, true, event || isFormSubmission);
    if (newLoadType == FrameLoadTypeReload)
        request.setCachePolicy(ReloadIgnoringCacheData);

    ASSERT(newLoadType != FrameLoadTypeSame);

    NavigationAction action(URL, newLoadType, isFormSubmission, event);

    RefPtr<FormState> formState;
    if (form && !values.isEmpty())
        formState = FormState::create(form, values, m_frame);
    
    if (!frameName.isEmpty()) {
        if (Frame* targetFrame = m_frame->tree()->find(frameName))
            targetFrame->loader()->load(URL, referrer, newLoadType, String(), event, form, values);
        else
            checkNewWindowPolicy(action, request, formState.release(), frameName);
        return;
    }

    RefPtr<DocumentLoader> oldDocumentLoader = m_documentLoader;

    bool sameURL = shouldTreatURLAsSameAsCurrent(URL);
    
    // Make sure to do scroll to anchor processing even if the URL is
    // exactly the same so pages with '#' links and DHTML side effects
    // work properly.
    if (!isFormSubmission
        && newLoadType != FrameLoadTypeReload
        && newLoadType != FrameLoadTypeSame
        && !shouldReload(URL, url())
        // We don't want to just scroll if a link from within a
        // frameset is trying to reload the frameset into _top.
        && !m_frame->isFrameSet()) {

        // Just do anchor navigation within the existing content.
        
        // We don't do this if we are submitting a form, explicitly reloading,
        // currently displaying a frameset, or if the new URL does not have a fragment.
        // These rules are based on what KHTML was doing in KHTMLPart::openURL.
        
        // FIXME: What about load types other than Standard and Reload?
        
        oldDocumentLoader->setTriggeringAction(action);
        stopPolicyCheck();
        checkNavigationPolicy(request, oldDocumentLoader.get(), formState.release(),
            callContinueFragmentScrollAfterNavigationPolicy, this);
    } else {
        // must grab this now, since this load may stop the previous load and clear this flag
        bool isRedirect = m_quickRedirectComing;
        load(request, action, newLoadType, formState.release());
        if (isRedirect) {
            m_quickRedirectComing = false;
            if (m_provisionalDocumentLoader)
                m_provisionalDocumentLoader->setIsClientRedirect(true);
        } else if (sameURL)
            // Example of this case are sites that reload the same URL with a different cookie
            // driving the generated content, or a master frame with links that drive a target
            // frame, where the user has clicked on the same link repeatedly.
            m_loadType = FrameLoadTypeSame;
    }
}

void FrameLoader::load(const ResourceRequest& request)
{
    // FIXME: is this the right place to reset loadType? Perhaps this should be done after loading is finished or aborted.
    m_loadType = FrameLoadTypeStandard;
    load(m_client->createDocumentLoader(request).get());
}

void FrameLoader::load(const ResourceRequest& request, const String& frameName)
{
    if (frameName.isEmpty()) {
        load(request);
        return;
    }

    Frame* frame = m_frame->tree()->find(frameName);
    if (frame) {
        frame->loader()->load(request);
        return;
    }

    checkNewWindowPolicy(NavigationAction(request.url(), NavigationTypeOther), request, 0, frameName);
}

void FrameLoader::load(const ResourceRequest& request, const NavigationAction& action, FrameLoadType type, PassRefPtr<FormState> formState)
{
    RefPtr<DocumentLoader> loader = m_client->createDocumentLoader(request);
    setPolicyDocumentLoader(loader.get());

    loader->setTriggeringAction(action);
    if (m_documentLoader)
        loader->setOverrideEncoding(m_documentLoader->overrideEncoding());

    load(loader.get(), type, formState);
}

void FrameLoader::load(DocumentLoader* newDocumentLoader)
{
    stopPolicyCheck();
    setPolicyDocumentLoader(newDocumentLoader);

    ResourceRequest& r = newDocumentLoader->request();
    addExtraFieldsToRequest(r, true, false);
    FrameLoadType type;

    if (shouldTreatURLAsSameAsCurrent(newDocumentLoader->originalRequest().url())) {
        r.setCachePolicy(ReloadIgnoringCacheData);
        type = FrameLoadTypeSame;
    } else
        type = FrameLoadTypeStandard;

    if (m_documentLoader)
        newDocumentLoader->setOverrideEncoding(m_documentLoader->overrideEncoding());
    
    // When we loading alternate content for an unreachable URL that we're
    // visiting in the b/f list, we treat it as a reload so the b/f list 
    // is appropriately maintained.
    if (shouldReloadToHandleUnreachableURL(newDocumentLoader->originalRequest())) {
        ASSERT(type == FrameLoadTypeStandard);
        type = FrameLoadTypeReload;
    }

    load(newDocumentLoader, type, 0);
}

void FrameLoader::load(DocumentLoader* loader, FrameLoadType type, PassRefPtr<FormState> formState)
{
    ASSERT(m_client->hasWebView());

    // Unfortunately the view must be non-nil, this is ultimately due
    // to parser requiring a FrameView.  We should fix this dependency.

    ASSERT(m_client->hasFrameView());

    m_policyLoadType = type;

    if (Frame* parent = m_frame->tree()->parent())
        loader->setOverrideEncoding(parent->loader()->documentLoader()->overrideEncoding());

    stopPolicyCheck();
    setPolicyDocumentLoader(loader);

    checkNavigationPolicy(loader->request(), loader, formState,
        callContinueLoadAfterNavigationPolicy, this);
}

bool FrameLoader::canLoad(const KURL& url, const String& referrer, bool& hideReferrer)
{
    bool referrerIsWebURL = referrer.startsWith("http:", false) || referrer.startsWith("https:", false);
    bool referrerIsLocalURL = referrer.startsWith("file:", false) || referrer.startsWith("applewebdata:");
    bool URLIsFileURL = url.protocol().startsWith("file", false);
    bool referrerIsSecureURL = referrer.startsWith("https:", false);
    bool URLIsSecureURL = url.protocol().startsWith("https", false);
    
    hideReferrer = !referrerIsWebURL || (referrerIsSecureURL && !URLIsSecureURL);
    return !URLIsFileURL || referrerIsLocalURL;
}

bool FrameLoader::startLoadingMainResource(ResourceRequest& request, id identifier)
{
    ASSERT(!m_mainResourceLoader);
    m_mainResourceLoader = MainResourceLoader::create(m_frame);
    m_mainResourceLoader->setIdentifier(identifier);
    // FIXME: is there any way the extra fields could have not been added by now?
    addExtraFieldsToRequest(request, true, false);
    if (!m_mainResourceLoader->load(request.nsURLRequest())) {
        // FIXME: If this should really be caught, we should just ASSERT this doesn't happen;
        // should it be caught by other parts of WebKit or other parts of the app?
        LOG_ERROR("could not create WebResourceHandle for URL %@ -- should be caught by policy handler level", request.url().getNSURL());
        m_mainResourceLoader = 0;
        return false;
    }
    return true;
}

// FIXME: Poor method name; also, why is this not part of startProvisionalLoad:?
void FrameLoader::startLoading()
{
    if (!m_provisionalDocumentLoader)
        return;

    m_provisionalDocumentLoader->prepareForLoadStart();

    if (m_mainResourceLoader)
        return;

    m_provisionalDocumentLoader->setLoadingFromPageCache(false);

    id identifier = m_client->dispatchIdentifierForInitialRequest
        (m_provisionalDocumentLoader.get(), m_provisionalDocumentLoader->originalRequest());
        
    if (!startLoadingMainResource(m_provisionalDocumentLoader->actualRequest(), identifier))
        m_provisionalDocumentLoader->updateLoading();
}

void FrameLoader::cancelMainResourceLoad(const ResourceError& error)
{
    m_mainResourceLoader->cancel(error);
}

id FrameLoader::identifierForInitialRequest(NSURLRequest *clientRequest)
{
    return m_client->dispatchIdentifierForInitialRequest(activeDocumentLoader(), clientRequest);
}

void FrameLoader::applyUserAgent(NSMutableURLRequest *request)
{
    static String lastUserAgent;
    static RetainPtr<NSString> lastUserAgentNSString;

    String userAgent = client()->userAgent();
    ASSERT(!userAgent.isNull());
    if (userAgent != lastUserAgent) {
        lastUserAgent = userAgent;
        lastUserAgentNSString = userAgent;
    }

    [request setValue:lastUserAgentNSString.get() forHTTPHeaderField:@"User-Agent"];
}

void FrameLoader::applyUserAgent(ResourceRequest& request)
{
    String userAgent = client()->userAgent();
    ASSERT(!userAgent.isNull());
    request.setHTTPUserAgent(userAgent);
}

NSURLRequest *FrameLoader::willSendRequest(ResourceLoader* loader, NSMutableURLRequest *clientRequest, NSURLResponse *redirectResponse)
{
    applyUserAgent(clientRequest);
    return m_client->dispatchWillSendRequest(activeDocumentLoader(), loader->identifier(), clientRequest, redirectResponse);
}

void FrameLoader::didReceiveAuthenticationChallenge(ResourceLoader* loader, NSURLAuthenticationChallenge *currentWebChallenge)
{
    m_client->dispatchDidReceiveAuthenticationChallenge(activeDocumentLoader(), loader->identifier(), currentWebChallenge);
}

void FrameLoader::didCancelAuthenticationChallenge(ResourceLoader* loader, NSURLAuthenticationChallenge *currentWebChallenge)
{
    m_client->dispatchDidCancelAuthenticationChallenge(activeDocumentLoader(), loader->identifier(), currentWebChallenge);
}

void FrameLoader::didReceiveResponse(ResourceLoader* loader, NSURLResponse *r)
{
    activeDocumentLoader()->addResponse(r);
    
    m_client->incrementProgress(loader->identifier(), r);
    m_client->dispatchDidReceiveResponse(activeDocumentLoader(), loader->identifier(), r);
}

void FrameLoader::didReceiveData(ResourceLoader* loader, const char* data, int length, int lengthReceived)
{
    m_client->incrementProgress(loader->identifier(), data, length);
    m_client->dispatchDidReceiveContentLength(activeDocumentLoader(), loader->identifier(), lengthReceived);
}

void FrameLoader::didFailToLoad(ResourceLoader* loader, const ResourceError& error)
{
    m_client->completeProgress(loader->identifier());
    if (error)
        m_client->dispatchDidFailLoading(activeDocumentLoader(), loader->identifier(), error);
}

const ResourceRequest& FrameLoader::originalRequest() const
{
    return activeDocumentLoader()->originalRequestCopy();
}

void FrameLoader::receivedMainResourceError(const ResourceError& error, bool isComplete)
{
    // Retain because the stop may release the last reference to it.
    RefPtr<Frame> protect(m_frame);

    RefPtr<DocumentLoader> loader = activeDocumentLoader();
    
    if (isComplete) {
        // FIXME: Don't want to do this if an entirely new load is going, so should check
        // that both data sources on the frame are either this or nil.
        stop();
        if (m_client->shouldFallBack(error))
            handleFallbackContent();
    }
    
    if (m_state == FrameStateProvisional) {
#ifdef MULTIPLE_FORM_SUBMISSION_PROTECTION
        KURL failedURL = m_provisionalDocumentLoader->originalRequestCopy().url();
        didNotOpenURL(failedURL);
#endif
        // We might have made a page cache item, but now we're bailing out due to an error before we ever
        // transitioned to the new page (before WebFrameState == commit).  The goal here is to restore any state
        // so that the existing view (that wenever got far enough to replace) can continue being used.
        Document* document = m_frame->document();
        if (document)
            document->setInPageCache(false);
        invalidateCurrentItemPageCache();
        
        // Call clientRedirectCancelledOrFinished here so that the frame load delegate is notified that the redirect's
        // status has changed, if there was a redirect. The frame load delegate may have saved some state about
        // the redirect in its -webView:willPerformClientRedirectToURL:delay:fireDate:forFrame:. Since we are definitely
        // not going to use this provisional resource, as it was cancelled, notify the frame load delegate that the redirect
        // has ended.
        if (m_sentRedirectNotification)
            clientRedirectCancelledOrFinished(false);
    }
    
    
    loader->mainReceivedError(error, isComplete);
}

void FrameLoader::callContinueFragmentScrollAfterNavigationPolicy(void* argument,
    const ResourceRequest& request, PassRefPtr<FormState>, bool shouldContinue)
{
    FrameLoader* loader = static_cast<FrameLoader*>(argument);
    loader->continueFragmentScrollAfterNavigationPolicy(request, shouldContinue);
}

void FrameLoader::continueFragmentScrollAfterNavigationPolicy(const ResourceRequest& request, bool shouldContinue)
{
    KURL URL = request.url();
    
    bool isRedirect = m_quickRedirectComing;
    m_quickRedirectComing = false;
    
    m_documentLoader->replaceRequestURLForAnchorScroll(URL);
    if (!isRedirect && !shouldTreatURLAsSameAsCurrent(URL)) {
        // NB: must happen after _setURL, since we add based on the current request.
        // Must also happen before we openURL and displace the scroll position, since
        // adding the BF item will save away scroll state.
        
        // NB2:  If we were loading a long, slow doc, and the user anchor nav'ed before
        // it was done, currItem is now set the that slow doc, and prevItem is whatever was
        // before it.  Adding the b/f item will bump the slow doc down to prevItem, even
        // though its load is not yet done.  I think this all works out OK, for one because
        // we have already saved away the scroll and doc state for the long slow load,
        // but it's not an obvious case.

        addHistoryItemForFragmentScroll();
    }
    
    scrollToAnchor(URL);
    
    if (!isRedirect)
        // This will clear previousItem from the rest of the frame tree that didn't
        // doing any loading. We need to make a pass on this now, since for anchor nav
        // we'll not go through a real load and reach Completed state.
        checkLoadComplete();
 
    dispatchDidChangeLocationWithinPage();
    m_client->didFinishLoad();
}

void FrameLoader::opened()
{
    if (m_loadType == FrameLoadTypeStandard && m_documentLoader->isClientRedirect())
        updateHistoryForClientRedirect();

    if (m_documentLoader->isLoadingFromPageCache()) {
        // Force a layout to update view size and thereby update scrollbars.
        m_client->forceLayout();

        const ResponseVector& responses = m_documentLoader->responses();
        size_t count = responses.size();
        for (size_t i = 0; i < count; i++) {
            NSURLResponse *response = responses[i].nsURLResponse();
            // FIXME: If the WebKit client changes or cancels the request, this is not respected.
            ResourceError error;
            id identifier;
            NSURLRequest *request = [[NSURLRequest alloc] initWithURL:[response URL]];
            requestFromDelegate(request, identifier, error);
            sendRemainingDelegateMessages(identifier, response, (unsigned)[response expectedContentLength], error);
            [request release];
        }
        
        m_client->loadedFromPageCache();

        m_documentLoader->setPrimaryLoadComplete(true);

        // FIXME: Why only this frame and not parent frames?
        checkLoadCompleteForThisFrame();
    }
}

KURL FrameLoader::dataURLBaseFromRequest(const ResourceRequest& request) const
{
    if (WebDataRequestParameters* params = [request.nsURLRequest() _webDataRequestParametersForReading])
        return params->baseURL;
    return KURL();
}

const ResourceRequest& FrameLoader::initialRequest() const
{
    return activeDocumentLoader()->initialRequest();
}

void FrameLoader::receivedData(const char* data, int length)
{
    activeDocumentLoader()->receivedData(data, length);
}

void FrameLoader::setRequest(const ResourceRequest& request)
{
    activeDocumentLoader()->setRequest(request);
}

void FrameLoader::setResponse(const ResourceResponse& response)
{
    activeDocumentLoader()->setResponse(response);
}

void FrameLoader::mainReceivedError(const ResourceError& error, bool isComplete)
{
    activeDocumentLoader()->mainReceivedError(error, isComplete);
}

ResourceError FrameLoader::cancelledError(const ResourceRequest& request) const
{
    return m_client->cancelledError(request);
}

ResourceError FrameLoader::fileDoesNotExistError(const ResourceResponse& response) const
{
    return m_client->fileDoesNotExistError(response);    
}

bool FrameLoader::willUseArchive(ResourceLoader* loader, NSURLRequest *request, const KURL& originalURL) const
{
    return m_client->willUseArchive(loader, request, originalURL);
}

void FrameLoader::handleUnimplementablePolicy(const ResourceError& error)
{
    m_delegateIsHandlingUnimplementablePolicy = true;
    m_client->dispatchUnableToImplementPolicy(error);
    m_delegateIsHandlingUnimplementablePolicy = false;
}

void FrameLoader::cannotShowMIMEType(const ResourceResponse& response)
{
    handleUnimplementablePolicy(m_client->cannotShowMIMETypeError(response));
}

ResourceError FrameLoader::interruptionForPolicyChangeError(const ResourceRequest& request)
{
    return m_client->interruptForPolicyChangeError(request);
}

void FrameLoader::checkNavigationPolicy(const ResourceRequest& newRequest, NavigationPolicyDecisionFunction function, void* argument)
{
    checkNavigationPolicy(newRequest, activeDocumentLoader(), 0, function, argument);
}

void FrameLoader::checkContentPolicy(const String& MIMEType, ContentPolicyDecisionFunction function, void* argument)
{
    m_policyCheck.set(function, argument);
    m_client->dispatchDecidePolicyForMIMEType(&FrameLoader::continueAfterContentPolicy,
        MIMEType, activeDocumentLoader()->request());
}

bool FrameLoader::shouldReloadToHandleUnreachableURL(const ResourceRequest& request)
{
    KURL unreachableURL;
#if PLATFORM(MAC)
    unreachableURL = [request.nsURLRequest() _webDataRequestUnreachableURL];
#endif

    if (unreachableURL.isEmpty())
        return false;

    if (!isBackForwardLoadType(m_policyLoadType))
        return false;

    // We only treat unreachableURLs specially during the delegate callbacks
    // for provisional load errors and navigation policy decisions. The former
    // case handles well-formed URLs that can't be loaded, and the latter
    // case handles malformed URLs and unknown schemes. Loading alternate content
    // at other times behaves like a standard load.
    DocumentLoader* compareDocumentLoader = 0;
    if (m_delegateIsDecidingNavigationPolicy || m_delegateIsHandlingUnimplementablePolicy)
        compareDocumentLoader = m_policyDocumentLoader.get();
    else if (m_delegateIsHandlingProvisionalLoadError)
        compareDocumentLoader = m_provisionalDocumentLoader.get();

    return compareDocumentLoader && unreachableURL != compareDocumentLoader->request().url();
}

void FrameLoader::reloadAllowingStaleData(const String& encoding)
{
    if (!m_documentLoader)
        return;

    ResourceRequest request = m_documentLoader->request();
    KURL unreachableURL = m_documentLoader->unreachableURL();
    if (!unreachableURL.isEmpty())
        request.setURL(unreachableURL);

    request.setCachePolicy(ReturnCacheDataElseLoad);

    RefPtr<DocumentLoader> loader = m_client->createDocumentLoader(request);
    setPolicyDocumentLoader(loader.get());

    loader->setOverrideEncoding(encoding);

    load(loader.get(), FrameLoadTypeReloadAllowingStaleData, 0);
}

void FrameLoader::reload()
{
    if (!m_documentLoader)
        return;

    ResourceRequest& initialRequest = m_documentLoader->request();
    
    // If a window is created by javascript, its main frame can have an empty but non-nil URL.
    // Reloading in this case will lose the current contents (see 4151001).
    if (initialRequest.url().isEmpty())
        return;

    // Replace error-page URL with the URL we were trying to reach.
    KURL unreachableURL;
#if PLATFORM(MAC)
    unreachableURL = [initialRequest.nsURLRequest() _webDataRequestUnreachableURL];
#endif
    if (!unreachableURL.isEmpty())
        initialRequest = ResourceRequest(unreachableURL);
    
    RefPtr<DocumentLoader> loader = m_client->createDocumentLoader(initialRequest);
    setPolicyDocumentLoader(loader.get());

    ResourceRequest& request = loader->request();

    request.setCachePolicy(ReloadIgnoringCacheData);

    // If we're about to re-post, set up action so the application can warn the user.
    if (request.httpMethod() == "POST")
        loader->setTriggeringAction(NavigationAction(request.url(), NavigationTypeFormResubmitted));

    loader->setOverrideEncoding(m_documentLoader->overrideEncoding());
    
    load(loader.get(), FrameLoadTypeReload, 0);
}

void FrameLoader::committedLoad(DocumentLoader* loader, const char* data, int length)
{
    m_client->committedLoad(loader, data, length);
}

void FrameLoader::setMainDocumentError(DocumentLoader* loader, const ResourceError& error)
{
    m_client->setMainDocumentError(loader, error);
}

void FrameLoader::mainReceivedCompleteError(DocumentLoader* loader, const ResourceError& error)
{
    loader->setPrimaryLoadComplete(true);
    m_client->dispatchDidLoadMainResource(activeDocumentLoader());
    checkLoadComplete();
}

void FrameLoader::didChangeTitle(DocumentLoader* loader)
{
    m_client->didChangeTitle(loader);

    // The title doesn't get communicated to the WebView until we are committed.
    if (loader->isCommitted())
        if (NSURL *urlForHistory = canonicalURL(loader->urlForHistory().getNSURL())) {
            // Must update the entries in the back-forward list too.
            // This must go through the WebFrame because it has the right notion of the current b/f item.
            m_client->setTitle(loader->title(), urlForHistory);
            m_client->setMainFrameDocumentReady(true); // update observers with new DOMDocument
            m_client->dispatchDidReceiveTitle(loader->title());
        }
}

void FrameLoader::checkNewWindowPolicy(const NavigationAction& action, const ResourceRequest& request,
    PassRefPtr<FormState> formState, const String& frameName)
{
    m_policyCheck.set(request, formState, frameName,
        callContinueLoadAfterNewWindowPolicy, this);
    m_client->dispatchDecidePolicyForNewWindowAction(&FrameLoader::continueAfterNewWindowPolicy,
        action, request, frameName);
}

void FrameLoader::continueAfterNewWindowPolicy(PolicyAction policy)
{
    PolicyCheck check = m_policyCheck;
    m_policyCheck.clear();

    switch (policy) {
        case PolicyIgnore:
            check.clearRequest();
            break;
        case PolicyDownload:
            m_client->startDownload(check.request());
            check.clearRequest();
            break;
        case PolicyUse:
            break;
    }

    check.call(policy == PolicyUse);
}

void FrameLoader::checkNavigationPolicy(const ResourceRequest& request, DocumentLoader* loader,
    PassRefPtr<FormState> formState, NavigationPolicyDecisionFunction function, void* argument)
{
    NavigationAction action = loader->triggeringAction();
    if (action.isEmpty()) {
        action = NavigationAction(request.url(), NavigationTypeOther);
        loader->setTriggeringAction(action);
    }
        
    // Don't ask more than once for the same request or if we are loading an empty URL.
    // This avoids confusion on the part of the client.
    if (request == loader->lastCheckedRequest() || request.url().isEmpty()) {
        function(argument, request, 0, true);
        return;
    }
    
    // We are always willing to show alternate content for unreachable URLs;
    // treat it like a reload so it maintains the right state for b/f list.
#if PLATFORM(MAC)
    if ([request.nsURLRequest() _webDataRequestUnreachableURL]) {
        if (isBackForwardLoadType(m_policyLoadType))
            m_policyLoadType = FrameLoadTypeReload;
        function(argument, request, 0, true);
        return;
    }
#endif
    
    loader->setLastCheckedRequest(request);

    m_policyCheck.set(request, formState, function, argument);

    m_delegateIsDecidingNavigationPolicy = true;
    m_client->dispatchDecidePolicyForNavigationAction(&FrameLoader::continueAfterNavigationPolicy,
        action, request);
    m_delegateIsDecidingNavigationPolicy = false;
}

void FrameLoader::continueAfterNavigationPolicy(PolicyAction policy)
{
    PolicyCheck check = m_policyCheck;
    m_policyCheck.clear();

    switch (policy) {
        case PolicyIgnore:
            check.clearRequest();
            break;
        case PolicyDownload:
            m_client->startDownload(check.request());
            check.clearRequest();
            break;
        case PolicyUse: {
            ResourceRequest request(check.request());
            if (!m_client->canHandleRequest(request)) {
                handleUnimplementablePolicy(m_client->cannotShowURLError(check.request()));
                check.clearRequest();
            }
            break;
        }
    }

    check.call(policy == PolicyUse);
}

void FrameLoader::callContinueLoadAfterNavigationPolicy(void* argument,
    const ResourceRequest& request, PassRefPtr<FormState> formState, bool shouldContinue)
{
    FrameLoader* loader = static_cast<FrameLoader*>(argument);
    loader->continueLoadAfterNavigationPolicy(request, formState, shouldContinue);
}

void FrameLoader::continueLoadAfterNavigationPolicy(const ResourceRequest& request, PassRefPtr<FormState> formState, bool shouldContinue)
{
    // If we loaded an alternate page to replace an unreachableURL, we'll get in here with a
    // nil policyDataSource because loading the alternate page will have passed
    // through this method already, nested; otherwise, policyDataSource should still be set.
    ASSERT(m_policyDocumentLoader || !m_provisionalDocumentLoader->unreachableURL().isEmpty());

    bool isTargetItem = m_provisionalHistoryItem ? m_provisionalHistoryItem->isTargetItem() : false;

    // Two reasons we can't continue:
    //    1) Navigation policy delegate said we can't so request is nil. A primary case of this 
    //       is the user responding Cancel to the form repost nag sheet.
    //    2) User responded Cancel to an alert popped up by the before unload event handler.
    // The "before unload" event handler runs only for the main frame.
    bool canContinue = shouldContinue && (!isLoadingMainFrame() || Mac(m_frame)->shouldClose());

    if (!canContinue) {
        // If we were waiting for a quick redirect, but the policy delegate decided to ignore it, then we 
        // need to report that the client redirect was cancelled.
        if (m_quickRedirectComing)
            clientRedirectCancelledOrFinished(false);

        setPolicyDocumentLoader(0);

        // If the navigation request came from the back/forward menu, and we punt on it, we have the 
        // problem that we have optimistically moved the b/f cursor already, so move it back.  For sanity, 
        // we only do this when punting a navigation for the target frame or top-level frame.  
        if ((isTargetItem || isLoadingMainFrame()) && isBackForwardLoadType(m_policyLoadType) && m_frame->page()) {
            Frame* mainFrame = m_frame->page()->mainFrame();
            if (HistoryItem* resetItem = mainFrame->loader()->m_currentHistoryItem.get())
                m_frame->page()->backForwardList()->goToItem(resetItem);
        }
        return;
    }

    FrameLoadType type = m_policyLoadType;
    stopAllLoaders();
    setProvisionalDocumentLoader(m_policyDocumentLoader.get());
    m_loadType = type;
    setState(FrameStateProvisional);

    setPolicyDocumentLoader(0);

    if (isBackForwardLoadType(type) && loadProvisionalItemFromPageCache())
        return;

    if (formState)
        m_client->dispatchWillSubmitForm(&FrameLoader::continueAfterWillSubmitForm, formState);
    else
        continueAfterWillSubmitForm();
}

void FrameLoader::checkLoadCompleteForThisFrame()
{
    ASSERT(m_client->hasWebView());

    switch (m_state) {
        case FrameStateProvisional: {
            if (m_delegateIsHandlingProvisionalLoadError)
                return;

            RefPtr<DocumentLoader> pdl = m_provisionalDocumentLoader;
            if (!pdl)
                return;
                
            // If we've received any errors we may be stuck in the provisional state and actually complete.
            NSError *error = pdl->mainDocumentError();
            if (!error)
                return;

            // Check all children first.
            RefPtr<HistoryItem> item;
            if (isBackForwardLoadType(loadType()) && m_frame == m_frame->page()->mainFrame())
                item = m_currentHistoryItem;
                
            bool shouldReset = true;
            if (!pdl->isLoadingInAPISense()) {
                m_delegateIsHandlingProvisionalLoadError = true;
                m_client->dispatchDidFailProvisionalLoad(error);
                m_delegateIsHandlingProvisionalLoadError = false;

                // FIXME: can stopping loading here possibly have any effect, if isLoading is false,
                // which it must be to be in this branch of the if? And is it OK to just do a full-on
                // stopAllLoaders instead of stopLoadingSubframes?
                stopLoadingSubframes();
                pdl->stopLoading();

                // Finish resetting the load state, but only if another load hasn't been started by the
                // delegate callback.
                if (pdl == m_provisionalDocumentLoader)
                    clearProvisionalLoad();
                else if (m_documentLoader) {
                    KURL unreachableURL = m_documentLoader->unreachableURL();
                    if (!unreachableURL.isEmpty() && unreachableURL == pdl->request().url())
                        shouldReset = false;
                }
            }
            if (shouldReset && item && m_frame->page())
                 m_frame->page()->backForwardList()->goToItem(item.get());

            return;
        }
        
        case FrameStateCommittedPage: {
            DocumentLoader* dl = m_documentLoader.get();            
            if (dl->isLoadingInAPISense())
                return;

            markLoadComplete();

            // FIXME: Is this subsequent work important if we already navigated away?
            // Maybe there are bugs because of that, or extra work we can skip because
            // the new page is ready.

            m_client->forceLayoutForNonHTML();
             
            // If the user had a scroll point, scroll to it, overriding the anchor point if any.
            if ((isBackForwardLoadType(m_loadType) || m_loadType == FrameLoadTypeReload)
                    && m_frame->page() && m_frame->page()->backForwardList())
                restoreScrollPositionAndViewState();

            NSError *error = dl->mainDocumentError();
            if (error)
                m_client->dispatchDidFailLoad(error);
            else
                m_client->dispatchDidFinishLoad();

            m_client->progressCompleted();
            return;
        }
        
        case FrameStateComplete:
            // Even if already complete, we might have set a previous item on a frame that
            // didn't do any data loading on the past transaction. Make sure to clear these out.
            m_client->frameLoadCompleted();
            return;
    }

    ASSERT_NOT_REACHED();
}

void FrameLoader::callContinueLoadAfterNewWindowPolicy(void* argument,
    const ResourceRequest& request, PassRefPtr<FormState> formState, const String& frameName, bool shouldContinue)
{
    FrameLoader* loader = static_cast<FrameLoader*>(argument);
    loader->continueLoadAfterNewWindowPolicy(request, formState, frameName, shouldContinue);
}

void FrameLoader::continueLoadAfterNewWindowPolicy(const ResourceRequest& request,
    PassRefPtr<FormState> formState, const String& frameName, bool shouldContinue)
{
    if (!shouldContinue)
        return;

    RefPtr<Frame> frame = m_frame;
    RefPtr<Frame> mainFrame = m_client->dispatchCreatePage(nil);
    if (!mainFrame)
        return;

    mainFrame->tree()->setName(frameName);
    mainFrame->loader()->m_client->dispatchShow();
    mainFrame->loader()->setOpener(frame.get());
    mainFrame->loader()->load(request, NavigationAction(), FrameLoadTypeStandard, formState);
}

void FrameLoader::sendRemainingDelegateMessages(id identifier, NSURLResponse *response, unsigned length, const ResourceError& error)
{    
    if (response)
        m_client->dispatchDidReceiveResponse(m_documentLoader.get(), identifier, response);
    
    if (length > 0)
        m_client->dispatchDidReceiveContentLength(m_documentLoader.get(), identifier, length);
    
    if (!error)
        m_client->dispatchDidFinishLoading(m_documentLoader.get(), identifier);
    else
        m_client->dispatchDidFailLoading(m_documentLoader.get(), identifier, error);
}

NSURLRequest *FrameLoader::requestFromDelegate(NSURLRequest *request, id& identifier, ResourceError& error)
{
    ASSERT(request != nil);

    identifier = m_client->dispatchIdentifierForInitialRequest(m_documentLoader.get(), request); 
    NSURLRequest *newRequest = m_client->dispatchWillSendRequest(m_documentLoader.get(), identifier, request, nil);

    if (newRequest == nil)
        error = m_client->cancelledError(request);
    else
        error = ResourceError();

    return newRequest;
}

void FrameLoader::loadedResourceFromMemoryCache(NSURLRequest *request, NSURLResponse *response, int length)
{
    if (m_client->dispatchDidLoadResourceFromMemoryCache(m_documentLoader.get(), request, response, length))
        return;

    id identifier;
    ResourceError error;
    requestFromDelegate(request, identifier, error);
    sendRemainingDelegateMessages(identifier, response, length, error);
}

void FrameLoader::post(const KURL& URL, const String& referrer, const String& frameName, PassRefPtr<FormData> formData, 
    const String& contentType, Event* event, HTMLFormElement* form, const HashMap<String, String>& formValues)
{
    // When posting, use the NSURLRequestReloadIgnoringCacheData load flag.
    // This prevents a potential bug which may cause a page with a form that uses itself
    // as an action to be returned from the cache without submitting.

    // FIXME: Where's the code that implements what the comment above says?

    ResourceRequest request(URL);
    addExtraFieldsToRequest(request, true, true);

    if (!referrer.isEmpty())
        request.setHTTPReferrer(referrer);
    request.setHTTPMethod("POST");
    request.setHTTPBody(formData);
    request.setHTTPContentType(contentType);

    NavigationAction action(URL, FrameLoadTypeStandard, true, event);

    RefPtr<FormState> formState;
    if (form && !formValues.isEmpty())
        formState = FormState::create(form, formValues, m_frame);

    if (!frameName.isEmpty()) {
        if (Frame* targetFrame = m_frame->tree()->find(frameName))
            targetFrame->loader()->load(request, action, FrameLoadTypeStandard, formState.release());
        else
            checkNewWindowPolicy(action, request, formState.release(), frameName);
    } else
        load(request, action, FrameLoadTypeStandard, formState.release());
}

void FrameLoader::addExtraFieldsToRequest(NSMutableURLRequest *request, bool mainResource, bool alwaysFromRequest)
{
    applyUserAgent(request);
    
    if (m_loadType == FrameLoadTypeReload)
        [request setValue:@"max-age=0" forHTTPHeaderField:@"Cache-Control"];
    
    // Don't set the cookie policy URL if it's already been set.
    if (![request mainDocumentURL]) {
        if (mainResource && (isLoadingMainFrame() || alwaysFromRequest))
            [request setMainDocumentURL:[request URL]];
        else
            [request setMainDocumentURL:m_frame->page()->mainFrame()->loader()->url().getNSURL()];
    }
    
    if (mainResource)
        [request setValue:@"text/xml,application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,image/png,*/*;q=0.5" forHTTPHeaderField:@"Accept"];
}

bool FrameLoader::isReloading() const
{
    return documentLoader()->request().cachePolicy() == ReloadIgnoringCacheData;
}

String FrameLoader::referrer() const
{
    return documentLoader()->request().httpReferrer();
}

void FrameLoader::loadEmptyDocumentSynchronously()
{
    ResourceRequest request(KURL(""));
    load(request);
}

void FrameLoader::loadResourceSynchronously(const ResourceRequest& request, ResourceResponse& response, Vector<char>& data)
{
    // Since this is a subresource, we can load any URL (we ignore the return value).
    // But we still want to know whether we should hide the referrer or not, so we call the canLoad method.
    String referrer = m_outgoingReferrer;
    bool hideReferrer;
    canLoad(request.url(), referrer, hideReferrer);
    if (hideReferrer)
        referrer = String();
    
    ResourceRequest initialRequest = request;
    initialRequest.setTimeoutInterval(10);
    
    if (initialRequest.isConditional())
        initialRequest.setCachePolicy(ReloadIgnoringCacheData);
    else
        initialRequest.setCachePolicy(documentLoader()->request().cachePolicy());
    
    if (!referrer.isEmpty())
        initialRequest.setHTTPReferrer(referrer);
    
    initialRequest.setMainDocumentURL(m_frame->page()->mainFrame()->loader()->documentLoader()->request().url());
    initialRequest.setHTTPUserAgent(client()->userAgent());
    
    ResourceError error;
    id identifier = nil;    
    NSURLRequest *newRequest = requestFromDelegate(initialRequest.nsURLRequest(), identifier, error);

    if (error.isNull()) {
        ASSERT(newRequest != nil);
        ResourceError error;
        
        didTellBridgeAboutLoad(KURL([newRequest URL]).url());

        ResourceHandle::loadResourceSynchronously(newRequest, error, response, data);
    }
    
    sendRemainingDelegateMessages(identifier, response.nsURLResponse(), data.size(), error);
}

Frame* FrameLoader::createFrame(const KURL& url, const String& name, HTMLFrameOwnerElement* ownerElement, const String& referrer)
{
    BOOL allowsScrolling = YES;
    int marginWidth = -1;
    int marginHeight = -1;
    if (ownerElement->hasTagName(frameTag) || ownerElement->hasTagName(iframeTag)) {
        HTMLFrameElement* o = static_cast<HTMLFrameElement*>(ownerElement);
        allowsScrolling = o->scrollingMode() != ScrollbarAlwaysOff;
        marginWidth = o->getMarginWidth();
        marginHeight = o->getMarginHeight();
    }

    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    
    return [Mac(m_frame)->bridge() createChildFrameNamed:name
                                                 withURL:url.getNSURL()
                                                referrer:referrer 
                                              ownerElement:ownerElement
                                         allowsScrolling:allowsScrolling
                                             marginWidth:marginWidth
                                            marginHeight:marginHeight];

    END_BLOCK_OBJC_EXCEPTIONS;
    return 0;
}

ObjectContentType FrameLoader::objectContentType(const KURL& url, const String& mimeType)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    return (ObjectContentType)[Mac(m_frame)->bridge() determineObjectFromMIMEType:mimeType URL:url.getNSURL()];
    END_BLOCK_OBJC_EXCEPTIONS;
    return ObjectContentNone;
}

static NSArray* nsArray(const Vector<String>& vector)
{
    unsigned len = vector.size();
    NSMutableArray* array = [NSMutableArray arrayWithCapacity:len];
    for (unsigned x = 0; x < len; x++)
        [array addObject:vector[x]];
    return array;
}

Widget* FrameLoader::createPlugin(Element* element, const KURL& url,
    const Vector<String>& paramNames, const Vector<String>& paramValues,
    const String& mimeType)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    return new Widget([Mac(m_frame)->bridge() viewForPluginWithURL:url.getNSURL()
                                  attributeNames:nsArray(paramNames)
                                  attributeValues:nsArray(paramValues)
                                  MIMEType:mimeType
                                  DOMElement:[DOMElement _elementWith:element]
                                loadManually:m_frame->document()->isPluginDocument()]);
    END_BLOCK_OBJC_EXCEPTIONS;
    return 0;
}

void FrameLoader::redirectDataToPlugin(Widget* pluginWidget)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    [Mac(m_frame)->bridge() redirectDataToPlugin:pluginWidget->getView()];
    END_BLOCK_OBJC_EXCEPTIONS;
}

Widget* FrameLoader::createJavaAppletWidget(const IntSize& size, Element* element, const HashMap<String, String>& args)
{
    Widget* result = new Widget;
    
    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    
    NSMutableArray *attributeNames = [[NSMutableArray alloc] init];
    NSMutableArray *attributeValues = [[NSMutableArray alloc] init];
    
    DeprecatedString baseURLString;
    HashMap<String, String>::const_iterator end = args.end();
    for (HashMap<String, String>::const_iterator it = args.begin(); it != end; ++it) {
        if (it->first.lower() == "baseurl")
            baseURLString = it->second.deprecatedString();
        [attributeNames addObject:it->first];
        [attributeValues addObject:it->second];
    }
    
    if (baseURLString.isEmpty())
        baseURLString = m_frame->document()->baseURL();

    result->setView([Mac(m_frame)->bridge() viewForJavaAppletWithFrame:NSMakeRect(0, 0, size.width(), size.height())
                                         attributeNames:attributeNames
                                        attributeValues:attributeValues
                                                baseURL:completeURL(baseURLString).getNSURL()
                                             DOMElement:[DOMElement _elementWith:element]]);
    [attributeNames release];
    [attributeValues release];
    m_frame->view()->addChild(result);
    
    END_BLOCK_OBJC_EXCEPTIONS;
    
    return result;
}

void FrameLoader::partClearedInBegin()
{
    if (m_frame->settings()->isJavaScriptEnabled())
        [Mac(m_frame)->bridge() windowObjectCleared];
}

String FrameLoader::overrideMediaType() const
{
    NSString *overrideType = [Mac(m_frame)->bridge() overrideMediaType];
    if (overrideType)
        return overrideType;
    return String();
}

NSData *FrameLoader::mainResourceData() const
{
    if (!m_mainResourceLoader)
        return nil;
    return m_mainResourceLoader->resourceData();
}

bool FrameLoader::canGoBackOrForward(int distance) const
{
    return [Mac(m_frame)->bridge() canGoBackOrForward:distance];
}

KURL FrameLoader::originalRequestURL() const
{
    return activeDocumentLoader()->initialRequest().url();
}

int FrameLoader::getHistoryLength()
{
    return [Mac(m_frame)->bridge() historyLength];
}

KURL FrameLoader::historyURL(int distance)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    return [Mac(m_frame)->bridge() historyURL:distance];
    END_BLOCK_OBJC_EXCEPTIONS;
    return KURL();
}

void FrameLoader::didFinishLoad(ResourceLoader* loader)
{    
    m_client->completeProgress(loader->identifier());
    m_client->dispatchDidFinishLoading(activeDocumentLoader(), loader->identifier());
}

void FrameLoader::setTitle(const String& title)
{
    documentLoader()->setTitle(title);
}

void FrameLoader::closeBridge()
{
    [Mac(m_frame)->bridge() close];
}

PolicyCheck::PolicyCheck()
    : m_navigationFunction(0)
    , m_newWindowFunction(0)
    , m_contentFunction(0)
{
}

void PolicyCheck::clear()
{
    clearRequest();
    m_navigationFunction = 0;
    m_newWindowFunction = 0;
    m_contentFunction = 0;
}

void PolicyCheck::set(const ResourceRequest& request, PassRefPtr<FormState> formState,
    NavigationPolicyDecisionFunction function, void* argument)
{
    m_request = request;
    m_formState = formState;
    m_frameName = String();

    m_navigationFunction = function;
    m_newWindowFunction = 0;
    m_contentFunction = 0;
    m_argument = argument;
}

void PolicyCheck::set(const ResourceRequest& request, PassRefPtr<FormState> formState,
    const String& frameName, NewWindowPolicyDecisionFunction function, void* argument)
{
    m_request = request;
    m_formState = formState;
    m_frameName = frameName;

    m_navigationFunction = 0;
    m_newWindowFunction = function;
    m_contentFunction = 0;
    m_argument = argument;
}

void PolicyCheck::set(ContentPolicyDecisionFunction function, void* argument)
{
    m_request = ResourceRequest();
    m_formState = 0;
    m_frameName = String();

    m_navigationFunction = 0;
    m_newWindowFunction = 0;
    m_contentFunction = function;
    m_argument = argument;
}

void PolicyCheck::call(bool shouldContinue)
{
    if (m_navigationFunction)
        m_navigationFunction(m_argument, m_request, m_formState.get(), shouldContinue);
    if (m_newWindowFunction)
        m_newWindowFunction(m_argument, m_request, m_formState.get(), m_frameName, shouldContinue);
    ASSERT(!m_contentFunction);
}

void PolicyCheck::call(PolicyAction action)
{
    ASSERT(!m_navigationFunction);
    ASSERT(!m_newWindowFunction);
    ASSERT(m_contentFunction);
    m_contentFunction(m_argument, action);
}

void PolicyCheck::clearRequest()
{
    m_request = ResourceRequest();
    m_formState = 0;
    m_frameName = String();
}

}
