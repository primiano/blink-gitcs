# WebCore - qmake build info
CONFIG += building-libs
CONFIG += depend_includepath

symbian: {
    TARGET.EPOCALLOWDLLDATA=1
    TARGET.EPOCHEAPSIZE = 0x20000 0x2000000 // Min 128kB, Max 32MB
    TARGET.CAPABILITY = All -Tcb

    webkitlibs.sources = QtWebKit.dll
    webkitlibs.path = /sys/bin
    vendorinfo = \
        "; Localised Vendor name" \
        "%{\"Nokia, Qt\"}" \
        " " \
        "; Unique Vendor name" \
        ":\"Nokia, Qt\"" \
        " "
    webkitlibs.pkg_prerules = vendorinfo

    DEPLOYMENT += webkitlibs

    TARGET.UID3 = 0x200267C2
    # Need to guarantee that these come before system includes of /epoc32/include
    MMP_RULES += "USERINCLUDE rendering"
    MMP_RULES += "USERINCLUDE platform/text"
    # RO text (code) section in qtwebkit.dll exceeds allocated space for gcce udeb target.
    # Move RW-section base address to start from 0xE00000 instead of the toolchain default 0x400000.
    QMAKE_LFLAGS.ARMCC += --rw-base 0xE00000
    MMP_RULES += ALWAYS_BUILD_AS_ARM
    QMAKE_CXXFLAGS.ARMCC += -OTime -O3
}

include($$PWD/../WebKit.pri)

TEMPLATE = lib
TARGET = QtWebKit

contains(QT_CONFIG, embedded):CONFIG += embedded

isEmpty(OUTPUT_DIR): OUTPUT_DIR = ..

CONFIG(standalone_package) {
    isEmpty(WC_GENERATED_SOURCES_DIR):WC_GENERATED_SOURCES_DIR = $$PWD/generated
    isEmpty(JSC_GENERATED_SOURCES_DIR):JSC_GENERATED_SOURCES_DIR = $$PWD/../JavaScriptCore/generated

    CONFIG(QTDIR_build):include($$QT_SOURCE_TREE/src/qbase.pri)
    else: VERSION = 4.7.0

    PRECOMPILED_HEADER = $$PWD/../WebKit/qt/WebKit_pch.h
    DEFINES *= NDEBUG
} else {
    isEmpty(WC_GENERATED_SOURCES_DIR):WC_GENERATED_SOURCES_DIR = generated
    isEmpty(JSC_GENERATED_SOURCES_DIR):JSC_GENERATED_SOURCES_DIR = ../JavaScriptCore/generated

    CONFIG(debug, debug|release) {
        OBJECTS_DIR = obj/debug
    } else { # Release
        OBJECTS_DIR = obj/release
    }

}

!CONFIG(QTDIR_build) {
    DESTDIR = $$OUTPUT_DIR/lib
    !static: DEFINES += QT_MAKEDLL
}

unix {
    QMAKE_PKGCONFIG_REQUIRES = QtCore QtGui QtNetwork
}

unix:!mac:*-g++*:QMAKE_CXXFLAGS += -ffunction-sections -fdata-sections 
unix:!mac:*-g++*:QMAKE_LFLAGS += -Wl,--gc-sections

CONFIG(release):!CONFIG(standalone_package) {
    contains(QT_CONFIG, reduce_exports):CONFIG += hide_symbols
    unix:contains(QT_CONFIG, reduce_relocations):CONFIG += bsymbolic_functions
}

linux-*: DEFINES += HAVE_STDINT_H
freebsd-*: DEFINES += HAVE_PTHREAD_NP_H

DEFINES += BUILD_WEBKIT

# Remove whole program optimizations due to miscompilations
win32-msvc2005|win32-msvc2008:{
    QMAKE_CFLAGS_RELEASE -= -GL
    QMAKE_CXXFLAGS_RELEASE -= -GL
}

win32-*: DEFINES += _HAS_TR1=0
wince* {
#    DEFINES += ENABLE_SVG=0 ENABLE_XPATH=0 ENABLE_XBL=0 \
#               ENABLE_SVG_ANIMATION=0 ENABLE_SVG_USE=0  \
#               ENABLE_SVG_FOREIGN_OBJECT=0 ENABLE_SVG_AS_IMAGE=0

    INCLUDEPATH += $$PWD/../JavaScriptCore/os-wince
    INCLUDEPATH += $$PWD/../JavaScriptCore/os-win32
}

# Pick up 3rdparty libraries from INCLUDE/LIB just like with MSVC
win32-g++ {
    TMPPATH            = $$quote($$(INCLUDE))
    QMAKE_INCDIR_POST += $$split(TMPPATH,";")
    TMPPATH            = $$quote($$(LIB))
    QMAKE_LIBDIR_POST += $$split(TMPPATH,";")
}

# Assume that symbian OS always comes with sqlite
symbian:!CONFIG(QTDIR_build): CONFIG += system-sqlite



RESOURCES += \
    $$PWD/../WebCore/WebCore.qrc

!symbian {
    RESOURCES += $$PWD/../WebCore/inspector/front-end/WebKit.qrc
}

include($$PWD/../JavaScriptCore/JavaScriptCore.pri)

# Disable HTML5 media compilation if phonon is unavailable
!contains(DEFINES, ENABLE_VIDEO=1) {
    !contains(QT_CONFIG, phonon) {
        DEFINES -= ENABLE_VIDEO=1
        DEFINES += ENABLE_VIDEO=0
    }
}

# Extract sources to build from the generator definitions
defineTest(addExtraCompiler) {
    isEqual($${1}.wkAddOutputToSources, false): return(true)

    outputRule = $$eval($${1}.output)
    input = $$eval($${1}.input)
    input = $$eval($$input)

    for(file,input) {
        base = $$basename(file)
        base ~= s/\..+//
        newfile=$$replace(outputRule,\\$\\{QMAKE_FILE_BASE\\},$$base)
        SOURCES += $$newfile
    }
    SOURCES += $$eval($${1}.wkExtraSources)
    export(SOURCES)

    return(true)
}
include(WebCore.pri)

INCLUDEPATH = \
    $$PWD \
    $$PWD/accessibility \
    $$PWD/bindings/js \
    $$PWD/bridge \
    $$PWD/bridge/c \
    $$PWD/css \
    $$PWD/dom \
    $$PWD/dom/default \
    $$PWD/editing \
    $$PWD/history \
    $$PWD/html \
    $$PWD/html/canvas \
    $$PWD/inspector \
    $$PWD/loader \
    $$PWD/loader/appcache \
    $$PWD/loader/archive \
    $$PWD/loader/icon \
    $$PWD/notifications \
    $$PWD/page \
    $$PWD/page/animation \
    $$PWD/platform \
    $$PWD/platform/animation \
    $$PWD/platform/graphics \
    $$PWD/platform/graphics/filters \
    $$PWD/platform/graphics/transforms \
    $$PWD/platform/image-decoders \
    $$PWD/platform/mock \
    $$PWD/platform/network \
    $$PWD/platform/sql \
    $$PWD/platform/text \
    $$PWD/plugins \
    $$PWD/rendering \
    $$PWD/rendering/style \
    $$PWD/storage \
    $$PWD/svg \
    $$PWD/svg/animation \
    $$PWD/svg/graphics \
    $$PWD/svg/graphics/filters \
    $$PWD/websockets \
    $$PWD/wml \
    $$PWD/workers \
    $$PWD/xml \
    $$WC_GENERATED_SOURCES_DIR \
    $$INCLUDEPATH

INCLUDEPATH = \
    $$PWD/bridge/qt \
    $$PWD/page/qt \
    $$PWD/platform/graphics/qt \
    $$PWD/platform/network/qt \
    $$PWD/platform/qt \
    $$PWD/../WebKit/qt/WebCoreSupport \
    $$INCLUDEPATH

QT += network


DASHBOARDSUPPORTCSSPROPERTIES = $$PWD/css/DashboardSupportCSSPropertyNames.in


contains(DEFINES, ENABLE_SVG=1) {
    EXTRACSSPROPERTIES += $$PWD/css/SVGCSSPropertyNames.in
    EXTRACSSVALUES += $$PWD/css/SVGCSSValueKeywords.in
}

contains(DEFINES, ENABLE_WCSS=1) {
    EXTRACSSPROPERTIES += $$PWD/css/WCSSPropertyNames.in
    EXTRACSSVALUES += $$PWD/css/WCSSValueKeywords.in
}

