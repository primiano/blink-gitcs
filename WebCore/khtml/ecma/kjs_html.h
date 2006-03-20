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

#include "JSElement.h"
#include "kjs_dom.h"

namespace WebCore {
    class CanvasRenderingContext2D;
    class HTMLCollection;
    class HTMLDocument;
    class HTMLElement;
    class CanvasGradient;
    class CanvasPattern;
    class HTMLSelectElement;
    class HTMLTableCaptionElement;
    class HTMLTableSectionElement;
}

namespace KJS {

  class JSAbstractEventListener;

  class JSHTMLDocument : public DOMDocument {
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

  class JSHTMLElement : public WebCore::JSElement {
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

    static const ClassInfo html_info, head_info, link_info, title_info,
      meta_info, base_info, isIndex_info, style_info, body_info, form_info,
      select_info, optGroup_info, option_info, input_info, textArea_info,
      button_info, label_info, fieldSet_info, legend_info, ul_info, ol_info,
      dl_info, dir_info, menu_info, li_info, div_info, p_info, heading_info,
      blockQuote_info, q_info, pre_info, br_info, baseFont_info, font_info,
      hr_info, mod_info, a_info, canvas_info, img_info, object_info, param_info,
      applet_info, map_info, area_info, script_info, table_info,
      caption_info, col_info, tablesection_info, tr_info,
      tablecell_info, frameSet_info, frame_info, iFrame_info, marquee_info;

