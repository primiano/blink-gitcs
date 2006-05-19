// -*- c-basic-offset: 2 -*-
/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 1999 Harri Porten (porten@kde.org)
 *  Copyright (C) 2004, 2006 Apple Computer, Inc.
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef KJS_HTML_H_
#define KJS_HTML_H_

#include "JSDocument.h"
#include "JSElement.h"
#include "JSHTMLElement.h"

namespace WebCore {
    class HTMLCollection;
    class HTMLDocument;
    class HTMLElement;
    class HTMLSelectElement;
    class HTMLTableCaptionElement;
    class HTMLTableSectionElement;
}

namespace KJS {

  class JSAbstractEventListener;

  KJS_DEFINE_PROTOTYPE_WITH_PROTOTYPE(JSHTMLDocumentProto, WebCore::JSDocumentProto)

  class JSHTMLDocument : public WebCore::JSDocument {
  public:
    JSHTMLDocument(ExecState *exec, WebCore::HTMLDocument *d);
    virtual bool getOwnPropertySlot(ExecState *, const Identifier&, PropertySlot&);
    JSValue *getValueProperty(ExecState *exec, int token) const;
    virtual void put(ExecState *exec, const Identifier &propertyName, JSValue *value, int attr = None);
    void putValueProperty(ExecState *exec, int token, JSValue *value, int /*attr*/);
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { Title, Referrer, Domain, URL, Body, Location, Cookie,
           Images, Applets, Embeds, Links, Forms, Anchors, Scripts, All, Clear, Open, Close,
           Write, WriteLn, GetElementsByName, CaptureEvents, ReleaseEvents,
           BgColor, FgColor, AlinkColor, LinkColor, VlinkColor, LastModified, Height, Width, Dir, DesignMode };
  private:
    static JSValue *namedItemGetter(ExecState *, JSObject *, const Identifier&, const PropertySlot&);
  };

  // The inheritance chain for JSHTMLElement is a bit different from other
  // classes that are "half-autogenerated". Because we return different ClassInfo structs
  // depending on the type of element, we inherit JSHTMLElement from WebCore::JSHTMLElement
  // instead of the other way around. 
  KJS_DEFINE_PROTOTYPE_WITH_PROTOTYPE(JSHTMLElementProto, WebCore::JSHTMLElementProto)

  class JSHTMLElement : public WebCore::JSHTMLElement {
  public:
    JSHTMLElement(ExecState *exec, WebCore::HTMLElement *e);
    virtual bool getOwnPropertySlot(ExecState *, const Identifier&, PropertySlot&);
    JSValue *getValueProperty(ExecState *exec, int token) const;
    virtual void put(ExecState *exec, const Identifier &propertyName, JSValue *value, int attr = None);
    void putValueProperty(ExecState *exec, int token, JSValue *value, int);
    virtual UString toString(ExecState *exec) const;
    virtual void pushEventHandlerScope(ExecState *exec, ScopeChain &scope) const;
    virtual JSValue *callAsFunction(ExecState *exec, JSObject *thisObj, const List&args);
    virtual bool implementsCall() const;
    virtual const ClassInfo* classInfo() const;
    static const ClassInfo info;

    static const ClassInfo form_info,
      select_info, option_info, input_info, object_info, 
      embed_info, table_info, caption_info, col_info, tablesection_info, tr_info,
      tablecell_info, frameSet_info, frame_info, iFrame_info, marquee_info;