SOURCES += \
    accessibility/AccessibilityImageMapLink.cpp \
    accessibility/AccessibilityMediaControls.cpp \    
    accessibility/AccessibilityObject.cpp \    
    accessibility/AccessibilityList.cpp \    
    accessibility/AccessibilityListBox.cpp \    
    accessibility/AccessibilityListBoxOption.cpp \    
    accessibility/AccessibilityRenderObject.cpp \    
    accessibility/AccessibilitySlider.cpp \    
    accessibility/AccessibilityARIAGrid.cpp \    
    accessibility/AccessibilityARIAGridCell.cpp \    
    accessibility/AccessibilityARIAGridRow.cpp \    
    accessibility/AccessibilityTable.cpp \    
    accessibility/AccessibilityTableCell.cpp \    
    accessibility/AccessibilityTableColumn.cpp \    
    accessibility/AccessibilityTableHeaderContainer.cpp \    
    accessibility/AccessibilityTableRow.cpp \    
    accessibility/AXObjectCache.cpp \
    bindings/js/GCController.cpp \
    bindings/js/JSCallbackData.cpp \
    bindings/js/JSAttrCustom.cpp \
    bindings/js/JSCDATASectionCustom.cpp \
    bindings/js/JSCanvasRenderingContextCustom.cpp \
    bindings/js/JSCanvasRenderingContext2DCustom.cpp \
    bindings/js/JSClipboardCustom.cpp \
    bindings/js/JSConsoleCustom.cpp \
    bindings/js/JSCSSRuleCustom.cpp \
    bindings/js/JSCSSRuleListCustom.cpp \
    bindings/js/JSCSSStyleDeclarationCustom.cpp \
    bindings/js/JSCSSValueCustom.cpp \
    bindings/js/JSCoordinatesCustom.cpp \
    bindings/js/JSCustomPositionCallback.cpp \
    bindings/js/JSCustomPositionErrorCallback.cpp \
    bindings/js/JSCustomVoidCallback.cpp \
    bindings/js/JSCustomXPathNSResolver.cpp \
    bindings/js/JSDataGridColumnListCustom.cpp \
    bindings/js/JSDataGridDataSource.cpp \
    bindings/js/JSDocumentCustom.cpp \
    bindings/js/JSDocumentFragmentCustom.cpp \
    bindings/js/JSDOMGlobalObject.cpp \
    bindings/js/JSDOMWindowBase.cpp \
    bindings/js/JSDOMWindowCustom.cpp \
    bindings/js/JSDOMWindowShell.cpp \
    bindings/js/JSElementCustom.cpp \
    bindings/js/JSEventCustom.cpp \
    bindings/js/JSEventSourceConstructor.cpp \
    bindings/js/JSEventSourceCustom.cpp \
    bindings/js/JSEventTarget.cpp \
    bindings/js/JSExceptionBase.cpp \
    bindings/js/JSGeolocationCustom.cpp \
    bindings/js/JSHistoryCustom.cpp \
    bindings/js/JSHTMLAppletElementCustom.cpp \
    bindings/js/JSHTMLCanvasElementCustom.cpp \
    bindings/js/JSHTMLAllCollectionCustom.cpp \
    bindings/js/JSHTMLCollectionCustom.cpp \
    bindings/js/JSHTMLDataGridElementCustom.cpp \
    bindings/js/JSHTMLDocumentCustom.cpp \
    bindings/js/JSHTMLElementCustom.cpp \
    bindings/js/JSHTMLEmbedElementCustom.cpp \
    bindings/js/JSHTMLFormElementCustom.cpp \
    bindings/js/JSHTMLFrameElementCustom.cpp \
    bindings/js/JSHTMLFrameSetElementCustom.cpp \
    bindings/js/JSHTMLIFrameElementCustom.cpp \
    bindings/js/JSHTMLInputElementCustom.cpp \
    bindings/js/JSHTMLObjectElementCustom.cpp \
    bindings/js/JSHTMLOptionsCollectionCustom.cpp \
    bindings/js/JSHTMLSelectElementCustom.cpp \
    bindings/js/JSImageConstructor.cpp \
    bindings/js/JSImageDataCustom.cpp \
    bindings/js/JSInjectedScriptHostCustom.cpp \
    bindings/js/JSInspectedObjectWrapper.cpp \
    bindings/js/JSInspectorFrontendHostCustom.cpp \
    bindings/js/JSInspectorCallbackWrapper.cpp \
    bindings/js/JSLocationCustom.cpp \
    bindings/js/JSNamedNodeMapCustom.cpp \
    bindings/js/JSNavigatorCustom.cpp  \
    bindings/js/JSNodeCustom.cpp \
    bindings/js/JSNodeFilterCondition.cpp \
    bindings/js/JSNodeFilterCustom.cpp \
    bindings/js/JSNodeIteratorCustom.cpp \
    bindings/js/JSNodeListCustom.cpp \
    bindings/js/JSOptionConstructor.cpp \
    bindings/js/JSQuarantinedObjectWrapper.cpp \
    bindings/js/JSStyleSheetCustom.cpp \
    bindings/js/JSStyleSheetListCustom.cpp \
    bindings/js/JSTextCustom.cpp \
    bindings/js/JSTreeWalkerCustom.cpp \
    bindings/js/JSWebKitCSSMatrixConstructor.cpp \
    bindings/js/JSWebKitPointConstructor.cpp \
    bindings/js/JSXMLHttpRequestConstructor.cpp \
    bindings/js/JSXMLHttpRequestCustom.cpp \
    bindings/js/JSXMLHttpRequestUploadCustom.cpp \
    bindings/js/JSPluginCustom.cpp \
    bindings/js/JSPluginArrayCustom.cpp \
    bindings/js/JSMessageChannelConstructor.cpp \
    bindings/js/JSMessageChannelCustom.cpp \
    bindings/js/JSMessageEventCustom.cpp \
    bindings/js/JSMessagePortCustom.cpp \
    bindings/js/JSMessagePortCustom.h \
    bindings/js/JSMimeTypeArrayCustom.cpp \
    bindings/js/JSDOMBinding.cpp \
    bindings/js/JSEventListener.cpp \
    bindings/js/JSLazyEventListener.cpp \
    bindings/js/JSPluginElementFunctions.cpp \
    bindings/js/JSPopStateEventCustom.cpp \
    bindings/js/ScriptArray.cpp \
    bindings/js/ScriptCachedFrameData.cpp \
    bindings/js/ScriptCallFrame.cpp \
    bindings/js/ScriptCallStack.cpp \
    bindings/js/ScriptController.cpp \
    bindings/js/ScriptEventListener.cpp \
    bindings/js/ScriptFunctionCall.cpp \
    bindings/js/ScriptObject.cpp \
    bindings/js/ScriptState.cpp \
    bindings/js/ScriptValue.cpp \
    bindings/js/ScheduledAction.cpp \
    bindings/js/SerializedScriptValue.cpp \
    bindings/ScriptControllerBase.cpp \
    bridge/Bridge.cpp \
    bridge/IdentifierRep.cpp \
    bridge/NP_jsobject.cpp \
    bridge/npruntime.cpp \
    bridge/runtime_array.cpp \
    bridge/runtime_method.cpp \
    bridge/runtime_object.cpp \
    bridge/runtime_root.cpp \
    bridge/c/c_class.cpp \
    bridge/c/c_instance.cpp \
    bridge/c/c_runtime.cpp \
    bridge/c/c_utility.cpp \
    css/CSSBorderImageValue.cpp \
    css/CSSCanvasValue.cpp \
    css/CSSCharsetRule.cpp \
    css/CSSComputedStyleDeclaration.cpp \
    css/CSSCursorImageValue.cpp \
    css/CSSFontFace.cpp \
    css/CSSFontFaceRule.cpp \
    css/CSSFontFaceSrcValue.cpp \
    css/CSSFontSelector.cpp \
    css/CSSFontFaceSource.cpp \
    css/CSSFunctionValue.cpp \
    css/CSSGradientValue.cpp \
    css/CSSHelper.cpp \
    css/CSSImageValue.cpp \
    css/CSSImageGeneratorValue.cpp \
    css/CSSImportRule.cpp \
    css/CSSInheritedValue.cpp \
    css/CSSInitialValue.cpp \
    css/CSSMediaRule.cpp \
    css/CSSMutableStyleDeclaration.cpp \
    css/CSSPageRule.cpp \
    css/CSSParser.cpp \
    css/CSSParserValues.cpp \
    css/CSSPrimitiveValue.cpp \
    css/CSSProperty.cpp \
    css/CSSPropertyLonghand.cpp \
    css/CSSReflectValue.cpp \
    css/CSSRule.cpp \
    css/CSSRuleList.cpp \
    css/CSSSelector.cpp \
    css/CSSSelectorList.cpp \
    css/CSSSegmentedFontFace.cpp \
    css/CSSStyleDeclaration.cpp \
    css/CSSStyleRule.cpp \
    css/CSSStyleSelector.cpp \
    css/CSSStyleSheet.cpp \
    css/CSSTimingFunctionValue.cpp \
    css/CSSUnicodeRangeValue.cpp \
    css/CSSValueList.cpp \
    css/CSSVariableDependentValue.cpp \
    css/CSSVariablesDeclaration.cpp \
    css/CSSVariablesRule.cpp \
    css/FontFamilyValue.cpp \
    css/FontValue.cpp \
    css/MediaFeatureNames.cpp \
    css/Media.cpp \
    css/MediaList.cpp \
    css/MediaQuery.cpp \
    css/MediaQueryEvaluator.cpp \
    css/MediaQueryExp.cpp \
    css/RGBColor.cpp \
    css/ShadowValue.cpp \
    css/StyleBase.cpp \
    css/StyleList.cpp \
    css/StyleSheet.cpp \
    css/StyleSheetList.cpp \
    css/WebKitCSSKeyframeRule.cpp \
    css/WebKitCSSKeyframesRule.cpp \
    css/WebKitCSSMatrix.cpp \
    css/WebKitCSSTransformValue.cpp \
    dom/ActiveDOMObject.cpp \
    dom/Attr.cpp \
    dom/Attribute.cpp \
    dom/BeforeTextInsertedEvent.cpp \
    dom/BeforeUnloadEvent.cpp \
    dom/CDATASection.cpp \
    dom/CharacterData.cpp \
    dom/CheckedRadioButtons.cpp \
    dom/ChildNodeList.cpp \
    dom/ClassNodeList.cpp \
    dom/ClientRect.cpp \
    dom/ClientRectList.cpp \
    dom/Clipboard.cpp \
    dom/ClipboardEvent.cpp \
    dom/Comment.cpp \
    dom/CompositionEvent.cpp \
    dom/ContainerNode.cpp \
    dom/CSSMappedAttributeDeclaration.cpp \
    dom/Document.cpp \
    dom/DocumentFragment.cpp \
    dom/DocumentType.cpp \
    dom/DOMImplementation.cpp \
    dom/DynamicNodeList.cpp \
    dom/EditingText.cpp \
    dom/Element.cpp \
    dom/Entity.cpp \
    dom/EntityReference.cpp \
    dom/ErrorEvent.cpp \
    dom/Event.cpp \
    dom/EventNames.cpp \
    dom/EventTarget.cpp \
    dom/ExceptionBase.cpp \
    dom/ExceptionCode.cpp \
    dom/InputElement.cpp \
    dom/KeyboardEvent.cpp \
    dom/MappedAttribute.cpp \
    dom/MessageChannel.cpp \
    dom/MessageEvent.cpp \
    dom/MessagePort.cpp \
    dom/MessagePortChannel.cpp \
    dom/MouseEvent.cpp \
    dom/MouseRelatedEvent.cpp \
    dom/MutationEvent.cpp \
    dom/NamedAttrMap.cpp \
    dom/NamedMappedAttrMap.cpp \
    dom/NameNodeList.cpp \
    dom/Node.cpp \
    dom/NodeFilterCondition.cpp \
    dom/NodeFilter.cpp \
    dom/NodeIterator.cpp \
    dom/Notation.cpp \
    dom/OptionGroupElement.cpp \
    dom/OptionElement.cpp \
    dom/OverflowEvent.cpp \
    dom/PageTransitionEvent.cpp \
    dom/PopStateEvent.cpp \
    dom/Position.cpp \
    dom/PositionIterator.cpp \
    dom/ProcessingInstruction.cpp \
    dom/ProgressEvent.cpp \
    dom/QualifiedName.cpp \
    dom/Range.cpp \
    dom/RegisteredEventListener.cpp \
    dom/ScriptElement.cpp \
    dom/ScriptExecutionContext.cpp \
    dom/SelectElement.cpp \
    dom/SelectorNodeList.cpp \
    dom/SpaceSplitString.cpp \
    dom/StaticNodeList.cpp \
    dom/StyledElement.cpp \
    dom/StyleElement.cpp \
    dom/TagNodeList.cpp \
    dom/Text.cpp \
    dom/TextEvent.cpp \
    dom/Touch.cpp \
    dom/TouchEvent.cpp \
    dom/TouchList.cpp \
    dom/Traversal.cpp \
    dom/TreeWalker.cpp \
    dom/UIEvent.cpp \
    dom/UIEventWithKeyState.cpp \
    dom/WebKitAnimationEvent.cpp \
    dom/WebKitTransitionEvent.cpp \
    dom/WheelEvent.cpp \
    dom/XMLTokenizer.cpp \
    dom/XMLTokenizerQt.cpp \
    dom/default/PlatformMessagePortChannel.cpp \
    editing/AppendNodeCommand.cpp \
    editing/ApplyStyleCommand.cpp \
    editing/BreakBlockquoteCommand.cpp \
    editing/CompositeEditCommand.cpp \
    editing/CreateLinkCommand.cpp \
    editing/DeleteButtonController.cpp \
    editing/DeleteButton.cpp \
    editing/DeleteFromTextNodeCommand.cpp \
    editing/DeleteSelectionCommand.cpp \
    editing/EditCommand.cpp \
    editing/Editor.cpp \
    editing/EditorCommand.cpp \
    editing/FormatBlockCommand.cpp \
    editing/htmlediting.cpp \
    editing/HTMLInterchange.cpp \
    editing/IndentOutdentCommand.cpp \
    editing/InsertIntoTextNodeCommand.cpp \
    editing/InsertLineBreakCommand.cpp \
    editing/InsertListCommand.cpp \
    editing/InsertNodeBeforeCommand.cpp \
    editing/InsertParagraphSeparatorCommand.cpp \
    editing/InsertTextCommand.cpp \
    editing/JoinTextNodesCommand.cpp \
    editing/markup.cpp \
    editing/MergeIdenticalElementsCommand.cpp \
    editing/ModifySelectionListLevel.cpp \
    editing/MoveSelectionCommand.cpp \
    editing/RemoveCSSPropertyCommand.cpp \
    editing/RemoveFormatCommand.cpp \
    editing/RemoveNodeCommand.cpp \
    editing/RemoveNodePreservingChildrenCommand.cpp \
    editing/ReplaceNodeWithSpanCommand.cpp \
    editing/ReplaceSelectionCommand.cpp \
    editing/SelectionController.cpp \
    editing/SetNodeAttributeCommand.cpp \
    editing/SmartReplace.cpp \
    editing/SmartReplaceICU.cpp \
    editing/SplitElementCommand.cpp \
    editing/SplitTextNodeCommand.cpp \
    editing/SplitTextNodeContainingElementCommand.cpp \
    editing/TextIterator.cpp \
    editing/TypingCommand.cpp \
    editing/UnlinkCommand.cpp \
    editing/VisiblePosition.cpp \
    editing/VisibleSelection.cpp \
    editing/visible_units.cpp \
    editing/WrapContentsInDummySpanCommand.cpp \
    history/BackForwardList.cpp \
    history/CachedFrame.cpp \
    history/CachedPage.cpp \
    history/HistoryItem.cpp \
    history/qt/HistoryItemQt.cpp \
    history/PageCache.cpp \
    html/canvas/CanvasGradient.cpp \
    html/canvas/CanvasPattern.cpp \
    html/canvas/CanvasPixelArray.cpp \
    html/canvas/CanvasRenderingContext.cpp \
    html/canvas/CanvasRenderingContext2D.cpp \
    html/canvas/CanvasStyle.cpp \
    html/CollectionCache.cpp \
    html/DataGridColumn.cpp \
    html/DataGridColumnList.cpp \
    html/DOMDataGridDataSource.cpp \
    html/File.cpp \
    html/FileList.cpp \
    html/FormDataList.cpp \
    html/HTMLAllCollection.cpp \
    html/HTMLAnchorElement.cpp \
    html/HTMLAppletElement.cpp \
    html/HTMLAreaElement.cpp \
    html/HTMLBaseElement.cpp \
    html/HTMLBaseFontElement.cpp \
    html/HTMLBlockquoteElement.cpp \
    html/HTMLBodyElement.cpp \
    html/HTMLBRElement.cpp \
    html/HTMLButtonElement.cpp \
    html/HTMLCanvasElement.cpp \
    html/HTMLCollection.cpp \
    html/HTMLDataGridElement.cpp \
    html/HTMLDataGridCellElement.cpp \
    html/HTMLDataGridColElement.cpp \
    html/HTMLDataGridRowElement.cpp \
    html/HTMLDataListElement.cpp \
    html/HTMLDirectoryElement.cpp \
    html/HTMLDivElement.cpp \
    html/HTMLDListElement.cpp \
    html/HTMLDocument.cpp \
    html/HTMLElement.cpp \
    html/HTMLEmbedElement.cpp \
    html/HTMLFieldSetElement.cpp \
    html/HTMLFontElement.cpp \
    html/HTMLFormCollection.cpp \
    html/HTMLFormElement.cpp \
    html/HTMLFrameElementBase.cpp \
    html/HTMLFrameElement.cpp \
    html/HTMLFrameOwnerElement.cpp \
    html/HTMLFrameSetElement.cpp \
    html/HTMLFormControlElement.cpp \
    html/HTMLHeadElement.cpp \
    html/HTMLHeadingElement.cpp \
    html/HTMLHRElement.cpp \
    html/HTMLHtmlElement.cpp \
    html/HTMLIFrameElement.cpp \
    html/HTMLImageElement.cpp \
    html/HTMLImageLoader.cpp \
    html/HTMLInputElement.cpp \
    html/HTMLIsIndexElement.cpp \
    html/HTMLKeygenElement.cpp \
    html/HTMLLabelElement.cpp \
    html/HTMLLegendElement.cpp \
    html/HTMLLIElement.cpp \
    html/HTMLLinkElement.cpp \
    html/HTMLMapElement.cpp \
    html/HTMLMarqueeElement.cpp \
    html/HTMLMenuElement.cpp \
    html/HTMLMetaElement.cpp \
    html/HTMLModElement.cpp \
    html/HTMLNameCollection.cpp \
    html/HTMLObjectElement.cpp \
    html/HTMLOListElement.cpp \
    html/HTMLOptGroupElement.cpp \
    html/HTMLOptionElement.cpp \
    html/HTMLOptionsCollection.cpp \
    html/HTMLParagraphElement.cpp \
    html/HTMLParamElement.cpp \
    html/HTMLParser.cpp \
    html/HTMLParserErrorCodes.cpp \
    html/HTMLPlugInElement.cpp \
    html/HTMLPlugInImageElement.cpp \
    html/HTMLPreElement.cpp \
    html/HTMLQuoteElement.cpp \
    html/HTMLScriptElement.cpp \
    html/HTMLSelectElement.cpp \
    html/HTMLStyleElement.cpp \
    html/HTMLTableCaptionElement.cpp \
    html/HTMLTableCellElement.cpp \
    html/HTMLTableColElement.cpp \
    html/HTMLTableElement.cpp \
    html/HTMLTablePartElement.cpp \
    html/HTMLTableRowElement.cpp \
    html/HTMLTableRowsCollection.cpp \
    html/HTMLTableSectionElement.cpp \
    html/HTMLTextAreaElement.cpp \
    html/HTMLTitleElement.cpp \
    html/HTMLTokenizer.cpp \
    html/HTMLUListElement.cpp \
    html/HTMLViewSourceDocument.cpp \
    html/ISODateTime.cpp \
    html/ImageData.cpp \
    html/PreloadScanner.cpp \
    html/ValidityState.cpp \
    inspector/ConsoleMessage.cpp \
    inspector/InjectedScriptHost.cpp \
    inspector/InspectorBackend.cpp \
    inspector/InspectorController.cpp \
    inspector/InspectorDatabaseResource.cpp \
    inspector/InspectorDOMAgent.cpp \
    inspector/InspectorDOMStorageResource.cpp \
    inspector/InspectorFrontend.cpp \
    inspector/InspectorFrontendHost.cpp \
    inspector/InspectorResource.cpp \
    inspector/InspectorTimelineAgent.cpp \
    inspector/TimelineRecordFactory.cpp \
    loader/archive/ArchiveFactory.cpp \
    loader/archive/ArchiveResource.cpp \
    loader/archive/ArchiveResourceCollection.cpp \
    loader/Cache.cpp \
    loader/CachedCSSStyleSheet.cpp \
    loader/CachedFont.cpp \
    loader/CachedImage.cpp \
    loader/CachedResourceClientWalker.cpp \
    loader/CachedResourceHandle.cpp \
    loader/CachedResource.cpp \
    loader/CachedScript.cpp \
    loader/CachedXSLStyleSheet.cpp \
    loader/CrossOriginAccessControl.cpp \
    loader/CrossOriginPreflightResultCache.cpp \
    loader/DocLoader.cpp \
    loader/DocumentLoader.cpp \
    loader/DocumentThreadableLoader.cpp \
    loader/FormState.cpp \
    loader/FrameLoader.cpp \
    loader/HistoryController.cpp \
    loader/FTPDirectoryDocument.cpp \
    loader/FTPDirectoryParser.cpp \
    loader/icon/IconLoader.cpp \
    loader/ImageDocument.cpp \
    loader/ImageLoader.cpp \
    loader/loader.cpp \
    loader/MainResourceLoader.cpp \
    loader/MediaDocument.cpp \
    loader/NavigationAction.cpp \
    loader/NetscapePlugInStreamLoader.cpp \
    loader/PlaceholderDocument.cpp \
    loader/PluginDocument.cpp \
    loader/PolicyCallback.cpp \
    loader/PolicyChecker.cpp \
    loader/ProgressTracker.cpp \
    loader/RedirectScheduler.cpp \
    loader/Request.cpp \
    loader/ResourceLoader.cpp \
    loader/ResourceLoadNotifier.cpp \
    loader/SubresourceLoader.cpp \
    loader/TextDocument.cpp \
    loader/TextResourceDecoder.cpp \
    loader/ThreadableLoader.cpp \
    page/animation/AnimationBase.cpp \
    page/animation/AnimationController.cpp \
    page/animation/CompositeAnimation.cpp \
    page/animation/ImplicitAnimation.cpp \
    page/animation/KeyframeAnimation.cpp \
    page/BarInfo.cpp \
    page/Chrome.cpp \
    page/Console.cpp \
    page/ContextMenuController.cpp \
    page/DOMSelection.cpp \
    page/DOMTimer.cpp \
    page/DOMWindow.cpp \
    page/Navigator.cpp \
    page/NavigatorBase.cpp \
    page/DragController.cpp \
    page/EventHandler.cpp \
    page/EventSource.cpp \
    page/FocusController.cpp \
    page/Frame.cpp \
    page/FrameTree.cpp \
    page/FrameView.cpp \
    page/Geolocation.cpp \
    page/GeolocationController.cpp \
    page/History.cpp \
    page/Location.cpp \
    page/MouseEventWithHitTestResults.cpp \
    page/OriginAccessEntry.cpp \
    page/Page.cpp \
    page/PageGroup.cpp \
    page/PageGroupLoadDeferrer.cpp \
    page/PluginHalter.cpp \
    page/PrintContext.cpp \
    page/SecurityOrigin.cpp \
    page/Screen.cpp \
    page/Settings.cpp \
    page/UserContentURLPattern.cpp \
    page/WindowFeatures.cpp \
    page/XSSAuditor.cpp \
    plugins/PluginData.cpp \
    plugins/PluginArray.cpp \
    plugins/Plugin.cpp \
    plugins/PluginMainThreadScheduler.cpp \
    plugins/MimeType.cpp \
    plugins/MimeTypeArray.cpp \
    platform/animation/Animation.cpp \
    platform/animation/AnimationList.cpp \
    platform/Arena.cpp \
    platform/text/AtomicString.cpp \
    platform/text/Base64.cpp \
    platform/text/BidiContext.cpp \
    platform/ContentType.cpp \
    platform/ContextMenu.cpp \
    platform/CrossThreadCopier.cpp \
    platform/text/CString.cpp \
    platform/DeprecatedPtrListImpl.cpp \
    platform/DragData.cpp \
    platform/DragImage.cpp \
    platform/FileChooser.cpp \
    platform/GeolocationService.cpp \
    platform/image-decoders/qt/RGBA32BufferQt.cpp \
    platform/graphics/FontDescription.cpp \
    platform/graphics/FontFallbackList.cpp \
    platform/graphics/FontFamily.cpp \
    platform/graphics/BitmapImage.cpp \
    platform/graphics/Color.cpp \
    platform/graphics/FloatPoint3D.cpp \
    platform/graphics/FloatPoint.cpp \
    platform/graphics/FloatQuad.cpp \
    platform/graphics/FloatRect.cpp \
    platform/graphics/FloatSize.cpp \
    platform/graphics/FontData.cpp \
    platform/graphics/Font.cpp \
    platform/graphics/FontCache.cpp \
    platform/graphics/GeneratedImage.cpp \
    platform/graphics/Gradient.cpp \
    platform/graphics/GraphicsContext.cpp \
    platform/graphics/GraphicsTypes.cpp \
    platform/graphics/Image.cpp \
    platform/graphics/ImageBuffer.cpp \
    platform/graphics/ImageSource.cpp \
    platform/graphics/IntRect.cpp \
    platform/graphics/Path.cpp \
    platform/graphics/PathTraversalState.cpp \
    platform/graphics/Pattern.cpp \
    platform/graphics/Pen.cpp \
    platform/graphics/SegmentedFontData.cpp \
    platform/graphics/SimpleFontData.cpp \
    platform/graphics/transforms/TransformationMatrix.cpp \
    platform/graphics/transforms/MatrixTransformOperation.cpp \
    platform/graphics/transforms/Matrix3DTransformOperation.cpp \
    platform/graphics/transforms/PerspectiveTransformOperation.cpp \
    platform/graphics/transforms/RotateTransformOperation.cpp \
    platform/graphics/transforms/ScaleTransformOperation.cpp \
    platform/graphics/transforms/SkewTransformOperation.cpp \
    platform/graphics/transforms/TransformOperations.cpp \
    platform/graphics/transforms/TranslateTransformOperation.cpp \
    platform/KURL.cpp \
    platform/Length.cpp \
    platform/LinkHash.cpp \
    platform/Logging.cpp \
    platform/MIMETypeRegistry.cpp \
    platform/mock/GeolocationServiceMock.cpp \
    platform/network/AuthenticationChallengeBase.cpp \
    platform/network/Credential.cpp \
    platform/network/FormData.cpp \
    platform/network/FormDataBuilder.cpp \
    platform/network/HTTPHeaderMap.cpp \
    platform/network/HTTPParsers.cpp \
    platform/network/NetworkStateNotifier.cpp \
    platform/network/ProtectionSpace.cpp \
    platform/network/ResourceErrorBase.cpp \
    platform/network/ResourceHandle.cpp \
    platform/network/ResourceRequestBase.cpp \
    platform/network/ResourceResponseBase.cpp \
    platform/text/RegularExpression.cpp \
    platform/Scrollbar.cpp \
    platform/ScrollbarThemeComposite.cpp \
    platform/ScrollView.cpp \
    platform/text/SegmentedString.cpp \
    platform/SharedBuffer.cpp \
    platform/text/String.cpp \
    platform/text/StringBuilder.cpp \
    platform/text/StringImpl.cpp \
    platform/text/TextCodec.cpp \
    platform/text/TextCodecLatin1.cpp \
    platform/text/TextCodecUserDefined.cpp \
    platform/text/TextCodecUTF16.cpp \
    platform/text/TextEncoding.cpp \
    platform/text/TextEncodingDetectorNone.cpp \
    platform/text/TextEncodingRegistry.cpp \
    platform/text/TextStream.cpp \
    platform/ThreadGlobalData.cpp \
    platform/ThreadTimers.cpp \
    platform/Timer.cpp \
    platform/text/UnicodeRange.cpp \
    platform/Widget.cpp \
    plugins/PluginDatabase.cpp \
    plugins/PluginDebug.cpp \
    plugins/PluginInfoStore.cpp \
    plugins/PluginPackage.cpp \
    plugins/PluginStream.cpp \
    plugins/PluginView.cpp \
    rendering/AutoTableLayout.cpp \
    rendering/break_lines.cpp \
    rendering/CounterNode.cpp \
    rendering/EllipsisBox.cpp \
    rendering/FixedTableLayout.cpp \
    rendering/HitTestResult.cpp \
    rendering/InlineBox.cpp \
    rendering/InlineFlowBox.cpp \
    rendering/InlineTextBox.cpp \
    rendering/LayoutState.cpp \
    rendering/RenderApplet.cpp \
    rendering/RenderArena.cpp \
    rendering/RenderBlock.cpp \
    rendering/RenderBlockLineLayout.cpp \
    rendering/RenderBox.cpp \
    rendering/RenderBoxModelObject.cpp \
    rendering/RenderBR.cpp \
    rendering/RenderButton.cpp \
    rendering/RenderCounter.cpp \
    rendering/RenderDataGrid.cpp \
    rendering/RenderEmbeddedObject.cpp \
    rendering/RenderFieldset.cpp \
    rendering/RenderFileUploadControl.cpp \
    rendering/RenderFlexibleBox.cpp \
    rendering/RenderFrame.cpp \
    rendering/RenderFrameSet.cpp \
    rendering/RenderHTMLCanvas.cpp \
    rendering/RenderImage.cpp \
    rendering/RenderImageGeneratedContent.cpp \
    rendering/RenderInline.cpp \
    rendering/RenderLayer.cpp \
    rendering/RenderLineBoxList.cpp \
    rendering/RenderListBox.cpp \
    rendering/RenderListItem.cpp \
    rendering/RenderListMarker.cpp \
    rendering/RenderMarquee.cpp \
    rendering/RenderMenuList.cpp \
    rendering/RenderObject.cpp \
    rendering/RenderObjectChildList.cpp \
    rendering/RenderPart.cpp \
    rendering/RenderPartObject.cpp \
    rendering/RenderReplaced.cpp \
    rendering/RenderReplica.cpp \
    rendering/RenderRuby.cpp \
    rendering/RenderRubyBase.cpp \
    rendering/RenderRubyRun.cpp \
    rendering/RenderRubyText.cpp \
    rendering/RenderScrollbar.cpp \
    rendering/RenderScrollbarPart.cpp \
    rendering/RenderScrollbarTheme.cpp \
    rendering/RenderSlider.cpp \
    rendering/RenderTable.cpp \
    rendering/RenderTableCell.cpp \
    rendering/RenderTableCol.cpp \
    rendering/RenderTableRow.cpp \
    rendering/RenderTableSection.cpp \
    rendering/RenderText.cpp \
    rendering/RenderTextControl.cpp \
    rendering/RenderTextControlMultiLine.cpp \
    rendering/RenderTextControlSingleLine.cpp \
    rendering/RenderTextFragment.cpp \
    rendering/RenderTheme.cpp \
    rendering/RenderTreeAsText.cpp \
    rendering/RenderView.cpp \
    rendering/RenderWidget.cpp \
    rendering/RenderWordBreak.cpp \
    rendering/RootInlineBox.cpp \
    rendering/SVGRenderTreeAsText.cpp \
    rendering/ScrollBehavior.cpp \
    rendering/TextControlInnerElements.cpp \
    rendering/TransformState.cpp \
    rendering/style/BindingURI.cpp \
    rendering/style/ContentData.cpp \
    rendering/style/CounterDirectives.cpp \
    rendering/style/FillLayer.cpp \
    rendering/style/KeyframeList.cpp \
    rendering/style/NinePieceImage.cpp \
    rendering/style/RenderStyle.cpp \
    rendering/style/ShadowData.cpp \
    rendering/style/StyleBackgroundData.cpp \
    rendering/style/StyleBoxData.cpp \
    rendering/style/StyleCachedImage.cpp \
    rendering/style/StyleFlexibleBoxData.cpp \
    rendering/style/StyleGeneratedImage.cpp \
    rendering/style/StyleInheritedData.cpp \
    rendering/style/StyleMarqueeData.cpp \
    rendering/style/StyleMultiColData.cpp \
    rendering/style/StyleRareInheritedData.cpp \
    rendering/style/StyleRareNonInheritedData.cpp \
    rendering/style/StyleSurroundData.cpp \
    rendering/style/StyleTransformData.cpp \
    rendering/style/StyleVisualData.cpp \
    xml/DOMParser.cpp \
    xml/XMLHttpRequest.cpp \
    xml/XMLHttpRequestUpload.cpp \
    xml/XMLSerializer.cpp 

