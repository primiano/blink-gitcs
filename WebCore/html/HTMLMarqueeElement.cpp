/**
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2007 Apple Inc. All rights reserved.
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
#include "HTMLMarqueeElement.h"

#include "CSSPropertyNames.h"
#include "CSSValueKeywords.h"
#include "HTMLNames.h"
#include "MappedAttribute.h"
#include "RenderLayer.h"
#include "RenderMarquee.h"

namespace WebCore {

using namespace HTMLNames;

// WinIE uses 60ms as the minimum delay by default.
const int defaultMinimumDelay = 60;

HTMLMarqueeElement::HTMLMarqueeElement(const QualifiedName& tagName, Document* doc)
    : HTMLElement(tagName, doc)
    , ActiveDOMObject(doc, this)
    , m_minimumDelay(defaultMinimumDelay)
{
    ASSERT(hasTagName(marqueeTag));
}

bool HTMLMarqueeElement::mapToEntry(const QualifiedName& attrName, MappedAttributeEntry& result) const
{
    if (attrName == widthAttr ||
        attrName == heightAttr ||
        attrName == bgcolorAttr ||
        attrName == vspaceAttr ||
        attrName == hspaceAttr ||
        attrName == scrollamountAttr ||
        attrName == scrolldelayAttr ||
        attrName == loopAttr ||
        attrName == behaviorAttr ||
        attrName == directionAttr) {
        result = eUniversal;
        return false;
    }

    return HTMLElement::mapToEntry(attrName, result);
}

void HTMLMarqueeElement::parseMappedAttribute(MappedAttribute *attr)
{
    if (attr->name() == widthAttr) {
        if (!attr->value().isEmpty())
            addCSSLength(attr, CSSPropertyWidth, attr->value());
    } else if (attr->name() == heightAttr) {
        if (!attr->value().isEmpty())
            addCSSLength(attr, CSSPropertyHeight, attr->value());
    } else if (attr->name() == bgcolorAttr) {
        if (!attr->value().isEmpty())
            addCSSColor(attr, CSSPropertyBackgroundColor, attr->value());
    } else if (attr->name() == vspaceAttr) {
        if (!attr->value().isEmpty()) {
            addCSSLength(attr, CSSPropertyMarginTop, attr->value());
            addCSSLength(attr, CSSPropertyMarginBottom, attr->value());
        }
    } else if (attr->name() == hspaceAttr) {
        if (!attr->value().isEmpty()) {
            addCSSLength(attr, CSSPropertyMarginLeft, attr->value());
            addCSSLength(attr, CSSPropertyMarginRight, attr->value());
        }
    } else if (attr->name() == scrollamountAttr) {
        if (!attr->value().isEmpty())
            addCSSLength(attr, CSSPropertyWebkitMarqueeIncrement, attr->value());
    } else if (attr->name() == scrolldelayAttr) {
        if (!attr->value().isEmpty())
            addCSSLength(attr, CSSPropertyWebkitMarqueeSpeed, attr->value());
    } else if (attr->name() == loopAttr) {
        if (!attr->value().isEmpty()) {
            if (attr->value() == "-1" || equalIgnoringCase(attr->value(), "infinite"))
                addCSSProperty(attr, CSSPropertyWebkitMarqueeRepetition, CSSValueInfinite);
            else
                addCSSLength(attr, CSSPropertyWebkitMarqueeRepetition, attr->value());
        }
    } else if (attr->name() == behaviorAttr) {
        if (!attr->value().isEmpty())
            addCSSProperty(attr, CSSPropertyWebkitMarqueeStyle, attr->value());
    } else if (attr->name() == directionAttr) {
        if (!attr->value().isEmpty())
            addCSSProperty(attr, CSSPropertyWebkitMarqueeDirection, attr->value());
    } else if (attr->name() == truespeedAttr)
        m_minimumDelay = !attr->isEmpty() ? 0 : defaultMinimumDelay;
    else
        HTMLElement::parseMappedAttribute(attr);
}

void HTMLMarqueeElement::start()
{
    if (renderer() && renderer()->hasLayer() && renderBox()->layer()->marquee())
        renderBox()->layer()->marquee()->start();
}

void HTMLMarqueeElement::stop()
{
    if (renderer() && renderer()->hasLayer() && renderBox()->layer()->marquee())
        renderBox()->layer()->marquee()->stop();
}

bool HTMLMarqueeElement::canSuspend() const
{
    return true;
}

void HTMLMarqueeElement::suspend()
{
    if (renderer() && renderer()->hasLayer() && renderBox()->layer()->marquee())
        renderBox()->layer()->marquee()->suspend();
}
    
void HTMLMarqueeElement::resume()
{
    if (renderer() && renderer()->hasLayer() && renderBox()->layer()->marquee())
        renderBox()->layer()->marquee()->updateMarqueePosition();
}

} // namespace WebCore
