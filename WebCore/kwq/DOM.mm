/*
 * Copyright (C) 2004 Apple Computer, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source exceptionCode must retain the above copyright
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

#import "DOM.h"

#include <objc/objc-class.h>

#import <dom/dom_doc.h>
#import <dom/dom_element.h>
#import <dom/dom_exception.h>
#import <dom/dom_node.h>
#import <dom/dom_string.h>
#import <dom/dom_text.h>
#import <dom/dom_xml.h>
#import <dom/dom2_range.h>
#import <html/html_elementimpl.h>
#import <xml/dom_docimpl.h>
#import <xml/dom_elementimpl.h>
#import <xml/dom_nodeimpl.h>
#import <xml/dom_stringimpl.h>
#import <xml/dom_textimpl.h>
#import <xml/dom_xmlimpl.h>
#import <xml/dom2_rangeimpl.h>
#import <xml/dom2_viewsimpl.h>

#import "DOMInternal.h"
#import "KWQAssertions.h"

using DOM::Attr;
using DOM::AttrImpl;
using DOM::CharacterDataImpl;
using DOM::DocumentFragmentImpl;
using DOM::DocumentType;
using DOM::DocumentTypeImpl;
using DOM::Document;
using DOM::DocumentImpl;
using DOM::DOMImplementationImpl;
using DOM::DOMString;
using DOM::DOMStringImpl;
using DOM::Element;
using DOM::ElementImpl;
using DOM::EntityImpl;
using DOM::HTMLElementImpl;
using DOM::NamedNodeMap;
using DOM::NamedNodeMapImpl;
using DOM::Node;
using DOM::NodeImpl;
using DOM::NodeListImpl;
using DOM::NotationImpl;
using DOM::ProcessingInstructionImpl;
using DOM::Range;
using DOM::RangeException;
using DOM::RangeImpl;
using DOM::TextImpl;

@interface DOMAttr (WebCoreInternal)
+ (DOMAttr *)_attrWithImpl:(AttrImpl *)impl;
- (AttrImpl *)_attrImpl;
@end

@interface DOMDocumentFragment (WebCoreInternal)
+ (DOMDocumentFragment *)_documentFragmentWithImpl:(DocumentFragmentImpl *)impl;
@end

@interface DOMImplementation (WebCoreInternal)
+ (DOMImplementation *)_DOMImplementationWithImpl:(DOMImplementationImpl *)impl;
- (DOMImplementationImpl *)_DOMImplementationImpl;
@end

@interface DOMNamedNodeMap (WebCoreInternal)
+ (DOMNamedNodeMap *)_namedNodeMapWithImpl:(NamedNodeMapImpl *)impl;
@end

//------------------------------------------------------------------------------------------
// Factory methods

inline NamedNodeMap NamedNodeMapImpl::createInstance(NamedNodeMapImpl *impl)
{
    return NamedNodeMap(impl);
}

inline Attr AttrImpl::createInstance(AttrImpl *impl)
{
    return Attr(impl);
}

inline Element ElementImpl::createInstance(ElementImpl *impl)
{
    return Element(impl);
}

inline DocumentType DocumentTypeImpl::createInstance(DocumentTypeImpl *impl)
{
    return DocumentType(impl);
}

inline Document DocumentImpl::createInstance(DocumentImpl *impl)
{
    return Document(impl);
}

//------------------------------------------------------------------------------------------
// DOMObject

@implementation DOMObject

// Prevent creation of DOM objects by clients who just "[[xxx alloc] init]".
- (id)init
{
    [NSException raise:NSGenericException format:@"+[%s init]: should never be used", [self class]->name];
    [self release];
    return nil;
}

- (void)dealloc
{
    if (_internal) {
        removeDOMWrapperForImpl(_internal);
    }
    [super dealloc];
}

- (id)copyWithZone:(NSZone *)zone
{
    return [self retain];
}

@end

@implementation DOMObject (WebCoreInternal)

- (id)_init
{
    return [super init];
}

@end

//------------------------------------------------------------------------------------------
// DOMNode

@implementation DOMNode

- (void)dealloc
{
    if (_internal) {
        reinterpret_cast<NodeImpl *>(_internal)->deref();
    }
    [super dealloc];
}

- (NSString *)nodeName
{
    return [self _nodeImpl]->nodeName();
}

- (NSString *)nodeValue
{
    // Documentation says we can raise a DOMSTRING_SIZE_ERR.
    // However, the lower layer does not report that error up to us.
    return [self _nodeImpl]->nodeValue();
}

- (void)setNodeValue:(NSString *)string
{
    ASSERT(string);
    
    int exceptionCode = 0;
    [self _nodeImpl]->setNodeValue(string, exceptionCode);
    raiseOnDOMError(exceptionCode);
}

- (unsigned short)nodeType
{
    return [self _nodeImpl]->nodeType();
}

- (DOMNode *)parentNode
{
    return [DOMNode _nodeWithImpl:[self _nodeImpl]->parentNode()];
}

- (DOMNodeList *)childNodes
{
    return [DOMNodeList _nodeListWithImpl:[self _nodeImpl]->childNodes()];
}

- (DOMNode *)firstChild
{
    return [DOMNode _nodeWithImpl:[self _nodeImpl]->firstChild()];
}

- (DOMNode *)lastChild
{
    return [DOMNode _nodeWithImpl:[self _nodeImpl]->lastChild()];
}

- (DOMNode *)previousSibling
{
    return [DOMNode _nodeWithImpl:[self _nodeImpl]->previousSibling()];
}

- (DOMNode *)nextSibling
{
    return [DOMNode _nodeWithImpl:[self _nodeImpl]->nextSibling()];
}

- (DOMNamedNodeMap *)attributes
{
    // DOM level 2 core specification says: 
    // A NamedNodeMap containing the attributes of this node (if it is an Element) or null otherwise.
    return nil;
}

- (DOMDocument *)ownerDocument
{
    return [DOMDocument _documentWithImpl:[self _nodeImpl]->getDocument()];
}

- (DOMNode *)insertBefore:(DOMNode *)newChild :(DOMNode *)refChild
{
    ASSERT(newChild);
    ASSERT(refChild);

    int exceptionCode = 0;
    DOMNode *result = [DOMNode _nodeWithImpl:[self _nodeImpl]->insertBefore([newChild _nodeImpl], [refChild _nodeImpl], exceptionCode)];
    raiseOnDOMError(exceptionCode);
    return result;
}

- (DOMNode *)replaceChild:(DOMNode *)newChild :(DOMNode *)oldChild
{
    ASSERT(newChild);
    ASSERT(oldChild);

    int exceptionCode = 0;
    DOMNode *result = [DOMNode _nodeWithImpl:[self _nodeImpl]->replaceChild([newChild _nodeImpl], [oldChild _nodeImpl], exceptionCode)];
    raiseOnDOMError(exceptionCode);
    return result;
}

- (DOMNode *)removeChild:(DOMNode *)oldChild
{
    ASSERT(oldChild);

    int exceptionCode = 0;
    DOMNode *result = [DOMNode _nodeWithImpl:[self _nodeImpl]->removeChild([oldChild _nodeImpl], exceptionCode)];
    raiseOnDOMError(exceptionCode);
    return result;
}

- (DOMNode *)appendChild:(DOMNode *)newChild
{
    ASSERT(newChild);

    int exceptionCode = 0;
    DOMNode *result = [DOMNode _nodeWithImpl:[self _nodeImpl]->appendChild([newChild _nodeImpl], exceptionCode)];
    raiseOnDOMError(exceptionCode);
    return result;
}

- (BOOL)hasChildNodes
{
    return [self _nodeImpl]->hasChildNodes();
}

- (DOMNode *)cloneNode:(BOOL)deep
{
    return [DOMNode _nodeWithImpl:[self _nodeImpl]->cloneNode(deep)];
}

- (void)normalize
{
    [self _nodeImpl]->normalize();
}

- (BOOL)isSupported:(NSString *)feature :(NSString *)version
{
    ASSERT(feature);
    ASSERT(version);

    // Method not reflected in DOM::NodeImpl interface
    return Node([self _nodeImpl]).isSupported(feature, version);
}

- (NSString *)namespaceURI
{
    // Method not reflected in DOM::NodeImpl interface
    return Node([self _nodeImpl]).namespaceURI();
}

- (NSString *)prefix
{
    return [self _nodeImpl]->prefix();
}

- (void)setPrefix:(NSString *)prefix
{
    ASSERT(prefix);

    int exceptionCode = 0;
    [self _nodeImpl]->setPrefix(prefix, exceptionCode);
    raiseOnDOMError(exceptionCode);
}

- (NSString *)localName
{
    return [self _nodeImpl]->localName();
}

- (BOOL)hasAttributes
{
    // Method not reflected in DOM::NodeImpl interface
    return Node([self _nodeImpl]).hasAttributes();
}

@end

@implementation DOMNode (DOMExtensions)

- (NSString *)HTMLString
{
    return [self _nodeImpl]->recursive_toHTML(true).getNSString();
}

@end

@implementation DOMNode (WebCoreInternal)

- (id)_initWithNodeImpl:(NodeImpl *)impl
{
    ASSERT(impl);

    [super _init];
    _internal = reinterpret_cast<DOMObjectInternal *>(impl);
    impl->ref();
    setDOMWrapperForImpl(self, impl);
    return self;
}

+ (DOMNode *)_nodeWithImpl:(NodeImpl *)impl
{
    if (!impl)
        return nil;
    
    id cachedInstance;
    cachedInstance = getDOMWrapperForImpl(impl);
    if (cachedInstance)
        return [[cachedInstance retain] autorelease];
    
    Class wrapperClass = nil;
    switch (impl->nodeType()) {
    case Node::ELEMENT_NODE:
        wrapperClass = [DOMElement class];
        break;
    case Node::ATTRIBUTE_NODE:
        wrapperClass = [DOMAttr class];
        break;
    case Node::TEXT_NODE:
        wrapperClass = [DOMText class];
        break;
    case Node::CDATA_SECTION_NODE:
        wrapperClass = [DOMCDATASection class];
        break;
    case Node::ENTITY_REFERENCE_NODE:
        wrapperClass = [DOMEntityReference class];
        break;
    case Node::ENTITY_NODE:
        wrapperClass = [DOMEntity class];
        break;
    case Node::PROCESSING_INSTRUCTION_NODE:
        wrapperClass = [DOMProcessingInstruction class];
        break;
    case Node::COMMENT_NODE:
        wrapperClass = [DOMComment class];
        break;
    case Node::DOCUMENT_NODE:
        wrapperClass = [DOMDocument class];
        break;
    case Node::DOCUMENT_TYPE_NODE:
        wrapperClass = [DOMDocumentType class];
        break;
    case Node::DOCUMENT_FRAGMENT_NODE:
        wrapperClass = [DOMDocumentFragment class];
        break;
    case Node::NOTATION_NODE:
        wrapperClass = [DOMNotation class];
        break;
    }
    return [[[wrapperClass alloc] _initWithNodeImpl:impl] autorelease];
}

- (NodeImpl *)_nodeImpl
{
    return reinterpret_cast<NodeImpl *>(_internal);
}

@end

//------------------------------------------------------------------------------------------
// DOMNamedNodeMap

@implementation DOMNamedNodeMap

- (void)dealloc
{
    if (_internal) {
        reinterpret_cast<NamedNodeMapImpl *>(_internal)->deref();
    }
    [super dealloc];
}

- (NamedNodeMapImpl *)_namedNodeMapImpl
{
    return reinterpret_cast<NamedNodeMapImpl *>(_internal);
}

- (DOMNode *)getNamedItem:(NSString *)name
{
    ASSERT(name);

    // Method not reflected in DOM::NamedNodeMapImpl interface
    NamedNodeMap map = NamedNodeMapImpl::createInstance([self _namedNodeMapImpl]);
    Node result(map.getNamedItem(name));
    return [DOMNode _nodeWithImpl:result.handle()];
}

- (DOMNode *)setNamedItem:(DOMNode *)arg
{
    ASSERT(arg);

    // Method not reflected in DOM::NamedNodeMapImpl interface
    try {
        NamedNodeMap map = NamedNodeMapImpl::createInstance([self _namedNodeMapImpl]);
        Node result(map.setNamedItem([arg _nodeImpl]));
        return [DOMNode _nodeWithImpl:result.handle()];
    } 
    catch (const DOM::DOMException &e) {
        raiseOnDOMError(e.code);
        return nil;
    }
}

- (DOMNode *)removeNamedItem:(NSString *)name
{
    ASSERT(name);

    // Method not reflected in DOM::NamedNodeMapImpl interface
    try {
        NamedNodeMap map = NamedNodeMapImpl::createInstance([self _namedNodeMapImpl]);
        Node result(map.removeNamedItem(name));
        return [DOMNode _nodeWithImpl:result.handle()];
    } 
    catch (const DOM::DOMException &e) {
        raiseOnDOMError(e.code);
        return nil;
    }
}

- (DOMNode *)item:(unsigned long)index
{
    return [DOMNode _nodeWithImpl:[self _namedNodeMapImpl]->item(index)];
}

- (unsigned long)length
{
    return [self _namedNodeMapImpl]->length();
}

- (DOMNode *)getNamedItemNS:(NSString *)namespaceURI :(NSString *)localName
{
    if (!namespaceURI || !localName) {
        return nil;
    }

    // Method not reflected in DOM::NamedNodeMapImpl interface
    NamedNodeMap map = NamedNodeMapImpl::createInstance([self _namedNodeMapImpl]);
    Node result(map.getNamedItemNS(namespaceURI, localName));
    return [DOMNode _nodeWithImpl:result.handle()];
}

- (DOMNode *)setNamedItemNS:(DOMNode *)arg
{
    ASSERT(arg);

    // Method not reflected in DOM::NamedNodeMapImpl interface
    try {
        NamedNodeMap map = NamedNodeMapImpl::createInstance([self _namedNodeMapImpl]);
        Node result(map.setNamedItemNS([arg _nodeImpl]));
        return [DOMNode _nodeWithImpl:result.handle()];
    } 
    catch (const DOM::DOMException &e) {
        raiseOnDOMError(e.code);
        return nil;
    }
}

- (DOMNode *)removeNamedItemNS:(NSString *)namespaceURI :(NSString *)localName
{
    ASSERT(namespaceURI);
    ASSERT(localName);

    // Method not reflected in DOM::NamedNodeMapImpl interface
    try {
        NamedNodeMap map = NamedNodeMapImpl::createInstance([self _namedNodeMapImpl]);
        Node result(map.removeNamedItemNS(namespaceURI, localName));
        return [DOMNode _nodeWithImpl:result.handle()];
    } 
    catch (const DOM::DOMException &e) {
        raiseOnDOMError(e.code);
        return nil;
    }
}

@end

@implementation DOMNamedNodeMap (WebCoreInternal)

- (id)_initWithNamedNodeMapImpl:(NamedNodeMapImpl *)impl
{
    ASSERT(impl);

    [super _init];
    _internal = reinterpret_cast<DOMObjectInternal *>(impl);
    impl->ref();
    setDOMWrapperForImpl(self, impl);
    return self;
}

+ (DOMNamedNodeMap *)_namedNodeMapWithImpl:(NamedNodeMapImpl *)impl
{
    if (!impl)
        return nil;
    
    id cachedInstance;
    cachedInstance = getDOMWrapperForImpl(impl);
    if (cachedInstance)
        return [[cachedInstance retain] autorelease];
    
    return [[[self alloc] _initWithNamedNodeMapImpl:impl] autorelease];
}

@end

//------------------------------------------------------------------------------------------
// DOMNodeList

@implementation DOMNodeList

- (void)dealloc
{
    if (_internal) {
        reinterpret_cast<NodeListImpl *>(_internal)->deref();
    }
    [super dealloc];
}

- (NodeListImpl *)_nodeListImpl
{
    return reinterpret_cast<NodeListImpl *>(_internal);
}

- (DOMNode *)item:(unsigned long)index
{
    return [DOMNode _nodeWithImpl:[self _nodeListImpl]->item(index)];
}

- (unsigned long)length
{
    return [self _nodeListImpl]->length();
}

@end

@implementation DOMNodeList (WebCoreInternal)

- (id)_initWithNodeListImpl:(NodeListImpl *)impl
{
    ASSERT(impl);

    [super _init];
    _internal = reinterpret_cast<DOMObjectInternal *>(impl);
    impl->ref();
    setDOMWrapperForImpl(self, impl);
    return self;
}

+ (DOMNodeList *)_nodeListWithImpl:(NodeListImpl *)impl
{
    if (!impl)
        return nil;
    
    id cachedInstance;
    cachedInstance = getDOMWrapperForImpl(impl);
    if (cachedInstance)
        return [[cachedInstance retain] autorelease];
    
    return [[[self alloc] _initWithNodeListImpl:impl] autorelease];
}

@end

//------------------------------------------------------------------------------------------
// DOMImplementation

@implementation DOMImplementation

- (void)dealloc
{
    if (_internal) {
        reinterpret_cast<DOMImplementationImpl *>(_internal)->deref();
    }
    [super dealloc];
}

- (BOOL)hasFeature:(NSString *)feature :(NSString *)version
{
    ASSERT(feature);
    ASSERT(version);

    return [self _DOMImplementationImpl]->hasFeature(feature, version);
}

- (DOMDocumentType *)createDocumentType:(NSString *)qualifiedName :(NSString *)publicId :(NSString *)systemId
{
    ASSERT(qualifiedName);
    ASSERT(publicId);
    ASSERT(systemId);

    int exceptionCode = 0;
    DocumentTypeImpl *impl = [self _DOMImplementationImpl]->createDocumentType(qualifiedName, publicId, systemId, exceptionCode);
    raiseOnDOMError(exceptionCode);
    return static_cast<DOMDocumentType *>([DOMNode _nodeWithImpl:impl]);
}

- (DOMDocument *)createDocument:(NSString *)namespaceURI :(NSString *)qualifiedName :(DOMDocumentType *)doctype
{
    ASSERT(namespaceURI);
    ASSERT(qualifiedName);

    int exceptionCode = 0;
    DocumentType dt = DocumentTypeImpl::createInstance(static_cast<DocumentTypeImpl *>([doctype _nodeImpl]));
    DocumentImpl *impl = [self _DOMImplementationImpl]->createDocument(namespaceURI, qualifiedName, dt, exceptionCode);
    raiseOnDOMError(exceptionCode);
    return static_cast<DOMDocument *>([DOMNode _nodeWithImpl:impl]);
}

@end

@implementation DOMImplementation (DOMImplementationCSS)

- (DOMCSSStyleSheet *)createCSSStyleSheet:(NSString *)title :(NSString *)media
{
    ASSERT(title);
    ASSERT(media);

    int exceptionCode = 0;
    DOMString titleString(title);
    DOMString mediaString(media);
    DOMCSSStyleSheet *result = [DOMCSSStyleSheet _CSSStyleSheetWithImpl:[self _DOMImplementationImpl]->createCSSStyleSheet(titleString.implementation(), mediaString.implementation(), exceptionCode)];
    raiseOnDOMError(exceptionCode);
    return result;
}

@end
 
@implementation DOMImplementation (WebInternal)

- (id)_initWithDOMImplementationImpl:(DOMImplementationImpl *)impl
{
    ASSERT(impl);

    [super _init];
    _internal = reinterpret_cast<DOMObjectInternal *>(impl);
    impl->ref();
    setDOMWrapperForImpl(self, impl);
    return self;
}

+ (DOMImplementation *)_DOMImplementationWithImpl:(DOMImplementationImpl *)impl
{
    if (!impl)
        return nil;
    
    id cachedInstance;
    cachedInstance = getDOMWrapperForImpl(impl);
    if (cachedInstance)
        return [[cachedInstance retain] autorelease];
    
    return [[[self alloc] _initWithDOMImplementationImpl:impl] autorelease];
}

- (DOMImplementationImpl *)_DOMImplementationImpl
{
    return reinterpret_cast<DOMImplementationImpl *>(_internal);
}

@end

//------------------------------------------------------------------------------------------
// DOMDocumentFragment

@implementation DOMDocumentFragment

@end

@implementation DOMDocumentFragment (WebCoreInternal)

+ (DOMDocumentFragment *)_documentFragmentWithImpl:(DocumentFragmentImpl *)impl
{
    return static_cast<DOMDocumentFragment *>([DOMNode _nodeWithImpl:impl]);
}

@end

//------------------------------------------------------------------------------------------
// DOMDocument

@implementation DOMDocument

- (DOMDocumentType *)doctype
{
    return static_cast<DOMDocumentType *>([DOMNode _nodeWithImpl:[self _documentImpl]->doctype()]);
}

- (DOMImplementation *)implementation
{
    return [DOMImplementation _DOMImplementationWithImpl:[self _documentImpl]->implementation()];
}

- (DOMElement *)documentElement
{
    return static_cast<DOMElement *>([DOMNode _nodeWithImpl:[self _documentImpl]->documentElement()]);
}

- (DOMElement *)createElement:(NSString *)tagName
{
    ASSERT(tagName);

    int exceptionCode = 0;
    DOMElement *result = static_cast<DOMElement *>([DOMNode _nodeWithImpl:[self _documentImpl]->createElement(tagName, exceptionCode)]);
    raiseOnDOMError(exceptionCode);
    return result;
}

- (DOMDocumentFragment *)createDocumentFragment
{
    return static_cast<DOMDocumentFragment *>([DOMNode _nodeWithImpl:[self _documentImpl]->createDocumentFragment()]);
}

- (DOMText *)createTextNode:(NSString *)data
{
    ASSERT(data);
    return static_cast<DOMText *>([DOMNode _nodeWithImpl:[self _documentImpl]->createTextNode(data)]);
}

- (DOMComment *)createComment:(NSString *)data
{
    ASSERT(data);
    return static_cast<DOMComment *>([DOMNode _nodeWithImpl:[self _documentImpl]->createComment(data)]);
}

- (DOMCDATASection *)createCDATASection:(NSString *)data
{
    ASSERT(data);

    // Documentation says we can raise a NOT_SUPPORTED_ERR.
    // However, the lower layer does not report that error up to us.
    return static_cast<DOMCDATASection *>([DOMNode _nodeWithImpl:[self _documentImpl]->createCDATASection(data)]);
}

- (DOMProcessingInstruction *)createProcessingInstruction:(NSString *)target :(NSString *)data
{
    ASSERT(target);
    ASSERT(data);

    // Documentation says we can raise a INVALID_CHARACTER_ERR or a NOT_SUPPORTED_ERR.
    // However, the lower layer does not report these errors up to us.
    return static_cast<DOMProcessingInstruction *>([DOMNode _nodeWithImpl:[self _documentImpl]->createProcessingInstruction(target, data)]);
}

- (DOMAttr *)createAttribute:(NSString *)name
{
    ASSERT(name);

    // Method not reflected in DOM::DocumentImpl interface
    try {
        Document doc(DocumentImpl::createInstance([self _documentImpl]));
        Attr result(doc.createAttribute(name));
        return static_cast<DOMAttr *>([DOMNode _nodeWithImpl:result.handle()]);
    } 
    catch (const DOM::DOMException &e) {
        raiseOnDOMError(e.code);
        return nil;
    }
}

- (DOMEntityReference *)createEntityReference:(NSString *)name
{
    ASSERT(name);

    // Documentation says we can raise a INVALID_CHARACTER_ERR or a NOT_SUPPORTED_ERR.
    // However, the lower layer does not report these errors up to us.
    return static_cast<DOMEntityReference *>([DOMNode _nodeWithImpl:[self _documentImpl]->createEntityReference(name)]);
}

- (DOMNodeList *)getElementsByTagName:(NSString *)tagname
{
    ASSERT(tagname);
    return [DOMNodeList _nodeListWithImpl:[self _documentImpl]->getElementsByTagNameNS(0, DOMString(tagname).implementation())];
}

- (DOMNode *)importNode:(DOMNode *)importedNode :(BOOL)deep
{
    int exceptionCode = 0;
    DOMNode *result = [DOMNode _nodeWithImpl:[self _documentImpl]->importNode([importedNode _nodeImpl], deep, exceptionCode)];
    raiseOnDOMError(exceptionCode);
    return result;
}

- (DOMElement *)createElementNS:(NSString *)namespaceURI :(NSString *)qualifiedName
{
    ASSERT(namespaceURI);
    ASSERT(qualifiedName);

    int exceptionCode = 0;
    DOMNode *result = [DOMNode _nodeWithImpl:[self _documentImpl]->createElementNS(namespaceURI, qualifiedName, exceptionCode)];
    raiseOnDOMError(exceptionCode);
    return static_cast<DOMElement *>(result);
}

- (DOMAttr *)createAttributeNS:(NSString *)namespaceURI :(NSString *)qualifiedName
{
    ASSERT(namespaceURI);
    ASSERT(qualifiedName);

    // Method not reflected in DOM::DocumentImpl interface
    try {
        Document doc(DocumentImpl::createInstance([self _documentImpl]));
        Attr result(doc.createAttributeNS(namespaceURI, qualifiedName));
        return static_cast<DOMAttr *>([DOMNode _nodeWithImpl:result.handle()]);
    } 
    catch (const DOM::DOMException &e) {
        raiseOnDOMError(e.code);
        return nil;
    }
}

- (DOMNodeList *)getElementsByTagNameNS:(NSString *)namespaceURI :(NSString *)localName
{
    ASSERT(namespaceURI);
    ASSERT(localName);

    return [DOMNodeList _nodeListWithImpl:[self _documentImpl]->getElementsByTagNameNS(DOMString(namespaceURI).implementation(), DOMString(localName).implementation())];
}

- (DOMElement *)getElementById:(NSString *)elementId
{
    ASSERT(elementId);

    return static_cast<DOMElement *>([DOMNode _nodeWithImpl:[self _documentImpl]->getElementById(elementId)]);
}

@end

@implementation DOMDocument (DOMDocumentRange)

- (DOMRange *)createRange
{
    return [DOMRange _rangeWithImpl:[self _documentImpl]->createRange()];
}

@end

@implementation DOMDocument (DOMDocumentCSS)

- (DOMCSSStyleDeclaration *)getComputedStyle:(DOMElement *)elt :(NSString *)pseudoElt
{
    ElementImpl *elementImpl = [elt _elementImpl];
    DOMString pseudoEltString(pseudoElt);
    return [DOMCSSStyleDeclaration _styleDeclarationWithImpl:[self _documentImpl]->defaultView()->getComputedStyle(elementImpl, pseudoEltString.implementation())];
}

- (DOMCSSStyleDeclaration *)getOverrideStyle:(DOMElement *)elt :(NSString *)pseudoElt;
{
    // FIXME: This is unimplemented by khtml, 
    // so for now, we just return the computed style
    return [self getComputedStyle:elt :pseudoElt];
}

@end

@implementation DOMDocument (DOMDocumentStyle)

- (DOMStyleSheetList *)styleSheets
{
    return [DOMStyleSheetList _styleSheetListWithImpl:[self _documentImpl]->styleSheets()];
}

@end

@implementation DOMDocument (WebCoreInternal)

+ (DOMDocument *)_documentWithImpl:(DocumentImpl *)impl
{
    return static_cast<DOMDocument *>([DOMNode _nodeWithImpl:impl]);
}

- (DocumentImpl *)_documentImpl
{
    return static_cast<DocumentImpl *>(reinterpret_cast<NodeImpl *>(_internal));
}

@end

//------------------------------------------------------------------------------------------
// DOMCharacterData

@implementation DOMCharacterData

- (CharacterDataImpl *)_characterDataImpl
{
    return static_cast<CharacterDataImpl *>(reinterpret_cast<NodeImpl *>(_internal));
}

- (NSString *)data
{
    // Documentation says we can raise a DOMSTRING_SIZE_ERR.
    // However, the lower layer does not report that error up to us.
    return [self _characterDataImpl]->data();
}

- (void)setData:(NSString *)data
{
    ASSERT(data);
    
    int exceptionCode = 0;
    [self _characterDataImpl]->setData(data, exceptionCode);
    raiseOnDOMError(exceptionCode);
}

- (unsigned long)length
{
    return [self _characterDataImpl]->length();
}

- (NSString *)substringData:(unsigned long)offset :(unsigned long)count
{
    int exceptionCode = 0;
    NSString *result = [self _characterDataImpl]->substringData(offset, count, exceptionCode);
    raiseOnDOMError(exceptionCode);
    return result;
}

- (void)appendData:(NSString *)arg
{
    ASSERT(arg);
    
    int exceptionCode = 0;
    [self _characterDataImpl]->appendData(arg, exceptionCode);
    raiseOnDOMError(exceptionCode);
}

- (void)insertData:(unsigned long)offset :(NSString *)arg
{
    ASSERT(arg);
    
    int exceptionCode = 0;
    [self _characterDataImpl]->insertData(offset, arg, exceptionCode);
    raiseOnDOMError(exceptionCode);
}

- (void)deleteData:(unsigned long)offset :(unsigned long) count;
{
    int exceptionCode = 0;
    [self _characterDataImpl]->deleteData(offset, count, exceptionCode);
    raiseOnDOMError(exceptionCode);
}

- (void)replaceData:(unsigned long)offset :(unsigned long)count :(NSString *)arg
{
    ASSERT(arg);

    int exceptionCode = 0;
    [self _characterDataImpl]->replaceData(offset, count, arg, exceptionCode);
    raiseOnDOMError(exceptionCode);
}

@end

//------------------------------------------------------------------------------------------
// DOMAttr

@implementation DOMAttr

- (NSString *)name
{
    return [self _attrImpl]->nodeName();
}

- (BOOL)specified
{
    return [self _attrImpl]->specified();
}

- (NSString *)value
{
    return [self _attrImpl]->nodeValue();
}

- (void)setValue:(NSString *)value
{
    ASSERT(value);

    int exceptionCode = 0;
    [self _attrImpl]->setValue(value, exceptionCode);
    raiseOnDOMError(exceptionCode);
}

- (DOMElement *)ownerElement
{
    return [DOMElement _elementWithImpl:[self _attrImpl]->ownerElement()];
}

@end

@implementation DOMAttr (WebCoreInternal)

+ (DOMAttr *)_attrWithImpl:(AttrImpl *)impl
{
    return static_cast<DOMAttr *>([DOMNode _nodeWithImpl:impl]);
}

- (AttrImpl *)_attrImpl
{
    return static_cast<AttrImpl *>(reinterpret_cast<NodeImpl *>(_internal));
}

@end

//------------------------------------------------------------------------------------------
// DOMElement

@implementation DOMElement

- (NSString *)tagName
{
    return [self _elementImpl]->tagName();
}

- (DOMNamedNodeMap *)attributes
{
    return [DOMNamedNodeMap _namedNodeMapWithImpl:[self _elementImpl]->attributes()];
}

- (NSString *)getAttribute:(NSString *)name
{
    ASSERT(name);
    return [self _elementImpl]->getAttribute(name);
}

- (void)setAttribute:(NSString *)name :(NSString *)value
{
    ASSERT(name);
    ASSERT(value);

    // Method not reflected in DOM::ElementImpl interface
    try {
        Element element(ElementImpl::createInstance([self _elementImpl]));
        element.setAttribute(name, value);
    } 
    catch (const DOM::DOMException &e) {
        raiseOnDOMError(e.code);
    }
}

- (void)removeAttribute:(NSString *)name
{
    ASSERT(name);

    // Method not reflected in DOM::ElementImpl interface
    try {
        Element element(ElementImpl::createInstance([self _elementImpl]));
        element.removeAttribute(name);
    } 
    catch (const DOM::DOMException &e) {
        raiseOnDOMError(e.code);
    }
}

- (DOMAttr *)getAttributeNode:(NSString *)name
{
    ASSERT(name);

    // Method not reflected in DOM::ElementImpl interface
    Element element(ElementImpl::createInstance([self _elementImpl]));
    Attr result(element.getAttributeNode(name));
    return [DOMAttr _attrWithImpl:static_cast<AttrImpl *>(result.handle())];
}

- (DOMAttr *)setAttributeNode:(DOMAttr *)newAttr
{
    ASSERT(newAttr);

    // Method not reflected in DOM::ElementImpl interface
    try {
        Element element(ElementImpl::createInstance([self _elementImpl]));
        Attr attr(AttrImpl::createInstance([newAttr _attrImpl]));
        Attr result(element.setAttributeNode(attr));
        return [DOMAttr _attrWithImpl:static_cast<AttrImpl *>(result.handle())];
    } 
    catch (const DOM::DOMException &e) {
        raiseOnDOMError(e.code);
        return nil;
    }
}

- (DOMAttr *)removeAttributeNode:(DOMAttr *)oldAttr
{
    ASSERT(oldAttr);

    // Method not reflected in DOM::ElementImpl interface
    try {
        Element element(ElementImpl::createInstance([self _elementImpl]));
        Attr attr(AttrImpl::createInstance([oldAttr _attrImpl]));
        Attr result(element.removeAttributeNode(attr));
        return [DOMAttr _attrWithImpl:static_cast<AttrImpl *>(result.handle())];
    } 
    catch (const DOM::DOMException &e) {
        raiseOnDOMError(e.code);
        return nil;
    }
}

- (DOMNodeList *)getElementsByTagName:(NSString *)name
{
    ASSERT(name);

    return [DOMNodeList _nodeListWithImpl:[self _elementImpl]->getElementsByTagNameNS(0, DOMString(name).implementation())];
}

- (NSString *)getAttributeNS:(NSString *)namespaceURI :(NSString *)localName
{
    ASSERT(namespaceURI);
    ASSERT(localName);

    Element element(ElementImpl::createInstance([self _elementImpl]));
    return element.getAttributeNS(namespaceURI, localName);
}

- (void)setAttributeNS:(NSString *)namespaceURI :(NSString *)qualifiedName :(NSString *)value
{
    ASSERT(namespaceURI);
    ASSERT(qualifiedName);
    ASSERT(value);

    // Method not reflected in DOM::ElementImpl interface
    try {
        Element element(ElementImpl::createInstance([self _elementImpl]));
        element.setAttributeNS(namespaceURI, qualifiedName, value);
    }
    catch (const DOM::DOMException &e) {
        raiseOnDOMError(e.code);
    }
}

- (void)removeAttributeNS:(NSString *)namespaceURI :(NSString *)localName
{
    ASSERT(namespaceURI);
    ASSERT(localName);

    // Method not reflected in DOM::ElementImpl interface
    try {
        Element element(ElementImpl::createInstance([self _elementImpl]));
        element.removeAttributeNS(namespaceURI, localName);
    } 
    catch (const DOM::DOMException &e) {
        raiseOnDOMError(e.code);
    }
}

- (DOMAttr *)getAttributeNodeNS:(NSString *)namespaceURI :(NSString *)localName
{
    ASSERT(namespaceURI);
    ASSERT(localName);

    // Method not reflected in DOM::ElementImpl interface
    Element element(ElementImpl::createInstance([self _elementImpl]));
    Attr result(element.getAttributeNodeNS(namespaceURI, localName));
    return [DOMAttr _attrWithImpl:static_cast<AttrImpl *>(result.handle())];
}

- (DOMAttr *)setAttributeNodeNS:(DOMAttr *)newAttr
{
    ASSERT(newAttr);

    // Method not reflected in DOM::ElementImpl interface
    try {
        Element element(ElementImpl::createInstance([self _elementImpl]));
        Attr attr(AttrImpl::createInstance([newAttr _attrImpl]));
        Attr result(element.setAttributeNodeNS(attr));
        return [DOMAttr _attrWithImpl:static_cast<AttrImpl *>(result.handle())];
    } 
    catch (const DOM::DOMException &e) {
        raiseOnDOMError(e.code);
        return nil;
    }
}

- (DOMNodeList *)getElementsByTagNameNS:(NSString *)namespaceURI :(NSString *)localName
{
    ASSERT(namespaceURI);
    ASSERT(localName);

    return [DOMNodeList _nodeListWithImpl:[self _elementImpl]->getElementsByTagNameNS(DOMString(namespaceURI).implementation(), DOMString(localName).implementation())];
}

- (BOOL)hasAttribute:(NSString *)name
{
    ASSERT(name);

    // Method not reflected in DOM::ElementImpl interface
    Element element(ElementImpl::createInstance([self _elementImpl]));
    return element.hasAttribute(name);
}

- (BOOL)hasAttributeNS:(NSString *)namespaceURI :(NSString *)localName
{
    ASSERT(namespaceURI);
    ASSERT(localName);

    // Method not reflected in DOM::ElementImpl interface
    Element element(ElementImpl::createInstance([self _elementImpl]));
    return element.hasAttributeNS(namespaceURI, localName);
}

@end

@implementation DOMElement (DOMElementCSS)

- (DOMCSSStyleDeclaration *)style
{
    ElementImpl *impl = [self _elementImpl];
    if (impl->isHTMLElement())
        return [DOMCSSStyleDeclaration _styleDeclarationWithImpl:static_cast<HTMLElementImpl *>(impl)->getInlineStyleDecl()];
    return nil;
}

@end

@implementation DOMElement (WebCoreInternal)

+ (DOMElement *)_elementWithImpl:(ElementImpl *)impl
{
    return static_cast<DOMElement *>([DOMNode _nodeWithImpl:impl]);
}

- (ElementImpl *)_elementImpl
{
    return static_cast<ElementImpl *>(reinterpret_cast<NodeImpl *>(_internal));
}

@end

//------------------------------------------------------------------------------------------
// DOMText

@implementation DOMText

- (TextImpl *)_textImpl
{
    return static_cast<TextImpl *>(reinterpret_cast<NodeImpl *>(_internal));
}

- (DOMText *)splitText:(unsigned long)offset
{
    int exceptionCode = 0;
    DOMNode *result = [DOMNode _nodeWithImpl:[self _textImpl]->splitText(offset, exceptionCode)];
    raiseOnDOMError(exceptionCode);
    return static_cast<DOMText *>(result);
}

@end

//------------------------------------------------------------------------------------------
// DOMComment

@implementation DOMComment

@end

//------------------------------------------------------------------------------------------
// DOMCDATASection

@implementation DOMCDATASection

@end

//------------------------------------------------------------------------------------------
// DOMDocumentType

@implementation DOMDocumentType

- (DocumentTypeImpl *)_documentTypeImpl
{
    return static_cast<DocumentTypeImpl *>(reinterpret_cast<NodeImpl *>(_internal));
}

- (NSString *)name
{
    return [self _documentTypeImpl]->publicId();
}

- (DOMNamedNodeMap *)entities
{
    return [DOMNamedNodeMap _namedNodeMapWithImpl:[self _documentTypeImpl]->entities()];
}

- (DOMNamedNodeMap *)notations
{
    return [DOMNamedNodeMap _namedNodeMapWithImpl:[self _documentTypeImpl]->notations()];
}

- (NSString *)publicId
{
    return [self _documentTypeImpl]->publicId();
}

- (NSString *)systemId
{
    return [self _documentTypeImpl]->systemId();
}

- (NSString *)internalSubset
{
    return [self _documentTypeImpl]->internalSubset();
}

@end

//------------------------------------------------------------------------------------------
// DOMNotation

@implementation DOMNotation

- (NotationImpl *)_notationImpl
{
    return static_cast<NotationImpl *>(reinterpret_cast<NodeImpl *>(_internal));
}

- (NSString *)publicId
{
    return [self _notationImpl]->publicId();
}

- (NSString *)systemId
{
    return [self _notationImpl]->systemId();
}

@end

//------------------------------------------------------------------------------------------
// DOMEntity

@implementation DOMEntity

- (EntityImpl *)_entityImpl
{
    return static_cast<EntityImpl *>(reinterpret_cast<NodeImpl *>(_internal));
}

- (NSString *)publicId
{
    return [self _entityImpl]->publicId();
}

- (NSString *)systemId
{
    return [self _entityImpl]->systemId();
}

- (NSString *)notationName
{
    return [self _entityImpl]->notationName();
}

@end

//------------------------------------------------------------------------------------------
// DOMEntityReference

@implementation DOMEntityReference

@end

//------------------------------------------------------------------------------------------
// DOMProcessingInstruction

@implementation DOMProcessingInstruction

- (ProcessingInstructionImpl *)_processingInstructionImpl
{
    return static_cast<ProcessingInstructionImpl *>(reinterpret_cast<NodeImpl *>(_internal));
}

- (NSString *)target
{
    return [self _processingInstructionImpl]->target();
}

- (NSString *)data
{
    return [self _processingInstructionImpl]->data();
}

- (void)setData:(NSString *)data
{
    ASSERT(data);

    int exceptionCode = 0;
    [self _processingInstructionImpl]->setData(data, exceptionCode);
    raiseOnDOMError(exceptionCode);
}

@end

//------------------------------------------------------------------------------------------
// DOMRange

@implementation DOMRange

- (void)dealloc
{
    if (_internal) {
        reinterpret_cast<RangeImpl *>(_internal)->deref();
    }
    [super dealloc];
}

- (DOMNode *)startContainer
{
    int exceptionCode = 0;
    DOMNode *result = [DOMNode _nodeWithImpl:[self _rangeImpl]->startContainer(exceptionCode)];
    raiseOnDOMError(exceptionCode);
    return result;
}

- (long)startOffset
{
    int exceptionCode = 0;
    long result = [self _rangeImpl]->startOffset(exceptionCode);
    raiseOnDOMError(exceptionCode);
    return result;
}

- (DOMNode *)endContainer
{
    int exceptionCode = 0;
    DOMNode *result = [DOMNode _nodeWithImpl:[self _rangeImpl]->endContainer(exceptionCode)];
    raiseOnDOMError(exceptionCode);
    return result;
}

- (long)endOffset
{
    int exceptionCode = 0;
    long result = [self _rangeImpl]->endOffset(exceptionCode);
    raiseOnDOMError(exceptionCode);
    return result;
}

- (BOOL)collapsed
{
    int exceptionCode = 0;
    BOOL result = [self _rangeImpl]->collapsed(exceptionCode);
    raiseOnDOMError(exceptionCode);
    return result;
}

- (DOMNode *)commonAncestorContainer
{
    int exceptionCode = 0;
    DOMNode *result = [DOMNode _nodeWithImpl:[self _rangeImpl]->commonAncestorContainer(exceptionCode)];
    raiseOnDOMError(exceptionCode);
    return result;
}

- (void)setStart:(DOMNode *)refNode :(long)offset
{
    int exceptionCode = 0;
    [self _rangeImpl]->setStart([refNode _nodeImpl], offset, exceptionCode);
    raiseOnDOMError(exceptionCode);
}

- (void)setEnd:(DOMNode *)refNode :(long)offset
{
    int exceptionCode = 0;
    [self _rangeImpl]->setEnd([refNode _nodeImpl], offset, exceptionCode);
    raiseOnDOMError(exceptionCode);
}

- (void)setStartBefore:(DOMNode *)refNode
{
    int exceptionCode = 0;
    [self _rangeImpl]->setStartBefore([refNode _nodeImpl], exceptionCode);
    raiseOnDOMError(exceptionCode);
}

- (void)setStartAfter:(DOMNode *)refNode
{
    int exceptionCode = 0;
    [self _rangeImpl]->setStartAfter([refNode _nodeImpl], exceptionCode);
    raiseOnDOMError(exceptionCode);
}

- (void)setEndBefore:(DOMNode *)refNode
{
    int exceptionCode = 0;
    [self _rangeImpl]->setEndBefore([refNode _nodeImpl], exceptionCode);
    raiseOnDOMError(exceptionCode);
}

- (void)setEndAfter:(DOMNode *)refNode
{
    int exceptionCode = 0;
    [self _rangeImpl]->setEndAfter([refNode _nodeImpl], exceptionCode);
    raiseOnDOMError(exceptionCode);
}

- (void)collapse:(BOOL)toStart
{
    int exceptionCode = 0;
    [self _rangeImpl]->collapse(toStart, exceptionCode);
    raiseOnDOMError(exceptionCode);
}

- (void)selectNode:(DOMNode *)refNode
{
    int exceptionCode = 0;
    [self _rangeImpl]->selectNode([refNode _nodeImpl], exceptionCode);
    raiseOnDOMError(exceptionCode);
}

- (void)selectNodeContents:(DOMNode *)refNode
{
    int exceptionCode = 0;
    [self _rangeImpl]->selectNodeContents([refNode _nodeImpl], exceptionCode);
    raiseOnDOMError(exceptionCode);
}

- (short)compareBoundaryPoints:(unsigned short)how :(DOMRange *)sourceRange
{
    int exceptionCode = 0;
    short result = [self _rangeImpl]->compareBoundaryPoints(static_cast<Range::CompareHow>(how), [sourceRange _rangeImpl], exceptionCode);
    raiseOnDOMError(exceptionCode);
    return result;
}

- (void)deleteContents
{
    int exceptionCode = 0;
    [self _rangeImpl]->deleteContents(exceptionCode);
    raiseOnDOMError(exceptionCode);
}

- (DOMDocumentFragment *)extractContents
{
    int exceptionCode = 0;
    DOMDocumentFragment *result = [DOMDocumentFragment _documentFragmentWithImpl:[self _rangeImpl]->extractContents(exceptionCode)];
    raiseOnDOMError(exceptionCode);
    return result;
}

- (DOMDocumentFragment *)cloneContents
{
    int exceptionCode = 0;
    DOMDocumentFragment *result = [DOMDocumentFragment _documentFragmentWithImpl:[self _rangeImpl]->cloneContents(exceptionCode)];
    raiseOnDOMError(exceptionCode);
    return result;
}

- (void)insertNode:(DOMNode *)newNode
{
    int exceptionCode = 0;
    [self _rangeImpl]->insertNode([newNode _nodeImpl], exceptionCode);
    raiseOnDOMError(exceptionCode);
}

- (void)surroundContents:(DOMNode *)newParent
{
    int exceptionCode = 0;
    [self _rangeImpl]->surroundContents([newParent _nodeImpl], exceptionCode);
    raiseOnDOMError(exceptionCode);
}

- (DOMRange *)cloneRange
{
    int exceptionCode = 0;
    DOMRange *result = [DOMRange _rangeWithImpl:[self _rangeImpl]->cloneRange(exceptionCode)];
    raiseOnDOMError(exceptionCode);
    return result;
}

- (NSString *)toString
{
    int exceptionCode = 0;
    NSString *result = [self _rangeImpl]->toString(exceptionCode);
    raiseOnDOMError(exceptionCode);
    return result;
}

- (void)detach
{
    int exceptionCode = 0;
    [self _rangeImpl]->detach(exceptionCode);
    raiseOnDOMError(exceptionCode);
}

@end

@implementation DOMRange (WebCoreInternal)

- (id)_initWithRangeImpl:(RangeImpl *)impl
{
    ASSERT(impl);

    [super _init];
    _internal = reinterpret_cast<DOMObjectInternal *>(impl);
    impl->ref();
    setDOMWrapperForImpl(self, impl);
    return self;
}

+ (DOMRange *)_rangeWithImpl:(RangeImpl *)impl
{
    if (!impl)
        return nil;
    
    id cachedInstance;
    cachedInstance = getDOMWrapperForImpl(impl);
    if (cachedInstance)
        return [[cachedInstance retain] autorelease];
    
    return [[[self alloc] _initWithRangeImpl:impl] autorelease];
}

- (RangeImpl *)_rangeImpl
{
    return reinterpret_cast<RangeImpl *>(_internal);
}

@end

//------------------------------------------------------------------------------------------
