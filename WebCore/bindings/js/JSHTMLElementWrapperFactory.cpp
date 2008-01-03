/*
 *  Copyright (C) 2006, 2007 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "JSHTMLElementWrapperFactory.h"

#include "HTMLAnchorElement.h"
#include "HTMLAppletElement.h"
#include "HTMLAreaElement.h"
#include "HTMLAudioElement.h"
#include "HTMLBRElement.h"
#include "HTMLBaseElement.h"
#include "HTMLBaseFontElement.h"
#include "HTMLBlockquoteElement.h"
#include "HTMLBodyElement.h"
#include "HTMLButtonElement.h"
#include "HTMLCanvasElement.h"
#include "HTMLDListElement.h"
#include "HTMLDirectoryElement.h"
#include "HTMLDivElement.h"
#include "HTMLEmbedElement.h"
#include "HTMLFieldSetElement.h"
#include "HTMLFontElement.h"
#include "HTMLFormElement.h"
#include "HTMLFrameElement.h"
#include "HTMLFrameSetElement.h"
#include "HTMLHRElement.h"
#include "HTMLHeadElement.h"
#include "HTMLHeadingElement.h"
#include "HTMLHtmlElement.h"
#include "HTMLIFrameElement.h"
#include "HTMLImageElement.h"
#include "HTMLInputElement.h"
#include "HTMLIsIndexElement.h"
#include "HTMLLIElement.h"
#include "HTMLLabelElement.h"
#include "HTMLLegendElement.h"
#include "HTMLLinkElement.h"
#include "HTMLMapElement.h"
#include "HTMLMarqueeElement.h"
#include "HTMLMenuElement.h"
#include "HTMLMetaElement.h"
#include "HTMLModElement.h"
#include "HTMLOListElement.h"
#include "HTMLObjectElement.h"
#include "HTMLOptGroupElement.h"
#include "HTMLOptionElement.h"
#include "HTMLParagraphElement.h"
#include "HTMLParamElement.h"
#include "HTMLPreElement.h"
#include "HTMLQuoteElement.h"
#include "HTMLScriptElement.h"
#include "HTMLSelectElement.h"
#include "HTMLSourceElement.h"
#include "HTMLStyleElement.h"
#include "HTMLTableCaptionElement.h"
#include "HTMLTableCellElement.h"
#include "HTMLTableColElement.h"
#include "HTMLTableElement.h"
#include "HTMLTableRowElement.h"
#include "HTMLTableSectionElement.h"
#include "HTMLTextAreaElement.h"
#include "HTMLTitleElement.h"
#include "HTMLUListElement.h"
#include "HTMLVideoElement.h"

#include "HTMLNames.h"

#include "JSHTMLAnchorElement.h"
#include "JSHTMLAppletElement.h"
#include "JSHTMLAreaElement.h"
#include "JSHTMLBRElement.h"
#include "JSHTMLBaseElement.h"
#include "JSHTMLBaseFontElement.h"
#include "JSHTMLBlockquoteElement.h"
#include "JSHTMLBodyElement.h"
#include "JSHTMLButtonElement.h"
#include "JSHTMLCanvasElement.h"
#include "JSHTMLDListElement.h"
#include "JSHTMLDirectoryElement.h"
#include "JSHTMLDivElement.h"
#include "JSHTMLEmbedElement.h"
#include "JSHTMLFieldSetElement.h"
#include "JSHTMLFontElement.h"
#include "JSHTMLFormElement.h"
#include "JSHTMLFrameElement.h"
#include "JSHTMLFrameSetElement.h"
#include "JSHTMLHRElement.h"
#include "JSHTMLHeadElement.h"
#include "JSHTMLHeadingElement.h"
#include "JSHTMLHtmlElement.h"
#include "JSHTMLIFrameElement.h"
#include "JSHTMLImageElement.h"
#include "JSHTMLInputElement.h"
#include "JSHTMLIsIndexElement.h"
#include "JSHTMLLIElement.h"
#include "JSHTMLLabelElement.h"
#include "JSHTMLLegendElement.h"
#include "JSHTMLLinkElement.h"
#include "JSHTMLMapElement.h"
#include "JSHTMLMarqueeElement.h"
#include "JSHTMLMenuElement.h"
#include "JSHTMLMetaElement.h"
#include "JSHTMLModElement.h"
#include "JSHTMLOListElement.h"
#include "JSHTMLObjectElement.h"
#include "JSHTMLOptGroupElement.h"
#include "JSHTMLOptionElement.h"
#include "JSHTMLParagraphElement.h"
#include "JSHTMLParamElement.h"
#include "JSHTMLPreElement.h"
#include "JSHTMLQuoteElement.h"
#include "JSHTMLScriptElement.h"
#include "JSHTMLSelectElement.h"
#include "JSHTMLStyleElement.h"
#include "JSHTMLTableCaptionElement.h"
#include "JSHTMLTableCellElement.h"
#include "JSHTMLTableColElement.h"
#include "JSHTMLTableElement.h"
#include "JSHTMLTableRowElement.h"
#include "JSHTMLTableSectionElement.h"
#include "JSHTMLTextAreaElement.h"
#include "JSHTMLTitleElement.h"
#include "JSHTMLUListElement.h"

#if ENABLE(VIDEO)
#include "JSHTMLAudioElement.h"
#include "JSHTMLSourceElement.h"
#include "JSHTMLVideoElement.h"
#endif

#include "kjs_html.h"

using namespace KJS;

// FIXME: Eventually this file should be autogenerated, just like HTMLNames, HTMLElementFactory, etc.

namespace WebCore {

using namespace HTMLNames;

typedef JSNode* (*CreateHTMLElementWrapperFunction)(ExecState*, PassRefPtr<HTMLElement>);

#define FOR_EACH_TAG(macro) \
    macro(a, Anchor) \
    macro(applet, Applet) \
    macro(area, Area) \
    macro(base, Base) \
    macro(basefont, BaseFont) \
    macro(blockquote, Blockquote) \
    macro(body, Body) \
    macro(br, BR) \
    macro(button, Button) \
    macro(canvas, Canvas) \
    macro(caption, TableCaption) \
    macro(col, TableCol) \
    macro(del, Mod) \
    macro(dir, Directory) \
    macro(div, Div) \
    macro(dl, DList) \
    macro(embed, Embed) \
    macro(fieldset, FieldSet) \
    macro(font, Font) \
    macro(form, Form) \
    macro(frame, Frame) \
    macro(frameset, FrameSet) \
    macro(h1, Heading) \
    macro(head, Head) \
    macro(hr, HR) \
    macro(html, Html) \
    macro(iframe, IFrame) \
    macro(img, Image) \
    macro(input, Input) \
    macro(isindex, IsIndex) \
    macro(label, Label) \
    macro(legend, Legend) \
    macro(li, LI) \
    macro(link, Link) \
    macro(map, Map) \
    macro(marquee, Marquee) \
    macro(menu, Menu) \
    macro(meta, Meta) \
    macro(object, Object) \
    macro(ol, OList) \
    macro(optgroup, OptGroup) \
    macro(option, Option) \
    macro(p, Paragraph) \
    macro(param, Param) \
    macro(pre, Pre) \
    macro(q, Quote) \
    macro(script, Script) \
    macro(select, Select) \
    macro(style, Style) \
    macro(table, Table) \
    macro(tbody, TableSection) \
    macro(td, TableCell) \
    macro(textarea, TextArea) \
    macro(tr, TableRow) \
    macro(title, Title) \
    macro(ul, UList) \
    // end of macro

#define FOR_EACH_VIDEO_TAG(macro) \
    macro(audio, Audio) \
    macro(source, Source) \
    macro(video, Video) \
    // end of macro

#define CREATE_WRAPPER_FUNCTION(tag, name) \
static JSNode* create##name##Wrapper(ExecState* exec, PassRefPtr<HTMLElement> element) \
{ \
    return new JSHTML##name##Element(JSHTML##name##ElementPrototype::self(exec), static_cast<HTML##name##Element*>(element.get())); \
}
FOR_EACH_TAG(CREATE_WRAPPER_FUNCTION)
#if ENABLE(VIDEO)
    FOR_EACH_VIDEO_TAG(CREATE_WRAPPER_FUNCTION)
#endif
#undef CREATE_WRAPPER_FUNCTION

JSNode* createJSHTMLWrapper(ExecState* exec, PassRefPtr<HTMLElement> element)
{
    static HashMap<AtomicStringImpl*, CreateHTMLElementWrapperFunction> map;
    if (map.isEmpty()) {
#define ADD_TO_HASH_MAP(tag, name) map.set(tag##Tag.localName().impl(), create##name##Wrapper);
FOR_EACH_TAG(ADD_TO_HASH_MAP)
#if ENABLE(VIDEO)
FOR_EACH_VIDEO_TAG(ADD_TO_HASH_MAP)
#endif
#undef ADD_TO_HASH_MAP
        map.set(colgroupTag.localName().impl(), createTableColWrapper);
        map.set(h2Tag.localName().impl(), createHeadingWrapper);
        map.set(h3Tag.localName().impl(), createHeadingWrapper);
        map.set(h4Tag.localName().impl(), createHeadingWrapper);
        map.set(h5Tag.localName().impl(), createHeadingWrapper);
        map.set(h6Tag.localName().impl(), createHeadingWrapper);
        map.set(imageTag.localName().impl(), createImageWrapper);
        map.set(insTag.localName().impl(), createModWrapper);
        map.set(keygenTag.localName().impl(), createSelectWrapper);
        map.set(listingTag.localName().impl(), createPreWrapper);
        map.set(tfootTag.localName().impl(), createTableSectionWrapper);
        map.set(thTag.localName().impl(), createTableCellWrapper);
        map.set(theadTag.localName().impl(), createTableSectionWrapper);
        map.set(xmpTag.localName().impl(), createPreWrapper);
    }
    CreateHTMLElementWrapperFunction createWrapperFunction = map.get(element->localName().impl());
    if (createWrapperFunction)
        return createWrapperFunction(exec, element);
    return new JSHTMLElement(JSHTMLElementPrototype::self(exec), element.get());
}

} // namespace WebCore
