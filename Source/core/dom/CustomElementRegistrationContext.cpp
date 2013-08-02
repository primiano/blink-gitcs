/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Google Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
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
#include "core/dom/CustomElementRegistrationContext.h"

#include "HTMLNames.h"
#include "MathMLNames.h"
#include "SVGNames.h"
#include "core/dom/CustomElement.h"
#include "core/dom/CustomElementDefinition.h"
#include "core/dom/Element.h"
#include "core/html/HTMLElement.h"
#include "core/html/HTMLUnknownElement.h"
#include "core/svg/SVGUnknownElement.h"
#include "wtf/RefPtr.h"

namespace WebCore {

void CustomElementRegistrationContext::registerElement(Document* document, CustomElementConstructorBuilder* constructorBuilder, const AtomicString& type, ExceptionCode& ec)
{
    CustomElementDefinition* definition = m_registry.registerElement(document, constructorBuilder, type, ec);

    if (!definition)
        return;

    // Upgrade elements that were waiting for this definition.
    const CustomElementUpgradeCandidateMap::ElementSet& upgradeCandidates = m_candidates.takeUpgradeCandidatesFor(definition->descriptor());
    for (CustomElementUpgradeCandidateMap::ElementSet::const_iterator it = upgradeCandidates.begin(); it != upgradeCandidates.end(); ++it)
        didResolveElement(definition, *it);
}

PassRefPtr<Element> CustomElementRegistrationContext::createCustomTagElement(Document* document, const QualifiedName& tagName)
{
    ASSERT(isCustomTagName(tagName.localName()));

    if (!document)
        return 0;

    RefPtr<Element> element;

    if (HTMLNames::xhtmlNamespaceURI == tagName.namespaceURI()) {
        element = HTMLElement::create(tagName, document);
    } else if (SVGNames::svgNamespaceURI == tagName.namespaceURI()) {
        element = SVGUnknownElement::create(tagName, document);
    } else {
        // XML elements are not custom elements, so return early.
        return Element::create(tagName, document);
    }

    resolve(element.get(), nullAtom);
    return element.release();
}

void CustomElementRegistrationContext::didGiveTypeExtension(Element* element, const AtomicString& type)
{
    resolve(element, type);
}

void CustomElementRegistrationContext::resolve(Element* element, const AtomicString& typeExtension)
{
    // If an element has a custom tag name it takes precedence over
    // the "is" attribute (if any).
    const AtomicString& type = isCustomTagName(element->localName())
        ? element->localName()
        : typeExtension;
    ASSERT(!type.isNull());

    CustomElementDescriptor descriptor(type, element->namespaceURI(), element->localName());
    CustomElementDefinition* definition = m_registry.find(descriptor);
    if (definition)
        didResolveElement(definition, element);
    else
        didCreateUnresolvedElement(descriptor, element);
}

void CustomElementRegistrationContext::didResolveElement(CustomElementDefinition* definition, Element* element)
{
    CustomElement::define(element, definition);
}

void CustomElementRegistrationContext::didCreateUnresolvedElement(const CustomElementDescriptor& descriptor, Element* element)
{
    m_candidates.add(descriptor, element);
}

void CustomElementRegistrationContext::customElementWasDestroyed(Element* element)
{
    m_candidates.remove(element);
}

PassRefPtr<CustomElementRegistrationContext> CustomElementRegistrationContext::create()
{
    return adoptRef(new CustomElementRegistrationContext());
}

bool CustomElementRegistrationContext::isValidTypeName(const AtomicString& name)
{
    if (notFound == name.find('-'))
        return false;

    DEFINE_STATIC_LOCAL(Vector<AtomicString>, reservedNames, ());
    if (reservedNames.isEmpty()) {
        reservedNames.append(MathMLNames::annotation_xmlTag.localName());
        reservedNames.append(SVGNames::color_profileTag.localName());
        reservedNames.append(SVGNames::font_faceTag.localName());
        reservedNames.append(SVGNames::font_face_srcTag.localName());
        reservedNames.append(SVGNames::font_face_uriTag.localName());
        reservedNames.append(SVGNames::font_face_formatTag.localName());
        reservedNames.append(SVGNames::font_face_nameTag.localName());
        reservedNames.append(SVGNames::missing_glyphTag.localName());
    }

    if (notFound != reservedNames.find(name))
        return false;

    return Document::isValidName(name.string());
}

bool CustomElementRegistrationContext::isCustomTagName(const AtomicString& localName)
{
    return isValidTypeName(localName);
}

void CustomElementRegistrationContext::setIsAttributeAndTypeExtension(Element* element, const AtomicString& type)
{
    ASSERT(element);
    ASSERT(!type.isEmpty());
    element->setAttribute(HTMLNames::isAttr, type);
    setTypeExtension(element, type);
}

void CustomElementRegistrationContext::setTypeExtension(Element* element, const AtomicString& type)
{
    if (!element->isHTMLElement() && !element->isSVGElement())
        return;

    if (isCustomTagName(element->localName()))
        return; // custom tags take precedence over type extensions

    if (CustomElementRegistrationContext* context = element->document()->registrationContext())
        context->didGiveTypeExtension(element, type);
}

} // namespace WebCore