HEADERS += \
    accessibility/AccessibilityARIAGridCell.h \
    accessibility/AccessibilityARIAGrid.h \
    accessibility/AccessibilityARIAGridRow.h \
    accessibility/AccessibilityImageMapLink.h \
    accessibility/AccessibilityListBox.h \
    accessibility/AccessibilityListBoxOption.h \
    accessibility/AccessibilityList.h \
    accessibility/AccessibilityMediaControls.h \
    accessibility/AccessibilityObject.h \
    accessibility/AccessibilityRenderObject.h \
    accessibility/AccessibilitySlider.h \
    accessibility/AccessibilityTableCell.h \
    accessibility/AccessibilityTableColumn.h \
    accessibility/AccessibilityTable.h \
    accessibility/AccessibilityTableHeaderContainer.h \
    accessibility/AccessibilityTableRow.h \
    accessibility/AXObjectCache.h \
    bindings/js/CachedScriptSourceProvider.h \
    bindings/js/DOMObjectWithSVGContext.h \
    bindings/js/GCController.h \
    bindings/js/JSCallbackData.h \
    bindings/js/JSAudioConstructor.h \
    bindings/js/JSCSSStyleDeclarationCustom.h \
    bindings/js/JSCustomPositionCallback.h \
    bindings/js/JSCustomPositionErrorCallback.h \
    bindings/js/JSCustomSQLStatementCallback.h \
    bindings/js/JSCustomSQLStatementErrorCallback.h \
    bindings/js/JSCustomSQLTransactionCallback.h \
    bindings/js/JSCustomSQLTransactionErrorCallback.h \
    bindings/js/JSCustomVoidCallback.h \
    bindings/js/JSCustomXPathNSResolver.h \
    bindings/js/JSDataGridDataSource.h \
    bindings/js/JSDOMBinding.h \
    bindings/js/JSDOMGlobalObject.h \
    bindings/js/JSDOMWindowBase.h \
    bindings/js/JSDOMWindowBase.h \
    bindings/js/JSDOMWindowCustom.h \
    bindings/js/JSDOMWindowShell.h \
    bindings/js/JSEventListener.h \
    bindings/js/JSEventSourceConstructor.h \
    bindings/js/JSEventTarget.h \
    bindings/js/JSHistoryCustom.h \
    bindings/js/JSHTMLAppletElementCustom.h \
    bindings/js/JSHTMLEmbedElementCustom.h \
    bindings/js/JSHTMLInputElementCustom.h \
    bindings/js/JSHTMLObjectElementCustom.h \
    bindings/js/JSHTMLSelectElementCustom.h \
    bindings/js/JSImageConstructor.h \
    bindings/js/JSInspectedObjectWrapper.h \
    bindings/js/JSInspectorCallbackWrapper.h \
    bindings/js/JSLazyEventListener.h \
    bindings/js/JSLocationCustom.h \
    bindings/js/JSMessageChannelConstructor.h \
    bindings/js/JSNodeFilterCondition.h \
    bindings/js/JSOptionConstructor.h \
    bindings/js/JSPluginElementFunctions.h \
    bindings/js/JSQuarantinedObjectWrapper.h \
    bindings/js/JSSharedWorkerConstructor.h \
    bindings/js/JSStorageCustom.h \
    bindings/js/JSWebKitCSSMatrixConstructor.h \
    bindings/js/JSWebKitPointConstructor.h \
    bindings/js/JSWorkerConstructor.h \
    bindings/js/JSWorkerContextBase.h \
    bindings/js/JSWorkerContextBase.h \
    bindings/js/JSXMLHttpRequestConstructor.h \
    bindings/js/JSXSLTProcessorConstructor.h \
    bindings/js/ScheduledAction.h \
    bindings/js/ScriptArray.h \
    bindings/js/ScriptCachedFrameData.h \
    bindings/js/ScriptCallFrame.h \
    bindings/js/ScriptCallStack.h \
    bindings/js/ScriptController.h \
    bindings/js/ScriptEventListener.h \
    bindings/js/ScriptFunctionCall.h \
    bindings/js/ScriptObject.h \
    bindings/js/ScriptSourceCode.h \
    bindings/js/ScriptSourceProvider.h \
    bindings/js/ScriptState.h \
    bindings/js/ScriptValue.h \
    bindings/js/SerializedScriptValue.h \
    bindings/js/StringSourceProvider.h \
    bindings/js/WorkerScriptController.h \
    bridge/Bridge.h \
    bridge/c/c_class.h \
    bridge/c/c_instance.h \
    bridge/c/c_runtime.h \
    bridge/c/c_utility.h \
    bridge/IdentifierRep.h \
    bridge/NP_jsobject.h \
    bridge/npruntime.h \
    bridge/qt/qt_class.h \
    bridge/qt/qt_instance.h \
    bridge/qt/qt_runtime.h \
    bridge/runtime_array.h \
    bridge/runtime_method.h \
    bridge/runtime_object.h \
    bridge/runtime_root.h \
    css/CSSBorderImageValue.h \
    css/CSSCanvasValue.h \
    css/CSSCharsetRule.h \
    css/CSSComputedStyleDeclaration.h \
    css/CSSCursorImageValue.h \
    css/CSSFontFace.h \
    css/CSSFontFaceRule.h \
    css/CSSFontFaceSource.h \
    css/CSSFontFaceSrcValue.h \
    css/CSSFontSelector.h \
    css/CSSFunctionValue.h \
    css/CSSGradientValue.h \
    css/CSSHelper.h \
    css/CSSImageGeneratorValue.h \
    css/CSSImageValue.h \
    css/CSSImportRule.h \
    css/CSSInheritedValue.h \
    css/CSSInitialValue.h \
    css/CSSMediaRule.h \
    css/CSSMutableStyleDeclaration.h \
    css/CSSPageRule.h \
    css/CSSParser.h \
    css/CSSParserValues.h \
    css/CSSPrimitiveValue.h \
    css/CSSProperty.h \
    css/CSSPropertyLonghand.h \
    css/CSSReflectValue.h \
    css/CSSRule.h \
    css/CSSRuleList.h \
    css/CSSSegmentedFontFace.h \
    css/CSSSelector.h \
    css/CSSSelectorList.h \
    css/CSSStyleDeclaration.h \
    css/CSSStyleRule.h \
    css/CSSStyleSelector.h \
    css/CSSStyleSheet.h \
    css/CSSTimingFunctionValue.h \
    css/CSSUnicodeRangeValue.h \
    css/CSSValueList.h \
    css/CSSVariableDependentValue.h \
    css/CSSVariablesDeclaration.h \
    css/CSSVariablesRule.h \
    css/FontFamilyValue.h \
    css/FontValue.h \
    css/MediaFeatureNames.h \
    css/Media.h \
    css/MediaList.h \
    css/MediaQueryEvaluator.h \
    css/MediaQueryExp.h \
    css/MediaQuery.h \
    css/RGBColor.h \
    css/ShadowValue.h \
    css/StyleBase.h \
    css/StyleList.h \
    css/StyleSheet.h \
    css/StyleSheetList.h \
    css/WebKitCSSKeyframeRule.h \
    css/WebKitCSSKeyframesRule.h \
    css/WebKitCSSMatrix.h \
    css/WebKitCSSTransformValue.h \
    dom/ActiveDOMObject.h \
    dom/Attr.h \
    dom/Attribute.h \
    dom/BeforeTextInsertedEvent.h \
    dom/BeforeUnloadEvent.h \
    dom/CDATASection.h \
    dom/CharacterData.h \
    dom/CheckedRadioButtons.h \
    dom/ChildNodeList.h \
    dom/ClassNodeList.h \
    dom/ClientRect.h \
    dom/ClientRectList.h \
    dom/ClipboardEvent.h \
    dom/Clipboard.h \
    dom/Comment.h \
    dom/ContainerNode.h \
    dom/CSSMappedAttributeDeclaration.h \
    dom/default/PlatformMessagePortChannel.h \
    dom/DocumentFragment.h \
    dom/Document.h \
    dom/DocumentType.h \
    dom/DOMImplementation.h \
    dom/DynamicNodeList.h \
    dom/EditingText.h \
    dom/Element.h \
    dom/Entity.h \
    dom/EntityReference.h \
    dom/Event.h \
    dom/EventNames.h \
    dom/EventTarget.h \
    dom/ExceptionBase.h \
    dom/ExceptionCode.h \
    dom/InputElement.h \
    dom/KeyboardEvent.h \
    dom/MappedAttribute.h \
    dom/MessageChannel.h \
    dom/MessageEvent.h \
    dom/MessagePortChannel.h \
    dom/MessagePort.h \
    dom/MouseEvent.h \
    dom/MouseRelatedEvent.h \
    dom/MutationEvent.h \
    dom/NamedAttrMap.h \
    dom/NamedMappedAttrMap.h \
    dom/NameNodeList.h \
    dom/NodeFilterCondition.h \
    dom/NodeFilter.h \
    dom/Node.h \
    dom/NodeIterator.h \
    dom/Notation.h \
    dom/OptionElement.h \
    dom/OptionGroupElement.h \
    dom/OverflowEvent.h \
    dom/PageTransitionEvent.h \
    dom/Position.h \
    dom/PositionIterator.h \
    dom/ProcessingInstruction.h \
    dom/ProgressEvent.h \
    dom/QualifiedName.h \
    dom/Range.h \
    dom/RegisteredEventListener.h \
    dom/ScriptElement.h \
    dom/ScriptExecutionContext.h \
    dom/SelectElement.h \
    dom/SelectorNodeList.h \
    dom/SpaceSplitString.h \
    dom/StaticNodeList.h \
    dom/StyledElement.h \
    dom/StyleElement.h \
    dom/TagNodeList.h \
    dom/TextEvent.h \
    dom/Text.h \
    dom/Touch.h \
    dom/TouchEvent.h \
    dom/TouchList.h \
    dom/TransformSource.h \
    dom/Traversal.h \
    dom/TreeWalker.h \
    dom/UIEvent.h \
    dom/UIEventWithKeyState.h \
    dom/WebKitAnimationEvent.h \
    dom/WebKitTransitionEvent.h \
    dom/WheelEvent.h \
    dom/XMLTokenizer.h \
    editing/AppendNodeCommand.h \
    editing/ApplyStyleCommand.h \
    editing/BreakBlockquoteCommand.h \
    editing/CompositeEditCommand.h \
    editing/CreateLinkCommand.h \
    editing/DeleteButtonController.h \
    editing/DeleteButton.h \
    editing/DeleteFromTextNodeCommand.h \
    editing/DeleteSelectionCommand.h \
    editing/EditCommand.h \
    editing/Editor.h \
    editing/FormatBlockCommand.h \
    editing/htmlediting.h \
    editing/HTMLInterchange.h \
    editing/IndentOutdentCommand.h \
    editing/InsertIntoTextNodeCommand.h \
    editing/InsertLineBreakCommand.h \
    editing/InsertListCommand.h \
    editing/InsertNodeBeforeCommand.h \
    editing/InsertParagraphSeparatorCommand.h \
    editing/InsertTextCommand.h \
    editing/JoinTextNodesCommand.h \
    editing/markup.h \
    editing/MergeIdenticalElementsCommand.h \
    editing/ModifySelectionListLevel.h \
    editing/MoveSelectionCommand.h \
    editing/RemoveCSSPropertyCommand.h \
    editing/RemoveFormatCommand.h \
    editing/RemoveNodeCommand.h \
    editing/RemoveNodePreservingChildrenCommand.h \
    editing/ReplaceNodeWithSpanCommand.h \
    editing/ReplaceSelectionCommand.h \
    editing/SelectionController.h \
    editing/SetNodeAttributeCommand.h \
    editing/SmartReplace.h \
    editing/SplitElementCommand.h \
    editing/SplitTextNodeCommand.h \
    editing/SplitTextNodeContainingElementCommand.h \
    editing/TextIterator.h \
    editing/TypingCommand.h \
    editing/UnlinkCommand.h \
    editing/VisiblePosition.h \
    editing/VisibleSelection.h \
    editing/visible_units.h \
    editing/WrapContentsInDummySpanCommand.h \
    history/BackForwardList.h \
    history/CachedFrame.h \
    history/CachedPage.h \
    history/HistoryItem.h \
    history/PageCache.h \
    html/canvas/CanvasGradient.h \
    html/canvas/CanvasPattern.h \
    html/canvas/CanvasPixelArray.h \
    html/canvas/CanvasRenderingContext.h \
    html/canvas/CanvasRenderingContext2D.h \
    html/canvas/CanvasStyle.h \
    html/CollectionCache.h \
    html/DataGridColumn.h \
    html/DataGridColumnList.h \
    html/DOMDataGridDataSource.h \
    html/File.h \
    html/FileList.h \
    html/FormDataList.h \
    html/HTMLAllCollection.h \
    html/HTMLAnchorElement.h \
    html/HTMLAppletElement.h \
    html/HTMLAreaElement.h \
    html/HTMLAudioElement.h \
    html/HTMLBaseElement.h \
    html/HTMLBaseFontElement.h \
    html/HTMLBlockquoteElement.h \
    html/HTMLBodyElement.h \
    html/HTMLBRElement.h \
    html/HTMLButtonElement.h \
    html/HTMLCanvasElement.h \
    html/HTMLCollection.h \
    html/HTMLDataGridCellElement.h \
    html/HTMLDataGridColElement.h \
    html/HTMLDataGridElement.h \
    html/HTMLDataGridRowElement.h \
    html/HTMLDirectoryElement.h \
    html/HTMLDivElement.h \
    html/HTMLDListElement.h \
    html/HTMLDocument.h \
    html/HTMLElement.h \
    html/HTMLEmbedElement.h \
    html/HTMLFieldSetElement.h \
    html/HTMLFontElement.h \
    html/HTMLFormCollection.h \
    html/HTMLFormControlElement.h \
    html/HTMLFormElement.h \
    html/HTMLFrameElementBase.h \
    html/HTMLFrameElement.h \
    html/HTMLFrameOwnerElement.h \
    html/HTMLFrameSetElement.h \
    html/HTMLHeadElement.h \
    html/HTMLHeadingElement.h \
    html/HTMLHRElement.h \
    html/HTMLHtmlElement.h \
    html/HTMLIFrameElement.h \
    html/HTMLImageElement.h \
    html/HTMLImageLoader.h \
    html/HTMLInputElement.h \
    html/HTMLIsIndexElement.h \
    html/HTMLKeygenElement.h \
    html/HTMLLabelElement.h \
    html/HTMLLegendElement.h \
    html/HTMLLIElement.h \
    html/HTMLLinkElement.h \
    html/HTMLMapElement.h \
    html/HTMLMarqueeElement.h \
    html/HTMLMediaElement.h \
    html/HTMLMenuElement.h \
    html/HTMLMetaElement.h \
    html/HTMLModElement.h \
    html/HTMLNameCollection.h \
    html/HTMLNoScriptElement.h \
    html/HTMLObjectElement.h \
    html/HTMLOListElement.h \
    html/HTMLOptGroupElement.h \
    html/HTMLOptionElement.h \
    html/HTMLOptionsCollection.h \
    html/HTMLParagraphElement.h \
    html/HTMLParamElement.h \
    html/HTMLParserErrorCodes.h \
    html/HTMLParser.h \
    html/HTMLPlugInElement.h \
    html/HTMLPlugInImageElement.h \
    html/HTMLPreElement.h \
    html/HTMLQuoteElement.h \
    html/HTMLScriptElement.h \
    html/HTMLSelectElement.h \
    html/HTMLSourceElement.h \
    html/HTMLStyleElement.h \
    html/HTMLTableCaptionElement.h \
    html/HTMLTableCellElement.h \
    html/HTMLTableColElement.h \
    html/HTMLTableElement.h \
    html/HTMLTablePartElement.h \
    html/HTMLTableRowElement.h \
    html/HTMLTableRowsCollection.h \
    html/HTMLTableSectionElement.h \
    html/HTMLTextAreaElement.h \
    html/HTMLTitleElement.h \
    html/HTMLTokenizer.h \
    html/HTMLUListElement.h \
    html/HTMLVideoElement.h \
    html/HTMLViewSourceDocument.h \
    html/ISODateTime.h \
    html/ImageData.h \
    html/PreloadScanner.h \
    html/TimeRanges.h \
    html/ValidityState.h \
    inspector/ConsoleMessage.h \
    inspector/InjectedScriptHost.h \
    inspector/InspectorBackend.h \
    inspector/InspectorController.h \
    inspector/InspectorDatabaseResource.h \
    inspector/InspectorDOMStorageResource.h \
    inspector/InspectorFrontend.h \
    inspector/InspectorFrontendHost.h \
    inspector/InspectorResource.h \
    inspector/InspectorTimelineAgent.h \
    inspector/JavaScriptCallFrame.h \
    inspector/JavaScriptDebugServer.h \
    inspector/JavaScriptProfile.h \
    inspector/JavaScriptProfileNode.h \
    inspector/TimelineRecordFactory.h \
    loader/appcache/ApplicationCacheGroup.h \
    loader/appcache/ApplicationCacheHost.h \
    loader/appcache/ApplicationCache.h \
    loader/appcache/ApplicationCacheResource.h \
    loader/appcache/ApplicationCacheStorage.h \
    loader/appcache/DOMApplicationCache.h \
    loader/appcache/ManifestParser.h \
    loader/archive/ArchiveFactory.h \
    loader/archive/ArchiveResourceCollection.h \
    loader/archive/ArchiveResource.h \
    loader/CachedCSSStyleSheet.h \
    loader/CachedFont.h \
    loader/CachedImage.h \
    loader/CachedResourceClientWalker.h \
    loader/CachedResource.h \
    loader/CachedResourceHandle.h \
    loader/CachedScript.h \
    loader/CachedXSLStyleSheet.h \
    loader/Cache.h \
    loader/CrossOriginAccessControl.h \
    loader/CrossOriginPreflightResultCache.h \
    loader/DocLoader.h \
    loader/DocumentLoader.h \
    loader/DocumentThreadableLoader.h \
    loader/FormState.h \
    loader/FrameLoader.h \
    loader/FTPDirectoryDocument.h \
    loader/FTPDirectoryParser.h \
    loader/icon/IconDatabase.h \
    loader/icon/IconLoader.h \
    loader/icon/IconRecord.h \
    loader/icon/PageURLRecord.h \
    loader/ImageDocument.h \
    loader/ImageLoader.h \
    loader/loader.h \
    loader/MainResourceLoader.h \
    loader/MediaDocument.h \
    loader/NavigationAction.h \
    loader/NetscapePlugInStreamLoader.h \
    loader/PlaceholderDocument.h \
    loader/PluginDocument.h \
    loader/ProgressTracker.h \
    loader/Request.h \
    loader/ResourceLoader.h \
    loader/SubresourceLoader.h \
    loader/TextDocument.h \
    loader/TextResourceDecoder.h \
    loader/ThreadableLoader.h \
    loader/WorkerThreadableLoader.h \
    page/animation/AnimationBase.h \
    page/animation/AnimationController.h \
    page/animation/CompositeAnimation.h \
    page/animation/ImplicitAnimation.h \
    page/animation/KeyframeAnimation.h \
    page/BarInfo.h \
    page/Chrome.h \
    page/Console.h \
    page/ContextMenuController.h \
    page/ContextMenuProvider.h \
    page/Coordinates.h \
    page/DOMSelection.h \
    page/DOMTimer.h \
    page/DOMWindow.h \
    page/DragController.h \
    page/EventHandler.h \
    page/EventSource.h \
    page/FocusController.h \
    page/Frame.h \
    page/FrameTree.h \
    page/FrameView.h \
    page/Geolocation.h \
    page/Geoposition.h \
    page/HaltablePlugin.h \
    page/History.h \
    page/Location.h \
    page/MouseEventWithHitTestResults.h \
    page/NavigatorBase.h \
    page/Navigator.h \
    page/PageGroup.h \
    page/PageGroupLoadDeferrer.h \
    page/Page.h \
    page/PluginHalter.h \
    page/PluginHalterClient.h \
    page/PrintContext.h \
    page/Screen.h \
    page/SecurityOrigin.h \
    page/Settings.h \
    page/WindowFeatures.h \
    page/WorkerNavigator.h \
    page/XSSAuditor.h \
    platform/animation/Animation.h \
    platform/animation/AnimationList.h \
    platform/Arena.h \
    platform/ContentType.h \
    platform/ContextMenu.h \
    platform/CrossThreadCopier.h \
    platform/DeprecatedPtrListImpl.h \
    platform/DragData.h \
    platform/DragImage.h \
    platform/FileChooser.h \
    platform/GeolocationService.h \
    platform/image-decoders/ImageDecoder.h \
    platform/mock/GeolocationServiceMock.h \
    platform/graphics/BitmapImage.h \
    platform/graphics/Color.h \
    platform/graphics/filters/FEBlend.h \
    platform/graphics/filters/FEColorMatrix.h \
    platform/graphics/filters/FEComponentTransfer.h \
    platform/graphics/filters/FEComposite.h \
    platform/graphics/filters/FEGaussianBlur.h \
    platform/graphics/filters/FilterEffect.h \
    platform/graphics/filters/SourceAlpha.h \
    platform/graphics/filters/SourceGraphic.h \
    platform/graphics/FloatPoint3D.h \
    platform/graphics/FloatPoint.h \
    platform/graphics/FloatQuad.h \
    platform/graphics/FloatRect.h \
    platform/graphics/FloatSize.h \
    platform/graphics/FontData.h \
    platform/graphics/FontDescription.h \
    platform/graphics/FontFamily.h \
    platform/graphics/Font.h \
    platform/graphics/GeneratedImage.h \
    platform/graphics/Gradient.h \
    platform/graphics/GraphicsContext.h \
    platform/graphics/GraphicsTypes.h \
    platform/graphics/Image.h \
    platform/graphics/ImageSource.h \
    platform/graphics/IntRect.h \
    platform/graphics/MediaPlayer.h \
    platform/graphics/Path.h \
    platform/graphics/PathTraversalState.h \
    platform/graphics/Pattern.h \
    platform/graphics/Pen.h \
    platform/graphics/qt/FontCustomPlatformData.h \
    platform/graphics/qt/ImageDecoderQt.h \
    platform/graphics/qt/StillImageQt.h \
    platform/graphics/SegmentedFontData.h \
    platform/graphics/SimpleFontData.h \
    platform/graphics/transforms/Matrix3DTransformOperation.h \
    platform/graphics/transforms/MatrixTransformOperation.h \
    platform/graphics/transforms/PerspectiveTransformOperation.h \
    platform/graphics/transforms/RotateTransformOperation.h \
    platform/graphics/transforms/ScaleTransformOperation.h \
    platform/graphics/transforms/SkewTransformOperation.h \
    platform/graphics/transforms/TransformationMatrix.h \
    platform/graphics/transforms/TransformOperations.h \
    platform/graphics/transforms/TranslateTransformOperation.h \
    platform/KURL.h \
    platform/Length.h \
    platform/LinkHash.h \
    platform/Logging.h \
    platform/MIMETypeRegistry.h \
    platform/network/AuthenticationChallengeBase.h \
    platform/network/AuthenticationClient.h \
    platform/network/Credential.h \
    platform/network/FormDataBuilder.h \
    platform/network/FormData.h \
    platform/network/HTTPHeaderMap.h \
    platform/network/HTTPParsers.h \
    platform/network/NetworkStateNotifier.h \
    platform/network/ProtectionSpace.h \
    platform/network/qt/QNetworkReplyHandler.h \
    platform/network/ResourceErrorBase.h \
    platform/network/ResourceHandle.h \
    platform/network/ResourceRequestBase.h \
    platform/network/ResourceResponseBase.h \
    platform/PlatformTouchEvent.h \
    platform/PlatformTouchPoint.h \
    platform/qt/ClipboardQt.h \
    platform/qt/QWebPageClient.h \
    platform/qt/QtAbstractWebPopup.h \
    platform/qt/RenderThemeQt.h \
    platform/qt/ScrollbarThemeQt.h \
    platform/Scrollbar.h \
    platform/ScrollbarThemeComposite.h \
    platform/ScrollView.h \
    platform/SharedBuffer.h \
    platform/sql/SQLiteDatabase.h \
    platform/sql/SQLiteFileSystem.h \
    platform/sql/SQLiteStatement.h \
    platform/sql/SQLiteTransaction.h \
    platform/sql/SQLValue.h \
    platform/text/AtomicString.h \
    platform/text/Base64.h \
    platform/text/BidiContext.h \
    platform/text/CString.h \
    platform/text/qt/TextCodecQt.h \
    platform/text/RegularExpression.h \
    platform/text/SegmentedString.h \
    platform/text/StringBuilder.h \
    platform/text/StringImpl.h \
    platform/text/TextCodec.h \
    platform/text/TextCodecLatin1.h \
    platform/text/TextCodecUserDefined.h \
    platform/text/TextCodecUTF16.h \
    platform/text/TextEncoding.h \
    platform/text/TextEncodingRegistry.h \
    platform/text/TextStream.h \
    platform/text/UnicodeRange.h \
    platform/ThreadGlobalData.h \
    platform/ThreadTimers.h \
    platform/Timer.h \
    platform/Widget.h \
    plugins/MimeTypeArray.h \
    plugins/MimeType.h \
    plugins/PluginArray.h \
    plugins/PluginDatabase.h \
    plugins/PluginData.h \
    plugins/PluginDebug.h \
    plugins/Plugin.h \
    plugins/PluginInfoStore.h \
    plugins/PluginMainThreadScheduler.h \
    plugins/PluginPackage.h \
    plugins/PluginStream.h \
    plugins/PluginView.h \
    plugins/win/PluginMessageThrottlerWin.h \
    rendering/AutoTableLayout.h \
    rendering/break_lines.h \
    rendering/CounterNode.h \
    rendering/EllipsisBox.h \
    rendering/FixedTableLayout.h \
    rendering/HitTestResult.h \
    rendering/InlineBox.h \
    rendering/InlineFlowBox.h \
    rendering/InlineTextBox.h \
    rendering/LayoutState.h \
    rendering/MediaControlElements.h \
    rendering/PointerEventsHitRules.h \
    rendering/RenderApplet.h \
    rendering/RenderArena.h \
    rendering/RenderBlock.h \
    rendering/RenderBox.h \
    rendering/RenderBoxModelObject.h \
    rendering/RenderBR.h \
    rendering/RenderButton.h \
    rendering/RenderCounter.h \
    rendering/RenderDataGrid.h \
    rendering/RenderEmbeddedObject.h \
    rendering/RenderFieldset.h \
    rendering/RenderFileUploadControl.h \
    rendering/RenderFlexibleBox.h \
    rendering/RenderForeignObject.h \
    rendering/RenderFrame.h \
    rendering/RenderFrameSet.h \
    rendering/RenderHTMLCanvas.h \
    rendering/RenderImageGeneratedContent.h \
    rendering/RenderImage.h \
    rendering/RenderInline.h \
    rendering/RenderLayer.h \
    rendering/RenderLineBoxList.h \
    rendering/RenderListBox.h \
    rendering/RenderListItem.h \
    rendering/RenderListMarker.h \
    rendering/RenderMarquee.h \
    rendering/RenderMedia.h \
    rendering/RenderMenuList.h \
    rendering/RenderObjectChildList.h \
    rendering/RenderObject.h \
    rendering/RenderPart.h \
    rendering/RenderPartObject.h \
    rendering/RenderPath.h \
    rendering/RenderReplaced.h \
    rendering/RenderReplica.h \
    rendering/RenderRuby.h \
    rendering/RenderRubyBase.h \
    rendering/RenderRubyRun.h \
    rendering/RenderRubyText.h \
    rendering/RenderScrollbar.h \
    rendering/RenderScrollbarPart.h \
    rendering/RenderScrollbarTheme.h \
    rendering/RenderSlider.h \
    rendering/RenderSVGBlock.h \
    rendering/RenderSVGContainer.h \
    rendering/RenderSVGGradientStop.h \
    rendering/RenderSVGHiddenContainer.h \
    rendering/RenderSVGImage.h \
    rendering/RenderSVGInline.h \
    rendering/RenderSVGInlineText.h \
    rendering/RenderSVGModelObject.h \
    rendering/RenderSVGRoot.h \
    rendering/RenderSVGShadowTreeRootContainer.h \
    rendering/RenderSVGText.h \
    rendering/RenderSVGTextPath.h \
    rendering/RenderSVGTransformableContainer.h \
    rendering/RenderSVGTSpan.h \
    rendering/RenderSVGViewportContainer.h \
    rendering/RenderTableCell.h \
    rendering/RenderTableCol.h \
    rendering/RenderTable.h \
    rendering/RenderTableRow.h \
    rendering/RenderTableSection.h \
    rendering/RenderTextControl.h \
    rendering/RenderTextControlMultiLine.h \
    rendering/RenderTextControlSingleLine.h \
    rendering/RenderTextFragment.h \
    rendering/RenderText.h \
    rendering/RenderTheme.h \
    rendering/RenderTreeAsText.h \
    rendering/RenderVideo.h \
    rendering/RenderView.h \
    rendering/RenderWidget.h \
    rendering/RenderWordBreak.h \
    rendering/RootInlineBox.h \
    rendering/ScrollBehavior.h \
    rendering/style/BindingURI.h \
    rendering/style/ContentData.h \
    rendering/style/CounterDirectives.h \
    rendering/style/CursorData.h \
    rendering/style/CursorList.h \
    rendering/style/FillLayer.h \
    rendering/style/KeyframeList.h \
    rendering/style/NinePieceImage.h \
    rendering/style/RenderStyle.h \
    rendering/style/ShadowData.h \
    rendering/style/StyleBackgroundData.h \
    rendering/style/StyleBoxData.h \
    rendering/style/StyleCachedImage.h \
    rendering/style/StyleFlexibleBoxData.h \
    rendering/style/StyleGeneratedImage.h \
    rendering/style/StyleInheritedData.h \
    rendering/style/StyleMarqueeData.h \
    rendering/style/StyleMultiColData.h \
    rendering/style/StyleRareInheritedData.h \
    rendering/style/StyleRareNonInheritedData.h \
    rendering/style/StyleReflection.h \
    rendering/style/StyleSurroundData.h \
    rendering/style/StyleTransformData.h \
    rendering/style/StyleVisualData.h \
    rendering/style/SVGRenderStyleDefs.h \
    rendering/style/SVGRenderStyle.h \
    rendering/SVGCharacterLayoutInfo.h \
    rendering/SVGInlineFlowBox.h \
    rendering/SVGInlineTextBox.h \
    rendering/SVGMarkerData.h \
    rendering/SVGMarkerLayoutInfo.h \
    rendering/SVGRenderSupport.h \
    rendering/SVGRenderTreeAsText.h \
    rendering/SVGRootInlineBox.h \
    rendering/SVGShadowTreeElements.h \
    rendering/TextControlInnerElements.h \
    rendering/TransformState.h \
    svg/animation/SMILTimeContainer.h \
    svg/animation/SMILTime.h \
    svg/animation/SVGSMILElement.h \
    svg/ColorDistance.h \
    svg/graphics/filters/SVGFEConvolveMatrix.h \
    svg/graphics/filters/SVGFEDiffuseLighting.h \
    svg/graphics/filters/SVGFEDisplacementMap.h \
    svg/graphics/filters/SVGFEFlood.h \
    svg/graphics/filters/SVGFEImage.h \
    svg/graphics/filters/SVGFEMerge.h \
    svg/graphics/filters/SVGFEMorphology.h \
    svg/graphics/filters/SVGFEOffset.h \
    svg/graphics/filters/SVGFESpecularLighting.h \
    svg/graphics/filters/SVGFETile.h \
    svg/graphics/filters/SVGFETurbulence.h \
    svg/graphics/filters/SVGFilterBuilder.h \
    svg/graphics/filters/SVGFilter.h \
    svg/graphics/filters/SVGLightSource.h \
    svg/graphics/SVGImage.h \
    svg/graphics/SVGPaintServerGradient.h \
    svg/graphics/SVGPaintServer.h \
    svg/graphics/SVGPaintServerLinearGradient.h \
    svg/graphics/SVGPaintServerPattern.h \
    svg/graphics/SVGPaintServerRadialGradient.h \
    svg/graphics/SVGPaintServerSolid.h \
    svg/graphics/SVGResourceClipper.h \
    svg/graphics/SVGResourceFilter.h \
    svg/graphics/SVGResource.h \
    svg/graphics/SVGResourceMarker.h \
    svg/graphics/SVGResourceMasker.h \
    svg/SVGAElement.h \
    svg/SVGAltGlyphElement.h \
    svg/SVGAngle.h \
    svg/SVGAnimateColorElement.h \
    svg/SVGAnimatedPathData.h \
    svg/SVGAnimatedPoints.h \
    svg/SVGAnimateElement.h \
    svg/SVGAnimateMotionElement.h \
    svg/SVGAnimateTransformElement.h \
    svg/SVGAnimationElement.h \
    svg/SVGCircleElement.h \
    svg/SVGClipPathElement.h \
    svg/SVGColor.h \
    svg/SVGComponentTransferFunctionElement.h \
    svg/SVGCursorElement.h \
    svg/SVGDefsElement.h \
    svg/SVGDescElement.h \
    svg/SVGDocumentExtensions.h \
    svg/SVGDocument.h \
    svg/SVGElement.h \
    svg/SVGElementInstance.h \
    svg/SVGElementInstanceList.h \
    svg/SVGEllipseElement.h \
    svg/SVGExternalResourcesRequired.h \
    svg/SVGFEBlendElement.h \
    svg/SVGFEColorMatrixElement.h \
    svg/SVGFEComponentTransferElement.h \
    svg/SVGFECompositeElement.h \
    svg/SVGFEDiffuseLightingElement.h \
    svg/SVGFEDisplacementMapElement.h \
    svg/SVGFEDistantLightElement.h \
    svg/SVGFEFloodElement.h \
    svg/SVGFEFuncAElement.h \
    svg/SVGFEFuncBElement.h \
    svg/SVGFEFuncGElement.h \
    svg/SVGFEFuncRElement.h \
    svg/SVGFEGaussianBlurElement.h \
    svg/SVGFEImageElement.h \
    svg/SVGFELightElement.h \
    svg/SVGFEMergeElement.h \
    svg/SVGFEMergeNodeElement.h \
    svg/SVGFEMorphologyElement.h \
    svg/SVGFEOffsetElement.h \
    svg/SVGFEPointLightElement.h \
    svg/SVGFESpecularLightingElement.h \
    svg/SVGFESpotLightElement.h \
    svg/SVGFETileElement.h \
    svg/SVGFETurbulenceElement.h \
    svg/SVGFilterElement.h \
    svg/SVGFilterPrimitiveStandardAttributes.h \
    svg/SVGFitToViewBox.h \
    svg/SVGFontData.h \
    svg/SVGFontElement.h \
    svg/SVGFontFaceElement.h \
    svg/SVGFontFaceFormatElement.h \
    svg/SVGFontFaceNameElement.h \
    svg/SVGFontFaceSrcElement.h \
    svg/SVGFontFaceUriElement.h \
    svg/SVGForeignObjectElement.h \
    svg/SVGGElement.h \
    svg/SVGGlyphElement.h \
    svg/SVGGradientElement.h \
    svg/SVGHKernElement.h \
    svg/SVGImageElement.h \
    svg/SVGImageLoader.h \
    svg/SVGLangSpace.h \
    svg/SVGLength.h \
    svg/SVGLengthList.h \
    svg/SVGLinearGradientElement.h \
    svg/SVGLineElement.h \
    svg/SVGLocatable.h \
    svg/SVGMarkerElement.h \
    svg/SVGMaskElement.h \
    svg/SVGMetadataElement.h \
    svg/SVGMissingGlyphElement.h \
    svg/SVGMPathElement.h \
    svg/SVGNumberList.h \
    svg/SVGPaint.h \
    svg/SVGParserUtilities.h \
    svg/SVGPathElement.h \
    svg/SVGPathSegArc.h \
    svg/SVGPathSegClosePath.h \
    svg/SVGPathSegCurvetoCubic.h \
    svg/SVGPathSegCurvetoCubicSmooth.h \
    svg/SVGPathSegCurvetoQuadratic.h \
    svg/SVGPathSegCurvetoQuadraticSmooth.h \
    svg/SVGPathSegLineto.h \
    svg/SVGPathSegLinetoHorizontal.h \
    svg/SVGPathSegLinetoVertical.h \
    svg/SVGPathSegList.h \
    svg/SVGPathSegMoveto.h \
    svg/SVGPatternElement.h \
    svg/SVGPointList.h \
    svg/SVGPolyElement.h \
    svg/SVGPolygonElement.h \
    svg/SVGPolylineElement.h \
    svg/SVGPreserveAspectRatio.h \
    svg/SVGRadialGradientElement.h \
    svg/SVGRectElement.h \
    svg/SVGScriptElement.h \
    svg/SVGSetElement.h \
    svg/SVGStopElement.h \
    svg/SVGStringList.h \
    svg/SVGStylable.h \
    svg/SVGStyledElement.h \
    svg/SVGStyledLocatableElement.h \
    svg/SVGStyledTransformableElement.h \
    svg/SVGStyleElement.h \
    svg/SVGSVGElement.h \
    svg/SVGSwitchElement.h \
    svg/SVGSymbolElement.h \
    svg/SVGTests.h \
    svg/SVGTextContentElement.h \
    svg/SVGTextElement.h \
    svg/SVGTextPathElement.h \
    svg/SVGTextPositioningElement.h \
    svg/SVGTitleElement.h \
    svg/SVGTransformable.h \
    svg/SVGTransformDistance.h \
    svg/SVGTransform.h \
    svg/SVGTransformList.h \
    svg/SVGTRefElement.h \
    svg/SVGTSpanElement.h \
    svg/SVGURIReference.h \
    svg/SVGUseElement.h \
    svg/SVGViewElement.h \
    svg/SVGViewSpec.h \
    svg/SVGZoomAndPan.h \
    svg/SVGZoomEvent.h \
    svg/SynchronizablePropertyController.h \
    wml/WMLAccessElement.h \
    wml/WMLAElement.h \
    wml/WMLAnchorElement.h \
    wml/WMLBRElement.h \
    wml/WMLCardElement.h \
    wml/WMLDocument.h \
    wml/WMLDoElement.h \
    wml/WMLElement.h \
    wml/WMLErrorHandling.h \
    wml/WMLEventHandlingElement.h \
    wml/WMLFieldSetElement.h \
    wml/WMLFormControlElement.h \
    wml/WMLGoElement.h \
    wml/WMLImageElement.h \
    wml/WMLImageLoader.h \
    wml/WMLInputElement.h \
    wml/WMLInsertedLegendElement.h \
    wml/WMLIntrinsicEvent.h \
    wml/WMLIntrinsicEventHandler.h \
    wml/WMLMetaElement.h \
    wml/WMLNoopElement.h \
    wml/WMLOnEventElement.h \
    wml/WMLOptGroupElement.h \
    wml/WMLOptionElement.h \
    wml/WMLPageState.h \
    wml/WMLPElement.h \
    wml/WMLPostfieldElement.h \
    wml/WMLPrevElement.h \
    wml/WMLRefreshElement.h \
    wml/WMLSelectElement.h \
    wml/WMLSetvarElement.h \
    wml/WMLTableElement.h \
    wml/WMLTaskElement.h \
    wml/WMLTemplateElement.h \
    wml/WMLTimerElement.h \
    wml/WMLVariables.h \
    workers/AbstractWorker.h \
    workers/DedicatedWorkerContext.h \
    workers/DedicatedWorkerThread.h \
    workers/SharedWorker.h \
    workers/WorkerContext.h \
    workers/Worker.h \
    workers/WorkerLocation.h \
    workers/WorkerMessagingProxy.h \
    workers/WorkerRunLoop.h \
    workers/WorkerScriptLoader.h \
    workers/WorkerThread.h \
    xml/DOMParser.h \
    xml/NativeXPathNSResolver.h \
    xml/XMLHttpRequest.h \
    xml/XMLHttpRequestUpload.h \
    xml/XMLSerializer.h \
    xml/XPathEvaluator.h \
    xml/XPathExpression.h \
    xml/XPathExpressionNode.h \
    xml/XPathFunctions.h \
    xml/XPathNamespace.h \
    xml/XPathNodeSet.h \
    xml/XPathNSResolver.h \
    xml/XPathParser.h \
    xml/XPathPath.h \
    xml/XPathPredicate.h \
    xml/XPathResult.h \
    xml/XPathStep.h \
    xml/XPathUtil.h \
    xml/XPathValue.h \
    xml/XPathVariableReference.h \
    xml/XSLImportRule.h \
    xml/XSLStyleSheet.h \
    xml/XSLTExtensions.h \
    xml/XSLTProcessor.h \
    xml/XSLTUnicodeSort.h \
    $$PWD/../WebKit/qt/Api/qwebplugindatabase_p.h \
    $$PWD/../WebKit/qt/WebCoreSupport/QtFallbackWebPopup.h \
    $$PWD/../WebKit/qt/WebCoreSupport/FrameLoaderClientQt.h \
    $$PWD/platform/network/qt/DnsPrefetchHelper.h

