/*
    Copyright (C) 2004, 2005 Nikolas Zimmermann <wildfox@kde.org>
                  2004, 2005 Rob Buis <buis@kde.org>

    Based on khtml code by:
    Copyright (C) 1999 Lars Knoll (knoll@kde.org)
              (C) 1999 Antti Koivisto (koivisto@kde.org)
              (C) 2001 Dirk Mueller (mueller@kde.org)
              (C) 2002-2003 Apple Computer, Inc.

    This file is part of the KDE project

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include <kstaticdeleter.h>

#include "kdom.h"
#include "kdomls.h"
#include <kdom/Helper.h>
#include "Namespace.h"
#include "KDOMCache.h"
#include "kdomevents.h"
#include "RenderStyle.h"
#include "ElementImpl.h"
#include "LSInputImpl.h"
#include "LSOutputImpl.h"
#include "LSParserImpl.h"
#include "DocumentImpl.h"
#include "CDFInterface.h"
#include "MediaListImpl.h"
#include "DocumentTypeImpl.h"
#include "kdom/css/CSSStyleSelector.h"
#include "CSSStyleSheetImpl.h"
#include "LSSerializerImpl.h"
#include "DOMImplementationImpl.h"


using namespace KDOM;

static KStaticDeleter<DOMImplementationImpl> instanceDeleter;
DOMImplementationImpl *DOMImplementationImpl::s_instance = 0;

DOMImplementationImpl::DOMImplementationImpl()
{
    m_cdfInterface = 0;

    Cache::init();
}

DOMImplementationImpl::~DOMImplementationImpl()
{
    // clean up static data
    CSSStyleSelector::clear();
    RenderStyle::cleanup();

    delete m_cdfInterface;
}

DOMImplementationImpl *DOMImplementationImpl::self()
{
    if(!s_instance)
        s_instance = instanceDeleter.setObject(s_instance, new DOMImplementationImpl());

    return s_instance;
}

CDFInterface *DOMImplementationImpl::cdfInterface() const
{
    if(!m_cdfInterface)
        m_cdfInterface = createCDFInterface();
    
    return m_cdfInterface;
}

bool DOMImplementationImpl::hasFeature(DOMStringImpl *feature, DOMStringImpl *_version) const
{
    DOMString upFeature = DOMString(feature).upper();
    DOMString version(_version);

    if(upFeature[0] == '+')
        upFeature.remove(0, 1);

    if((upFeature == "XML" || upFeature == "CORE" || upFeature == "EVENTS" || upFeature == "UIEVENTS" ||
        upFeature == "MOUSEEVENTS" || upFeature == "MUTATIONEVENTS" || upFeature == "LS-ASYNC" ||
        upFeature == "LS" || upFeature == "XPATH" || upFeature == "RANGE" || upFeature == "TRAVERSAL") &&
       (version == "1.0" || version == "2.0" || version == "3.0" || version.isEmpty()))
    {
        return true;
    }

    return false;
}

DOMObjectImpl *DOMImplementationImpl::getFeature(DOMStringImpl *, DOMStringImpl *) const
{
    // FIXME!
    return 0;
}

DocumentTypeImpl *DOMImplementationImpl::createDocumentType(DOMStringImpl *qualifiedName,
                                                            DOMStringImpl *publicId,
                                                            DOMStringImpl *systemId) const
{
    // INVALID_CHARACTER_ERR: Raised if the specified qualified name contains an illegal character.
    if((!qualifiedName || qualifiedName->isEmpty()) || !Helper::ValidateAttributeName(qualifiedName))
        throw new DOMExceptionImpl(INVALID_CHARACTER_ERR);

    // NAMESPACE_ERR: Raised if no qualifiedName supplied (not mentioned in the spec!)
    // FIXME: This is never reached! What now?
    if(!qualifiedName || qualifiedName->isEmpty())
        throw new DOMExceptionImpl(NAMESPACE_ERR);

    // NAMESPACE_ERR: Raised if the qualifiedName is malformed.
    Helper::CheckMalformedQualifiedName(qualifiedName);

    return new DocumentTypeImpl(new DocumentPtr(), qualifiedName, publicId, systemId);
}

DocumentImpl *DOMImplementationImpl::createDocument(DOMStringImpl *namespaceURI,
                                                    DOMStringImpl *qualifiedName,
                                                    DocumentTypeImpl *docType,
                                                    bool createDocElement,
                                                    KDOMView *) const
{
    if(namespaceURI)
        namespaceURI->ref();
    if(qualifiedName)
        qualifiedName->ref();

    int temp;
    Helper::CheckQualifiedName(qualifiedName, namespaceURI, temp, 
                               true /*nameCanBeNull*/,
                               true /*nameCanBeEmpty, see #61650*/);

    // WRONG_DOCUMENT_ERR: Raised if docType has already been used with a different
    //                       document or was created from a different implementation.
    if(docType != 0 && docType->ownerDocument() != 0)
        throw new DOMExceptionImpl(WRONG_DOCUMENT_ERR);

    DocumentImpl *doc = new DocumentImpl(const_cast<DOMImplementationImpl *>(this), 0 /* default: no view! */);
    if(docType)
        doc->setDocType(docType);

    /* isEmpty instead if isNull, for the reason given in #61650 */
    if(qualifiedName && !qualifiedName->isEmpty() && namespaceURI && !namespaceURI->isEmpty() && createDocElement)
        doc->appendChild(doc->createElementNS(namespaceURI, qualifiedName));

    if(namespaceURI)
        namespaceURI->deref();
    if(qualifiedName)
        qualifiedName->deref();

    return doc;
}