    // FIXME: Might make sense to combine this with ClassInfo some day.
    typedef JSValue *(JSHTMLElement::*GetterFunction)(ExecState *exec, int token) const;
    typedef void (JSHTMLElement::*SetterFunction)(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    struct Accessors { GetterFunction m_getter; SetterFunction m_setter; };
    const Accessors* accessors() const;
    static const Accessors html_accessors, head_accessors, link_accessors, title_accessors,
      meta_accessors, base_accessors, isIndex_accessors, style_accessors, body_accessors, form_accessors,
      select_accessors, optGroup_accessors, option_accessors, input_accessors, textArea_accessors,
      button_accessors, label_accessors, fieldSet_accessors, legend_accessors, ul_accessors, ol_accessors,
      dl_accessors, dir_accessors, menu_accessors, li_accessors, div_accessors, p_accessors, heading_accessors,
      blockQuote_accessors, q_accessors, pre_accessors, br_accessors, baseFont_accessors, font_accessors,
      hr_accessors, mod_accessors, a_accessors, canvas_accessors, img_accessors, object_accessors, param_accessors,
      applet_accessors, map_accessors, area_accessors, script_accessors, table_accessors,
      caption_accessors, col_accessors, tablesection_accessors, tr_accessors,
      tablecell_accessors, frameSet_accessors, frame_accessors, iFrame_accessors, marquee_accessors;

    JSValue *htmlGetter(ExecState* exec, int token) const;
    void  htmlSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *headGetter(ExecState* exec, int token) const;
    void  headSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *linkGetter(ExecState* exec, int token) const;
    void  linkSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *titleGetter(ExecState* exec, int token) const;
    void  titleSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *metaGetter(ExecState* exec, int token) const;
    void  metaSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *baseGetter(ExecState* exec, int token) const;
    void  baseSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *isIndexGetter(ExecState* exec, int token) const;
    void  isIndexSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *styleGetter(ExecState* exec, int token) const;
    void  styleSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *bodyGetter(ExecState* exec, int token) const;
    void  bodySetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *formGetter(ExecState* exec, int token) const;
    void  formSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *selectGetter(ExecState* exec, int token) const;
    void  selectSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *optGroupGetter(ExecState* exec, int token) const;
    void  optGroupSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *optionGetter(ExecState* exec, int token) const;
    void  optionSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *inputGetter(ExecState* exec, int token) const;
    void  inputSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *textAreaGetter(ExecState* exec, int token) const;
    void  textAreaSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *buttonGetter(ExecState* exec, int token) const;
    void  buttonSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *labelGetter(ExecState* exec, int token) const;
    void  labelSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *fieldSetGetter(ExecState* exec, int token) const;
    void  fieldSetSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *legendGetter(ExecState* exec, int token) const;
    void  legendSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *uListGetter(ExecState* exec, int token) const;
    void  uListSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *oListGetter(ExecState* exec, int token) const;
    void  oListSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *dListGetter(ExecState* exec, int token) const;
    void  dListSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *dirGetter(ExecState* exec, int token) const;
    void  dirSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *menuGetter(ExecState* exec, int token) const;
    void  menuSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *liGetter(ExecState* exec, int token) const;
    void  liSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *divGetter(ExecState* exec, int token) const;
    void  divSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *paragraphGetter(ExecState* exec, int token) const;
    void  paragraphSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *headingGetter(ExecState* exec, int token) const;
    void  headingSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *blockQuoteGetter(ExecState* exec, int token) const;
    void  blockQuoteSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *quoteGetter(ExecState* exec, int token) const;
    void  quoteSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *preGetter(ExecState* exec, int token) const;
    void  preSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *brGetter(ExecState* exec, int token) const;
    void  brSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *baseFontGetter(ExecState* exec, int token) const;
    void  baseFontSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *fontGetter(ExecState* exec, int token) const;
    void  fontSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *hrGetter(ExecState* exec, int token) const;
    void  hrSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *modGetter(ExecState* exec, int token) const;
    void  modSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *anchorGetter(ExecState* exec, int token) const;
    void  anchorSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *imageGetter(ExecState* exec, int token) const;
    void  imageSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *objectGetter(ExecState* exec, int token) const;
    void  objectSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *paramGetter(ExecState* exec, int token) const;
    void  paramSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *appletGetter(ExecState* exec, int token) const;
    void  appletSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *mapGetter(ExecState* exec, int token) const;
    void  mapSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *areaGetter(ExecState* exec, int token) const;
    void  areaSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
    JSValue *scriptGetter(ExecState* exec, int token) const;
    void  scriptSetter(ExecState *exec, int token, JSValue *value, const WebCore::String& str);
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

    enum { HtmlVersion, HeadProfile, LinkHref, LinkRel, LinkMedia,
           LinkCharset, LinkDisabled, LinkHrefLang, LinkRev, LinkTarget, LinkType,
           LinkSheet, TitleText, MetaName, MetaHttpEquiv, MetaContent, MetaScheme,
           BaseHref, BaseTarget, IsIndexForm, IsIndexPrompt, StyleDisabled,
           StyleSheet, StyleType, StyleMedia, BodyBackground, BodyVLink, BodyText,
           BodyLink, BodyALink, BodyBgColor, BodyScrollLeft, BodyScrollTop, BodyScrollHeight, BodyScrollWidth,
           FormAction, FormEncType, FormElements, FormLength, FormAcceptCharset,
           FormReset, FormTarget, FormName, FormMethod, FormSubmit, SelectAdd,
           SelectTabIndex, SelectValue, SelectSelectedIndex, SelectLength,
           SelectRemove, SelectForm, SelectBlur, SelectType, SelectOptions,
           SelectDisabled, SelectMultiple, SelectName, SelectSize, SelectFocus,
           OptGroupDisabled, OptGroupLabel, OptionIndex, OptionSelected,
           OptionForm, OptionText, OptionDefaultSelected, OptionDisabled,
           OptionLabel, OptionValue, InputBlur, InputReadOnly, InputAccept,
           InputSize, InputDefaultValue, InputTabIndex, InputValue, InputType,
           InputFocus, InputMaxLength, InputDefaultChecked, InputDisabled,
           InputChecked, InputIndeterminate, InputForm, InputAccessKey, InputAlign, InputAlt,
           InputName, InputSrc, InputUseMap, InputSelect, InputClick,
           InputSelectionStart, InputSelectionEnd, InputSetSelectionRange,
           TextAreaAccessKey, TextAreaName, TextAreaDefaultValue, TextAreaSelect, TextAreaSetSelectionRange,
           TextAreaCols, TextAreaDisabled, TextAreaForm, TextAreaType,
           TextAreaTabIndex, TextAreaReadOnly, TextAreaRows, TextAreaValue,
           TextAreaSelectionStart, TextAreaSelectionEnd,
           TextAreaBlur, TextAreaFocus, ButtonBlur, ButtonFocus, ButtonForm, ButtonTabIndex, ButtonName,
           ButtonDisabled, ButtonAccessKey, ButtonType, ButtonValue, LabelHtmlFor,
           LabelForm, LabelFocus, LabelAccessKey, FieldSetForm, LegendForm, LegendAccessKey,
           LegendAlign, LegendFocus, UListType, UListCompact, OListStart, OListCompact,
           OListType, DListCompact, DirectoryCompact, MenuCompact, LIType,
           LIValue, DivAlign, ParagraphAlign, HeadingAlign, BlockQuoteCite,
           QuoteCite, PreWidth, PreWrap, BRClear, BaseFontColor, BaseFontSize,
           BaseFontFace, FontColor, FontSize, FontFace, HRWidth, HRNoShade,
           HRAlign, HRSize, ModCite, ModDateTime, AnchorShape, AnchorRel,
           AnchorAccessKey, AnchorCoords, AnchorHref, AnchorProtocol, AnchorHost,
           AnchorCharset, AnchorHrefLang, AnchorHostname, AnchorType, AnchorFocus,
           AnchorPort, AnchorPathName, AnchorHash, AnchorSearch, AnchorName,
           AnchorRev, AnchorTabIndex, AnchorTarget, AnchorText, AnchorBlur, AnchorToString, 
           ImageName, ImageAlign, ImageHspace, ImageVspace, ImageUseMap, ImageAlt,
           ImageLowSrc, ImageWidth, ImageIsMap, ImageBorder, ImageHeight, ImageComplete,
           ImageLongDesc, ImageSrc, ImageX, ImageY, ObjectHspace, ObjectHeight, ObjectAlign,
           ObjectBorder, ObjectCode, ObjectType, ObjectVspace, ObjectArchive,
           ObjectDeclare, ObjectForm, ObjectCodeBase, ObjectCodeType, ObjectData,
           ObjectName, ObjectStandby, ObjectTabIndex, ObjectUseMap, ObjectWidth, ObjectContentDocument,
           ParamName, ParamType, ParamValueType, ParamValue, AppletArchive,
           AppletAlt, AppletCode, AppletWidth, AppletAlign, AppletCodeBase,
           AppletName, AppletHeight, AppletHspace, AppletObject, AppletVspace,
           MapAreas, MapName, AreaHash, AreaHref, AreaTarget, AreaPort, AreaShape,
           AreaCoords, AreaAlt, AreaAccessKey, AreaNoHref, AreaHost, AreaProtocol,
           AreaHostName, AreaPathName, AreaSearch, AreaTabIndex, ScriptEvent,
           ScriptType, ScriptHtmlFor, ScriptText, ScriptSrc, ScriptCharset,
           ScriptDefer, TableSummary, TableTBodies, TableTHead, TableCellPadding,
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
           ElementInnerHTML, ElementTitle, ElementId, ElementDir, ElementLang,
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
  
  ////////////////////// Option Object ////////////////////////

  class OptionConstructorImp : public JSObject {
  public:
    OptionConstructorImp(ExecState *exec, WebCore::Document *d);
    virtual bool implementsConstruct() const;
    virtual JSObject *construct(ExecState *exec, const List &args);
  private:
    RefPtr<WebCore::Document> m_doc;
  };

  ////////////////////// Image Object ////////////////////////

  class ImageConstructorImp : public JSObject {
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
