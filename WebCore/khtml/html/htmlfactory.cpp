/*
 * This file is part of the HTML DOM implementation for KDE.
 *
 * Copyright (C) 2005 Apple Computer, Inc.
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
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"
#include "htmlfactory.h"

#include "html_baseimpl.h"
#include "html_blockimpl.h"
#include "html_canvasimpl.h"
#include "html_documentimpl.h"
#include "html_headimpl.h"
#include "html_imageimpl.h"
#include "html_listimpl.h"
#include "html_tableimpl.h"
#include "html_objectimpl.h"

#include "HTMLCollectionImpl.h"
#include "HTMLFormElementImpl.h"
#include "HTMLInputElementImpl.h"
#include "HTMLIsIndexElementImpl.h"
#include "HTMLFieldSetElementImpl.h"
#include "HTMLLabelElementImpl.h"
#include "HTMLLegendElementImpl.h"
#include "HTMLButtonElementImpl.h"
#include "HTMLOptionElementImpl.h"
#include "HTMLOptGroupElementImpl.h"
#include "HTMLSelectElementImpl.h"
#include "HTMLTextAreaElementImpl.h"
#include "HTMLBaseFontElementImpl.h"

#include <kxmlcore/HashMap.h>

namespace DOM
{

using namespace HTMLNames;

typedef HTMLElementImpl* (*ConstructorFunc)(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser);
typedef HashMap<AtomicStringImpl*, ConstructorFunc> FunctionMap;
static FunctionMap* gFunctionMap;

HTMLElementImpl* htmlConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLHtmlElementImpl(docPtr);
}

HTMLElementImpl* headConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLHeadElementImpl(docPtr);
}

HTMLElementImpl* bodyConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLBodyElementImpl(docPtr);
}

HTMLElementImpl* baseConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLBaseElementImpl(docPtr);
}

HTMLElementImpl* linkConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLLinkElementImpl(docPtr);
}

HTMLElementImpl* metaConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLMetaElementImpl(docPtr);
}

HTMLElementImpl* styleConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLStyleElementImpl(docPtr);
}

HTMLElementImpl* titleConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLTitleElementImpl(docPtr);
}

HTMLElementImpl* frameConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLFrameElementImpl(docPtr);
}

HTMLElementImpl* framesetConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLFrameSetElementImpl(docPtr);
}

HTMLElementImpl* iframeConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLIFrameElementImpl(docPtr);
}

HTMLElementImpl* formConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLFormElementImpl(docPtr);
}

HTMLElementImpl* buttonConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLButtonElementImpl(docPtr, form);
}

HTMLElementImpl* inputConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLInputElementImpl(docPtr, form);
}

HTMLElementImpl* isindexConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLIsIndexElementImpl(docPtr, form);
}

HTMLElementImpl* fieldsetConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLFieldSetElementImpl(docPtr, form);
}

HTMLElementImpl* labelConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLLabelElementImpl(docPtr);
}

HTMLElementImpl* legendConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLLegendElementImpl(docPtr, form);
}

HTMLElementImpl* optgroupConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLOptGroupElementImpl(docPtr, form);
}

HTMLElementImpl* optionConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLOptionElementImpl(docPtr, form);
}

HTMLElementImpl* selectConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLSelectElementImpl(docPtr, form);
}

HTMLElementImpl* textareaConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLTextAreaElementImpl(docPtr, form);
}

HTMLElementImpl* dlConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLDListElementImpl(docPtr);
}

HTMLElementImpl* ulConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLUListElementImpl(docPtr);
}

HTMLElementImpl* olConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLOListElementImpl(docPtr);
}

HTMLElementImpl* dirConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLDirectoryElementImpl(docPtr);
}

HTMLElementImpl* menuConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLMenuElementImpl(docPtr);
}

HTMLElementImpl* liConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLLIElementImpl(docPtr);
}

HTMLElementImpl* blockquoteConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLBlockquoteElementImpl(docPtr);
}

HTMLElementImpl* divConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLDivElementImpl(docPtr);
}

HTMLElementImpl* headingConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLHeadingElementImpl(QualifiedName(nullAtom, tagName, xhtmlNamespaceURI), docPtr);
}

HTMLElementImpl* hrConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLHRElementImpl(docPtr);
}

HTMLElementImpl* paragraphConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLParagraphElementImpl(docPtr);
}

HTMLElementImpl* preConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLPreElementImpl(QualifiedName(nullAtom, tagName, xhtmlNamespaceURI), docPtr);
}

HTMLElementImpl* basefontConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLBaseFontElementImpl(docPtr);
}

HTMLElementImpl* fontConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLFontElementImpl(docPtr);
}

HTMLElementImpl* modConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLModElementImpl(QualifiedName(nullAtom, tagName, xhtmlNamespaceURI), docPtr);
}

HTMLElementImpl* anchorConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLAnchorElementImpl(docPtr);
}

HTMLElementImpl* imageConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLImageElementImpl(docPtr, form);
}

HTMLElementImpl* mapConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLMapElementImpl(docPtr);
}

HTMLElementImpl* areaConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLAreaElementImpl(docPtr);
}

HTMLElementImpl* canvasConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLCanvasElementImpl(docPtr);
}

HTMLElementImpl* appletConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLAppletElementImpl(docPtr);
}

HTMLElementImpl* embedConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLEmbedElementImpl(docPtr);
}

HTMLElementImpl* objectConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    HTMLObjectElementImpl * object = new HTMLObjectElementImpl(docPtr);
    object->setComplete(!createdByParser);
    return object;
}

HTMLElementImpl* paramConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLParamElementImpl(docPtr);
}

HTMLElementImpl* scriptConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    HTMLScriptElementImpl* script = new HTMLScriptElementImpl(docPtr);
    script->setCreatedByParser(createdByParser);
    return script;
}

HTMLElementImpl* tableConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLTableElementImpl(docPtr);
}

HTMLElementImpl* tableCaptionConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLTableCaptionElementImpl(docPtr);
}

HTMLElementImpl* tableColConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLTableColElementImpl(QualifiedName(nullAtom, tagName, xhtmlNamespaceURI), docPtr);
}

HTMLElementImpl* tableRowConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLTableRowElementImpl(docPtr);
}

HTMLElementImpl* tableCellConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLTableCellElementImpl(QualifiedName(nullAtom, tagName, xhtmlNamespaceURI), docPtr);
}

HTMLElementImpl* tableSectionConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLTableSectionElementImpl(QualifiedName(nullAtom, tagName, xhtmlNamespaceURI), docPtr, false);
}

HTMLElementImpl* brConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLBRElementImpl(docPtr);
}

HTMLElementImpl* quoteConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLQuoteElementImpl(docPtr);
}

HTMLElementImpl* marqueeConstructor(const AtomicString& tagName, DocumentImpl* docPtr, HTMLFormElementImpl* form, bool createdByParser)
{
    return new HTMLMarqueeElementImpl(docPtr);
}

HTMLElementImpl* HTMLElementFactory::createHTMLElement(const AtomicString& tagName, DocumentImpl* doc, HTMLFormElementImpl* form, bool createdByParser)
{
    if (!doc)
        return 0; // Don't allow elements to ever be made without having a doc.

    DocumentImpl* docPtr = doc;
    if (!gFunctionMap) {
        // Create the table.
        gFunctionMap = new FunctionMap;
        
        // Populate it with constructor functions.
        gFunctionMap->set(htmlTag.localName().impl(), htmlConstructor);
        gFunctionMap->set(headTag.localName().impl(), headConstructor);
        gFunctionMap->set(bodyTag.localName().impl(), bodyConstructor);
        gFunctionMap->set(baseTag.localName().impl(), baseConstructor);
        gFunctionMap->set(linkTag.localName().impl(), linkConstructor);
        gFunctionMap->set(metaTag.localName().impl(), metaConstructor);
        gFunctionMap->set(styleTag.localName().impl(), styleConstructor);
        gFunctionMap->set(titleTag.localName().impl(), titleConstructor);
        gFunctionMap->set(frameTag.localName().impl(), frameConstructor);
        gFunctionMap->set(framesetTag.localName().impl(), framesetConstructor);
        gFunctionMap->set(iframeTag.localName().impl(), iframeConstructor);
        gFunctionMap->set(formTag.localName().impl(), formConstructor);
        gFunctionMap->set(buttonTag.localName().impl(), buttonConstructor);
        gFunctionMap->set(inputTag.localName().impl(), inputConstructor);
        gFunctionMap->set(isindexTag.localName().impl(), isindexConstructor);
        gFunctionMap->set(fieldsetTag.localName().impl(), fieldsetConstructor);
        gFunctionMap->set(labelTag.localName().impl(), labelConstructor);
        gFunctionMap->set(legendTag.localName().impl(), legendConstructor);
        gFunctionMap->set(optgroupTag.localName().impl(), optgroupConstructor);
        gFunctionMap->set(optionTag.localName().impl(), optionConstructor);
        gFunctionMap->set(selectTag.localName().impl(), selectConstructor);
        gFunctionMap->set(textareaTag.localName().impl(), textareaConstructor);
        gFunctionMap->set(dlTag.localName().impl(), dlConstructor);
        gFunctionMap->set(olTag.localName().impl(), olConstructor);
        gFunctionMap->set(ulTag.localName().impl(), ulConstructor);
        gFunctionMap->set(dirTag.localName().impl(), dirConstructor);
        gFunctionMap->set(menuTag.localName().impl(), menuConstructor);
        gFunctionMap->set(liTag.localName().impl(), liConstructor);
        gFunctionMap->set(blockquoteTag.localName().impl(), blockquoteConstructor);
        gFunctionMap->set(divTag.localName().impl(), divConstructor);
        gFunctionMap->set(h1Tag.localName().impl(), headingConstructor);
        gFunctionMap->set(h2Tag.localName().impl(), headingConstructor);
        gFunctionMap->set(h3Tag.localName().impl(), headingConstructor);
        gFunctionMap->set(h4Tag.localName().impl(), headingConstructor);
        gFunctionMap->set(h5Tag.localName().impl(), headingConstructor);
        gFunctionMap->set(h6Tag.localName().impl(), headingConstructor);
        gFunctionMap->set(hrTag.localName().impl(), hrConstructor);
        gFunctionMap->set(pTag.localName().impl(), paragraphConstructor);
        gFunctionMap->set(preTag.localName().impl(), preConstructor);
        gFunctionMap->set(xmpTag.localName().impl(), preConstructor);
        gFunctionMap->set(basefontTag.localName().impl(), basefontConstructor);
        gFunctionMap->set(fontTag.localName().impl(), fontConstructor);
        gFunctionMap->set(delTag.localName().impl(), modConstructor);
        gFunctionMap->set(insTag.localName().impl(), modConstructor);
        gFunctionMap->set(aTag.localName().impl(), anchorConstructor);
        gFunctionMap->set(imageTag.localName().impl(), imageConstructor);
        gFunctionMap->set(imgTag.localName().impl(), imageConstructor);
        gFunctionMap->set(mapTag.localName().impl(), mapConstructor);
        gFunctionMap->set(areaTag.localName().impl(), areaConstructor);
        gFunctionMap->set(canvasTag.localName().impl(), canvasConstructor);
        gFunctionMap->set(appletTag.localName().impl(), appletConstructor);
        gFunctionMap->set(embedTag.localName().impl(), embedConstructor);
        gFunctionMap->set(objectTag.localName().impl(), objectConstructor);
        gFunctionMap->set(paramTag.localName().impl(), paramConstructor);
        gFunctionMap->set(scriptTag.localName().impl(), scriptConstructor);
        gFunctionMap->set(tableTag.localName().impl(), tableConstructor);
        gFunctionMap->set(captionTag.localName().impl(), tableCaptionConstructor);
        gFunctionMap->set(colgroupTag.localName().impl(), tableColConstructor);
        gFunctionMap->set(colTag.localName().impl(), tableColConstructor);
        gFunctionMap->set(trTag.localName().impl(), tableRowConstructor);
        gFunctionMap->set(tdTag.localName().impl(), tableCellConstructor);
        gFunctionMap->set(thTag.localName().impl(), tableCellConstructor);
        gFunctionMap->set(theadTag.localName().impl(), tableSectionConstructor);
        gFunctionMap->set(tbodyTag.localName().impl(), tableSectionConstructor);
        gFunctionMap->set(tfootTag.localName().impl(), tableSectionConstructor);
        gFunctionMap->set(brTag.localName().impl(), brConstructor);
        gFunctionMap->set(qTag.localName().impl(), quoteConstructor);
        gFunctionMap->set(marqueeTag.localName().impl(), marqueeConstructor);
    }
    
    ConstructorFunc func = gFunctionMap->get(tagName.impl());
    if (func)
        return (func)(tagName, docPtr, form, createdByParser);
 
    // elements with no special representation in the DOM
    return new HTMLElementImpl(QualifiedName(nullAtom, tagName, xhtmlNamespaceURI), docPtr);
}

}

