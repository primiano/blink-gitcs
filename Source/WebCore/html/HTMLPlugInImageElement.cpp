/*
 * Copyright (C) 2008, 2011, 2012 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "HTMLPlugInImageElement.h"

#include "Chrome.h"
#include "ChromeClient.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "FrameView.h"
#include "HTMLDivElement.h"
#include "HTMLImageLoader.h"
#include "Image.h"
#include "LocalizedStrings.h"
#include "Logging.h"
#include "MouseEvent.h"
#include "NodeRenderStyle.h"
#include "NodeRenderingContext.h"
#include "Page.h"
#include "PlugInClient.h"
#include "PlugInOriginHash.h"
#include "PluginViewBase.h"
#include "RenderEmbeddedObject.h"
#include "RenderImage.h"
#include "RenderSnapshottedPlugIn.h"
#include "SchemeRegistry.h"
#include "SecurityOrigin.h"
#include "Settings.h"
#include "ShadowRoot.h"
#include "StyleResolver.h"
#include "Text.h"

namespace WebCore {

using namespace HTMLNames;

static const int autoStartPlugInSizeDimensionThreshold = 1;
static const int autoShowLabelSizeWidthThreshold = 400;
static const int autoShowLabelSizeHeightThreshold = 300;

static const int sizingTinyDimensionThreshold = 40;
static const int sizingSmallWidthThreshold = 250;
static const int sizingMediumWidthThreshold = 600;

// This delay should not exceed the snapshot delay in PluginView.cpp
static const double simulatedMouseClickTimerDelay = .75;

HTMLPlugInImageElement::HTMLPlugInImageElement(const QualifiedName& tagName, Document* document, bool createdByParser, PreferPlugInsForImagesOption preferPlugInsForImagesOption)
    : HTMLPlugInElement(tagName, document)
    // m_needsWidgetUpdate(!createdByParser) allows HTMLObjectElement to delay
    // widget updates until after all children are parsed.  For HTMLEmbedElement
    // this delay is unnecessary, but it is simpler to make both classes share
    // the same codepath in this class.
    , m_needsWidgetUpdate(!createdByParser)
    , m_shouldPreferPlugInsForImages(preferPlugInsForImagesOption == ShouldPreferPlugInsForImages)
    , m_needsDocumentActivationCallbacks(false)
    , m_shouldShowSnapshotLabelAutomatically(false)
    , m_simulatedMouseClickTimer(this, &HTMLPlugInImageElement::simulatedMouseClickTimerFired, simulatedMouseClickTimerDelay)
    , m_swapRendererTimer(this, &HTMLPlugInImageElement::swapRendererTimerFired)
{
    setHasCustomStyleCallbacks();
}

HTMLPlugInImageElement::~HTMLPlugInImageElement()
{
    if (m_needsDocumentActivationCallbacks)
        document()->unregisterForPageCacheSuspensionCallbacks(this);
}

RenderEmbeddedObject* HTMLPlugInImageElement::renderEmbeddedObject() const
{
    // HTMLObjectElement and HTMLEmbedElement may return arbitrary renderers
    // when using fallback content.
    if (!renderer() || !renderer()->isEmbeddedObject())
        return 0;
    return toRenderEmbeddedObject(renderer());
}

bool HTMLPlugInImageElement::isImageType()
{
    if (m_serviceType.isEmpty() && protocolIs(m_url, "data"))
        m_serviceType = mimeTypeFromDataURL(m_url);

    if (Frame* frame = document()->frame()) {
        KURL completedURL = document()->completeURL(m_url);
        return frame->loader()->client()->objectContentType(completedURL, m_serviceType, shouldPreferPlugInsForImages()) == ObjectContentImage;
    }

    return Image::supportsType(m_serviceType);
}

// We don't use m_url, as it may not be the final URL that the object loads,
// depending on <param> values. 
bool HTMLPlugInImageElement::allowedToLoadFrameURL(const String& url)
{
    KURL completeURL = document()->completeURL(url);

    if (contentFrame() && protocolIsJavaScript(completeURL)
        && !document()->securityOrigin()->canAccess(contentDocument()->securityOrigin()))
        return false;

    return document()->frame()->isURLAllowed(completeURL);
}

// We don't use m_url, or m_serviceType as they may not be the final values
// that <object> uses depending on <param> values. 
bool HTMLPlugInImageElement::wouldLoadAsNetscapePlugin(const String& url, const String& serviceType)
{
    ASSERT(document());
    ASSERT(document()->frame());
    KURL completedURL;
    if (!url.isEmpty())
        completedURL = document()->completeURL(url);

    FrameLoader* frameLoader = document()->frame()->loader();
    ASSERT(frameLoader);
    if (frameLoader->client()->objectContentType(completedURL, serviceType, shouldPreferPlugInsForImages()) == ObjectContentNetscapePlugin)
        return true;
    return false;
}

RenderObject* HTMLPlugInImageElement::createRenderer(RenderArena* arena, RenderStyle* style)
{
    // Once a PlugIn Element creates its renderer, it needs to be told when the Document goes
    // inactive or reactivates so it can clear the renderer before going into the page cache.
    if (!m_needsDocumentActivationCallbacks) {
        m_needsDocumentActivationCallbacks = true;
        document()->registerForPageCacheSuspensionCallbacks(this);
    }

    if (displayState() == DisplayingSnapshot) {
        RenderSnapshottedPlugIn* renderSnapshottedPlugIn = new (arena) RenderSnapshottedPlugIn(this);
        renderSnapshottedPlugIn->updateSnapshot(m_snapshotImage);
        if (m_shouldShowSnapshotLabelAutomatically)
            renderSnapshottedPlugIn->setShouldShowLabelAutomatically();
        return renderSnapshottedPlugIn;
    }

    // Fallback content breaks the DOM->Renderer class relationship of this
    // class and all superclasses because createObject won't necessarily
    // return a RenderEmbeddedObject, RenderPart or even RenderWidget.
    if (useFallbackContent())
        return RenderObject::createObject(this, style);

    if (isImageType()) {
        RenderImage* image = new (arena) RenderImage(this);
        image->setImageResource(RenderImageResource::create());
        return image;
    }

    return new (arena) RenderEmbeddedObject(this);
}

bool HTMLPlugInImageElement::willRecalcStyle(StyleChange)
{
    // FIXME: Why is this necessary?  Manual re-attach is almost always wrong.
    if (!useFallbackContent() && needsWidgetUpdate() && renderer() && !isImageType() && (displayState() != DisplayingSnapshot))
        reattach();
    return true;
}

void HTMLPlugInImageElement::attach()
{
    PostAttachCallbackDisabler disabler(this);

    bool isImage = isImageType();
    
    if (!isImage)
        queuePostAttachCallback(&HTMLPlugInImageElement::updateWidgetCallback, this);

    HTMLPlugInElement::attach();

    if (isImage && renderer() && !useFallbackContent()) {
        if (!m_imageLoader)
            m_imageLoader = adoptPtr(new HTMLImageLoader(this));
        m_imageLoader->updateFromElement();
    }
}

void HTMLPlugInImageElement::detach()
{
    // FIXME: Because of the insanity that is HTMLPlugInImageElement::recalcStyle,
    // we can end up detaching during an attach() call, before we even have a
    // renderer.  In that case, don't mark the widget for update.
    if (attached() && renderer() && !useFallbackContent())
        // Update the widget the next time we attach (detaching destroys the plugin).
        setNeedsWidgetUpdate(true);
    HTMLPlugInElement::detach();
}

void HTMLPlugInImageElement::updateWidgetIfNecessary()
{
    document()->updateStyleIfNeeded();

    if (!needsWidgetUpdate() || useFallbackContent() || isImageType())
        return;

    if (!renderEmbeddedObject() || renderEmbeddedObject()->showsUnavailablePluginIndicator())
        return;

    updateWidget(CreateOnlyNonNetscapePlugins);
}

void HTMLPlugInImageElement::finishParsingChildren()
{
    HTMLPlugInElement::finishParsingChildren();
    if (useFallbackContent())
        return;
    
    setNeedsWidgetUpdate(true);
    if (inDocument())
        setNeedsStyleRecalc();    
}

void HTMLPlugInImageElement::didMoveToNewDocument(Document* oldDocument)
{
    if (m_needsDocumentActivationCallbacks) {
        if (oldDocument)
            oldDocument->unregisterForPageCacheSuspensionCallbacks(this);
        document()->registerForPageCacheSuspensionCallbacks(this);
    }

    if (m_imageLoader)
        m_imageLoader->elementDidMoveToNewDocument();
    HTMLPlugInElement::didMoveToNewDocument(oldDocument);
}

void HTMLPlugInImageElement::documentWillSuspendForPageCache()
{
    if (RenderStyle* renderStyle = this->renderStyle()) {
        m_customStyleForPageCache = RenderStyle::clone(renderStyle);
        m_customStyleForPageCache->setDisplay(NONE);
        recalcStyle(Force);
    }

    HTMLPlugInElement::documentWillSuspendForPageCache();
}

void HTMLPlugInImageElement::documentDidResumeFromPageCache()
{
    if (m_customStyleForPageCache) {
        m_customStyleForPageCache = 0;
        recalcStyle(Force);
    }
    
    HTMLPlugInElement::documentDidResumeFromPageCache();
}

PassRefPtr<RenderStyle> HTMLPlugInImageElement::customStyleForRenderer()
{
    if (!m_customStyleForPageCache)
        return document()->styleResolver()->styleForElement(this);
    return m_customStyleForPageCache;
}

void HTMLPlugInImageElement::updateWidgetCallback(Node* n, unsigned)
{
    static_cast<HTMLPlugInImageElement*>(n)->updateWidgetIfNecessary();
}

void HTMLPlugInImageElement::updateSnapshot(PassRefPtr<Image> image)
{
    if (displayState() > DisplayingSnapshot)
        return;

    m_snapshotImage = image;
    if (renderer()->isSnapshottedPlugIn()) {
        toRenderSnapshottedPlugIn(renderer())->updateSnapshot(image);
        return;
    }

    setDisplayState(DisplayingSnapshot);
    m_swapRendererTimer.startOneShot(0);
}

static AtomicString classNameForShadowRootSize(const IntSize& viewContentsSize, const Node* node)
{
    DEFINE_STATIC_LOCAL(const AtomicString, plugInTinySizeClassName, ("tiny", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(const AtomicString, plugInSmallSizeClassName, ("small", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(const AtomicString, plugInMediumSizeClassName, ("medium", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(const AtomicString, plugInLargeSizeClassName, ("large", AtomicString::ConstructFromLiteral));

    LayoutRect plugInClipRect = node->renderer()->absoluteClippedOverflowRect();
    LayoutRect viewContentsRect(LayoutPoint::zero(), LayoutSize(viewContentsSize));
    if (!viewContentsRect.contains(plugInClipRect)) {
        LOG(Plugins, "%p Plug-in rect: (%d %d, %d %d) not contained in document of size %d %d", node, plugInClipRect.pixelSnappedX(), plugInClipRect.pixelSnappedY(), plugInClipRect.pixelSnappedWidth(), plugInClipRect.pixelSnappedHeight(), viewContentsSize.width(), viewContentsSize.height());
        return nullAtom;
    }

    if (plugInClipRect.pixelSnappedWidth() < sizingTinyDimensionThreshold || plugInClipRect.pixelSnappedHeight() < sizingTinyDimensionThreshold) {
        LOG(Plugins, "%p Tiny Size: %d %d", node, plugInClipRect.pixelSnappedWidth(), plugInClipRect.pixelSnappedHeight());
        return plugInTinySizeClassName;
    }

    if (plugInClipRect.pixelSnappedWidth() < sizingSmallWidthThreshold) {
        LOG(Plugins, "%p Small Size: %d %d", node, plugInClipRect.pixelSnappedWidth(), plugInClipRect.pixelSnappedHeight());
        return plugInSmallSizeClassName;
    }

    if (plugInClipRect.pixelSnappedWidth() < sizingMediumWidthThreshold) {
        LOG(Plugins, "%p Medium Size: %d %d", node, plugInClipRect.pixelSnappedWidth(), plugInClipRect.pixelSnappedHeight());
        return plugInMediumSizeClassName;
    }

    return plugInLargeSizeClassName;
}

void HTMLPlugInImageElement::updateSnapshotInfo()
{
    ShadowRoot* root = userAgentShadowRoot();
    if (!root)
        return;

    Element* shadowContainer = static_cast<Element*>(root->firstChild());
    shadowContainer->setAttribute(classAttr, classNameForShadowRootSize(document()->page()->mainFrame()->view()->contentsSize(), this));   
}

void HTMLPlugInImageElement::didAddUserAgentShadowRoot(ShadowRoot* root)
{
    Document* doc = document();

    RefPtr<Element> shadowContainer = HTMLDivElement::create(doc);
    shadowContainer->setPseudo(AtomicString("-webkit-snapshotted-plugin-content", AtomicString::ConstructFromLiteral));

    RefPtr<Element> container = HTMLDivElement::create(doc);
    container->setAttribute(classAttr, AtomicString("snapshot-container", AtomicString::ConstructFromLiteral));

    RefPtr<Element> overlay = HTMLDivElement::create(doc);
    overlay->setAttribute(classAttr, AtomicString("snapshot-overlay", AtomicString::ConstructFromLiteral));
    container->appendChild(overlay, ASSERT_NO_EXCEPTION);

    RefPtr<Element> label = HTMLDivElement::create(doc);
    label->setAttribute(classAttr, AtomicString("snapshot-label", AtomicString::ConstructFromLiteral));

    String titleText = snapshottedPlugInLabelTitle();
    String subtitleText = snapshottedPlugInLabelSubtitle();
    if (document()->page()) {
        String clientTitleText = document()->page()->chrome()->client()->plugInStartLabelTitle();
        if (!clientTitleText.isEmpty())
            titleText = clientTitleText;
        String clientSubtitleText = document()->page()->chrome()->client()->plugInStartLabelSubtitle();
        if (!clientSubtitleText.isEmpty())
            subtitleText = clientSubtitleText;
    }

    RefPtr<Element> title = HTMLDivElement::create(doc);
    title->setAttribute(classAttr, AtomicString("snapshot-title", AtomicString::ConstructFromLiteral));
    title->appendChild(doc->createTextNode(titleText), ASSERT_NO_EXCEPTION);
    label->appendChild(title, ASSERT_NO_EXCEPTION);

    RefPtr<Element> subTitle = HTMLDivElement::create(doc);
    subTitle->setAttribute(classAttr, AtomicString("snapshot-subtitle", AtomicString::ConstructFromLiteral));
    subTitle->appendChild(doc->createTextNode(subtitleText), ASSERT_NO_EXCEPTION);
    label->appendChild(subTitle, ASSERT_NO_EXCEPTION);

    container->appendChild(label, ASSERT_NO_EXCEPTION);

    shadowContainer->appendChild(container, ASSERT_NO_EXCEPTION);
    root->appendChild(shadowContainer, ASSERT_NO_EXCEPTION);
}

void HTMLPlugInImageElement::swapRendererTimerFired(Timer<HTMLPlugInImageElement>*)
{
    ASSERT(displayState() == DisplayingSnapshot);
    if (userAgentShadowRoot())
        return;

    // Create a shadow root, which will trigger the code to add a snapshot container
    // and reattach, thus making a new Renderer.
    ensureUserAgentShadowRoot();
}

void HTMLPlugInImageElement::userDidClickSnapshot(PassRefPtr<MouseEvent> event)
{
    m_pendingClickEventFromSnapshot = event;
    if (document()->page() && !SchemeRegistry::shouldTreatURLSchemeAsLocal(document()->page()->mainFrame()->document()->baseURL().protocol()))
        document()->page()->plugInClient()->addAutoStartOrigin(document()->page()->mainFrame()->document()->baseURL().host(), m_plugInOriginHash);

    reattach();
}

void HTMLPlugInImageElement::dispatchPendingMouseClick()
{
    ASSERT(!m_simulatedMouseClickTimer.isActive());
    m_simulatedMouseClickTimer.restart();
}

void HTMLPlugInImageElement::simulatedMouseClickTimerFired(DeferrableOneShotTimer<HTMLPlugInImageElement>*)
{
    ASSERT(displayState() == PlayingWithPendingMouseClick);
    ASSERT(m_pendingClickEventFromSnapshot);

    dispatchSimulatedClick(m_pendingClickEventFromSnapshot.get(), SendMouseOverUpDownEvents, DoNotShowPressedLook);

    setDisplayState(Playing);
    m_pendingClickEventFromSnapshot = nullptr;
}

static bool shouldPlugInShowLabelAutomatically(const IntSize& viewContentsSize, const Node* node)
{
    LayoutRect plugInClipRect = node->renderer()->absoluteClippedOverflowRect();
    LayoutRect viewContentsRect(LayoutPoint::zero(), LayoutSize(viewContentsSize));
    if (!viewContentsRect.contains(plugInClipRect)) {
        LOG(Plugins, "%p Plug-in rect: (%d %d, %d %d) not contained in document of size %d %d", node, plugInClipRect.pixelSnappedX(), plugInClipRect.pixelSnappedY(), plugInClipRect.pixelSnappedWidth(), plugInClipRect.pixelSnappedHeight(), viewContentsSize.width(), viewContentsSize.height());
        return false;
    }

    if (plugInClipRect.pixelSnappedWidth() < autoShowLabelSizeWidthThreshold
        || plugInClipRect.pixelSnappedHeight() < autoShowLabelSizeHeightThreshold) {
        LOG(Plugins, "%p Size: %d %d", node, plugInClipRect.pixelSnappedWidth(), plugInClipRect.pixelSnappedHeight());
        return false;
    }

    LOG(Plugins, "%p Auto-show label", node);
    return true;
}

void HTMLPlugInImageElement::subframeLoaderWillCreatePlugIn(const KURL& url)
{
    if (!document()->page()
        || !document()->page()->settings()->plugInSnapshottingEnabled())
        return;

    if (document()->isPluginDocument() && document()->frame() == document()->page()->mainFrame()) {
        LOG(Plugins, "%p Plug-in document in main frame", this);
        return;
    }

    if (ScriptController::processingUserGesture()) {
        LOG(Plugins, "%p Script is processing user gesture, set to play", this);
        return;
    }

    LayoutRect rect = toRenderEmbeddedObject(renderer())->contentBoxRect();
    int width = rect.width();
    int height = rect.height();
    if (!width || !height || (width <= autoStartPlugInSizeDimensionThreshold && height <= autoStartPlugInSizeDimensionThreshold)) {
        LOG(Plugins, "%p Plug-in is %dx%d, set to play", this, width, height);
        return;
    }

    if (!document()->page() || !document()->page()->plugInClient()) {
        setDisplayState(WaitingForSnapshot);
        return;
    }

    LOG(Plugins, "%p Plug-in URL: %s", this, m_url.utf8().data());
    LOG(Plugins, "            loaded URL: %s", url.string().utf8().data());

    m_plugInOriginHash = PlugInOriginHash::hash(this, url);
    if (m_plugInOriginHash && document()->page()->plugInClient()->isAutoStartOrigin(m_plugInOriginHash)) {
        LOG(Plugins, "%p Plug-in hash %x is auto-start, set to play", this, m_plugInOriginHash);
        return;
    }

    if (shouldPlugInShowLabelAutomatically(document()->page()->mainFrame()->view()->contentsSize(), this))
        setShouldShowSnapshotLabelAutomatically();

    LOG(Plugins, "%p Plug-in hash %x is %dx%d, origin is not auto-start, set to wait for snapshot", this, m_plugInOriginHash, width, height);
    // We may have got to this point by restarting a snapshotted plug-in, in which case we don't want to
    // reset the display state.
    if (displayState() != PlayingWithPendingMouseClick)
        setDisplayState(WaitingForSnapshot);
}

void HTMLPlugInImageElement::subframeLoaderDidCreatePlugIn(const Widget* widget)
{
    if (!widget->isPluginViewBase()
        || !static_cast<const PluginViewBase*>(widget)->shouldAlwaysAutoStart())
        return;

    LOG(Plugins, "%p Plug-in should auto-start, set to play", this);
    setDisplayState(Playing);
}

} // namespace WebCore