SOURCES += \
    accessibility/qt/AccessibilityObjectQt.cpp \
    bindings/js/ScriptControllerQt.cpp \
    bridge/qt/qt_class.cpp \
    bridge/qt/qt_instance.cpp \
    bridge/qt/qt_runtime.cpp \
    page/qt/DragControllerQt.cpp \
    page/qt/EventHandlerQt.cpp \
    page/qt/FrameQt.cpp \
    platform/graphics/qt/TransformationMatrixQt.cpp \
    platform/graphics/qt/ColorQt.cpp \
    platform/graphics/qt/FontQt.cpp \
    platform/graphics/qt/FontPlatformDataQt.cpp \
    platform/graphics/qt/FloatPointQt.cpp \
    platform/graphics/qt/FloatRectQt.cpp \
    platform/graphics/qt/GradientQt.cpp \
    platform/graphics/qt/GraphicsContextQt.cpp \
    platform/graphics/qt/IconQt.cpp \
    platform/graphics/qt/ImageBufferQt.cpp \
    platform/graphics/qt/ImageDecoderQt.cpp \
    platform/graphics/qt/ImageQt.cpp \
    platform/graphics/qt/IntPointQt.cpp \
    platform/graphics/qt/IntRectQt.cpp \
    platform/graphics/qt/IntSizeQt.cpp \
    platform/graphics/qt/PathQt.cpp \
    platform/graphics/qt/PatternQt.cpp \
    platform/graphics/qt/StillImageQt.cpp \
    platform/network/qt/ResourceHandleQt.cpp \
    platform/network/qt/ResourceRequestQt.cpp \
    platform/network/qt/DnsPrefetchHelper.cpp \
    platform/network/qt/QNetworkReplyHandler.cpp \
    editing/qt/EditorQt.cpp \
    platform/qt/ClipboardQt.cpp \
    platform/qt/ContextMenuItemQt.cpp \
    platform/qt/ContextMenuQt.cpp \
    platform/qt/CookieJarQt.cpp \
    platform/qt/CursorQt.cpp \
    platform/qt/DragDataQt.cpp \
    platform/qt/DragImageQt.cpp \
    platform/qt/EventLoopQt.cpp \
    platform/qt/FileChooserQt.cpp \
    platform/qt/FileSystemQt.cpp \
    platform/qt/SharedBufferQt.cpp \
    platform/graphics/qt/FontCacheQt.cpp \
    platform/graphics/qt/FontCustomPlatformData.cpp \
    platform/graphics/qt/GlyphPageTreeNodeQt.cpp \
    platform/graphics/qt/SimpleFontDataQt.cpp \
    platform/qt/KURLQt.cpp \
    platform/qt/Localizations.cpp \
    platform/qt/MIMETypeRegistryQt.cpp \
    platform/qt/PasteboardQt.cpp \
    platform/qt/PlatformKeyboardEventQt.cpp \
    platform/qt/PlatformMouseEventQt.cpp \
    platform/qt/PlatformScreenQt.cpp \
    platform/qt/PlatformTouchEventQt.cpp \
    platform/qt/PlatformTouchPointQt.cpp \
    platform/qt/PopupMenuQt.cpp \
    platform/qt/QtAbstractWebPopup.cpp \
    platform/qt/RenderThemeQt.cpp \
    platform/qt/ScrollbarQt.cpp \
    platform/qt/ScrollbarThemeQt.cpp \
    platform/qt/ScrollViewQt.cpp \
    platform/qt/SearchPopupMenuQt.cpp \
    platform/qt/SharedTimerQt.cpp \
    platform/qt/SoundQt.cpp \
    platform/qt/LoggingQt.cpp \
    platform/text/qt/StringQt.cpp \
    platform/qt/TemporaryLinkStubs.cpp \
    platform/text/qt/TextBoundariesQt.cpp \
    platform/text/qt/TextBreakIteratorQt.cpp \
    platform/text/qt/TextCodecQt.cpp \
    platform/qt/WheelEventQt.cpp \
    platform/qt/WidgetQt.cpp \
    plugins/qt/PluginDataQt.cpp \
    ../WebKit/qt/WebCoreSupport/QtFallbackWebPopup.cpp \
    ../WebKit/qt/WebCoreSupport/ChromeClientQt.cpp \
    ../WebKit/qt/WebCoreSupport/ContextMenuClientQt.cpp \
    ../WebKit/qt/WebCoreSupport/DragClientQt.cpp \
    ../WebKit/qt/WebCoreSupport/EditorClientQt.cpp \
    ../WebKit/qt/WebCoreSupport/EditCommandQt.cpp \
    ../WebKit/qt/WebCoreSupport/FrameLoaderClientQt.cpp \
    ../WebKit/qt/WebCoreSupport/InspectorClientQt.cpp \
    ../WebKit/qt/Api/qwebframe.cpp \
    ../WebKit/qt/Api/qgraphicswebview.cpp \
    ../WebKit/qt/Api/qwebpage.cpp \
    ../WebKit/qt/Api/qwebview.cpp \
    ../WebKit/qt/Api/qwebelement.cpp \
    ../WebKit/qt/Api/qwebhistory.cpp \
    ../WebKit/qt/Api/qwebsettings.cpp \
    ../WebKit/qt/Api/qwebhistoryinterface.cpp \
    ../WebKit/qt/Api/qwebplugindatabase.cpp \
    ../WebKit/qt/Api/qwebpluginfactory.cpp \
    ../WebKit/qt/Api/qwebsecurityorigin.cpp \
    ../WebKit/qt/Api/qwebdatabase.cpp \
    ../WebKit/qt/Api/qwebinspector.cpp \
    ../WebKit/qt/Api/qwebkitversion.cpp


    win32-*|wince*: SOURCES += platform/win/SystemTimeWin.cpp \
                               platform/graphics/win/TransformationMatrixWin.cpp

    mac {
        SOURCES += \
            platform/text/cf/StringCF.cpp \
            platform/text/cf/StringImplCF.cpp \
            platform/cf/SharedBufferCF.cpp \
            editing/SmartReplaceCF.cpp
        LIBS_PRIVATE += -framework Carbon -framework AppKit
    }

    win32-* {
        LIBS += -lgdi32
        LIBS += -luser32
        LIBS += -lwinmm
    }
    wince*: LIBS += -lmmtimer