    // FIXME: Might make sense to combine this with ClassInfo some day.
    typedef JSValue *(JSHTMLElement::*GetterFunction)(ExecState *exec, int token) const;
    typedef void (JSHTMLElement::*SetterFunction)(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    struct Accessors { GetterFunction m_getter; SetterFunction m_setter; };
    const Accessors* accessors() const;
    static const Accessors form_accessors,
      select_accessors, option_accessors, input_accessors, object_accessors, embed_accessors, table_accessors,
      caption_accessors, col_accessors, tablesection_accessors, tr_accessors,
      tablecell_accessors, frameSet_accessors, frame_accessors, iFrame_accessors, marquee_accessors;

    JSValue *formGetter(ExecState* exec, int token) const;
    void  formSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *selectGetter(ExecState* exec, int token) const;
    void  selectSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *objectGetter(ExecState* exec, int token) const;
    void  objectSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *embedGetter(ExecState*, int token) const;
    void  embedSetter(ExecState*, int token, JSValue*, const WebCore::String&);
    JSValue *tableGetter(ExecState* exec, int token) const;
    void  tableSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *tableCaptionGetter(ExecState* exec, int token) const;
    void  tableCaptionSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *tableColGetter(ExecState* exec, int token) const;
    void  tableColSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *tableSectionGetter(ExecState* exec, int token) const;
    void  tableSectionSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *tableRowGetter(ExecState* exec, int token) const;
    void  tableRowSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *tableCellGetter(ExecState* exec, int token) const;
    void  tableCellSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *frameSetGetter(ExecState* exec, int token) const;
    void  frameSetSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *frameGetter(ExecState* exec, int token) const;
    void  frameSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *iFrameGetter(ExecState* exec, int token) const;
    void  iFrameSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *marqueeGetter(ExecState* exec, int token) const;
    void  marqueeSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);

    enum { 
           FormAction, FormEncType, FormElements, FormLength, FormAcceptCharset,
           FormReset, FormTarget, FormName, FormMethod, FormSubmit, SelectAdd,
           SelectTabIndex, SelectValue, SelectSelectedIndex, SelectLength,
           SelectRemove, SelectForm, SelectBlur, SelectType, SelectOptions,
           SelectDisabled, SelectMultiple, SelectName, SelectSize, SelectFocus,
           ObjectHspace, ObjectHeight, ObjectAlign,
           ObjectBorder, ObjectCode, ObjectType, ObjectVspace, ObjectArchive,
           ObjectDeclare, ObjectForm, ObjectCodeBase, ObjectCodeType, ObjectData,
           ObjectName, ObjectStandby, ObjectTabIndex, ObjectUseMap, ObjectWidth, ObjectContentDocument,
           EmbedAlign, EmbedHeight, EmbedName, EmbedSrc, EmbedType, EmbedWidth,
           TableSummary, TableTBodies, TableTHead, TableCellPadding,
           TableDeleteCaption, TableCreateCaption, TableCaption, TableWidth,
           TableCreateTFoot, TableAlign, TableTFoot, TableDeleteRow,
           TableCellSpacing, TableRows, TableBgColor, TableBorder, TableFrame,
           TableRules, TableCreateTHead, TableDeleteTHead, TableDeleteTFoot,
           TableInsertRow, TableCaptionAlign, TableColCh, TableColChOff,
           TableColAlign, TableColSpan, TableColVAlign, TableColWidth,
           TableSectionCh, TableSectionDeleteRow, TableSectionChOff,
           TableSectionRows, TableSectionAlign, TableSectionVAlign,
           TableSectionInsertRow, TableRowSectionRowIndex, TableRowRowIndex,
           TableRowChOff, TableRowCells, TableRowVAlign, TableRowCh,
           TableRowAlign, TableRowBgColor, TableRowDeleteCell, TableRowInsertCell,
           TableCellColSpan, TableCellNoWrap, TableCellAbbr, TableCellHeight,
           TableCellWidth, TableCellCellIndex, TableCellChOff, TableCellBgColor,
           TableCellCh, TableCellVAlign, TableCellRowSpan, TableCellHeaders,
           TableCellAlign, TableCellAxis, TableCellScope, FrameSetCols,
           FrameSetRows, FrameSrc, FrameLocation, FrameFrameBorder, FrameScrolling,
           FrameMarginWidth, FrameLongDesc, FrameMarginHeight, FrameName, FrameContentDocument, FrameContentWindow, 
           FrameNoResize, FrameWidth, FrameHeight, IFrameLongDesc, IFrameDocument, IFrameAlign,
           IFrameFrameBorder, IFrameSrc, IFrameName, IFrameHeight,
           IFrameMarginHeight, IFrameMarginWidth, IFrameScrolling, IFrameWidth, IFrameContentDocument, IFrameContentWindow,
           MarqueeStart, MarqueeStop,
           GetContext,
           ElementInnerHTML, ElementId, ElementDir, ElementLang,
           ElementClassName, ElementInnerText, ElementDocument, ElementChildren, ElementContentEditable,
           ElementIsContentEditable, ElementOuterHTML, ElementOuterText};
  private:
    static JSValue *formIndexGetter(ExecState *exec, JSObject *, const Identifier&, const PropertySlot& slot);
    static JSValue *formNameGetter(ExecState *exec, JSObject *, const Identifier&, const PropertySlot& slot);
    static JSValue *selectIndexGetter(ExecState *exec, JSObject *, const Identifier&, const PropertySlot& slot);
    static JSValue *framesetNameGetter(ExecState *exec, JSObject *, const Identifier&, const PropertySlot& slot);
    static JSValue *frameWindowPropertyGetter(ExecState *exec, JSObject *, const Identifier&, const PropertySlot& slot);
    static JSValue *runtimeObjectGetter(ExecState *exec, JSObject *, const Identifier&, const PropertySlot& slot);
    static JSValue *runtimeObjectPropertyGetter(ExecState *exec, JSObject *, const Identifier&, const PropertySlot& slot);
  };

