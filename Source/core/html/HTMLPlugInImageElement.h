/*
 * Copyright (C) 2008, 2009, 2011, 2012 Apple Inc. All rights reserved.
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

#ifndef HTMLPlugInImageElement_h
#define HTMLPlugInImageElement_h

#include "core/html/HTMLPlugInElement.h"

#include "core/rendering/style/RenderStyle.h"
#include "wtf/OwnPtr.h"

namespace WebCore {

class HTMLImageLoader;
class FrameLoader;
class Image;
class MouseEvent;
class Widget;

// Base class for HTMLObjectElement and HTMLEmbedElement
class HTMLPlugInImageElement : public HTMLPlugInElement {
public:
    virtual ~HTMLPlugInImageElement();

    RenderEmbeddedObject* renderEmbeddedObject() const;

protected:
    HTMLPlugInImageElement(const QualifiedName& tagName, Document&, bool createdByParser, PreferPlugInsForImagesOption);

    OwnPtr<HTMLImageLoader> m_imageLoader;

    static void updateWidgetCallback(Node*);
    virtual void attach(const AttachContext& = AttachContext()) OVERRIDE;
    virtual void detach(const AttachContext& = AttachContext()) OVERRIDE;

    bool allowedToLoadFrameURL(const String& url);
    bool wouldLoadAsNetscapePlugin(const String& url, const String& serviceType);

    virtual void didMoveToNewDocument(Document& oldDocument) OVERRIDE;

    bool requestObject(const String& url, const String& mimeType, const Vector<String>& paramNames, const Vector<String>& paramValues);
    bool shouldUsePlugin(const KURL&, const String& mimeType, bool hasFallback, bool& useFallback);

private:
    virtual RenderObject* createRenderer(RenderStyle*);
    virtual void willRecalcStyle(StyleRecalcChange) OVERRIDE FINAL;

    virtual void finishParsingChildren();

    void updateWidgetIfNecessary();

    void swapRendererTimerFired(Timer<HTMLPlugInImageElement>*);

    void restartSimilarPlugIns();

    bool loadPlugin(const KURL&, const String& mimeType, const Vector<String>& paramNames, const Vector<String>& paramValues, bool useFallback);
    bool pluginIsLoadable(const KURL&, const String& mimeType);

    virtual bool isPlugInImageElement() const OVERRIDE { return true; }

    bool m_createdDuringUserGesture;
};

inline HTMLPlugInImageElement* toHTMLPlugInImageElement(Node* node)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!node || node->isPluginElement());
    HTMLPlugInElement* plugInElement = static_cast<HTMLPlugInElement*>(node);
    ASSERT_WITH_SECURITY_IMPLICATION(plugInElement->isPlugInImageElement());
    return static_cast<HTMLPlugInImageElement*>(plugInElement);
}

inline const HTMLPlugInImageElement* toHTMLPlugInImageElement(const Node* node)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!node || node->isPluginElement());
    const HTMLPlugInElement* plugInElement = static_cast<const HTMLPlugInElement*>(node);
    ASSERT_WITH_SECURITY_IMPLICATION(plugInElement->isPlugInImageElement());
    return static_cast<const HTMLPlugInImageElement*>(plugInElement);
}

// This will catch anyone doing an unnecessary cast.
void toHTMLPlugInImageElement(const HTMLPlugInImageElement*);

} // namespace WebCore

#endif // HTMLPlugInImageElement_h