contains(DEFINES, ENABLE_NETSCAPE_PLUGIN_API=1) {

    SOURCES += plugins/npapi.cpp

    symbian {
        SOURCES += \
        plugins/symbian/PluginPackageSymbian.cpp \
        plugins/symbian/PluginDatabaseSymbian.cpp \
        plugins/symbian/PluginViewSymbian.cpp \
        plugins/symbian/PluginContainerSymbian.cpp

        HEADERS += \
        plugins/symbian/PluginContainerSymbian.h \
        plugins/symbian/npinterface.h

        LIBS += -lefsrv

    } else {

        unix {
    
            mac {
                SOURCES += \
                    plugins/mac/PluginPackageMac.cpp \
                    plugins/mac/PluginViewMac.cpp
                OBJECTIVE_SOURCES += \
                    platform/text/mac/StringImplMac.mm \
                    platform/mac/WebCoreNSStringExtras.mm
                INCLUDEPATH += platform/mac
                # Note: XP_MACOSX is defined in npapi.h
            } else {
                !embedded: CONFIG += x11
                SOURCES += \
                    plugins/qt/PluginContainerQt.cpp \
                    plugins/qt/PluginPackageQt.cpp \
                    plugins/qt/PluginViewQt.cpp
                HEADERS += \
                    plugins/qt/PluginContainerQt.h
                DEFINES += XP_UNIX
            }
        }
    
        win32-* {
            INCLUDEPATH += $$PWD/plugins/win \
                           $$PWD/platform/win
    
            SOURCES += page/win/PageWin.cpp \
                       plugins/win/PluginDatabaseWin.cpp \
                       plugins/win/PluginPackageWin.cpp \
                       plugins/win/PluginMessageThrottlerWin.cpp \
                       plugins/win/PluginViewWin.cpp \
                       platform/win/BitmapInfo.cpp
    
            LIBS += \
                -ladvapi32 \
                -lgdi32 \
                -lshell32 \
                -lshlwapi \
                -luser32 \
                -lversion
        }
    }

} else {
    SOURCES += \
        plugins/PluginPackageNone.cpp \
        plugins/PluginViewNone.cpp
}