  WebCore::HTMLElement *toHTMLElement(JSValue *); // returns 0 if passed-in value is not a JSHTMLElement object
  WebCore::HTMLTableCaptionElement *toHTMLTableCaptionElement(JSValue *); // returns 0 if passed-in value is not a JSHTMLElement object for a HTMLTableCaptionElement
  WebCore::HTMLTableSectionElement *toHTMLTableSectionElement(JSValue *); // returns 0 if passed-in value is not a JSHTMLElement object for a HTMLTableSectionElement

  class JSHTMLCollection : public DOMObject {
  public:
    JSHTMLCollection(ExecState *exec, WebCore::HTMLCollection *c);
    ~JSHTMLCollection();
    virtual bool getOwnPropertySlot(ExecState *, const Identifier&, PropertySlot&);
    virtual JSValue *callAsFunction(ExecState *exec, JSObject *thisObj, const List&args);
    virtual bool implementsCall() const { return true; }
    virtual bool toBoolean(ExecState *) const { return true; }
    enum { Item, NamedItem, Tags };
    JSValue *getNamedItems(ExecState *exec, const Identifier &propertyName) const;
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    WebCore::HTMLCollection *impl() const { return m_impl.get(); }
  protected:
    RefPtr<WebCore::HTMLCollection> m_impl;
  private:
    static JSValue *lengthGetter(ExecState *exec, JSObject *, const Identifier&, const PropertySlot& slot);
    static JSValue *indexGetter(ExecState *exec, JSObject *, const Identifier&, const PropertySlot& slot);
    static JSValue *nameGetter(ExecState *exec, JSObject *, const Identifier&, const PropertySlot& slot);
  };

  class JSHTMLSelectCollection : public JSHTMLCollection {
  public:
    JSHTMLSelectCollection(ExecState *exec, WebCore::HTMLCollection *c, WebCore::HTMLSelectElement *e);
    virtual bool getOwnPropertySlot(ExecState *, const Identifier&, PropertySlot&);
    virtual void put(ExecState *exec, const Identifier &propertyName, JSValue *value, int attr = None);
  private:
    static JSValue *selectedIndexGetter(ExecState *exec, JSObject *, const Identifier&, const PropertySlot& slot);

    RefPtr<WebCore::HTMLSelectElement> m_element;
  };

  class HTMLAllCollection : public JSHTMLCollection {
  public:
    HTMLAllCollection(ExecState *exec, WebCore::HTMLCollection *c) :
      JSHTMLCollection(exec, c) { }
    virtual bool toBoolean(ExecState *) const { return false; }
    virtual bool masqueradeAsUndefined() const { return true; }
  };
  
  ////////////////////// Image Object ////////////////////////

  class ImageConstructorImp : public DOMObject {
  public:
    ImageConstructorImp(ExecState *exec, WebCore::Document *d);
    virtual bool implementsConstruct() const;
    virtual JSObject *construct(ExecState *exec, const List &args);
  private:
    RefPtr<WebCore::Document> m_doc;
  };

  JSValue *getHTMLCollection(ExecState *exec, WebCore::HTMLCollection *c);
  JSValue *getSelectHTMLCollection(ExecState *exec, WebCore::HTMLCollection *c, WebCore::HTMLSelectElement *e);
  JSValue *getAllHTMLCollection(ExecState *exec, WebCore::HTMLCollection *c);

} // namespace

#endif