CSSStyleSheetImpl *DOMImplementationImpl::createCSSStyleSheet(DOMStringImpl *title, DOMStringImpl *media) const
{
    // TODO : check whether media is valid
    CSSStyleSheetImpl *parent = 0;
    CSSStyleSheetImpl *sheet = new CSSStyleSheetImpl(parent, 0);
    sheet->setTitle(title);
    sheet->setMedia(new MediaListImpl(sheet, media));
    return sheet;
}

LSParserImpl *DOMImplementationImpl::createLSParser(unsigned short mode, DOMStringImpl *schemaTypeImpl) const
{
    DOMString schemaType(schemaTypeImpl);

    // NOT_SUPPORTED_ERR: Raised if the requested mode or schema type is not supported.
    if((mode == 0 || mode > MODE_ASYNCHRONOUS) ||
        !(schemaType == NS_SCHEMATYPE_DTD || schemaType == NS_SCHEMATYPE_WXS || schemaType.isEmpty()))
        throw new DOMExceptionImpl(NOT_SUPPORTED_ERR);

    LSParserImpl *ret = new LSParserImpl();
    ret->setASync(mode == MODE_ASYNCHRONOUS);
    return ret;
}

LSInputImpl *DOMImplementationImpl::createLSInput() const
{
    return new LSInputImpl();
}

LSOutputImpl *DOMImplementationImpl::createLSOutput() const
{
    return new LSOutputImpl();
}

LSSerializerImpl *DOMImplementationImpl::createLSSerializer() const
{
    return new LSSerializerImpl();
}

DocumentTypeImpl *DOMImplementationImpl::defaultDocumentType() const
{
    return createDocumentType(new DOMStringImpl("name"),
                              new DOMStringImpl("public"),
                              new DOMStringImpl("system"));
}

int DOMImplementationImpl::typeToId(DOMStringImpl *typeImpl)
{
    DOMString type(typeImpl);

    if(type == "DOMFocusIn") return DOMFOCUSIN_EVENT;
    else if(type == "DOMFocusOut") return DOMFOCUSOUT_EVENT;
    else if(type == "DOMActivate") return DOMACTIVATE_EVENT;
    else if(type == "click") return CLICK_EVENT;
    else if(type == "mousedown") return MOUSEDOWN_EVENT;
    else if(type == "mouseup") return MOUSEUP_EVENT;
    else if(type == "mouseover") return MOUSEOVER_EVENT;
    else if(type == "mousemove") return MOUSEMOVE_EVENT;
    else if(type == "mouseout") return MOUSEOUT_EVENT;
    else if(type == "keydown") return KEYDOWN_EVENT;
    else if(type == "keyup") return KEYUP_EVENT;
    else if(type == "DOMSubtreeModified") return DOMSUBTREEMODIFIED_EVENT;
    else if(type == "DOMNodeInserted") return DOMNODEINSERTED_EVENT;
    else if(type == "DOMNodeRemoved") return DOMNODEREMOVED_EVENT;
    else if(type == "DOMNodeRemovedFromDocument") return DOMNODEREMOVEDFROMDOCUMENT_EVENT;
    else if(type == "DOMNodeInsertedIntoDocument") return DOMNODEINSERTEDINTODOCUMENT_EVENT;
    else if(type == "DOMAttrModified") return DOMATTRMODIFIED_EVENT;
    else if(type == "DOMCharacterDataModified")    return DOMCHARACTERDATAMODIFIED_EVENT;

    return UNKNOWN_EVENT;
}

DOMStringImpl *DOMImplementationImpl::idToType(int id)
{
    QString ret;
    switch(id)
    {
        case DOMFOCUSIN_EVENT: ret = "DOMFocusIn";
        case DOMFOCUSOUT_EVENT: ret = "DOMFocusOut";
        case DOMACTIVATE_EVENT: ret = "DOMActivate";
        case CLICK_EVENT: ret = "click";
        case MOUSEDOWN_EVENT: ret = "mousedown";
        case MOUSEUP_EVENT: ret = "mouseup";
        case MOUSEOVER_EVENT: ret = "mouseover";
        case MOUSEMOVE_EVENT: ret = "mousemove";
        case MOUSEOUT_EVENT: ret = "mouseout";
        case KEYDOWN_EVENT: ret = "keydown";
        case KEYUP_EVENT: ret = "keyup";
        case DOMSUBTREEMODIFIED_EVENT: ret = "DOMSubtreeModified";
        case DOMNODEINSERTED_EVENT: ret = "DOMNodeInserted";
        case DOMNODEREMOVED_EVENT: ret = "DOMNodeRemoved";
        case DOMNODEREMOVEDFROMDOCUMENT_EVENT: ret = "DOMNodeRemovedFromDocument";
        case DOMNODEINSERTEDINTODOCUMENT_EVENT: ret = "DOMNodeInsertedIntoDocument";
        case DOMATTRMODIFIED_EVENT: ret = "DOMAttrModified";
        case DOMCHARACTERDATAMODIFIED_EVENT: ret = "DOMCharacterDataModified";
        default: ret = QString::null;
    }

    return new DOMStringImpl(ret);
}

CDFInterface *DOMImplementationImpl::createCDFInterface() const
{
    return new CDFInterface();
}

// vim:ts=4:noet