contains(DEFINES, ENABLE_SQLITE=1) {
    !system-sqlite:exists( $${SQLITE3SRCDIR}/sqlite3.c ) {
            # Build sqlite3 into WebCore from source
            # somewhat copied from $$QT_SOURCE_TREE/src/plugins/sqldrivers/sqlite/sqlite.pro
            INCLUDEPATH += $${SQLITE3SRCDIR}
            SOURCES += $${SQLITE3SRCDIR}/sqlite3.c
            DEFINES += SQLITE_CORE SQLITE_OMIT_LOAD_EXTENSION SQLITE_OMIT_COMPLETE
            CONFIG(release, debug|release): DEFINES *= NDEBUG
            contains(DEFINES, ENABLE_SINGLE_THREADED=1): DEFINES += SQLITE_THREADSAFE=0
    } else {
        # Use sqlite3 from the underlying OS
        CONFIG(QTDIR_build) {
            QMAKE_CXXFLAGS *= $$QT_CFLAGS_SQLITE
            LIBS *= $$QT_LFLAGS_SQLITE
        } else {
            INCLUDEPATH += $${SQLITE3SRCDIR}
            LIBS += -lsqlite3
        }
    }

    SOURCES += \
        platform/sql/SQLiteAuthorizer.cpp \
        platform/sql/SQLiteDatabase.cpp \
        platform/sql/SQLiteFileSystem.cpp \
        platform/sql/SQLiteStatement.cpp \
        platform/sql/SQLiteTransaction.cpp \
        platform/sql/SQLValue.cpp \
        storage/Database.cpp \
        storage/DatabaseAuthorizer.cpp
}


