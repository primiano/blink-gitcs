/*
 * Copyright (C) 2004 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2006 Samuel Weinig <sam.weinig@gmail.com>
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

#import "config.h"
#import "DOMHTML.h"

#import "DOMExtensions.h"
#import "DOMHTMLInternal.h"
#import "DOMInternal.h"
#import "DOMPrivate.h"
#import "DocumentFragment.h"
#import "FoundationExtras.h"
#import "FrameView.h"
#import "HTMLAppletElement.h"
#import "HTMLAreaElement.h"
#import "HTMLBRElement.h"
#import "HTMLBaseElement.h"
#import "HTMLBaseFontElement.h"
#import "HTMLBodyElement.h"
#import "HTMLButtonElement.h"
#import "HTMLDListElement.h"
#import "HTMLDirectoryElement.h"
#import "HTMLDivElement.h"
#import "HTMLDocument.h"
#import "HTMLEmbedElement.h"
#import "HTMLFieldSetElement.h"
#import "HTMLFontElement.h"
#import "HTMLFormCollection.h"
#import "HTMLFormElement.h"
#import "HTMLFrameSetElement.h"
#import "HTMLHRElement.h"
#import "HTMLHeadElement.h"
#import "HTMLHeadingElement.h"
#import "HTMLHtmlElement.h"
#import "HTMLIFrameElement.h"
#import "HTMLImageElement.h"
#import "HTMLIsIndexElement.h"
#import "HTMLLIElement.h"
#import "HTMLLabelElement.h"
#import "HTMLLegendElement.h"
#import "HTMLLinkElement.h"
#import "HTMLMapElement.h"
#import "HTMLMenuElement.h"
#import "HTMLMetaElement.h"
#import "HTMLNames.h"
#import "HTMLOListElement.h"
#import "HTMLObjectElement.h"
#import "HTMLOptGroupElement.h"
#import "HTMLOptionElement.h"
#import "HTMLOptionsCollection.h"
#import "HTMLParagraphElement.h"
#import "HTMLParamElement.h"
#import "HTMLPreElement.h"
#import "HTMLScriptElement.h"
#import "HTMLSelectElement.h"
#import "HTMLStyleElement.h"
#import "HTMLTableCaptionElement.h"
#import "HTMLTableCellElement.h"
#import "HTMLTableColElement.h"
#import "HTMLTableElement.h"
#import "HTMLTableRowElement.h"
#import "HTMLTableSectionElement.h"
#import "HTMLTextAreaElement.h"
#import "HTMLTitleElement.h"
#import "HTMLUListElement.h"
#import "KURL.h"
#import "NameNodeList.h"
#import "Range.h"
#import "RenderTextControl.h"
#import "csshelper.h"
#import "markup.h"

using namespace WebCore;
using namespace HTMLNames;

// FIXME: This code should be using the impl methods instead of doing so many get/setAttribute calls.
// FIXME: This code should be generated.


//------------------------------------------------------------------------------------------
// DOMHTMLCollection

@implementation DOMHTMLCollection (WebCoreInternal)

- (id)_initWithCollection:(HTMLCollection *)impl
{
    ASSERT(impl);
    
    [super _init];
    _internal = DOM_cast<DOMObjectInternal*>(impl);
    impl->ref();
    addDOMWrapper(self, impl);
    return self;
}

+ (DOMHTMLCollection *)_collectionWith:(HTMLCollection *)impl
{
    if (!impl)
        return nil;
    
    id cachedInstance;
    cachedInstance = getDOMWrapper(impl);
    if (cachedInstance)
        return [[cachedInstance retain] autorelease];
    
    return [[[self alloc] _initWithCollection:impl] autorelease];
}

@end


//------------------------------------------------------------------------------------------
// DOMHTMLOptionsCollection

@implementation DOMHTMLOptionsCollection (WebCoreInternal)

- (id)_initWithOptionsCollection:(HTMLOptionsCollection *)impl
{
    ASSERT(impl);
    
    [super _init];
    _internal = DOM_cast<DOMObjectInternal*>(impl);
    impl->ref();
    addDOMWrapper(self, impl);
    return self;
}

+ (DOMHTMLOptionsCollection *)_optionsCollectionWith:(HTMLOptionsCollection *)impl
{
    if (!impl)
        return nil;
    
    id cachedInstance;
    cachedInstance = getDOMWrapper(impl);
    if (cachedInstance)
        return [[cachedInstance retain] autorelease];
    
    return [[[self alloc] _initWithOptionsCollection:impl] autorelease];
}

@end


//------------------------------------------------------------------------------------------
// DOMHTMLElement

@implementation DOMHTMLElement (WebCoreInternal)

+ (DOMHTMLElement *)_elementWith:(HTMLElement *)impl
{
    return static_cast<DOMHTMLElement*>([DOMNode _nodeWith:impl]);
}

- (HTMLElement *)_HTMLElement
{
    return static_cast<HTMLElement*>(DOM_cast<Node*>(_internal));
}

@end


//------------------------------------------------------------------------------------------
// DOMHTMLDocument

@implementation DOMHTMLDocument (WebPrivate)

- (DOMDocumentFragment *)_createDocumentFragmentWithMarkupString:(NSString *)markupString baseURLString:(NSString *)baseURLString
{
    NSURL *baseURL = KURL([self _document]->completeURL(parseURL(baseURLString)).deprecatedString()).getNSURL();
    return [self createDocumentFragmentWithMarkupString:markupString baseURL:baseURL];
}

- (DOMDocumentFragment *)_createDocumentFragmentWithText:(NSString *)text
{
    return [self createDocumentFragmentWithText:text];
}

@end

@implementation DOMHTMLDocument (WebCoreInternal)

+ (DOMHTMLDocument *)_HTMLDocumentWith:(WebCore::HTMLDocument *)impl;
{
    return static_cast<DOMHTMLDocument *>([DOMNode _nodeWith:impl]);
}

@end


//------------------------------------------------------------------------------------------
// DOMHTMLLinkElement

@implementation DOMHTMLLinkElement (DOMHTMLLinkElementExtensions)

- (NSURL *)absoluteLinkURL
{
    return [self _getURLAttribute:@"href"];
}

@end


//------------------------------------------------------------------------------------------
// DOMHTMLFormElement

@implementation DOMHTMLFormElement (WebCoreInternal)

+ (DOMHTMLFormElement *)_formElementWith:(HTMLFormElement *)impl
{
    return static_cast<DOMHTMLFormElement*>([DOMNode _nodeWith:impl]);
}

@end


//------------------------------------------------------------------------------------------
// DOMHTMLOptionElement

@implementation DOMHTMLOptionElement

- (HTMLOptionElement *)_optionElement
{
    return static_cast<HTMLOptionElement*>(DOM_cast<Node*>(_internal));
}

- (DOMHTMLFormElement *)form
{
    return [DOMHTMLFormElement _formElementWith:[self _optionElement]->form()];
}

- (BOOL)defaultSelected
{
    return [self _optionElement]->defaultSelected();
}

- (void)setDefaultSelected:(BOOL)defaultSelected
{
    [self _optionElement]->setDefaultSelected(defaultSelected);
}

- (NSString *)text
{
    return [self _optionElement]->text();
}

- (int)index
{
    return [self _optionElement]->index();
}

- (BOOL)disabled
{
    return [self _optionElement]->disabled();
}

- (void)setDisabled:(BOOL)disabled
{
    [self _optionElement]->setDisabled(disabled);
}

- (NSString *)label
{
    return [self _optionElement]->label();
}

- (void)setLabel:(NSString *)label
{
    [self _optionElement]->setLabel(label);
}

- (BOOL)selected
{
    return [self _optionElement]->selected();
}

- (void)setSelected:(BOOL)selected
{
    [self _optionElement]->setSelected(selected);
}

- (NSString *)value
{
    return [self _optionElement]->value();
}

- (void)setValue:(NSString *)value
{
    String string = value;
    [self _optionElement]->setValue(string.impl());
}

@end


//------------------------------------------------------------------------------------------
// DOMHTMLInputElement

@implementation DOMHTMLInputElement (DOMHTMLInputElementExtensions)

- (NSString *)altDisplayString
{
    return [self _HTMLInputElement]->alt().replace('\\', [self _element]->document()->backslashAsCurrencySymbol());
}

- (NSURL *)absoluteImageURL
{
    if (![self _HTMLInputElement]->renderer() || ![self _HTMLInputElement]->renderer()->isImage())
        return nil;
    return [self _getURLAttribute:@"src"];
}

@end

@implementation DOMHTMLInputElement (WebCoreInternal)

- (WebCore::HTMLInputElement *)_HTMLInputElement
{
    return static_cast<WebCore::HTMLInputElement *>(DOM_cast<WebCore::Node *>(_internal));
}

@end


//------------------------------------------------------------------------------------------
// DOMHTMLBaseFontElement

@implementation DOMHTMLBaseFontElement

- (HTMLBaseFontElement *)_baseFontElement
{
    return static_cast<HTMLBaseFontElement*>(DOM_cast<Node*>(_internal));
}

- (NSString *)color
{
    return [self _baseFontElement]->getAttribute(colorAttr);
}

- (void)setColor:(NSString *)color
{
    [self _baseFontElement]->setAttribute(colorAttr, color);
}

- (NSString *)face
{
    return [self _baseFontElement]->getAttribute(faceAttr);
}

- (void)setFace:(NSString *)face
{
    [self _baseFontElement]->setAttribute(faceAttr, face);
}

- (NSString *)size
{
    return [self _baseFontElement]->getAttribute(sizeAttr);
}

- (void)setSize:(NSString *)size
{
    [self _baseFontElement]->setAttribute(sizeAttr, size);
}

@end

@implementation DOMHTMLFontElement

- (HTMLFontElement *)_fontElement
{
    return static_cast<HTMLFontElement*>(DOM_cast<Node*>(_internal));
}

- (NSString *)color
{
    return [self _fontElement]->getAttribute(colorAttr);
}

- (void)setColor:(NSString *)color
{
    [self _fontElement]->setAttribute(colorAttr, color);
}

- (NSString *)face
{
    return [self _fontElement]->getAttribute(faceAttr);
}

- (void)setFace:(NSString *)face
{
    [self _fontElement]->setAttribute(faceAttr, face);
}

- (NSString *)size
{
    return [self _fontElement]->getAttribute(sizeAttr);
}

- (void)setSize:(NSString *)size
{
    [self _fontElement]->setAttribute(sizeAttr, size);
}

@end

@implementation DOMHTMLHRElement

- (HTMLHRElement *)_HRElement
{
    return static_cast<HTMLHRElement*>(DOM_cast<Node*>(_internal));
}

- (NSString *)align
{
    return [self _HRElement]->getAttribute(alignAttr);
}

- (void)setAlign:(NSString *)align
{
    [self _HRElement]->setAttribute(alignAttr, align);
}

- (BOOL)noShade
{
    return [self _HRElement]->getAttribute(noshadeAttr).isNull();
}

- (void)setNoShade:(BOOL)noShade
{
    [self _HRElement]->setAttribute(noshadeAttr, noShade ? "" : 0);
}

- (NSString *)size
{
    return [self _HRElement]->getAttribute(sizeAttr);
}

- (void)setSize:(NSString *)size
{
    [self _HRElement]->setAttribute(sizeAttr, size);
}

- (NSString *)width
{
    return [self _HRElement]->getAttribute(widthAttr);
}

- (void)setWidth:(NSString *)width
{
    [self _HRElement]->setAttribute(widthAttr, width);
}

@end

@implementation DOMHTMLModElement

- (HTMLElement *)_modElement
{
    return static_cast<HTMLElement*>(DOM_cast<Node*>(_internal));
}

- (NSString *)cite
{
    return [self _modElement]->getAttribute(citeAttr);
}

- (void)setCite:(NSString *)cite
{
    [self _modElement]->setAttribute(citeAttr, cite);
}

- (NSString *)dateTime
{
    return [self _modElement]->getAttribute(datetimeAttr);
}

- (void)setDateTime:(NSString *)dateTime
{
    [self _modElement]->setAttribute(datetimeAttr, dateTime);
}

@end

@implementation DOMHTMLAnchorElement

- (HTMLAnchorElement *)_anchorElement
{
    return static_cast<HTMLAnchorElement*>(DOM_cast<Node*>(_internal));
}

- (NSString *)accessKey
{
    return [self _anchorElement]->getAttribute(accesskeyAttr);
}

- (void)setAccessKey:(NSString *)accessKey
{
    [self _anchorElement]->setAttribute(accesskeyAttr, accessKey);
}

- (NSString *)charset
{
    return [self _anchorElement]->getAttribute(charsetAttr);
}

- (void)setCharset:(NSString *)charset
{
    [self _anchorElement]->setAttribute(charsetAttr, charset);
}

- (NSString *)coords
{
    return [self _anchorElement]->getAttribute(coordsAttr);
}

- (void)setCoords:(NSString *)coords
{
    [self _anchorElement]->setAttribute(coordsAttr, coords);
}

- (NSURL *)absoluteLinkURL
{
    return [self _getURLAttribute:@"href"];
}

- (NSString *)href
{
    return [self _anchorElement]->href();
}

- (void)setHref:(NSString *)href
{
    [self _anchorElement]->setAttribute(hrefAttr, href);
}

- (NSString *)hreflang
{
    return [self _anchorElement]->hreflang();
}

- (void)setHreflang:(NSString *)hreflang
{
    [self _anchorElement]->setHreflang(hreflang);
}

- (NSString *)name
{
    return [self _anchorElement]->name();
}

- (void)setName:(NSString *)name
{
    [self _anchorElement]->setName(name);
}

- (NSString *)rel
{
    return [self _anchorElement]->rel();
}

- (void)setRel:(NSString *)rel
{
    [self _anchorElement]->setRel(rel);
}

- (NSString *)rev
{
    return [self _anchorElement]->rev();
}

- (void)setRev:(NSString *)rev
{
    [self _anchorElement]->setRev(rev);
}

- (NSString *)shape
{
    return [self _anchorElement]->shape();
}

- (void)setShape:(NSString *)shape
{
    [self _anchorElement]->setShape(shape);
}

- (int)tabIndex
{
    return [self _anchorElement]->tabIndex();
}

- (void)setTabIndex:(int)tabIndex
{
    [self _anchorElement]->setTabIndex(tabIndex);
}

- (NSString *)target
{
    return [self _anchorElement]->getAttribute(targetAttr);
}

- (void)setTarget:(NSString *)target
{
    [self _anchorElement]->setAttribute(targetAttr, target);
}

- (NSString *)type
{
    return [self _anchorElement]->getAttribute(typeAttr);
}

- (void)setType:(NSString *)type
{
    [self _anchorElement]->setAttribute(typeAttr, type);
}

- (void)blur
{
    HTMLAnchorElement *impl = [self _anchorElement];
    if (impl->document()->focusNode() == impl)
        impl->document()->setFocusNode(0);
}

- (void)focus
{
    HTMLAnchorElement *impl = [self _anchorElement];
    impl->document()->setFocusNode(static_cast<Element*>(impl));
}

@end

@implementation DOMHTMLImageElement

- (HTMLImageElement *)_imageElement
{
    return static_cast<HTMLImageElement*>(DOM_cast<Node*>(_internal));
}

- (NSString *)name
{
    return [self _imageElement]->getAttribute(nameAttr);
}

- (void)setName:(NSString *)name
{
    [self _imageElement]->setAttribute(nameAttr, name);
}

- (NSString *)align
{
    return [self _imageElement]->getAttribute(alignAttr);
}

- (void)setAlign:(NSString *)align
{
    [self _imageElement]->setAttribute(alignAttr, align);
}

- (NSString *)alt
{
    return [self _imageElement]->getAttribute(altAttr);
}

- (NSString *)altDisplayString
{
    String alt = [self _imageElement]->getAttribute(altAttr);
    return alt.replace('\\', [self _element]->document()->backslashAsCurrencySymbol());
}

- (void)setAlt:(NSString *)alt
{
    [self _imageElement]->setAttribute(altAttr, alt);
}

- (NSString *)border
{
    return [self _imageElement]->getAttribute(borderAttr);
}

- (void)setBorder:(NSString *)border
{
    [self _imageElement]->setAttribute(borderAttr, border);
}

- (int)height
{
    return [self _imageElement]->getAttribute(heightAttr).toInt();
}

- (void)setHeight:(int)height
{
    [self _imageElement]->setAttribute(heightAttr, String::number(height));
}

- (int)hspace
{
    return [self _imageElement]->getAttribute(hspaceAttr).toInt();
}

- (void)setHspace:(int)hspace
{
    [self _imageElement]->setAttribute(hspaceAttr, String::number(hspace));
}

- (BOOL)isMap
{
    return [self _imageElement]->getAttribute(ismapAttr).isNull();
}

- (void)setIsMap:(BOOL)isMap
{
    [self _imageElement]->setAttribute(ismapAttr, isMap ? "" : 0);
}

- (NSString *)longDesc
{
    return [self _imageElement]->getAttribute(longdescAttr);
}

- (void)setLongDesc:(NSString *)longDesc
{
    [self _imageElement]->setAttribute(longdescAttr, longDesc);
}

- (NSURL *)absoluteImageURL
{
    return [self _getURLAttribute:@"src"];
}

- (NSString *)src
{
    return [self _imageElement]->src();
}

- (void)setSrc:(NSString *)src
{
    [self _imageElement]->setAttribute(srcAttr, src);
}

- (NSString *)useMap
{
    return [self _imageElement]->getAttribute(usemapAttr);
}

- (void)setUseMap:(NSString *)useMap
{
    [self _imageElement]->setAttribute(usemapAttr, useMap);
}

- (int)vspace
{
    return [self _imageElement]->getAttribute(vspaceAttr).toInt();
}

- (void)setVspace:(int)vspace
{
    [self _imageElement]->setAttribute(vspaceAttr, String::number(vspace));
}

- (int)width
{
    return [self _imageElement]->getAttribute(widthAttr).toInt();
}

- (void)setWidth:(int)width
{
    [self _imageElement]->setAttribute(widthAttr, String::number(width));
}

@end

@implementation DOMHTMLObjectElement

- (HTMLObjectElement *)_objectElement
{
    return static_cast<HTMLObjectElement*>(DOM_cast<Node*>(_internal));
}

- (DOMHTMLFormElement *)form
{
    return [DOMHTMLFormElement _formElementWith:[self _objectElement]->form()];
}

- (NSString *)code
{
    return [self _objectElement]->getAttribute(codeAttr);
}

- (void)setCode:(NSString *)code
{
    [self _objectElement]->setAttribute(codeAttr, code);
}

- (NSString *)align
{
    return [self _objectElement]->getAttribute(alignAttr);
}

- (void)setAlign:(NSString *)align
{
    [self _objectElement]->setAttribute(alignAttr, align);
}

- (NSString *)archive
{
    return [self _objectElement]->getAttribute(archiveAttr);
}

- (void)setArchive:(NSString *)archive
{
    [self _objectElement]->setAttribute(archiveAttr, archive);
}

- (NSString *)border
{
    return [self _objectElement]->getAttribute(borderAttr);
}

- (void)setBorder:(NSString *)border
{
    [self _objectElement]->setAttribute(borderAttr, border);
}

- (NSString *)codeBase
{
    return [self _objectElement]->getAttribute(codebaseAttr);
}

- (void)setCodeBase:(NSString *)codeBase
{
    [self _objectElement]->setAttribute(codebaseAttr, codeBase);
}

- (NSString *)codeType
{
    return [self _objectElement]->getAttribute(codetypeAttr);
}

- (void)setCodeType:(NSString *)codeType
{
    [self _objectElement]->setAttribute(codetypeAttr, codeType);
}

- (NSURL *)absoluteImageURL
{
    if (![self _objectElement]->renderer() || ![self _objectElement]->renderer()->isImage())
        return nil;
    return [self _getURLAttribute:@"data"];
}

- (NSString *)data
{
    return [self _objectElement]->getAttribute(dataAttr);
}

- (void)setData:(NSString *)data
{
    [self _objectElement]->setAttribute(dataAttr, data);
}

- (BOOL)declare
{
    return [self _objectElement]->getAttribute(declareAttr).isNull();
}

- (void)setDeclare:(BOOL)declare
{
    [self _objectElement]->setAttribute(declareAttr, declare ? "" : 0);
}

- (NSString *)height
{
    return [self _objectElement]->getAttribute(heightAttr);
}

- (void)setHeight:(NSString *)height
{
    [self _objectElement]->setAttribute(heightAttr, height);
}

- (int)hspace
{
    return [self _objectElement]->getAttribute(hspaceAttr).toInt();
}

- (void)setHspace:(int)hspace
{
    [self _objectElement]->setAttribute(hspaceAttr, String::number(hspace));
}

- (NSString *)name
{
    return [self _objectElement]->getAttribute(nameAttr);
}

- (void)setName:(NSString *)name
{
    [self _objectElement]->setAttribute(nameAttr, name);
}

- (NSString *)standby
{
    return [self _objectElement]->getAttribute(standbyAttr);
}

- (void)setStandby:(NSString *)standby
{
    [self _objectElement]->setAttribute(standbyAttr, standby);
}

- (int)tabIndex
{
    return [self _objectElement]->getAttribute(tabindexAttr).toInt();
}

- (void)setTabIndex:(int)tabIndex
{
    [self _objectElement]->setAttribute(tabindexAttr, String::number(tabIndex));
}

- (NSString *)type
{
    return [self _objectElement]->getAttribute(typeAttr);
}

- (void)setType:(NSString *)type
{
    [self _objectElement]->setAttribute(typeAttr, type);
}

- (NSString *)useMap
{
    return [self _objectElement]->getAttribute(usemapAttr);
}

- (void)setUseMap:(NSString *)useMap
{
    [self _objectElement]->setAttribute(usemapAttr, useMap);
}

- (int)vspace
{
    return [self _objectElement]->getAttribute(vspaceAttr).toInt();
}

- (void)setVspace:(int)vspace
{
    [self _objectElement]->setAttribute(vspaceAttr, String::number(vspace));
}

- (NSString *)width
{
    return [self _objectElement]->getAttribute(widthAttr);
}

- (void)setWidth:(NSString *)width
{
    [self _objectElement]->setAttribute(widthAttr, width);
}

- (DOMDocument *)contentDocument
{
    return [DOMDocument _documentWith:[self _objectElement]->contentDocument()];
}

@end

@implementation DOMHTMLParamElement

- (HTMLParamElement *)_paramElement
{
    return static_cast<HTMLParamElement*>(DOM_cast<Node*>(_internal));
}

- (NSString *)name
{
    return [self _paramElement]->getAttribute(nameAttr);
}

- (void)setName:(NSString *)name
{
    [self _paramElement]->setAttribute(nameAttr, name);
}

- (NSString *)type
{
    return [self _paramElement]->getAttribute(typeAttr);
}

- (void)setType:(NSString *)type
{
    [self _paramElement]->setAttribute(typeAttr, type);
}

- (NSString *)value
{
    return [self _paramElement]->getAttribute(valueAttr);
}

- (void)setValue:(NSString *)value
{
    [self _paramElement]->setAttribute(valueAttr, value);
}

- (NSString *)valueType
{
    return [self _paramElement]->getAttribute(valuetypeAttr);
}

- (void)setValueType:(NSString *)valueType
{
    [self _paramElement]->setAttribute(valuetypeAttr, valueType);
}

@end

@implementation DOMHTMLAppletElement

- (HTMLAppletElement *)_appletElement
{
    return static_cast<HTMLAppletElement*>(DOM_cast<Node*>(_internal));
}

- (NSString *)align
{
    return [self _appletElement]->getAttribute(alignAttr);
}

- (void)setAlign:(NSString *)align
{
    [self _appletElement]->setAttribute(alignAttr, align);
}

- (NSString *)alt
{
    return [self _appletElement]->getAttribute(altAttr);
}

- (NSString *)altDisplayString
{
    String alt = [self _appletElement]->getAttribute(altAttr);
    return alt.replace('\\', [self _element]->document()->backslashAsCurrencySymbol());
}

- (void)setAlt:(NSString *)alt
{
    [self _appletElement]->setAttribute(altAttr, alt);
}

- (NSString *)archive
{
    return [self _appletElement]->getAttribute(archiveAttr);
}

- (void)setArchive:(NSString *)archive
{
    [self _appletElement]->setAttribute(archiveAttr, archive);
}

- (NSString *)code
{
    return [self _appletElement]->getAttribute(codeAttr);
}

- (void)setCode:(NSString *)code
{
    [self _appletElement]->setAttribute(codeAttr, code);
}

- (NSString *)codeBase
{
    return [self _appletElement]->getAttribute(codebaseAttr);
}

- (void)setCodeBase:(NSString *)codeBase
{
    [self _appletElement]->setAttribute(codebaseAttr, codeBase);
}

- (NSString *)height
{
    return [self _appletElement]->getAttribute(heightAttr);
}

- (void)setHeight:(NSString *)height
{
    [self _appletElement]->setAttribute(heightAttr, height);
}

- (int)hspace
{
    return [self _appletElement]->getAttribute(hspaceAttr).toInt();
}

- (void)setHspace:(int)hspace
{
    [self _appletElement]->setAttribute(hspaceAttr, String::number(hspace));
}

- (NSString *)name
{
    return [self _appletElement]->getAttribute(nameAttr);
}

- (void)setName:(NSString *)name
{
    [self _appletElement]->setAttribute(nameAttr, name);
}

- (NSString *)object
{
    return [self _appletElement]->getAttribute(objectAttr);
}

- (void)setObject:(NSString *)object
{
    [self _appletElement]->setAttribute(objectAttr, object);
}

- (int)vspace
{
    return [self _appletElement]->getAttribute(vspaceAttr).toInt();
}

- (void)setVspace:(int)vspace
{
    [self _appletElement]->setAttribute(vspaceAttr, String::number(vspace));
}

- (NSString *)width
{
    return [self _appletElement]->getAttribute(widthAttr);
}

- (void)setWidth:(NSString *)width
{
    [self _appletElement]->setAttribute(widthAttr, width);
}

@end

@implementation DOMHTMLMapElement

- (HTMLMapElement *)_mapElement
{
    return static_cast<HTMLMapElement*>(DOM_cast<Node*>(_internal));
}

- (DOMHTMLCollection *)areas
{
    HTMLCollection *collection = new HTMLCollection([self _mapElement], HTMLCollection::MapAreas);
    return [DOMHTMLCollection _collectionWith:collection];
}

- (NSString *)name
{
    return [self _mapElement]->getAttribute(nameAttr);
}

- (void)setName:(NSString *)name
{
    [self _mapElement]->setAttribute(nameAttr, name);
}

@end

@implementation DOMHTMLAreaElement

- (HTMLAreaElement *)_areaElement
{
    return static_cast<HTMLAreaElement*>(DOM_cast<Node*>(_internal));
}

- (NSString *)accessKey
{
    return [self _areaElement]->getAttribute(accesskeyAttr);
}

- (void)setAccessKey:(NSString *)accessKey
{
    [self _areaElement]->setAttribute(accesskeyAttr, accessKey);
}

- (NSString *)alt
{
    return [self _areaElement]->getAttribute(altAttr);
}

- (NSString *)altDisplayString
{
    String alt = [self _areaElement]->getAttribute(altAttr);
    return alt.replace('\\', [self _element]->document()->backslashAsCurrencySymbol());
}

- (void)setAlt:(NSString *)alt
{
    [self _areaElement]->setAttribute(altAttr, alt);
}

- (NSString *)coords
{
    return [self _areaElement]->getAttribute(coordsAttr);
}

- (void)setCoords:(NSString *)coords
{
    [self _areaElement]->setAttribute(coordsAttr, coords);
}

- (NSURL *)absoluteLinkURL
{
    return [self _getURLAttribute:@"href"];
}

- (NSString *)href
{
    return [self _areaElement]->href();
}

- (void)setHref:(NSString *)href
{
    [self _areaElement]->setAttribute(hrefAttr, href);
}

- (BOOL)noHref
{
    return [self _areaElement]->getAttribute(nohrefAttr).isNull();
}

- (void)setNoHref:(BOOL)noHref
{
    [self _areaElement]->setAttribute(nohrefAttr, noHref ? "" : 0);
}

- (NSString *)shape
{
    return [self _areaElement]->getAttribute(shapeAttr);
}

- (void)setShape:(NSString *)shape
{
    [self _areaElement]->setAttribute(shapeAttr, shape);
}

- (int)tabIndex
{
    return [self _areaElement]->getAttribute(tabindexAttr).toInt();
}

- (void)setTabIndex:(int)tabIndex
{
    [self _areaElement]->setAttribute(tabindexAttr, String::number(tabIndex));
}

- (NSString *)target
{
    return [self _areaElement]->getAttribute(targetAttr);
}

- (void)setTarget:(NSString *)target
{
    [self _areaElement]->setAttribute(targetAttr, target);
}

@end

@implementation DOMHTMLScriptElement

- (HTMLScriptElement *)_scriptElement
{
    return static_cast<HTMLScriptElement*>(DOM_cast<Node*>(_internal));
}

- (NSString *)text
{
    return [self _scriptElement]->getAttribute(textAttr);
}

- (void)setText:(NSString *)text
{
    [self _scriptElement]->setAttribute(textAttr, text);
}

- (NSString *)htmlFor
{
    ASSERT_WITH_MESSAGE(0, "not implemented by khtml");
    return nil;
}

- (void)setHtmlFor:(NSString *)htmlFor
{
    ASSERT_WITH_MESSAGE(0, "not implemented by khtml");
}

- (NSString *)event
{
    ASSERT_WITH_MESSAGE(0, "not implemented by khtml");
    return nil;
}

- (void)setEvent:(NSString *)event
{
    ASSERT_WITH_MESSAGE(0, "not implemented by khtml");
}

- (NSString *)charset
{
    return [self _scriptElement]->getAttribute(charsetAttr);
}

- (void)setCharset:(NSString *)charset
{
    [self _scriptElement]->setAttribute(charsetAttr, charset);
}

- (BOOL)defer
{
    return [self _scriptElement]->getAttribute(deferAttr).isNull();
}

- (void)setDefer:(BOOL)defer
{
    [self _scriptElement]->setAttribute(deferAttr, defer ? "" : 0);
}

- (NSString *)src
{
    return [self _scriptElement]->getAttribute(srcAttr);
}

- (void)setSrc:(NSString *)src
{
    [self _scriptElement]->setAttribute(srcAttr, src);
}

- (NSString *)type
{
    return [self _scriptElement]->getAttribute(typeAttr);
}

- (void)setType:(NSString *)type
{
    [self _scriptElement]->setAttribute(typeAttr, type);
}

@end

@implementation DOMHTMLTableCaptionElement

- (NSString *)align
{
    return [self _tableCaptionElement]->getAttribute(alignAttr);
}

- (void)setAlign:(NSString *)align
{
    [self _tableCaptionElement]->setAttribute(alignAttr, align);
}

@end

@implementation DOMHTMLTableCaptionElement (WebCoreInternal)

+ (DOMHTMLTableCaptionElement *)_tableCaptionElementWith:(HTMLTableCaptionElement *)impl
{
    return static_cast<DOMHTMLTableCaptionElement*>([DOMNode _nodeWith:impl]);
}

- (HTMLTableCaptionElement *)_tableCaptionElement
{
    return static_cast<HTMLTableCaptionElement*>(DOM_cast<Node*>(_internal));
}

@end

@implementation DOMHTMLTableSectionElement

- (NSString *)align
{
    return [self _tableSectionElement]->getAttribute(alignAttr);
}

- (void)setAlign:(NSString *)align
{
    [self _tableSectionElement]->setAttribute(alignAttr, align);
}

- (NSString *)ch
{
    return [self _tableSectionElement]->getAttribute(charoffAttr);
}

- (void)setCh:(NSString *)ch
{
    [self _tableSectionElement]->setAttribute(charoffAttr, ch);
}

- (NSString *)chOff
{
    return [self _tableSectionElement]->getAttribute(charoffAttr);
}

- (void)setChOff:(NSString *)chOff
{
    [self _tableSectionElement]->setAttribute(charoffAttr, chOff);
}

- (NSString *)vAlign
{
    return [self _tableSectionElement]->getAttribute(valignAttr);
}

- (void)setVAlign:(NSString *)vAlign
{
    [self _tableSectionElement]->setAttribute(valignAttr, vAlign);
}

- (DOMHTMLCollection *)rows
{
    HTMLCollection *collection = new HTMLCollection([self _tableSectionElement], HTMLCollection::TableRows);
    return [DOMHTMLCollection _collectionWith:collection];
}

- (DOMHTMLElement *)insertRow:(int)index
{
    ExceptionCode ec = 0;
    HTMLElement *impl = [self _tableSectionElement]->insertRow(index, ec);
    raiseOnDOMError(ec);
    return [DOMHTMLElement _elementWith:impl];
}

- (void)deleteRow:(int)index
{
    ExceptionCode ec = 0;
    [self _tableSectionElement]->deleteRow(index, ec);
    raiseOnDOMError(ec);
}

@end

@implementation DOMHTMLTableSectionElement (WebCoreInternal)

+ (DOMHTMLTableSectionElement *)_tableSectionElementWith:(HTMLTableSectionElement *)impl
{
    return static_cast<DOMHTMLTableSectionElement*>([DOMNode _nodeWith:impl]);
}

- (HTMLTableSectionElement *)_tableSectionElement
{
    return static_cast<HTMLTableSectionElement*>(DOM_cast<Node*>(_internal));
}

@end

@implementation DOMHTMLTableElement

- (DOMHTMLTableCaptionElement *)caption
{
    return [DOMHTMLTableCaptionElement _tableCaptionElementWith:[self _tableElement]->caption()];
}

- (void)setCaption:(DOMHTMLTableCaptionElement *)caption
{
    [self _tableElement]->setCaption([caption _tableCaptionElement]);
}

- (DOMHTMLTableSectionElement *)tHead
{
    return [DOMHTMLTableSectionElement _tableSectionElementWith:[self _tableElement]->tHead()];
}

- (void)setTHead:(DOMHTMLTableSectionElement *)tHead
{
    [self _tableElement]->setTHead([tHead _tableSectionElement]);
}

- (DOMHTMLTableSectionElement *)tFoot
{
    return [DOMHTMLTableSectionElement _tableSectionElementWith:[self _tableElement]->tFoot()];
}

- (void)setTFoot:(DOMHTMLTableSectionElement *)tFoot
{
    [self _tableElement]->setTFoot([tFoot _tableSectionElement]);
}

- (DOMHTMLCollection *)rows
{
    HTMLCollection *collection = new HTMLCollection([self _tableElement], HTMLCollection::TableRows);
    return [DOMHTMLCollection _collectionWith:collection];
}

- (DOMHTMLCollection *)tBodies
{
    HTMLCollection *collection = new HTMLCollection([self _tableElement], HTMLCollection::TableTBodies);
    return [DOMHTMLCollection _collectionWith:collection];
}

- (NSString *)align
{
    return [self _tableElement]->getAttribute(alignAttr);
}

- (void)setAlign:(NSString *)align
{
    [self _tableElement]->setAttribute(alignAttr, align);
}

- (NSString *)bgColor
{
    return [self _tableElement]->getAttribute(bgcolorAttr);
}

- (void)setBgColor:(NSString *)bgColor
{
    [self _tableElement]->setAttribute(bgcolorAttr, bgColor);
}

- (NSString *)border
{
    return [self _tableElement]->getAttribute(borderAttr);
}

- (void)setBorder:(NSString *)border
{
    [self _tableElement]->setAttribute(borderAttr, border);
}

- (NSString *)cellPadding
{
    return [self _tableElement]->getAttribute(cellpaddingAttr);
}

- (void)setCellPadding:(NSString *)cellPadding
{
    [self _tableElement]->setAttribute(cellpaddingAttr, cellPadding);
}

- (NSString *)cellSpacing
{
    return [self _tableElement]->getAttribute(cellspacingAttr);
}

- (void)setCellSpacing:(NSString *)cellSpacing
{
    [self _tableElement]->setAttribute(cellspacingAttr, cellSpacing);
}

- (NSString *)frameBorders
{
    return [self _tableElement]->getAttribute(frameAttr);
}

- (void)setFrameBorders:(NSString *)frameBorders
{
    [self _tableElement]->setAttribute(frameAttr, frameBorders);
}

- (NSString *)rules
{
    return [self _tableElement]->getAttribute(rulesAttr);
}

- (void)setRules:(NSString *)rules
{
    [self _tableElement]->setAttribute(rulesAttr, rules);
}

- (NSString *)summary
{
    return [self _tableElement]->getAttribute(summaryAttr);
}

- (void)setSummary:(NSString *)summary
{
    [self _tableElement]->setAttribute(summaryAttr, summary);
}

- (NSString *)width
{
    return [self _tableElement]->getAttribute(widthAttr);
}

- (void)setWidth:(NSString *)width
{
    [self _tableElement]->setAttribute(widthAttr, width);
}

- (DOMHTMLElement *)createTHead
{
    HTMLTableSectionElement *impl = static_cast<HTMLTableSectionElement*>([self _tableElement]->createTHead());
    return [DOMHTMLTableSectionElement _tableSectionElementWith:impl];
}

- (void)deleteTHead
{
    [self _tableElement]->deleteTHead();
}

- (DOMHTMLElement *)createTFoot
{
    HTMLTableSectionElement *impl = static_cast<HTMLTableSectionElement*>([self _tableElement]->createTFoot());
    return [DOMHTMLTableSectionElement _tableSectionElementWith:impl];
}

- (void)deleteTFoot
{
    [self _tableElement]->deleteTFoot();
}

- (DOMHTMLElement *)createCaption
{
    HTMLTableCaptionElement *impl = static_cast<HTMLTableCaptionElement*>([self _tableElement]->createCaption());
    return [DOMHTMLTableCaptionElement _tableCaptionElementWith:impl];
}

- (void)deleteCaption
{
    [self _tableElement]->deleteCaption();
}

- (DOMHTMLElement *)insertRow:(int)index
{
    ExceptionCode ec = 0;
    HTMLTableElement *impl = static_cast<HTMLTableElement*>([self _tableElement]->insertRow(index, ec));
    raiseOnDOMError(ec);
    return [DOMHTMLTableElement _tableElementWith:impl];
}

- (void)deleteRow:(int)index
{
    ExceptionCode ec = 0;
    [self _tableElement]->deleteRow(index, ec);
    raiseOnDOMError(ec);
}

@end

@implementation DOMHTMLTableElement (WebCoreInternal)

+ (DOMHTMLTableElement *)_tableElementWith:(HTMLTableElement *)impl
{
    return static_cast<DOMHTMLTableElement*>([DOMNode _nodeWith:impl]);
}

- (HTMLTableElement *)_tableElement
{
    return static_cast<HTMLTableElement*>(DOM_cast<Node*>(_internal));
}

@end

@implementation DOMHTMLTableColElement

- (HTMLTableColElement *)_tableColElement
{
    return static_cast<HTMLTableColElement*>(DOM_cast<Node*>(_internal));
}

- (NSString *)align
{
    return [self _tableColElement]->getAttribute(alignAttr);
}

- (void)setAlign:(NSString *)align
{
    [self _tableColElement]->setAttribute(alignAttr, align);
}

- (NSString *)ch
{
    return [self _tableColElement]->getAttribute(charoffAttr);
}

- (void)setCh:(NSString *)ch
{
    [self _tableColElement]->setAttribute(charoffAttr, ch);
}

- (NSString *)chOff
{
    return [self _tableColElement]->getAttribute(charoffAttr);
}

- (void)setChOff:(NSString *)chOff
{
    [self _tableColElement]->setAttribute(charoffAttr, chOff);
}

- (int)span
{
    return [self _tableColElement]->getAttribute(spanAttr).toInt();
}

- (void)setSpan:(int)span
{
    [self _tableColElement]->setAttribute(spanAttr, String::number(span));
}

- (NSString *)vAlign
{
    return [self _tableColElement]->getAttribute(valignAttr);
}

- (void)setVAlign:(NSString *)vAlign
{
    [self _tableColElement]->setAttribute(valignAttr, vAlign);
}

- (NSString *)width
{
    return [self _tableColElement]->getAttribute(widthAttr);
}

- (void)setWidth:(NSString *)width
{
    [self _tableColElement]->setAttribute(widthAttr, width);
}

@end

@implementation DOMHTMLTableRowElement

- (HTMLTableRowElement *)_tableRowElement
{
    return static_cast<HTMLTableRowElement*>(DOM_cast<Node*>(_internal));
}

- (int)rowIndex
{
    return [self _tableRowElement]->rowIndex();
}

- (int)sectionRowIndex
{
    return [self _tableRowElement]->sectionRowIndex();
}

- (DOMHTMLCollection *)cells
{
    HTMLCollection *collection = new HTMLCollection([self _tableRowElement], HTMLCollection::TRCells);
    return [DOMHTMLCollection _collectionWith:collection];
}

- (NSString *)align
{
    return [self _tableRowElement]->getAttribute(alignAttr);
}

- (void)setAlign:(NSString *)align
{
    [self _tableRowElement]->setAttribute(alignAttr, align);
}

- (NSString *)bgColor
{
    return [self _tableRowElement]->getAttribute(bgcolorAttr);
}

- (void)setBgColor:(NSString *)bgColor
{
    [self _tableRowElement]->setAttribute(bgcolorAttr, bgColor);
}

- (NSString *)ch
{
    return [self _tableRowElement]->getAttribute(charoffAttr);
}

- (void)setCh:(NSString *)ch
{
    [self _tableRowElement]->setAttribute(charoffAttr, ch);
}

- (NSString *)chOff
{
    return [self _tableRowElement]->getAttribute(charoffAttr);
}

- (void)setChOff:(NSString *)chOff
{
    [self _tableRowElement]->setAttribute(charoffAttr, chOff);
}

- (NSString *)vAlign
{
    return [self _tableRowElement]->getAttribute(valignAttr);
}

- (void)setVAlign:(NSString *)vAlign
{
    [self _tableRowElement]->setAttribute(valignAttr, vAlign);
}

- (DOMHTMLElement *)insertCell:(int)index
{
    ExceptionCode ec = 0;
    HTMLTableCellElement *impl = static_cast<HTMLTableCellElement*>([self _tableRowElement]->insertCell(index, ec));
    raiseOnDOMError(ec);
    return [DOMHTMLTableCellElement _tableCellElementWith:impl];
}

- (void)deleteCell:(int)index
{
    ExceptionCode ec = 0;
    [self _tableRowElement]->deleteCell(index, ec);
    raiseOnDOMError(ec);
}

@end

@implementation DOMHTMLTableCellElement

- (int)cellIndex
{
    return [self _tableCellElement]->cellIndex();
}

- (NSString *)abbr
{
    return [self _tableCellElement]->getAttribute(abbrAttr);
}

- (void)setAbbr:(NSString *)abbr
{
    [self _tableCellElement]->setAttribute(abbrAttr, abbr);
}

- (NSString *)align
{
    return [self _tableCellElement]->getAttribute(alignAttr);
}

- (void)setAlign:(NSString *)align
{
    [self _tableCellElement]->setAttribute(alignAttr, align);
}

- (NSString *)axis
{
    return [self _tableCellElement]->getAttribute(axisAttr);
}

- (void)setAxis:(NSString *)axis
{
    [self _tableCellElement]->setAttribute(axisAttr, axis);
}

- (NSString *)bgColor
{
    return [self _tableCellElement]->getAttribute(bgcolorAttr);
}

- (void)setBgColor:(NSString *)bgColor
{
    [self _tableCellElement]->setAttribute(bgcolorAttr, bgColor);
}

- (NSString *)ch
{
    return [self _tableCellElement]->getAttribute(charoffAttr);
}

- (void)setCh:(NSString *)ch
{
    [self _tableCellElement]->setAttribute(charoffAttr, ch);
}

- (NSString *)chOff
{
    return [self _tableCellElement]->getAttribute(charoffAttr);
}

- (void)setChOff:(NSString *)chOff
{
    [self _tableCellElement]->setAttribute(charoffAttr, chOff);
}

- (int)colSpan
{
    return [self _tableCellElement]->getAttribute(colspanAttr).toInt();
}

- (void)setColSpan:(int)colSpan
{
    [self _tableCellElement]->setAttribute(colspanAttr, String::number(colSpan));
}

- (NSString *)headers
{
    return [self _tableCellElement]->getAttribute(headersAttr);
}

- (void)setHeaders:(NSString *)headers
{
    [self _tableCellElement]->setAttribute(headersAttr, headers);
}

- (NSString *)height
{
    return [self _tableCellElement]->getAttribute(heightAttr);
}

- (void)setHeight:(NSString *)height
{
    [self _tableCellElement]->setAttribute(heightAttr, height);
}

- (BOOL)noWrap
{
    return [self _tableCellElement]->getAttribute(nowrapAttr).isNull();
}

- (void)setNoWrap:(BOOL)noWrap
{
    [self _tableCellElement]->setAttribute(nowrapAttr, noWrap ? "" : 0);
}

- (int)rowSpan
{
    return [self _tableCellElement]->getAttribute(rowspanAttr).toInt();
}

- (void)setRowSpan:(int)rowSpan
{
    [self _tableCellElement]->setAttribute(rowspanAttr, String::number(rowSpan));
}

- (NSString *)scope
{
    return [self _tableCellElement]->getAttribute(scopeAttr);
}

- (void)setScope:(NSString *)scope
{
    [self _tableCellElement]->setAttribute(scopeAttr, scope);
}

- (NSString *)vAlign
{
    return [self _tableCellElement]->getAttribute(valignAttr);
}

- (void)setVAlign:(NSString *)vAlign
{
    [self _tableCellElement]->setAttribute(valignAttr, vAlign);
}

- (NSString *)width
{
    return [self _tableCellElement]->getAttribute(widthAttr);
}

- (void)setWidth:(NSString *)width
{
    [self _tableCellElement]->setAttribute(widthAttr, width);
}

@end

@implementation DOMHTMLTableCellElement (WebCoreInternal)

+ (DOMHTMLTableCellElement *)_tableCellElementWith:(HTMLTableCellElement *)impl
{
    return static_cast<DOMHTMLTableCellElement*>([DOMNode _nodeWith:impl]);
}

- (HTMLTableCellElement *)_tableCellElement
{
    return static_cast<HTMLTableCellElement*>(DOM_cast<Node*>(_internal));
}

@end

@implementation DOMHTMLFrameSetElement

- (HTMLFrameSetElement *)_frameSetElement
{
    return static_cast<HTMLFrameSetElement*>(DOM_cast<Node*>(_internal));
}

- (NSString *)rows
{
    return [self _frameSetElement]->getAttribute(rowsAttr);
}

- (void)setRows:(NSString *)rows
{
    [self _frameSetElement]->setAttribute(rowsAttr, rows);
}

- (NSString *)cols
{
    return [self _frameSetElement]->getAttribute(colsAttr);
}

- (void)setCols:(NSString *)cols
{
    [self _frameSetElement]->setAttribute(colsAttr, cols);
}

@end

@implementation DOMHTMLFrameElement

- (HTMLFrameElement *)_frameElement
{
    return static_cast<HTMLFrameElement*>(DOM_cast<Node*>(_internal));
}

- (NSString *)frameBorder
{
    return [self _frameElement]->getAttribute(frameborderAttr);
}

- (void)setFrameBorder:(NSString *)frameBorder
{
    [self _frameElement]->setAttribute(frameborderAttr, frameBorder);
}

- (NSString *)longDesc
{
    return [self _frameElement]->getAttribute(longdescAttr);
}

- (void)setLongDesc:(NSString *)longDesc
{
    [self _frameElement]->setAttribute(longdescAttr, longDesc);
}

- (NSString *)marginHeight
{
    return [self _frameElement]->getAttribute(marginheightAttr);
}

- (void)setMarginHeight:(NSString *)marginHeight
{
    [self _frameElement]->setAttribute(marginheightAttr, marginHeight);
}

- (NSString *)marginWidth
{
    return [self _frameElement]->getAttribute(marginwidthAttr);
}

- (void)setMarginWidth:(NSString *)marginWidth
{
    [self _frameElement]->setAttribute(marginwidthAttr, marginWidth);
}

- (NSString *)name
{
    return [self _frameElement]->getAttribute(nameAttr);
}

- (void)setName:(NSString *)name
{
    [self _frameElement]->setAttribute(nameAttr, name);
}

- (BOOL)noResize
{
    return [self _frameElement]->getAttribute(noresizeAttr).isNull();
}

- (void)setNoResize:(BOOL)noResize
{
    [self _frameElement]->setAttribute(noresizeAttr, noResize ? "" : 0);
}

- (NSString *)scrolling
{
    return [self _frameElement]->getAttribute(scrollingAttr);
}

- (void)setScrolling:(NSString *)scrolling
{
    [self _frameElement]->setAttribute(scrollingAttr, scrolling);
}

- (NSString *)src
{
    return [self _frameElement]->getAttribute(srcAttr);
}

- (void)setSrc:(NSString *)src
{
    [self _frameElement]->setAttribute(srcAttr, src);
}

- (DOMDocument *)contentDocument
{
    return [DOMDocument _documentWith:[self _frameElement]->contentDocument()];
}

@end

@implementation DOMHTMLIFrameElement

- (HTMLIFrameElement *)_IFrameElement
{
    return static_cast<HTMLIFrameElement*>(DOM_cast<Node*>(_internal));
}

- (NSString *)align
{
    return [self _IFrameElement]->getAttribute(alignAttr);
}

- (void)setAlign:(NSString *)align
{
    [self _IFrameElement]->setAttribute(alignAttr, align);
}

- (NSString *)frameBorder
{
    return [self _IFrameElement]->getAttribute(frameborderAttr);
}

- (void)setFrameBorder:(NSString *)frameBorder
{
    [self _IFrameElement]->setAttribute(frameborderAttr, frameBorder);
}

- (NSString *)height
{
    return [self _IFrameElement]->getAttribute(heightAttr);
}

- (void)setHeight:(NSString *)height
{
    [self _IFrameElement]->setAttribute(heightAttr, height);
}

- (NSString *)longDesc
{
    return [self _IFrameElement]->getAttribute(longdescAttr);
}

- (void)setLongDesc:(NSString *)longDesc
{
    [self _IFrameElement]->setAttribute(longdescAttr, longDesc);
}

- (NSString *)marginHeight
{
    return [self _IFrameElement]->getAttribute(marginheightAttr);
}

- (void)setMarginHeight:(NSString *)marginHeight
{
    [self _IFrameElement]->setAttribute(marginheightAttr, marginHeight);
}

- (NSString *)marginWidth
{
    return [self _IFrameElement]->getAttribute(marginwidthAttr);
}

- (void)setMarginWidth:(NSString *)marginWidth
{
    [self _IFrameElement]->setAttribute(marginwidthAttr, marginWidth);
}

- (NSString *)name
{
    return [self _IFrameElement]->getAttribute(nameAttr);
}

- (void)setName:(NSString *)name
{
    [self _IFrameElement]->setAttribute(nameAttr, name);
}

- (BOOL)noResize
{
    return [self _IFrameElement]->getAttribute(noresizeAttr).isNull();
}

- (void)setNoResize:(BOOL)noResize
{
    [self _IFrameElement]->setAttribute(noresizeAttr, noResize ? "" : 0);
}

- (NSString *)scrolling
{
    return [self _IFrameElement]->getAttribute(scrollingAttr);
}

- (void)setScrolling:(NSString *)scrolling
{
    [self _IFrameElement]->setAttribute(scrollingAttr, scrolling);
}

- (NSString *)src
{
    return [self _IFrameElement]->getAttribute(srcAttr);
}

- (void)setSrc:(NSString *)src
{
    [self _IFrameElement]->setAttribute(srcAttr, src);
}

- (NSString *)width
{
    return [self _IFrameElement]->getAttribute(widthAttr);
}

- (void)setWidth:(NSString *)width
{
    [self _IFrameElement]->setAttribute(widthAttr, width);
}

- (DOMDocument *)contentDocument
{
    return [DOMDocument _documentWith:[self _IFrameElement]->contentDocument()];
}

@end

#pragma mark DOM EXTENSIONS

@implementation DOMHTMLEmbedElement

- (HTMLEmbedElement *)_embedElement
{
    return static_cast<HTMLEmbedElement*>(DOM_cast<Node*>(_internal));
}

- (NSString *)align
{
    return [self _embedElement]->getAttribute(alignAttr);
}

- (void)setAlign:(NSString *)align
{
    [self _embedElement]->setAttribute(alignAttr, align);
}

- (int)height
{
    return [self _embedElement]->getAttribute(heightAttr).toInt();
}

- (void)setHeight:(int)height
{
    [self _embedElement]->setAttribute(heightAttr, String::number(height));
}

- (NSString *)name
{
    return [self _embedElement]->getAttribute(nameAttr);
}

- (void)setName:(NSString *)name
{
    [self _embedElement]->setAttribute(nameAttr, name);
}

- (NSString *)src
{
    return [self _embedElement]->getAttribute(srcAttr);
}

- (void)setSrc:(NSString *)src
{
    [self _embedElement]->setAttribute(srcAttr, src);
}

- (NSString *)type
{
    return [self _embedElement]->getAttribute(typeAttr);
}

- (void)setType:(NSString *)type
{
    [self _embedElement]->setAttribute(typeAttr, type);
}

- (int)width
{
    return [self _embedElement]->getAttribute(widthAttr).toInt();
}

- (void)setWidth:(int)width
{
    [self _embedElement]->setAttribute(widthAttr, String::number(width));
}

@end

// These #imports and "usings" are used only by viewForElement and should be deleted 
// when that function goes away.
#import "RenderWidget.h"
using WebCore::RenderObject;
using WebCore::RenderWidget;

// This function is used only by the two FormAutoFillTransition categories, and will go away
// as soon as possible.
static NSView *viewForElement(DOMElement *element)
{
    RenderObject *renderer = [element _element]->renderer();
    if (renderer && renderer->isWidget()) {
        Widget *widget = static_cast<const RenderWidget*>(renderer)->widget();
        if (widget) {
            widget->populate();
            return widget->getView();
        }
    }
    return nil;
}

@implementation DOMHTMLInputElement(FormAutoFillTransition)

- (BOOL)_isTextField
{
    // We could make this public API as-is, or we could change it into a method that returns whether
    // the element is a text field or a button or ... ?
    static NSArray *textInputTypes = nil;
#ifndef NDEBUG
    static NSArray *nonTextInputTypes = nil;
#endif
    
    NSString *fieldType = [self type];
    
    // No type at all is treated as text type
    if ([fieldType length] == 0)
        return YES;
    
    if (textInputTypes == nil)
        textInputTypes = [[NSSet alloc] initWithObjects:@"text", @"password", @"search", @"isindex", nil];
    
    BOOL isText = [textInputTypes containsObject:[fieldType lowercaseString]];
    
#ifndef NDEBUG
    if (nonTextInputTypes == nil)
        nonTextInputTypes = [[NSSet alloc] initWithObjects:@"checkbox", @"radio", @"submit", @"reset", @"file", @"hidden", @"image", @"button", @"range", nil];
    
    // Catch cases where a new input type has been added that's not in these lists.
    ASSERT(isText || [nonTextInputTypes containsObject:[fieldType lowercaseString]]);
#endif    
    
    return isText;
}

- (NSRect)_rectOnScreen
{
    // Returns bounding rect of text field, in screen coordinates.
    NSView* view = [self _HTMLInputElement]->document()->view()->getDocumentView();
    NSRect result = [self boundingBox];
    result = [view convertRect:result toView:nil];
    result.origin = [[view window] convertBaseToScreen:result.origin];
    return result;
}

- (void)_replaceCharactersInRange:(NSRange)targetRange withString:(NSString *)replacementString selectingFromIndex:(int)index
{
    HTMLInputElement* inputElement = [self _HTMLInputElement];
    if (inputElement) {
        String newValue = inputElement->value().replace(targetRange.location, targetRange.length, replacementString);
        inputElement->setValue(newValue);
        inputElement->setSelectionRange(index, newValue.length());
    }
}

- (NSRange)_selectedRange
{
    HTMLInputElement* inputElement = [self _HTMLInputElement];
    if (inputElement) {
        int start = inputElement->selectionStart();
        int end = inputElement->selectionEnd();
        return NSMakeRange(start, end - start); 
    }
    return NSMakeRange(NSNotFound, 0);
}    

- (void)_setAutofilled:(BOOL)filled
{
    // This notifies the input element that the content has been autofilled
    // This allows WebKit to obey the -webkit-autofill pseudo style, which
    // changes the background color.
    HTMLInputElement* inputElement = [self _HTMLInputElement];
    if (inputElement)
        inputElement->setAutofilled(filled);
}

@end

@implementation DOMHTMLSelectElement(FormAutoFillTransition)

- (void)_activateItemAtIndex:(int)index
{
    NSPopUpButton *popUp = (NSPopUpButton *)viewForElement(self);
    [popUp selectItemAtIndex:index];
    // Must do this to simulate same side effect as if user made the choice
    [NSApp sendAction:[popUp action] to:[popUp target] from:popUp];
}

@end