contains(DEFINES, ENABLE_DATABASE=1) {
    SOURCES += \
        storage/ChangeVersionWrapper.cpp \
        storage/DatabaseTask.cpp \
        storage/DatabaseThread.cpp \
        storage/DatabaseTracker.cpp \
        storage/OriginQuotaManager.cpp \
        storage/OriginUsageRecord.cpp \
        storage/SQLResultSet.cpp \
        storage/SQLResultSetRowList.cpp \
        storage/SQLStatement.cpp \
        storage/SQLTransaction.cpp \
        storage/SQLTransactionClient.cpp \
        storage/SQLTransactionCoordinator.cpp \
        bindings/js/JSCustomSQLStatementCallback.cpp \
        bindings/js/JSCustomSQLStatementErrorCallback.cpp \
        bindings/js/JSCustomSQLTransactionCallback.cpp \
        bindings/js/JSCustomSQLTransactionErrorCallback.cpp \
        bindings/js/JSDatabaseCustom.cpp \
        bindings/js/JSSQLResultSetRowListCustom.cpp \
        bindings/js/JSSQLTransactionCustom.cpp
}

contains(DEFINES, ENABLE_DOM_STORAGE=1) {
    HEADERS += \
        storage/ChangeVersionWrapper.h \
        storage/DatabaseAuthorizer.h \
        storage/Database.h \
        storage/DatabaseTask.h \
        storage/DatabaseThread.h \
        storage/DatabaseTracker.h \
        storage/LocalStorageTask.h \
        storage/LocalStorageThread.h \
        storage/OriginQuotaManager.h \
        storage/OriginUsageRecord.h \
        storage/SQLResultSet.h \
        storage/SQLResultSetRowList.h \
        storage/SQLStatement.h \
        storage/SQLTransaction.h \
        storage/SQLTransactionClient.h \
        storage/SQLTransactionCoordinator.h \
        storage/StorageArea.h \
        storage/StorageAreaImpl.h \
        storage/StorageAreaSync.h \
        storage/StorageEvent.h \
        storage/StorageEventDispatcher.h \
        storage/Storage.h \
        storage/StorageMap.h \
        storage/StorageNamespace.h \
        storage/StorageNamespaceImpl.h \
        storage/StorageSyncManager.h

    SOURCES += \
        bindings/js/JSStorageCustom.cpp \
        storage/LocalStorageTask.cpp \
        storage/LocalStorageThread.cpp \
        storage/Storage.cpp \
        storage/StorageAreaImpl.cpp \
        storage/StorageAreaSync.cpp \
        storage/StorageEvent.cpp \
        storage/StorageEventDispatcher.cpp \
        storage/StorageMap.cpp \
        storage/StorageNamespace.cpp \
        storage/StorageNamespaceImpl.cpp \
        storage/StorageSyncManager.cpp
}

contains(DEFINES, ENABLE_ICONDATABASE=1) {
    SOURCES += \
        loader/icon/IconDatabase.cpp \
        loader/icon/IconRecord.cpp \
        loader/icon/PageURLRecord.cpp
} else {
    SOURCES += \
        loader/icon/IconDatabaseNone.cpp
}

contains(DEFINES, ENABLE_WORKERS=1) {
    SOURCES += \
        bindings/js/JSAbstractWorkerCustom.cpp \
        bindings/js/JSDedicatedWorkerContextCustom.cpp \
        bindings/js/JSWorkerConstructor.cpp \
        bindings/js/JSWorkerContextBase.cpp \
        bindings/js/JSWorkerContextCustom.cpp \
        bindings/js/JSWorkerCustom.cpp \
        bindings/js/WorkerScriptController.cpp \
        loader/WorkerThreadableLoader.cpp \
        page/WorkerNavigator.cpp \
        workers/AbstractWorker.cpp \
        workers/DedicatedWorkerContext.cpp \
        workers/DedicatedWorkerThread.cpp \
        workers/Worker.cpp \
        workers/WorkerContext.cpp \
        workers/WorkerLocation.cpp \
        workers/WorkerMessagingProxy.cpp \
        workers/WorkerRunLoop.cpp \
        workers/WorkerThread.cpp \
        workers/WorkerScriptLoader.cpp
}

contains(DEFINES, ENABLE_SHARED_WORKERS=1) {
    SOURCES += \
        bindings/js/JSSharedWorkerConstructor.cpp \
        bindings/js/JSSharedWorkerCustom.cpp \
        workers/DefaultSharedWorkerRepository.cpp \
        workers/SharedWorker.cpp \
        workers/SharedWorkerContext.cpp \
        workers/SharedWorkerThread.cpp
}

contains(DEFINES, ENABLE_VIDEO=1) {
    SOURCES += \
        html/HTMLAudioElement.cpp \
        html/HTMLMediaElement.cpp \
        html/HTMLSourceElement.cpp \
        html/HTMLVideoElement.cpp \
        html/TimeRanges.cpp \
        platform/graphics/MediaPlayer.cpp \
        rendering/MediaControlElements.cpp \
        rendering/RenderVideo.cpp \
        rendering/RenderMedia.cpp \
        bindings/js/JSAudioConstructor.cpp

        HEADERS += \
            platform/graphics/qt/MediaPlayerPrivatePhonon.h

        SOURCES += \
            platform/graphics/qt/MediaPlayerPrivatePhonon.cpp

        # Add phonon manually to prevent it from coming first in
        # the include paths, as Phonon's path.h conflicts with
        # WebCore's Path.h on case-insensitive filesystems.
        qtAddLibrary(phonon)
        INCLUDEPATH -= $$QMAKE_INCDIR_QT/phonon
        INCLUDEPATH += $$QMAKE_INCDIR_QT/phonon
        mac {
            INCLUDEPATH -= $$QMAKE_LIBDIR_QT/phonon.framework/Headers
            INCLUDEPATH += $$QMAKE_LIBDIR_QT/phonon.framework/Headers
        }

}

contains(DEFINES, ENABLE_XPATH=1) {
    SOURCES += \
        xml/NativeXPathNSResolver.cpp \
        xml/XPathEvaluator.cpp \
        xml/XPathExpression.cpp \
        xml/XPathExpressionNode.cpp \
        xml/XPathFunctions.cpp \
        xml/XPathNamespace.cpp \
        xml/XPathNodeSet.cpp \
        xml/XPathNSResolver.cpp \
        xml/XPathParser.cpp \
        xml/XPathPath.cpp \
        xml/XPathPredicate.cpp \
        xml/XPathResult.cpp \
        xml/XPathStep.cpp \
        xml/XPathUtil.cpp \
        xml/XPathValue.cpp \
        xml/XPathVariableReference.cpp
}

unix:!mac:CONFIG += link_pkgconfig

contains(DEFINES, ENABLE_XSLT=1) {
    QT += xmlpatterns

    SOURCES += \
        bindings/js/JSXSLTProcessorConstructor.cpp \
        bindings/js/JSXSLTProcessorCustom.cpp \
        dom/TransformSourceQt.cpp \
        xml/XSLStyleSheetQt.cpp \
        xml/XSLTProcessor.cpp \
        xml/XSLTProcessorQt.cpp
}

contains(DEFINES, ENABLE_FILTERS=1) {
    SOURCES += \
        platform/graphics/filters/FEBlend.cpp \
        platform/graphics/filters/FEColorMatrix.cpp \
        platform/graphics/filters/FEComponentTransfer.cpp \
        platform/graphics/filters/FEComposite.cpp \
        platform/graphics/filters/FEGaussianBlur.cpp \
        platform/graphics/filters/FilterEffect.cpp \
        platform/graphics/filters/SourceAlpha.cpp \
        platform/graphics/filters/SourceGraphic.cpp
}

contains(DEFINES, ENABLE_WML=1) {
    SOURCES += \
        wml/WMLAElement.cpp \
        wml/WMLAccessElement.cpp \
        wml/WMLAnchorElement.cpp \
        wml/WMLBRElement.cpp \
        wml/WMLCardElement.cpp \
        wml/WMLDoElement.cpp \
        wml/WMLDocument.cpp \
        wml/WMLElement.cpp \
        wml/WMLErrorHandling.cpp \
        wml/WMLEventHandlingElement.cpp \
        wml/WMLFormControlElement.cpp \
        wml/WMLFieldSetElement.cpp \
        wml/WMLGoElement.cpp \
        wml/WMLImageElement.cpp \
        wml/WMLImageLoader.cpp \
        wml/WMLInputElement.cpp \
        wml/WMLInsertedLegendElement.cpp \
        wml/WMLIntrinsicEvent.cpp \
        wml/WMLIntrinsicEventHandler.cpp \
        wml/WMLMetaElement.cpp \
        wml/WMLNoopElement.cpp \
        wml/WMLOnEventElement.cpp \
        wml/WMLPElement.cpp \
        wml/WMLOptGroupElement.cpp \
        wml/WMLOptionElement.cpp \
        wml/WMLPageState.cpp \
        wml/WMLPostfieldElement.cpp \
        wml/WMLPrevElement.cpp \
        wml/WMLRefreshElement.cpp \
        wml/WMLSelectElement.cpp \
        wml/WMLSetvarElement.cpp \
        wml/WMLTableElement.cpp \
        wml/WMLTaskElement.cpp \
        wml/WMLTemplateElement.cpp \
        wml/WMLTimerElement.cpp \
        wml/WMLVariables.cpp
}

contains(DEFINES, ENABLE_XHTMLMP=1) {
    SOURCES += \
        html/HTMLNoScriptElement.cpp
}

contains(DEFINES, ENABLE_QT_BEARER=1) {
    HEADERS += \
        platform/network/qt/NetworkStateNotifierPrivate.h

    SOURCES += \
        platform/network/qt/NetworkStateNotifierQt.cpp

    CONFIG += mobility
    MOBILITY += bearer
}

contains(DEFINES, ENABLE_SVG=1) {
    SOURCES += \
# TODO: this-one-is-not-auto-added! FIXME! tmp/SVGElementFactory.cpp \
        bindings/js/JSSVGElementInstanceCustom.cpp \
        bindings/js/JSSVGLengthCustom.cpp \
        bindings/js/JSSVGMatrixCustom.cpp \
        bindings/js/JSSVGPathSegCustom.cpp \
        bindings/js/JSSVGPathSegListCustom.cpp \
        css/SVGCSSComputedStyleDeclaration.cpp \
        css/SVGCSSParser.cpp \
        css/SVGCSSStyleSelector.cpp \
        rendering/style/SVGRenderStyle.cpp \
        rendering/style/SVGRenderStyleDefs.cpp \
        svg/SVGZoomEvent.cpp \
        rendering/PointerEventsHitRules.cpp \
        svg/SVGDocumentExtensions.cpp \
        svg/SVGImageLoader.cpp \
        svg/ColorDistance.cpp \
        svg/SVGAElement.cpp \
        svg/SVGAltGlyphElement.cpp \
        svg/SVGAngle.cpp \
        svg/SVGAnimateColorElement.cpp \
        svg/SVGAnimatedPathData.cpp \
        svg/SVGAnimatedPoints.cpp \
        svg/SVGAnimateElement.cpp \
        svg/SVGAnimateMotionElement.cpp \
        svg/SVGAnimateTransformElement.cpp \
        svg/SVGAnimationElement.cpp \
        svg/SVGCircleElement.cpp \
        svg/SVGClipPathElement.cpp \
        svg/SVGColor.cpp \
        svg/SVGComponentTransferFunctionElement.cpp \
        svg/SVGCursorElement.cpp \
        svg/SVGDefsElement.cpp \
        svg/SVGDescElement.cpp \
        svg/SVGDocument.cpp \
        svg/SVGElement.cpp \
        svg/SVGElementInstance.cpp \
        svg/SVGElementInstanceList.cpp \
        svg/SVGEllipseElement.cpp \
        svg/SVGExternalResourcesRequired.cpp \
        svg/SVGFEBlendElement.cpp \
        svg/SVGFEColorMatrixElement.cpp \
        svg/SVGFEComponentTransferElement.cpp \
        svg/SVGFECompositeElement.cpp \
        svg/SVGFEDiffuseLightingElement.cpp \
        svg/SVGFEDisplacementMapElement.cpp \
        svg/SVGFEDistantLightElement.cpp \
        svg/SVGFEFloodElement.cpp \
        svg/SVGFEFuncAElement.cpp \
        svg/SVGFEFuncBElement.cpp \
        svg/SVGFEFuncGElement.cpp \
        svg/SVGFEFuncRElement.cpp \
        svg/SVGFEGaussianBlurElement.cpp \
        svg/SVGFEImageElement.cpp \
        svg/SVGFELightElement.cpp \
        svg/SVGFEMergeElement.cpp \
        svg/SVGFEMergeNodeElement.cpp \
        svg/SVGFEMorphologyElement.cpp \
        svg/SVGFEOffsetElement.cpp \
        svg/SVGFEPointLightElement.cpp \
        svg/SVGFESpecularLightingElement.cpp \
        svg/SVGFESpotLightElement.cpp \
        svg/SVGFETileElement.cpp \
        svg/SVGFETurbulenceElement.cpp \
        svg/SVGFilterElement.cpp \
        svg/SVGFilterPrimitiveStandardAttributes.cpp \
        svg/SVGFitToViewBox.cpp \
        svg/SVGFont.cpp \
        svg/SVGFontData.cpp \
        svg/SVGFontElement.cpp \
        svg/SVGFontFaceElement.cpp \
        svg/SVGFontFaceFormatElement.cpp \
        svg/SVGFontFaceNameElement.cpp \
        svg/SVGFontFaceSrcElement.cpp \
        svg/SVGFontFaceUriElement.cpp \
        svg/SVGForeignObjectElement.cpp \
        svg/SVGGElement.cpp \
        svg/SVGGlyphElement.cpp \
        svg/SVGGradientElement.cpp \
        svg/SVGHKernElement.cpp \
        svg/SVGImageElement.cpp \
        svg/SVGLangSpace.cpp \
        svg/SVGLength.cpp \
        svg/SVGLengthList.cpp \
        svg/SVGLinearGradientElement.cpp \
        svg/SVGLineElement.cpp \
        svg/SVGLocatable.cpp \
        svg/SVGMarkerElement.cpp \
        svg/SVGMaskElement.cpp \
        svg/SVGMetadataElement.cpp \
        svg/SVGMissingGlyphElement.cpp \
        svg/SVGMPathElement.cpp \
        svg/SVGNumberList.cpp \
        svg/SVGPaint.cpp \
        svg/SVGParserUtilities.cpp \
        svg/SVGPathElement.cpp \
        svg/SVGPathSegArc.cpp \
        svg/SVGPathSegClosePath.cpp \
        svg/SVGPathSegCurvetoCubic.cpp \
        svg/SVGPathSegCurvetoCubicSmooth.cpp \
        svg/SVGPathSegCurvetoQuadratic.cpp \
        svg/SVGPathSegCurvetoQuadraticSmooth.cpp \
        svg/SVGPathSegLineto.cpp \
        svg/SVGPathSegLinetoHorizontal.cpp \
        svg/SVGPathSegLinetoVertical.cpp \
        svg/SVGPathSegList.cpp \
        svg/SVGPathSegMoveto.cpp \
        svg/SVGPatternElement.cpp \
        svg/SVGPointList.cpp \
        svg/SVGPolyElement.cpp \
        svg/SVGPolygonElement.cpp \
        svg/SVGPolylineElement.cpp \
        svg/SVGPreserveAspectRatio.cpp \
        svg/SVGRadialGradientElement.cpp \
        svg/SVGRectElement.cpp \
        svg/SVGScriptElement.cpp \
        svg/SVGSetElement.cpp \
        svg/SVGStopElement.cpp \
        svg/SVGStringList.cpp \
        svg/SVGStylable.cpp \
        svg/SVGStyledElement.cpp \
        svg/SVGStyledLocatableElement.cpp \
        svg/SVGStyledTransformableElement.cpp \
        svg/SVGStyleElement.cpp \
        svg/SVGSVGElement.cpp \
        svg/SVGSwitchElement.cpp \
        svg/SVGSymbolElement.cpp \
        svg/SVGTests.cpp \
        svg/SVGTextContentElement.cpp \
        svg/SVGTextElement.cpp \
        svg/SVGTextPathElement.cpp \
        svg/SVGTextPositioningElement.cpp \
        svg/SVGTitleElement.cpp \
        svg/SVGTransformable.cpp \
        svg/SVGTransform.cpp \
        svg/SVGTransformDistance.cpp \
        svg/SVGTransformList.cpp \
        svg/SVGTRefElement.cpp \
        svg/SVGTSpanElement.cpp \
        svg/SVGURIReference.cpp \
        svg/SVGUseElement.cpp \
        svg/SVGViewElement.cpp \
        svg/SVGViewSpec.cpp \
        svg/SVGZoomAndPan.cpp \
        svg/SynchronizablePropertyController.cpp \
        svg/animation/SMILTime.cpp \
        svg/animation/SMILTimeContainer.cpp \
        svg/animation/SVGSMILElement.cpp \
        svg/graphics/filters/SVGFEConvolveMatrix.cpp \
        svg/graphics/filters/SVGFEDiffuseLighting.cpp \
        svg/graphics/filters/SVGFEDisplacementMap.cpp \
        svg/graphics/filters/SVGFEFlood.cpp \
        svg/graphics/filters/SVGFEImage.cpp \
        svg/graphics/filters/SVGFEMerge.cpp \
        svg/graphics/filters/SVGFEMorphology.cpp \
        svg/graphics/filters/SVGFEOffset.cpp \
        svg/graphics/filters/SVGFESpecularLighting.cpp \
        svg/graphics/filters/SVGFETile.cpp \
        svg/graphics/filters/SVGFETurbulence.cpp \
        svg/graphics/filters/SVGFilter.cpp \
        svg/graphics/filters/SVGFilterBuilder.cpp \
        svg/graphics/filters/SVGLightSource.cpp \
        svg/graphics/SVGImage.cpp \
        svg/graphics/SVGPaintServer.cpp \
        svg/graphics/SVGPaintServerGradient.cpp \
        svg/graphics/SVGPaintServerLinearGradient.cpp \
        svg/graphics/SVGPaintServerPattern.cpp \
        svg/graphics/SVGPaintServerRadialGradient.cpp \
        svg/graphics/SVGPaintServerSolid.cpp \
        svg/graphics/SVGResourceClipper.cpp \
        svg/graphics/SVGResource.cpp \
        svg/graphics/SVGResourceFilter.cpp \
        svg/graphics/SVGResourceMarker.cpp \
        svg/graphics/SVGResourceMasker.cpp \
        rendering/RenderForeignObject.cpp \
        rendering/RenderPath.cpp \
        rendering/RenderSVGBlock.cpp \
        rendering/RenderSVGContainer.cpp \
        rendering/RenderSVGGradientStop.cpp \
        rendering/RenderSVGHiddenContainer.cpp \
        rendering/RenderSVGImage.cpp \
        rendering/RenderSVGInline.cpp \
        rendering/RenderSVGInlineText.cpp \
        rendering/RenderSVGModelObject.cpp \
        rendering/RenderSVGRoot.cpp \
        rendering/RenderSVGShadowTreeRootContainer.cpp \
        rendering/RenderSVGText.cpp \
        rendering/RenderSVGTextPath.cpp \
        rendering/RenderSVGTransformableContainer.cpp \
        rendering/RenderSVGTSpan.cpp \
        rendering/RenderSVGViewportContainer.cpp \
        rendering/SVGCharacterLayoutInfo.cpp \
        rendering/SVGInlineFlowBox.cpp \
        rendering/SVGInlineTextBox.cpp \
        rendering/SVGMarkerLayoutInfo.cpp \
        rendering/SVGRenderSupport.cpp \
        rendering/SVGRootInlineBox.cpp \
        rendering/SVGShadowTreeElements.cpp
}

contains(DEFINES, ENABLE_JAVASCRIPT_DEBUGGER=1) {
    SOURCES += \
        bindings/js/JSJavaScriptCallFrameCustom.cpp \
        inspector/JavaScriptCallFrame.cpp \
        inspector/JavaScriptDebugServer.cpp \
        inspector/JavaScriptProfile.cpp \
        inspector/JavaScriptProfileNode.cpp
}

contains(DEFINES, ENABLE_OFFLINE_WEB_APPLICATIONS=1) {
SOURCES += \
    loader/appcache/ApplicationCache.cpp \
    loader/appcache/ApplicationCacheGroup.cpp \
    loader/appcache/ApplicationCacheHost.cpp \
    loader/appcache/ApplicationCacheStorage.cpp \
    loader/appcache/ApplicationCacheResource.cpp \
    loader/appcache/DOMApplicationCache.cpp \
    loader/appcache/ManifestParser.cpp \
    bindings/js/JSDOMApplicationCacheCustom.cpp
}

contains(DEFINES, ENABLE_WEB_SOCKETS=1) {
SOURCES += \
    websockets/WebSocket.cpp \
    websockets/WebSocketChannel.cpp \
    websockets/WebSocketHandshake.cpp \
    platform/network/SocketStreamErrorBase.cpp \
    platform/network/SocketStreamHandleBase.cpp \
    platform/network/qt/SocketStreamHandleQt.cpp \
    bindings/js/JSWebSocketCustom.cpp \
    bindings/js/JSWebSocketConstructor.cpp

contains(DEFINES, ENABLE_WORKERS=1) {
SOURCES += \
    websockets/ThreadableWebSocketChannel.cpp \
    websockets/WorkerThreadableWebSocketChannel.cpp
}
}

include($$PWD/../WebKit/qt/Api/headers.pri)
HEADERS += $$WEBKIT_API_HEADERS
!CONFIG(standalone_package) {
    target.path = $$[QT_INSTALL_LIBS]
    headers.files = $$WEBKIT_API_HEADERS
    headers.path = $$[QT_INSTALL_HEADERS]/QtWebKit

    VERSION=$${QT_MAJOR_VERSION}.$${QT_MINOR_VERSION}.$${QT_PATCH_VERSION}

    win32-*|wince* {
        DLLDESTDIR = $$OUTPUT_DIR/bin

        dlltarget.commands = $(COPY_FILE) $(DESTDIR)$(TARGET) $$[QT_INSTALL_BINS]
        dlltarget.CONFIG = no_path
        INSTALLS += dlltarget
    }


    INSTALLS += target headers

    unix {
        CONFIG += create_pc create_prl
        QMAKE_PKGCONFIG_LIBDIR = $$target.path
        QMAKE_PKGCONFIG_INCDIR = $$headers.path
        QMAKE_PKGCONFIG_DESTDIR = pkgconfig
        lib_replace.match = $$DESTDIR
        lib_replace.replace = $$[QT_INSTALL_LIBS]
        QMAKE_PKGCONFIG_INSTALL_REPLACE += lib_replace
    }

    mac {
        !static:contains(QT_CONFIG, qt_framework):!CONFIG(webkit_no_framework) {
            !build_pass {
                message("Building QtWebKit as a framework, as that's how Qt was built. You can")
                message("override this by passing CONFIG+=webkit_no_framework to build-webkit.")

                CONFIG += build_all
            } else {
                debug_and_release:TARGET = $$qtLibraryTarget($$TARGET)
            }

            CONFIG += lib_bundle qt_no_framework_direct_includes qt_framework
            FRAMEWORK_HEADERS.version = Versions
            FRAMEWORK_HEADERS.files = $$WEBKIT_API_HEADERS
            FRAMEWORK_HEADERS.path = Headers
            QMAKE_BUNDLE_DATA += FRAMEWORK_HEADERS
        }

        QMAKE_LFLAGS_SONAME = "$${QMAKE_LFLAGS_SONAME}$${DESTDIR}$${QMAKE_DIR_SEP}"
        LIBS += -framework Carbon -framework AppKit
    }
}

CONFIG(standalone_package):isEqual(QT_MAJOR_VERSION, 4):greaterThan(QT_MINOR_VERSION, 4) {
    # start with 4.5
    # Remove the following 2 lines if you want debug information in WebCore
    CONFIG -= separate_debug_info
    CONFIG += no_debug_info
}

!win32-g++:win32:contains(QMAKE_HOST.arch, x86_64):{
    asm_compiler.commands = ml64 /c
    asm_compiler.commands +=  /Fo ${QMAKE_FILE_OUT} ${QMAKE_FILE_IN}
    asm_compiler.output = ${QMAKE_VAR_OBJECTS_DIR}${QMAKE_FILE_BASE}$${first(QMAKE_EXT_OBJ)}
    asm_compiler.input = ASM_SOURCES
    asm_compiler.variable_out = OBJECTS
    asm_compiler.name = compiling[asm] ${QMAKE_FILE_IN}
    silent:asm_compiler.commands = @echo compiling[asm] ${QMAKE_FILE_IN} && $$asm_compiler.commands
    QMAKE_EXTRA_COMPILERS += asm_compiler

    ASM_SOURCES += \
        plugins/win/PaintHooks.asm
   if(win32-msvc2005|win32-msvc2008):equals(TEMPLATE_PREFIX, "vc") {
        SOURCES += \
            plugins/win/PaintHooks.asm
    }
}

symbian {
    shared {
        contains(MMP_RULES, defBlock) {
            MMP_RULES -= defBlock

            MMP_RULES += "$${LITERAL_HASH}ifdef WINSCW" \
                    "DEFFILE ../WebKit/qt/symbian/bwins/$${TARGET}.def" \
                    "$${LITERAL_HASH}elif defined EABI" \
                    "DEFFILE ../WebKit/qt/symbian/eabi/$${TARGET}.def" \
                    "$${LITERAL_HASH}endif"
        } else {
            MMP_RULES += EXPORTUNFROZEN
        }
    }
}
