/*
 * Copyright (C) 2004 Apple Computer, Inc.  All rights reserved.
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

#import "KWQKHTMLPart.h"

#import "DOMInternal.h"
#import "KWQClipboard.h"
#import "KWQDOMNode.h"
#import "KWQDummyView.h"
#import "KWQEditCommand.h"
#import "KWQExceptions.h"
#import "KWQFormData.h"
#import "KWQFoundationExtras.h"
#import "KWQKJobClasses.h"
#import "KWQLogging.h"
#import "KWQPageState.h"
#import "KWQPrinter.h"
#import "KWQRegExp.h"
#import "KWQScrollBar.h"
#import "KWQWindowWidget.h"
#import "WebCoreBridge.h"
#import "WebCoreGraphicsBridge.h"
#import "WebCoreViewFactory.h"
#import "csshelper.h"
#import "dom2_eventsimpl.h"
#import "dom2_rangeimpl.h"
#import "dom_position.h"
#import "dom_textimpl.h"
#import "html_document.h"
#import "html_documentimpl.h"
#import "html_formimpl.h"
#import "html_misc.h"
#import "html_tableimpl.h"
#import "htmlattrs.h"
#import "htmltokenizer.h"
#import "khtmlpart_p.h"
#import "khtmlview.h"
#import "kjs_binding.h"
#import "kjs_window.h"
#import "render_canvas.h"
#import "render_frames.h"
#import "render_image.h"
#import "render_list.h"
#import "render_style.h"
#import "render_table.h"
#import "render_text.h"
#import "selection.h"
#import "visible_position.h"
#import "visible_text.h"
#import "visible_units.h"

#import <JavaScriptCore/identifier.h>
#import <JavaScriptCore/property_map.h>
#import <JavaScriptCore/runtime.h>
#import <JavaScriptCore/runtime_root.h>
#import <JavaScriptCore/WebScriptObjectPrivate.h>

#import "WebDashboardRegion.h"

#undef _KWQ_TIMING

using DOM::AtomicString;
using DOM::ClipboardEventImpl;
using DOM::DocumentFragmentImpl;
using DOM::DocumentImpl;
using DOM::DocumentMarker;
using DOM::DOMString;
using DOM::ElementImpl;
using DOM::EventImpl;
using DOM::HTMLDocumentImpl;
using DOM::HTMLElementImpl;
using DOM::HTMLFormElementImpl;
using DOM::HTMLFrameElementImpl;
using DOM::HTMLGenericFormElementImpl;
using DOM::HTMLTableCellElementImpl;
using DOM::Node;
using DOM::NodeImpl;
using DOM::Position;
using DOM::Range;
using DOM::RangeImpl;
using DOM::TextImpl;

using khtml::Cache;
using khtml::CharacterIterator;
using khtml::ChildFrame;
using khtml::Decoder;
using khtml::DashboardRegionValue;
using khtml::endOfWord;
using khtml::findPlainText;
using khtml::InlineTextBox;
using khtml::LeftWordIfOnBoundary;
using khtml::MouseDoubleClickEvent;
using khtml::MouseMoveEvent;
using khtml::MousePressEvent;
using khtml::MouseReleaseEvent;
using khtml::parseURL;
using khtml::PRE;
using khtml::RenderCanvas;
using khtml::RenderImage;
using khtml::RenderLayer;
using khtml::RenderListItem;
using khtml::RenderObject;
using khtml::RenderPart;
using khtml::RenderStyle;
using khtml::RenderTableCell;
using khtml::RenderText;
using khtml::RenderWidget;
using khtml::RightWordIfOnBoundary;
using khtml::Selection;
using khtml::setEnd;
using khtml::setStart;
using khtml::startOfWord;
using khtml::startVisiblePosition;
using khtml::TextIterator;
using khtml::UPSTREAM;
using khtml::VISIBLE;
using khtml::VisiblePosition;
using khtml::WordAwareIterator;

using KIO::Job;

using KJS::Interpreter;
using KJS::Location;
using KJS::SavedBuiltins;
using KJS::SavedProperties;
using KJS::ScheduledAction;
using KJS::Window;

using KJS::Bindings::Instance;

using KParts::ReadOnlyPart;
using KParts::URLArgs;

NSEvent *KWQKHTMLPart::_currentEvent = nil;
NSResponder *KWQKHTMLPart::_firstResponderAtMouseDownTime = nil;

void KHTMLPart::completed()
{
    KWQ(this)->_completed.call();
}

void KHTMLPart::completed(bool arg)
{
    KWQ(this)->_completed.call(arg);
}

void KHTMLPart::nodeActivated(const Node &)
{
}

bool KHTMLPart::openURL(const KURL &URL)
{
    ASSERT_NOT_REACHED();
    return true;
}

void KHTMLPart::onURL(const QString &)
{
}

void KHTMLPart::setStatusBarText(const QString &status)
{
    KWQ(this)->setStatusBarText(status);
}

void KHTMLPart::started(Job *j)
{
    KWQ(this)->_started.call(j);
}

static void redirectionTimerMonitor(void *context)
{
    KWQKHTMLPart *kwq = static_cast<KWQKHTMLPart *>(context);
    kwq->redirectionTimerStartedOrStopped();
}

KWQKHTMLPart::KWQKHTMLPart()
    : _started(this, SIGNAL(started(KIO::Job *)))
    , _completed(this, SIGNAL(completed()))
    , _completedWithBool(this, SIGNAL(completed(bool)))
    , _mouseDownView(nil)
    , _sendingEventToSubview(false)
    , _mouseDownMayStartDrag(false)
    , _mouseDownMayStartSelect(false)
    , _activationEventNumber(0)
    , _formValuesAboutToBeSubmitted(nil)
    , _formAboutToBeSubmitted(nil)
    , _windowWidget(NULL)
    , _drawSelectionOnly(false)
    , _bindingRoot(0)
    , _windowScriptObject(0)
    , _windowScriptNPObject(0)
    , _dragSrc(0)
    , _dragClipboard(0)
    , _elementToDraw(0)
{
    // Must init the cache before connecting to any signals
    Cache::init();

    // The widget is made outside this class in our case.
    KHTMLPart::init( 0, DefaultGUI );

    mutableInstances().prepend(this);
    d->m_redirectionTimer.setMonitor(redirectionTimerMonitor, this);
}

KWQKHTMLPart::~KWQKHTMLPart()
{
    cleanupPluginRootObjects();
    
    mutableInstances().remove(this);
    if (d->m_view) {
	d->m_view->deref();
    }
    freeClipboard();
    // these are all basic Foundation classes and our own classes - we
    // know they will not raise in dealloc, so no need to block
    // exceptions.
    KWQRelease(_formValuesAboutToBeSubmitted);
    KWQRelease(_formAboutToBeSubmitted);
    
    KWQRelease(_windowScriptObject);
    
    delete _windowWidget;
}

void KWQKHTMLPart::freeClipboard()
{
    if (_dragClipboard) {
        _dragClipboard->setAccessPolicy(KWQClipboard::Numb);
	_dragClipboard->deref();
        _dragClipboard = 0;
    }
}

void KWQKHTMLPart::setSettings (KHTMLSettings *settings)
{
    d->m_settings = settings;
}

QString KWQKHTMLPart::generateFrameName()
{
    KWQ_BLOCK_EXCEPTIONS;
    return QString::fromNSString([_bridge generateFrameName]);
    KWQ_UNBLOCK_EXCEPTIONS;

    return QString();
}

void KWQKHTMLPart::provisionalLoadStarted()
{
    // we don't want to wait until we get an actual http response back
    // to cancel pending redirects, otherwise they might fire before
    // that happens.
    cancelRedirection(true);
}

bool KWQKHTMLPart::openURL(const KURL &url)
{
    KWQ_BLOCK_EXCEPTIONS;

    bool userGesture = true;
    
    if (jScript() && jScript()->interpreter()) {
        KHTMLPart *rootPart = this;
        while (rootPart->parentPart() != 0)
            rootPart = rootPart->parentPart();
        KJS::ScriptInterpreter *interpreter = static_cast<KJS::ScriptInterpreter *>(KJSProxy::proxy(rootPart)->interpreter());
        userGesture = interpreter->wasRunByUserGesture();
    }

    // FIXME: The lack of args here to get the reload flag from
    // indicates a problem in how we use KHTMLPart::processObjectRequest,
    // where we are opening the URL before the args are set up.
    [_bridge loadURL:url.getNSURL()
            referrer:[_bridge referrer]
              reload:NO
         userGesture:userGesture
              target:nil
     triggeringEvent:nil
                form:nil
          formValues:nil];

    KWQ_UNBLOCK_EXCEPTIONS;

    return true;
}

void KWQKHTMLPart::openURLRequest(const KURL &url, const URLArgs &args)
{
    KWQ_BLOCK_EXCEPTIONS;

    [_bridge loadURL:url.getNSURL()
            referrer:[_bridge referrer]
              reload:args.reload
         userGesture:true
              target:args.frameName.getNSString()
     triggeringEvent:nil
                form:nil
          formValues:nil];

    KWQ_UNBLOCK_EXCEPTIONS;
}

void KWQKHTMLPart::didNotOpenURL(const KURL &URL)
{
    if (_submittedFormURL == URL) {
        _submittedFormURL = KURL();
    }
}

// Scans logically forward from "start", including any child frames
static HTMLFormElementImpl *scanForForm(NodeImpl *start)
{
    NodeImpl *n;
    for (n = start; n; n = n->traverseNextNode()) {
        NodeImpl::Id nodeID = idFromNode(n);
        if (nodeID == ID_FORM) {
            return static_cast<HTMLFormElementImpl *>(n);
        } else if (n->isHTMLElement()
                   && static_cast<HTMLElementImpl *>(n)->isGenericFormElement()) {
            return static_cast<HTMLGenericFormElementImpl *>(n)->form();
        } else if (nodeID == ID_FRAME || nodeID == ID_IFRAME) {
            NodeImpl *childDoc = static_cast<HTMLFrameElementImpl *>(n)->contentDocument();
            HTMLFormElementImpl *frameResult = scanForForm(childDoc);
            if (frameResult) {
                return frameResult;
            }
        }
    }
    return 0;
}

// We look for either the form containing the current focus, or for one immediately after it
HTMLFormElementImpl *KWQKHTMLPart::currentForm() const
{
    // start looking either at the active (first responder) node, or where the selection is
    NodeImpl *start = activeNode().handle();
    if (!start) {
        start = selectionStart();
    }

    // try walking up the node tree to find a form element
    NodeImpl *n;
    for (n = start; n; n = n->parentNode()) {
        if (idFromNode(n) == ID_FORM) {
            return static_cast<HTMLFormElementImpl *>(n);
        } else if (n->isHTMLElement()
                   && static_cast<HTMLElementImpl *>(n)->isGenericFormElement()) {
            return static_cast<HTMLGenericFormElementImpl *>(n)->form();
        }
    }

    // try walking forward in the node tree to find a form element
    if (!start) {
        start = xmlDocImpl();
    }
    return scanForForm(start);
}

// Either get cached regexp or build one that matches any of the labels.
// The regexp we build is of the form:  (STR1|STR2|STRN)
QRegExp *regExpForLabels(NSArray *labels)
{
    // All the ObjC calls in this method are simple array and string
    // calls which we can assume do not raise exceptions


    // Parallel arrays that we use to cache regExps.  In practice the number of expressions
    // that the app will use is equal to the number of locales is used in searching.
    static const unsigned int regExpCacheSize = 4;
    static NSMutableArray *regExpLabels = nil;
    static QPtrList <QRegExp> regExps;
    static QRegExp wordRegExp = QRegExp("\\w");

    QRegExp *result;
    if (!regExpLabels) {
        regExpLabels = [[NSMutableArray alloc] initWithCapacity:regExpCacheSize];
    }
    unsigned int cacheHit = [regExpLabels indexOfObject:labels];
    if (cacheHit != NSNotFound) {
        result = regExps.at(cacheHit);
    } else {
        QString pattern("(");
        unsigned int numLabels = [labels count];
        unsigned int i;
        for (i = 0; i < numLabels; i++) {
            QString label = QString::fromNSString([labels objectAtIndex:i]);

            bool startsWithWordChar = false;
            bool endsWithWordChar = false;
            if (label.length() != 0) {
                startsWithWordChar = wordRegExp.search(label.at(0)) >= 0;
                endsWithWordChar = wordRegExp.search(label.at(label.length() - 1)) >= 0;
            }
            
            if (i != 0) {
                pattern.append("|");
            }
            // Search for word boundaries only if label starts/ends with "word characters".
            // If we always searched for word boundaries, this wouldn't work for languages
            // such as Japanese.
            if (startsWithWordChar) {
                pattern.append("\\b");
            }
            pattern.append(label);
            if (endsWithWordChar) {
                pattern.append("\\b");
            }
        }
        pattern.append(")");
        result = new QRegExp(pattern, false);
    }

    // add regexp to the cache, making sure it is at the front for LRU ordering
    if (cacheHit != 0) {
        if (cacheHit != NSNotFound) {
            // remove from old spot
            [regExpLabels removeObjectAtIndex:cacheHit];
            regExps.remove(cacheHit);
        }
        // add to start
        [regExpLabels insertObject:labels atIndex:0];
        regExps.insert(0, result);
        // trim if too big
        if ([regExpLabels count] > regExpCacheSize) {
            [regExpLabels removeObjectAtIndex:regExpCacheSize];
            QRegExp *last = regExps.last();
            regExps.removeLast();
            delete last;
        }
    }
    return result;
}

NSString *KWQKHTMLPart::searchForLabelsAboveCell(QRegExp *regExp, HTMLTableCellElementImpl *cell)
{
    RenderTableCell *cellRenderer = static_cast<RenderTableCell *>(cell->renderer());
    RenderTableCell *cellAboveRenderer = cellRenderer->table()->cellAbove(cellRenderer);

    if (cellAboveRenderer) {
        HTMLTableCellElementImpl *aboveCell =
            static_cast<HTMLTableCellElementImpl *>(cellAboveRenderer->element());

        if (aboveCell) {
            // search within the above cell we found for a match
            for (NodeImpl *n = aboveCell->firstChild(); n; n = n->traverseNextNode(aboveCell)) {
                if (idFromNode(n) == ID_TEXT
                    && n->renderer() && n->renderer()->style()->visibility() == VISIBLE)
                {
                    // For each text chunk, run the regexp
                    QString nodeString = n->nodeValue().string();
                    int pos = regExp->searchRev(nodeString);
                    if (pos >= 0) {
                        return nodeString.mid(pos, regExp->matchedLength()).getNSString();
                    }
                }
            }
        }
    }
    // Any reason in practice to search all cells in that are above cell?
    return nil;
}

NSString *KWQKHTMLPart::searchForLabelsBeforeElement(NSArray *labels, ElementImpl *element)
{
    QRegExp *regExp = regExpForLabels(labels);
    // We stop searching after we've seen this many chars
    const unsigned int charsSearchedThreshold = 500;
    // This is the absolute max we search.  We allow a little more slop than
    // charsSearchedThreshold, to make it more likely that we'll search whole nodes.
    const unsigned int maxCharsSearched = 600;
    // If the starting element is within a table, the cell that contains it
    HTMLTableCellElementImpl *startingTableCell = 0;
    bool searchedCellAbove = false;

    // walk backwards in the node tree, until another element, or form, or end of tree
    int unsigned lengthSearched = 0;
    NodeImpl *n;
    for (n = element->traversePreviousNode();
         n && lengthSearched < charsSearchedThreshold;
         n = n->traversePreviousNode())
    {
        NodeImpl::Id nodeID = idFromNode(n);
        if (nodeID == ID_FORM
            || (n->isHTMLElement()
                && static_cast<HTMLElementImpl *>(n)->isGenericFormElement()))
        {
            // We hit another form element or the start of the form - bail out
            break;
        } else if (nodeID == ID_TD && !startingTableCell) {
            startingTableCell = static_cast<HTMLTableCellElementImpl *>(n);
        } else if (nodeID == ID_TR && startingTableCell) {
            NSString *result = searchForLabelsAboveCell(regExp, startingTableCell);
            if (result) {
                return result;
            }
            searchedCellAbove = true;
        } else if (nodeID == ID_TEXT
                   && n->renderer() && n->renderer()->style()->visibility() == VISIBLE)
        {
            // For each text chunk, run the regexp
            QString nodeString = n->nodeValue().string();
            // add 100 for slop, to make it more likely that we'll search whole nodes
            if (lengthSearched + nodeString.length() > maxCharsSearched) {
                nodeString = nodeString.right(charsSearchedThreshold - lengthSearched);
            }
            int pos = regExp->searchRev(nodeString);
            if (pos >= 0) {
                return nodeString.mid(pos, regExp->matchedLength()).getNSString();
            } else {
                lengthSearched += nodeString.length();
            }
        }
    }

    // If we started in a cell, but bailed because we found the start of the form or the
    // previous element, we still might need to search the row above us for a label.
    if (startingTableCell && !searchedCellAbove) {
         return searchForLabelsAboveCell(regExp, startingTableCell);
    } else {
        return nil;
    }
}

NSString *KWQKHTMLPart::matchLabelsAgainstElement(NSArray *labels, ElementImpl *element)
{
    QString name = element->getAttribute(ATTR_NAME).string();
    // Make numbers and _'s in field names behave like word boundaries, e.g., "address2"
    name.replace(QRegExp("[[:digit:]]"), " ");
    name.replace('_', ' ');
    
    QRegExp *regExp = regExpForLabels(labels);
    // Use the largest match we can find in the whole name string
    int pos;
    int length;
    int bestPos = -1;
    int bestLength = -1;
    int start = 0;
    do {
        pos = regExp->search(name, start);
        if (pos != -1) {
            length = regExp->matchedLength();
            if (length >= bestLength) {
                bestPos = pos;
                bestLength = length;
            }
            start = pos+1;
        }
    } while (pos != -1);

    if (bestPos != -1) {
        return name.mid(bestPos, bestLength).getNSString();
    } else {
        return nil;
    }
}

// Search from the end of the currently selected location if we are first responder, or from
// the beginning of the document if nothing is selected or we're not first responder.
bool KWQKHTMLPart::findString(NSString *string, bool forward, bool caseFlag, bool wrapFlag)
{
    QString target = QString::fromNSString(string);
    if (target.isEmpty()) {
        return false;
    }

    // Start on the correct edge of the selection, search to edge of document.
    Range searchRange(xmlDocImpl());
    searchRange.selectNodeContents(xmlDocImpl());
    if (selectionStart()) {
        if (forward) {
            setStart(searchRange, VisiblePosition(selection().end()));
        } else {
            setEnd(searchRange, VisiblePosition(selection().start()));
        }
    }

    // Do the search once, then do it a second time to handle wrapped search.
    // Searches some or all of document twice in the failure case, but that's probably OK.
    Range resultRange = findPlainText(searchRange, target, forward, caseFlag);
    if (resultRange.collapsed() && wrapFlag) {
        searchRange.selectNodeContents(xmlDocImpl());
        resultRange = findPlainText(searchRange, target, forward, caseFlag);
        // If we got back to the same place we started, that doesn't count as success.
        if (resultRange == selection().toRange()) {
            return false;
        }
    }

    if (resultRange.collapsed()) {
        return false;
    }

    setSelection(resultRange);
    jumpToSelection();
    return true;
}

void KWQKHTMLPart::clearRecordedFormValues()
{
    // It's safe to assume that our own classes and Foundation data
    // structures won't raise exceptions in dealloc

    KWQRelease(_formValuesAboutToBeSubmitted);
    _formValuesAboutToBeSubmitted = nil;
    KWQRelease(_formAboutToBeSubmitted);
    _formAboutToBeSubmitted = nil;
}

void KWQKHTMLPart::recordFormValue(const QString &name, const QString &value, HTMLFormElementImpl *element)
{
    // It's safe to assume that our own classes and basic Foundation
    // data structures won't raise exceptions

    if (!_formValuesAboutToBeSubmitted) {
        _formValuesAboutToBeSubmitted = KWQRetainNSRelease([[NSMutableDictionary alloc] init]);
        ASSERT(!_formAboutToBeSubmitted);
        _formAboutToBeSubmitted = KWQRetain([DOMElement _elementWithImpl:element]);
    } else {
        ASSERT([_formAboutToBeSubmitted _elementImpl] == element);
    }
    [_formValuesAboutToBeSubmitted setObject:value.getNSString() forKey:name.getNSString()];
}

void KWQKHTMLPart::submitForm(const KURL &url, const URLArgs &args)
{
    KWQ_BLOCK_EXCEPTIONS;

    // FIXME: We'd like to remove this altogether and fix the multiple form submission issue another way.
    // We do not want to submit more than one form from the same page,
    // nor do we want to submit a single form more than once.
    // This flag prevents these from happening; not sure how other browsers prevent this.
    // The flag is reset in each time we start handle a new mouse or key down event, and
    // also in setView since this part may get reused for a page from the back/forward cache.
    // The form multi-submit logic here is only needed when we are submitting a form that affects this frame.
    // FIXME: Frame targeting is only one of the ways the submission could end up doing something other
    // than replacing this frame's content, so this check is flawed. On the other hand, the check is hardly
    // needed any more now that we reset _submittedFormURL on each mouse or key down event.
    WebCoreBridge *target = args.frameName.isEmpty() ? _bridge : [_bridge findFrameNamed:args.frameName.getNSString()];
    KHTMLPart *targetPart = [target part];
    bool willReplaceThisFrame = false;
    for (KHTMLPart *p = this; p; p = p->parentPart()) {
        if (p == targetPart) {
            willReplaceThisFrame = true;
            break;
        }
    }
    if (willReplaceThisFrame) {
        if (_submittedFormURL == url) {
            return;
        }
        _submittedFormURL = url;
    }

    if (!args.doPost()) {
        [_bridge loadURL:url.getNSURL()
	        referrer:[_bridge referrer] 
                  reload:args.reload
             userGesture:true
  	          target:args.frameName.getNSString()
         triggeringEvent:_currentEvent
                    form:_formAboutToBeSubmitted
              formValues:_formValuesAboutToBeSubmitted];
    } else {
        ASSERT(args.contentType().startsWith("Content-Type: "));
        [_bridge postWithURL:url.getNSURL()
	            referrer:[_bridge referrer] 
                      target:args.frameName.getNSString()
                        data:arrayFromFormData(args.postData)
                 contentType:args.contentType().mid(14).getNSString()
             triggeringEvent:_currentEvent
                        form:_formAboutToBeSubmitted
                  formValues:_formValuesAboutToBeSubmitted];
    }
    clearRecordedFormValues();

    KWQ_UNBLOCK_EXCEPTIONS;
}

void KWQKHTMLPart::setEncoding(const QString &name, bool userChosen)
{
    if (!d->m_workingURL.isEmpty()) {
        receivedFirstData();
    }
    d->m_encoding = name;
    d->m_haveEncoding = userChosen;
}

void KWQKHTMLPart::addData(const char *bytes, int length)
{
    ASSERT(d->m_workingURL.isEmpty());
    ASSERT(d->m_doc);
    ASSERT(d->m_doc->parsing());
    write(bytes, length);
}

void KHTMLPart::frameDetached()
{
    // FIXME: This should be a virtual function, with the first part in KWQKHTMLPart, and the second
    // part in KHTMLPart, so it works for KHTML too.

    KWQ_BLOCK_EXCEPTIONS;
    [KWQ(this)->bridge() frameDetached];
    KWQ_UNBLOCK_EXCEPTIONS;

    KHTMLPart *parent = parentPart();
    if (parent) {
        FrameList& parentFrames = parent->d->m_frames;
        FrameIt end = parentFrames.end();
        for (FrameIt it = parentFrames.begin(); it != end; ++it) {
            ChildFrame &child = *it;
            if (child.m_part == this) {
                parent->disconnectChild(&child);
                parentFrames.remove(it);
                deref();
                break;
            }
        }
    }
}

void KWQKHTMLPart::urlSelected(const KURL &url, int button, int state, const URLArgs &args)
{
    KWQ_BLOCK_EXCEPTIONS;
    [_bridge loadURL:url.getNSURL()
            referrer:[_bridge referrer]
              reload:args.reload
         userGesture:true
              target:args.frameName.getNSString()
     triggeringEvent:_currentEvent
                form:nil
          formValues:nil];
    KWQ_UNBLOCK_EXCEPTIONS;
}

class KWQPluginPart : public ReadOnlyPart
{
    virtual bool openURL(const KURL &) { return true; }
    virtual bool closeURL() { return true; }
};

ReadOnlyPart *KWQKHTMLPart::createPart(const ChildFrame &child, const KURL &url, const QString &mimeType)
{
    KWQ_BLOCK_EXCEPTIONS;
    ReadOnlyPart *part;

    BOOL needFrame = [_bridge frameRequiredForMIMEType:mimeType.getNSString() URL:url.getNSURL()];
    if (child.m_type == ChildFrame::Object && !needFrame) {
        KWQPluginPart *newPart = new KWQPluginPart;
        newPart->setWidget(new QWidget([_bridge viewForPluginWithURL:url.getNSURL()
                                                      attributeNames:child.m_paramNames.getNSArray()
                                                     attributeValues:child.m_paramValues.getNSArray()
                                                             MIMEType:child.m_args.serviceType.getNSString()]));
        part = newPart;
    } else {
        LOG(Frames, "name %s", child.m_name.ascii());
        BOOL allowsScrolling = YES;
        int marginWidth = -1;
        int marginHeight = -1;
        if (child.m_type != ChildFrame::Object) {
            HTMLFrameElementImpl *o = static_cast<HTMLFrameElementImpl *>(child.m_frame->element());
            allowsScrolling = o->scrollingMode() != QScrollView::AlwaysOff;
            marginWidth = o->getMarginWidth();
            marginHeight = o->getMarginHeight();
        }
        WebCoreBridge *childBridge = [_bridge createChildFrameNamed:child.m_name.getNSString()
                                                            withURL:url.getNSURL()
                                                         renderPart:child.m_frame
                                                    allowsScrolling:allowsScrolling
                                                        marginWidth:marginWidth
                                                       marginHeight:marginHeight];
	// This call needs to return an object with a ref, since the caller will expect to own it.
	// childBridge owns the only ref so far.
	[childBridge part]->ref();
        part = [childBridge part];
    }

    return part;

    KWQ_UNBLOCK_EXCEPTIONS;

    return NULL;
}
    
void KWQKHTMLPart::setView(KHTMLView *view)
{
    // Detach the document now, so any onUnload handlers get run - if
    // we wait until the view is destroyed, then things won't be
    // hooked up enough for some JavaScript calls to work.
    if (d->m_doc && view == NULL) {
	d->m_doc->detach();
    }

    if (view) {
	view->ref();
    }
    if (d->m_view) {
	d->m_view->deref();
    }
    d->m_view = view;
    setWidget(view);
    
    // Only one form submission is allowed per view of a part.
    // Since this part may be getting reused as a result of being
    // pulled from the back/forward cache, reset this flag.
    _submittedFormURL = KURL();
}

KHTMLView *KWQKHTMLPart::view() const
{
    return d->m_view;
}

void KWQKHTMLPart::setTitle(const DOMString &title)
{
    QString text = title.string();
    text.replace('\\', backslashAsCurrencySymbol());

    KWQ_BLOCK_EXCEPTIONS;
    [_bridge setTitle:text.getNSString()];
    KWQ_UNBLOCK_EXCEPTIONS;
}

void KWQKHTMLPart::setStatusBarText(const QString &status)
{
    QString text = status;
    text.replace('\\', backslashAsCurrencySymbol());

    KWQ_BLOCK_EXCEPTIONS;
    [_bridge setStatusText:text.getNSString()];
    KWQ_UNBLOCK_EXCEPTIONS;
}

void KWQKHTMLPart::scheduleClose()
{
    KWQ_BLOCK_EXCEPTIONS;
    [_bridge closeWindowSoon];
    KWQ_UNBLOCK_EXCEPTIONS;
}

void KWQKHTMLPart::unfocusWindow()
{
    KWQ_BLOCK_EXCEPTIONS;
    [_bridge unfocusWindow];
    KWQ_UNBLOCK_EXCEPTIONS;
}

void KWQKHTMLPart::jumpToSelection()
{
    // Assumes that selection start will only ever be a text node. This is currently
    // true, but will it always be so?
    if (d->m_selection.start().isNotNull()) {
        RenderText *rt = dynamic_cast<RenderText *>(d->m_selection.start().node()->renderer());
        if (rt) {
            int x = 0, y = 0;
            rt->posOfChar(d->m_selection.start().offset(), x, y);
            // The -50 offset is copied from KHTMLPart::findTextNext, which sets the contents position
            // after finding a matched text string.
            d->m_view->setContentsPos(x - 50, y - 50);
        }
/*
        Something like this would fix <rdar://problem/3154293>: "Find Next should not scroll page if the next target is already visible"

        I think this would be a better way to do this, to avoid needless horizontal scrolling,
        but it is not feasible until selectionRect() returns a tighter rect around the
        selected text.  Right now it works at element granularity.
 
        NSView *docView = d->m_view->getDocumentView();

	KWQ_BLOCK_EXCEPTIONS;
        NSRect selRect = NSRect(selectionRect());
        NSRect visRect = [docView visibleRect];
        if (!NSContainsRect(visRect, selRect)) {
            // pad a bit so we overscroll slightly
            selRect = NSInsetRect(selRect, -10.0, -10.0);
            selRect = NSIntersectionRect(selRect, [docView bounds]);
            [docView scrollRectToVisible:selRect];
        }
	KWQ_UNBLOCK_EXCEPTIONS;
*/
    }
}

QString KWQKHTMLPart::advanceToNextMisspelling(bool startBeforeSelection)
{
    // The basic approach is to search in two phases - from the selection end to the end of the doc, and
    // then we wrap and search from the doc start to (approximately) where we started.
    
    // Start at the end of the selection, search to edge of document.  Starting at the selection end makes
    // repeated "check spelling" commands work.
    Range searchRange(xmlDocImpl());
    searchRange.selectNodeContents(xmlDocImpl());
    bool startedWithSelection = false;
    if (selectionStart()) {
        startedWithSelection = true;
        if (startBeforeSelection) {
            VisiblePosition start(selection().start());
            // We match AppKit's rule: Start 1 character before the selection.
            VisiblePosition oneBeforeStart = start.previous();
            setStart(searchRange, oneBeforeStart.isNotNull() ? oneBeforeStart : start);
        } else {
            setStart(searchRange, VisiblePosition(selection().end()));
        }
    }

    // If we're not in an editable node, try to find one, make that our range to work in
    NodeImpl *editableNodeImpl = searchRange.startContainer().handle();
    if (!editableNodeImpl->isContentEditable()) {
        editableNodeImpl = editableNodeImpl->nextEditable();
        if (!editableNodeImpl) {
            return QString();
        }
        searchRange.setStartBefore(editableNodeImpl);
        startedWithSelection = false;   // won't need to wrap
    }
    
    // topNode defines the whole range we want to operate on 
    Node topNode(editableNodeImpl->rootEditableElement());
    searchRange.setEndAfter(topNode);

    // Make sure start of searchRange is not in the middle of a word.  Jumping back a char and then
    // forward by a word happens to do the trick.
    if (startedWithSelection) {
        VisiblePosition oneBeforeStart = startVisiblePosition(searchRange).previous();
        if (oneBeforeStart.isNotNull()) {
            setStart(searchRange, endOfWord(oneBeforeStart));
        } // else we were already at the start of the editable node
    }
    
    if (searchRange.collapsed()) {
        return QString();       // nothing to search in
    }
    
    NSSpellChecker *checker = [NSSpellChecker sharedSpellChecker];
    WordAwareIterator it(searchRange);
    bool wrapped = false;
    
    // We go to the end of our first range instead of the start of it, just to be sure
    // we don't get foiled by any word boundary problems at the start.  It means we might
    // do a tiny bit more searching.
    Node searchEndAfterWrapNode = it.range().endContainer();
    long searchEndAfterWrapOffset = it.range().endOffset();

    while (1) {
        if (!it.atEnd()) {      // we may be starting at the end of the doc, and already by atEnd
            const QChar *chars = it.characters();
            long len = it.length();
            if (len > 1 || !chars[0].isSpace()) {
                NSString *chunk = [[NSString alloc] initWithCharactersNoCopy:(unichar *)chars length:len freeWhenDone:NO];
                NSRange misspelling = [checker checkSpellingOfString:chunk startingAt:0 language:nil wrap:NO inSpellDocumentWithTag:[_bridge spellCheckerDocumentTag] wordCount:NULL];
                [chunk release];
                if (misspelling.length > 0) {
                    // Build up result range and string.  Note the misspelling may span many text nodes,
                    // but the CharIterator insulates us from this complexity
                    Range misspellingRange(xmlDocImpl());
                    CharacterIterator chars(it.range());
                    chars.advance(misspelling.location);
                    misspellingRange.setStart(chars.range().startContainer(), chars.range().startOffset());
                    QString result = chars.string(misspelling.length);
                    misspellingRange.setEnd(chars.range().startContainer(), chars.range().startOffset());

                    setSelection(misspellingRange);
                    jumpToSelection();
                    // Mark misspelling in document.
                    xmlDocImpl()->addMarker(misspellingRange, DocumentMarker::Spelling);
                    return result;
                }
            }
        
            it.advance();
        }
        if (it.atEnd()) {
            if (wrapped || !startedWithSelection) {
                break;      // finished the second range, or we did the whole doc with the first range
            } else {
                // we've gone from the selection to the end of doc, now wrap around
                wrapped = YES;
                searchRange.setStartBefore(topNode);
                // going until the end of the very first chunk we tested is far enough
                searchRange.setEnd(searchEndAfterWrapNode, searchEndAfterWrapOffset);
                it = WordAwareIterator(searchRange);
            }
        }   
    }
    
    return QString();
}

void KWQKHTMLPart::redirectionTimerStartedOrStopped()
{
    // Don't report history navigations, just actual redirection.
    if (d->m_scheduledRedirection == historyNavigationScheduled) {
        return;
    }
    
    KWQ_BLOCK_EXCEPTIONS;
    if (d->m_redirectionTimer.isActive()) {
        [_bridge reportClientRedirectToURL:KURL(d->m_redirectURL).getNSURL()
                                     delay:d->m_delayRedirect
                                  fireDate:[d->m_redirectionTimer.getNSTimer() fireDate]
                               lockHistory:d->m_redirectLockHistory
                               isJavaScriptFormAction:d->m_executingJavaScriptFormAction];
    } else {
        [_bridge reportClientRedirectCancelled:d->m_cancelWithLoadInProgress];
    }
    KWQ_UNBLOCK_EXCEPTIONS;
}

void KWQKHTMLPart::paint(QPainter *p, const QRect &rect)
{
#ifndef NDEBUG
    bool fillWithRed;
    if (p->device()->devType() == QInternal::Printer)
        fillWithRed = false; // Printing, don't fill with red (can't remember why).
    else if (!xmlDocImpl() || xmlDocImpl()->ownerElement())
        fillWithRed = false; // Subframe, don't fill with red.
    else if (view() && view()->isTransparent())
        fillWithRed = false; // Transparent, don't fill with red.
    else if (_drawSelectionOnly)
        fillWithRed = false; // Selections are transparent, don't fill with red.
    else if (_elementToDraw != 0)
        fillWithRed = false; // Element images are transparent, don't fill with red.
    else
        fillWithRed = true;

    if (fillWithRed) {
        p->fillRect(rect.x(), rect.y(), rect.width(), rect.height(), QColor(0xFF, 0, 0));
    }
#endif

    if (renderer()) {
        // _elementToDraw is used to draw only one element
        RenderObject *eltRenderer = (_elementToDraw != 0) ? _elementToDraw.handle()->renderer() : 0;
        renderer()->layer()->paint(p, rect, _drawSelectionOnly, eltRenderer);

#if APPLE_CHANGES
        // Regions may have changed as a result of the visibility/z-index of element changing.
        if (renderer()->document()->dashboardRegionsDirty()){
            renderer()->canvas()->view()->updateDashboardRegions();
        }
#endif
    } else {
        ERROR("called KWQKHTMLPart::paint with nil renderer");
    }
}

void KWQKHTMLPart::adjustPageHeight(float *newBottom, float oldTop, float oldBottom, float bottomLimit)
{
    RenderCanvas *root = static_cast<RenderCanvas *>(xmlDocImpl()->renderer());
    if (root) {
        // Use a printer device, with painting disabled for the pagination phase
        QPainter painter(true);
        painter.setPaintingDisabled(true);

        root->setTruncatedAt((int)floor(oldBottom));
        QRect dirtyRect(0, (int)floor(oldTop),
                        root->docWidth(), (int)ceil(oldBottom-oldTop));
        root->layer()->paint(&painter, dirtyRect);
        *newBottom = root->bestTruncatedAt();
        if (*newBottom == 0) {
            *newBottom = oldBottom;
        }
    } else {
        *newBottom = oldBottom;
    }
}

RenderObject *KWQKHTMLPart::renderer() const
{
    DocumentImpl *doc = xmlDocImpl();
    return doc ? doc->renderer() : 0;
}

QString KWQKHTMLPart::userAgent() const
{
    KWQ_BLOCK_EXCEPTIONS;
    return QString::fromNSString([_bridge userAgentForURL:m_url.getNSURL()]);
    KWQ_UNBLOCK_EXCEPTIONS;
         
    return QString();
}

QString KWQKHTMLPart::mimeTypeForFileName(const QString &fileName) const
{
    NSString *ns = fileName.getNSString();

    KWQ_BLOCK_EXCEPTIONS;
    return QString::fromNSString([_bridge MIMETypeForPath:ns]);
    KWQ_UNBLOCK_EXCEPTIONS;

    return QString();
}

NSView *KWQKHTMLPart::nextKeyViewInFrame(NodeImpl *node, KWQSelectionDirection direction)
{
    DocumentImpl *doc = xmlDocImpl();
    if (!doc) {
        return nil;
    }
    for (;;) {
        node = direction == KWQSelectingNext
            ? doc->nextFocusNode(node) : doc->previousFocusNode(node);
        if (!node) {
            return nil;
        }
        RenderWidget *renderWidget = dynamic_cast<RenderWidget *>(node->renderer());
        if (renderWidget) {
            QWidget *widget = renderWidget->widget();
            KHTMLView *childFrameWidget = dynamic_cast<KHTMLView *>(widget);
            NSView *view = nil;
            if (childFrameWidget) {
                view = KWQ(childFrameWidget->part())->nextKeyViewInFrame(0, direction);
            } else if (widget) {
                view = widget->getView();
            }
            if (view) {
                return view;
            }
        }
        else {
            doc->setFocusNode(node);
            if (view()) {
                view()->ensureRectVisibleCentered(node->getRect());
            }
            [_bridge makeFirstResponder:[_bridge documentView]];
            return [_bridge documentView];
        }
    }
}

NSView *KWQKHTMLPart::nextKeyViewInFrameHierarchy(NodeImpl *node, KWQSelectionDirection direction)
{
    NSView *next = nextKeyViewInFrame(node, direction);
    if (next) {
        return next;
    }

    // remove focus from currently focused node
    DocumentImpl *doc = xmlDocImpl();
    if (doc) {
        doc->setFocusNode(0);
    }
    
    KWQKHTMLPart *parent = KWQ(parentPart());
    if (parent) {
        next = parent->nextKeyView(parent->childFrame(this)->m_frame->element(), direction);
        if (next) {
            return next;
        }
    }
    
    return nil;
}

NSView *KWQKHTMLPart::nextKeyView(NodeImpl *node, KWQSelectionDirection direction)
{
    KWQ_BLOCK_EXCEPTIONS;

    NSView * next = nextKeyViewInFrameHierarchy(node, direction);
    if (next) {
        return next;
    }

    // Look at views from the top level part up, looking for a next key view that we can use.

    next = direction == KWQSelectingNext
        ? [_bridge nextKeyViewOutsideWebFrameViews]
        : [_bridge previousKeyViewOutsideWebFrameViews];

    if (next) {
        return next;
    }

    KWQ_UNBLOCK_EXCEPTIONS;
    
    // If all else fails, make a loop by starting from 0.
    return nextKeyViewInFrameHierarchy(0, direction);
}

NSView *KWQKHTMLPart::nextKeyViewForWidget(QWidget *startingWidget, KWQSelectionDirection direction)
{
    // Use the event filter object to figure out which RenderWidget owns this QWidget and get to the DOM.
    // Then get the next key view in the order determined by the DOM.
    NodeImpl *node = nodeForWidget(startingWidget);
    ASSERT(node);
    return partForNode(node)->nextKeyView(node, direction);
}

bool KWQKHTMLPart::currentEventIsMouseDownInWidget(QWidget *candidate)
{
    KWQ_BLOCK_EXCEPTIONS;
    switch ([[NSApp currentEvent] type]) {
        case NSLeftMouseDown:
        case NSRightMouseDown:
        case NSOtherMouseDown:
            break;
        default:
            return NO;
    }
    KWQ_UNBLOCK_EXCEPTIONS;
    
    NodeImpl *node = nodeForWidget(candidate);
    ASSERT(node);
    return partForNode(node)->nodeUnderMouse() == node;
}

bool KWQKHTMLPart::currentEventIsKeyboardOptionTab()
{
    KWQ_BLOCK_EXCEPTIONS;
    NSEvent *evt = [NSApp currentEvent];
    if ([evt type] != NSKeyDown) {
        return NO;
    }

    if (([evt modifierFlags] & NSAlternateKeyMask) == 0) {
        return NO;
    }
    
    NSString *chars = [evt charactersIgnoringModifiers];
    if ([chars length] != 1)
        return NO;
    
    const unichar tabKey = 0x0009;
    const unichar shiftTabKey = 0x0019;
    unichar c = [chars characterAtIndex:0];
    if (c != tabKey && c != shiftTabKey)
        return NO;
    
    KWQ_UNBLOCK_EXCEPTIONS;
    return YES;
}

bool KWQKHTMLPart::handleKeyboardOptionTabInView(NSView *view)
{
    if (KWQKHTMLPart::currentEventIsKeyboardOptionTab()) {
        if (([[NSApp currentEvent] modifierFlags] & NSShiftKeyMask) != 0) {
            [[view window] selectKeyViewPrecedingView:view];
        } else {
            [[view window] selectKeyViewFollowingView:view];
        }
        return YES;
    }
    
    return NO;
}

bool KWQKHTMLPart::tabsToLinks() const
{
    if ([_bridge keyboardUIMode] & WebCoreKeyboardAccessTabsToLinks)
        return !KWQKHTMLPart::currentEventIsKeyboardOptionTab();
    else
        return KWQKHTMLPart::currentEventIsKeyboardOptionTab();
}

bool KWQKHTMLPart::tabsToAllControls() const
{
    WebCoreKeyboardUIMode keyboardUIMode = [_bridge keyboardUIMode];
    BOOL handlingOptionTab = KWQKHTMLPart::currentEventIsKeyboardOptionTab();

    // If tab-to-links is off, option-tab always highlights all controls
    if ((keyboardUIMode & WebCoreKeyboardAccessTabsToLinks) == 0 && handlingOptionTab) {
        return YES;
    }
    
    // If system preferences say to include all controls, we always include all controls
    if (keyboardUIMode & WebCoreKeyboardAccessFull) {
        return YES;
    }
    
    // Otherwise tab-to-links includes all controls, unless the sense is flipped via option-tab.
    if (keyboardUIMode & WebCoreKeyboardAccessTabsToLinks) {
        return !handlingOptionTab;
    }
    
    return handlingOptionTab;
}

KJS::Bindings::RootObject *KWQKHTMLPart::bindingRootObject()
{
    if (!_bindingRoot) {
        _bindingRoot = new KJS::Bindings::RootObject(0);    // The root gets deleted by JavaScriptCore.
        KJS::ObjectImp *win = static_cast<KJS::ObjectImp *>(KJS::Window::retrieveWindow(this));
        _bindingRoot->setRootObjectImp (win);
        _bindingRoot->setInterpreter (KJSProxy::proxy(this)->interpreter());
        addPluginRootObject (_bindingRoot);
    }
    return _bindingRoot;
}

WebScriptObject *KWQKHTMLPart::windowScriptObject()
{
    if (!_windowScriptObject) {
        KJS::ObjectImp *win = static_cast<KJS::ObjectImp *>(KJS::Window::retrieveWindow(this));
        _windowScriptObject = KWQRetainNSRelease([[WebScriptObject alloc] _initWithObjectImp:win root:bindingRootObject()]);
    }

    return _windowScriptObject;
}

NPObject *KWQKHTMLPart::windowScriptNPObject()
{
    if (!_windowScriptNPObject) {
        KJS::ObjectImp *win = static_cast<KJS::ObjectImp *>(KJS::Window::retrieveWindow(this));
        _windowScriptNPObject = _NPN_CreateScriptObject (0, win, bindingRootObject());
    }

    return _windowScriptNPObject;
}

void KWQKHTMLPart::partClearedInBegin()
{
    [_bridge windowObjectCleared];
}

QMap<int, ScheduledAction*> *KWQKHTMLPart::pauseActions(const void *key)
{
    if (d->m_doc && d->m_jscript) {
        Window *w = Window::retrieveWindow(this);
        if (w && w->hasTimeouts()) {
            return w->pauseTimeouts(key);
        }
    }
    return 0;
}

void KWQKHTMLPart::resumeActions(QMap<int, ScheduledAction*> *actions, const void *key)
{
    if (d->m_doc && d->m_jscript && d->m_bJScriptEnabled) {
        Window *w = Window::retrieveWindow(this);
        if (w) {
            w->resumeTimeouts(actions, key);
        }
    }
}

bool KWQKHTMLPart::canCachePage()
{
    // Only save page state if:
    // 1.  We're not a frame or frameset.
    // 2.  The page has no unload handler.
    // 3.  The page has no password fields.
    // 4.  The URL for the page is not https.
    // 5.  The page has no applets.
    if (d->m_frames.count() ||
        parentPart() ||
        m_url.protocol().startsWith("https") || 
	(d->m_doc && (htmlDocument().applets().length() != 0 ||
                      d->m_doc->hasWindowEventListener(EventImpl::UNLOAD_EVENT) ||
		      d->m_doc->hasPasswordField()))) {
        return false;
    }
    return true;
}

void KWQKHTMLPart::saveWindowProperties(SavedProperties *windowProperties)
{
    Window *window = Window::retrieveWindow(this);
    if (window)
        window->saveProperties(*windowProperties);
}

void KWQKHTMLPart::saveLocationProperties(SavedProperties *locationProperties)
{
    Window *window = Window::retrieveWindow(this);
    if (window) {
        Interpreter::lock();
        Location *location = window->location();
        Interpreter::unlock();
        location->saveProperties(*locationProperties);
    }
}

void KWQKHTMLPart::restoreWindowProperties(SavedProperties *windowProperties)
{
    Window *window = Window::retrieveWindow(this);
    if (window)
        window->restoreProperties(*windowProperties);
}

void KWQKHTMLPart::restoreLocationProperties(SavedProperties *locationProperties)
{
    Window *window = Window::retrieveWindow(this);
    if (window) {
        Interpreter::lock();
        Location *location = window->location();
        Interpreter::unlock();
        location->restoreProperties(*locationProperties);
    }
}

void KWQKHTMLPart::saveInterpreterBuiltins(SavedBuiltins &interpreterBuiltins)
{
    if (jScript() && jScript()->interpreter()) {
	jScript()->interpreter()->saveBuiltins(interpreterBuiltins);
    }
}

void KWQKHTMLPart::restoreInterpreterBuiltins(const SavedBuiltins &interpreterBuiltins)
{
    if (jScript() && jScript()->interpreter()) {
	jScript()->interpreter()->restoreBuiltins(interpreterBuiltins);
    }
}

void KWQKHTMLPart::openURLFromPageCache(KWQPageState *state)
{
    // It's safe to assume none of the KWQPageState methods will raise
    // exceptions, since KWQPageState is implemented by WebCore and
    // does not throw

    DocumentImpl *doc = [state document];
    KURL *url = [state URL];
    SavedProperties *windowProperties = [state windowProperties];
    SavedProperties *locationProperties = [state locationProperties];
    SavedBuiltins *interpreterBuiltins = [state interpreterBuiltins];
    QMap<int, ScheduledAction*> *actions = [state pausedActions];
    
    cancelRedirection();

    // We still have to close the previous part page.
    if (!d->m_restored){
        closeURL();
    }
            
    d->m_bComplete = false;
    
    // Don't re-emit the load event.
    d->m_bLoadEventEmitted = true;
    
    // delete old status bar msg's from kjs (if it _was_ activated on last URL)
    if( d->m_bJScriptEnabled )
    {
        d->m_kjsStatusBarText = QString::null;
        d->m_kjsDefaultStatusBarText = QString::null;
    }

    ASSERT (url);
    
    m_url = *url;
    
    // initializing m_url to the new url breaks relative links when opening such a link after this call and _before_ begin() is called (when the first
    // data arrives) (Simon)
    if(m_url.protocol().startsWith( "http" ) && !m_url.host().isEmpty() && m_url.path().isEmpty()) {
        m_url.setPath("/");
        emit d->m_extension->setLocationBarURL( m_url.prettyURL() );
    }
    
    // copy to m_workingURL after fixing m_url above
    d->m_workingURL = m_url;
        
    emit started( 0L );
    
    // -----------begin-----------
    clear();

    doc->setInPageCache(NO);

    d->m_bCleared = false;
    d->m_cacheId = 0;
    d->m_bComplete = false;
    d->m_bLoadEventEmitted = false;
    d->m_referrer = m_url.url();
    
    setView(doc->view());
    
    d->m_doc = doc;
    d->m_doc->ref();
    
    Decoder *decoder = doc->decoder();
    if (decoder) {
        decoder->ref();
    }
    if (d->m_decoder) {
        d->m_decoder->deref();
    }
    d->m_decoder = decoder;

    updatePolicyBaseURL();
        
    restoreWindowProperties (windowProperties);
    restoreLocationProperties (locationProperties);
    restoreInterpreterBuiltins (*interpreterBuiltins);

    if (actions)
        resumeActions (actions, state);
    
    checkCompleted();
}

KWQKHTMLPart *KWQKHTMLPart::partForWidget(const QWidget *widget)
{
    ASSERT_ARG(widget, widget);
    
    NodeImpl *node = nodeForWidget(widget);
    if (node) {
	return partForNode(node);
    }
    
    // Assume all widgets are either form controls, or KHTMLViews.
    const KHTMLView *view = dynamic_cast<const KHTMLView *>(widget);
    ASSERT(view);
    return KWQ(view->part());
}

WebCoreBridge *KWQKHTMLPart::bridgeForWidget(const QWidget *widget)
{
    ASSERT_ARG(widget, widget);
    
    KWQKHTMLPart *part = partForWidget(widget);
    ASSERT(part);
    return part->bridge();
}

KWQKHTMLPart *KWQKHTMLPart::partForNode(NodeImpl *node)
{
    ASSERT_ARG(node, node);
    return KWQ(node->getDocument()->part());
}

NSView *KWQKHTMLPart::documentViewForNode(DOM::NodeImpl *node)
{
    WebCoreBridge *bridge = partForNode(node)->bridge();
    return [bridge documentView];
}

NodeImpl *KWQKHTMLPart::nodeForWidget(const QWidget *widget)
{
    ASSERT_ARG(widget, widget);
    const QObject *o = widget->eventFilterObject();
    return o ? static_cast<const RenderWidget *>(o)->element() : 0;
}

void KWQKHTMLPart::setDocumentFocus(QWidget *widget)
{
    NodeImpl *node = nodeForWidget(widget);
    if (node) {
        node->getDocument()->setFocusNode(node);
    } else {
        ERROR("unable to clear focus because widget had no corresponding node");
    }
}

void KWQKHTMLPart::clearDocumentFocus(QWidget *widget)
{
    NodeImpl *node = nodeForWidget(widget);
    if (node) {
    	node->getDocument()->setFocusNode(0);
    } else {
        ERROR("unable to clear focus because widget had no corresponding node");
    }
}

void KWQKHTMLPart::saveDocumentState()
{
    // Do not save doc state if the page has a password field and a form that would be submitted
    // via https
    if (!(d->m_doc && d->m_doc->hasPasswordField() && d->m_doc->hasSecureForm())) {
	KWQ_BLOCK_EXCEPTIONS;
        [_bridge saveDocumentState];
	KWQ_UNBLOCK_EXCEPTIONS;
    }
}

void KWQKHTMLPart::restoreDocumentState()
{
    KWQ_BLOCK_EXCEPTIONS;
    [_bridge restoreDocumentState];
    KWQ_UNBLOCK_EXCEPTIONS;
}

QPtrList<KWQKHTMLPart> &KWQKHTMLPart::mutableInstances()
{
    static QPtrList<KWQKHTMLPart> instancesList;
    return instancesList;
}

void KWQKHTMLPart::updatePolicyBaseURL()
{
    // FIXME: docImpl() returns null for everything other than HTML documents; is this causing problems? -dwh
    if (parentPart() && parentPart()->docImpl()) {
        setPolicyBaseURL(parentPart()->docImpl()->policyBaseURL());
    } else {
        setPolicyBaseURL(m_url.url());
    }
}

void KWQKHTMLPart::setPolicyBaseURL(const DOMString &s)
{
    // FIXME: XML documents will cause this to return null.  docImpl() is
    // an HTMLdocument only. -dwh
    if (docImpl())
        docImpl()->setPolicyBaseURL(s);
    ConstFrameIt end = d->m_frames.end();
    for (ConstFrameIt it = d->m_frames.begin(); it != end; ++it) {
        ReadOnlyPart *subpart = (*it).m_part;
        static_cast<KWQKHTMLPart *>(subpart)->setPolicyBaseURL(s);
    }
}

QString KWQKHTMLPart::requestedURLString() const
{
    KWQ_BLOCK_EXCEPTIONS;
    return QString::fromNSString([_bridge requestedURLString]);
    KWQ_UNBLOCK_EXCEPTIONS;

    return QString();
}

QString KWQKHTMLPart::incomingReferrer() const
{
    KWQ_BLOCK_EXCEPTIONS;
    return QString::fromNSString([_bridge incomingReferrer]);
    KWQ_UNBLOCK_EXCEPTIONS;

    return QString();
}

void KWQKHTMLPart::forceLayout()
{
    KHTMLView *v = d->m_view;
    if (v) {
        v->layout();
        // We cannot unschedule a pending relayout, since the force can be called with
        // a tiny rectangle from a drawRect update.  By unscheduling we in effect
        // "validate" and stop the necessary full repaint from occurring.  Basically any basic
        // append/remove DHTML is broken by this call.  For now, I have removed the optimization
        // until we have a better invalidation stategy. -dwh
        //v->unscheduleRelayout();
    }
}

void KWQKHTMLPart::forceLayoutWithPageWidthRange(float minPageWidth, float maxPageWidth)
{
    // Dumping externalRepresentation(_part->renderer()).ascii() is a good trick to see
    // the state of things before and after the layout
    RenderCanvas *root = static_cast<RenderCanvas *>(xmlDocImpl()->renderer());
    if (root) {
        // This magic is basically copied from khtmlview::print
        int pageW = (int)ceil(minPageWidth);
        root->setWidth(pageW);
        root->setNeedsLayoutAndMinMaxRecalc();
        forceLayout();
        
        // If we don't fit in the minimum page width, we'll lay out again. If we don't fit in the
        // maximum page width, we will lay out to the maximum page width and clip extra content.
        // FIXME: We are assuming a shrink-to-fit printing implementation.  A cropping
        // implementation should not do this!
        int rightmostPos = root->rightmostPosition();
        if (rightmostPos > minPageWidth) {
            pageW = kMin(rightmostPos, (int)ceil(maxPageWidth));
            root->setWidth(pageW);
            root->setNeedsLayoutAndMinMaxRecalc();
            forceLayout();
        }
    }
}

void KWQKHTMLPart::sendResizeEvent()
{
    KHTMLView *v = d->m_view;
    if (v) {
	QResizeEvent e;
	v->resizeEvent(&e);
    }
}

void KWQKHTMLPart::sendScrollEvent()
{
    KHTMLView *v = d->m_view;
    if (v) {
        DocumentImpl *doc = xmlDocImpl();
        if (!doc)
            return;
        doc->dispatchHTMLEvent(EventImpl::SCROLL_EVENT, true, false);
    }
}

void KWQKHTMLPart::runJavaScriptAlert(const QString &message)
{
    QString text = message;
    text.replace('\\', backslashAsCurrencySymbol());
    KWQ_BLOCK_EXCEPTIONS;
    [_bridge runJavaScriptAlertPanelWithMessage:text.getNSString()];
    KWQ_UNBLOCK_EXCEPTIONS;
}

bool KWQKHTMLPart::runJavaScriptConfirm(const QString &message)
{
    QString text = message;
    text.replace('\\', backslashAsCurrencySymbol());

    KWQ_BLOCK_EXCEPTIONS;
    return [_bridge runJavaScriptConfirmPanelWithMessage:text.getNSString()];
    KWQ_UNBLOCK_EXCEPTIONS;

    return false;
}

bool KWQKHTMLPart::runJavaScriptPrompt(const QString &prompt, const QString &defaultValue, QString &result)
{
    QString promptText = prompt;
    promptText.replace('\\', backslashAsCurrencySymbol());
    QString defaultValueText = defaultValue;
    defaultValueText.replace('\\', backslashAsCurrencySymbol());

    KWQ_BLOCK_EXCEPTIONS;
    NSString *returnedText = nil;

    bool ok = [_bridge runJavaScriptTextInputPanelWithPrompt:prompt.getNSString()
	       defaultText:defaultValue.getNSString() returningText:&returnedText];

    if (ok) {
        result = QString::fromNSString(returnedText);
        result.replace(backslashAsCurrencySymbol(), '\\');
    }

    return ok;
    KWQ_UNBLOCK_EXCEPTIONS;
    
    return false;
}

bool KWQKHTMLPart::locationbarVisible()
{
    return [_bridge areToolbarsVisible];
}

bool KWQKHTMLPart::menubarVisible()
{
    // The menubar is always on in Mac OS X UI
    return true;
}

bool KWQKHTMLPart::personalbarVisible()
{
    return [_bridge areToolbarsVisible];
}

bool KWQKHTMLPart::scrollbarsVisible()
{
    if (!view())
	return false;

    if (view()->hScrollBarMode() == QScrollView::AlwaysOff || view()->vScrollBarMode() == QScrollView::AlwaysOff)
	return false;

    return true;
}

bool KWQKHTMLPart::statusbarVisible()
{
    return [_bridge isStatusBarVisible];
}

bool KWQKHTMLPart::toolbarVisible()
{
    return [_bridge areToolbarsVisible];
}

void KWQKHTMLPart::addMessageToConsole(const QString &message, unsigned lineNumber, const QString &sourceURL)
{
    NSDictionary *dictionary = [NSDictionary dictionaryWithObjectsAndKeys:
        message.getNSString(), @"message",
        [NSNumber numberWithInt: lineNumber], @"lineNumber",
        sourceURL.getNSString(), @"sourceURL",
        NULL];
    [_bridge addMessageToConsole:dictionary];
}

void KWQKHTMLPart::createEmptyDocument()
{
    // Although it's not completely clear from the name of this function,
    // it does nothing if we already have a document, and just creates an
    // empty one if we have no document at all.
    if (!d->m_doc) {
	KWQ_BLOCK_EXCEPTIONS;
        [_bridge loadEmptyDocumentSynchronously];
	KWQ_UNBLOCK_EXCEPTIONS;

	if (parentPart() && (parentPart()->childFrame(this)->m_type == ChildFrame::IFrame ||
			     parentPart()->childFrame(this)->m_type == ChildFrame::Object)) {
	    d->m_doc->setBaseURL(parentPart()->d->m_doc->baseURL());
	}
    }
}

void KWQKHTMLPart::addMetaData(const QString &key, const QString &value)
{
    d->m_job->addMetaData(key, value);
}

bool KWQKHTMLPart::keyEvent(NSEvent *event)
{
    KWQ_BLOCK_EXCEPTIONS;

    ASSERT([event type] == NSKeyDown || [event type] == NSKeyUp);

    // Check for cases where we are too early for events -- possible unmatched key up
    // from pressing return in the location bar.
    DocumentImpl *doc = xmlDocImpl();
    if (!doc) {
        return false;
    }
    NodeImpl *node = doc->focusNode();
    if (!node && docImpl()) {
	node = docImpl()->body();
    }
    if (!node) {
        return false;
    }
    
    if ([event type] == NSKeyDown) {
        prepareForUserAction();
    }

    NSEvent *oldCurrentEvent = _currentEvent;
    _currentEvent = KWQRetain(event);

    QKeyEvent qEvent(event);
    bool result = !node->dispatchKeyEvent(&qEvent);

    // We want to send both a down and a press for the initial key event.
    // To get KHTML to do this, we send a second KeyPress QKeyEvent with "is repeat" set to true,
    // which causes it to send a press to the DOM.
    // That's not a great hack; it would be good to do this in a better way.
    if ([event type] == NSKeyDown && ![event isARepeat]) {
	QKeyEvent repeatEvent(event, true);
        if (!node->dispatchKeyEvent(&repeatEvent)) {
	    result = true;
	}
    }

    ASSERT(_currentEvent == event);
    KWQRelease(event);
    _currentEvent = oldCurrentEvent;

    return result;

    KWQ_UNBLOCK_EXCEPTIONS;

    return false;
}

// This does the same kind of work that KHTMLPart::openURL does, except it relies on the fact
// that a higher level already checked that the URLs match and the scrolling is the right thing to do.
void KWQKHTMLPart::scrollToAnchor(const KURL &URL)
{
    cancelRedirection();

    m_url = URL;
    started(0);

    gotoAnchor();

    // It's important to model this as a load that starts and immediately finishes.
    // Otherwise, the parent frame may think we never finished loading.
    d->m_bComplete = false;
    checkCompleted();
}

bool KWQKHTMLPart::closeURL()
{
    saveDocumentState();
    return KHTMLPart::closeURL();
}

void KWQKHTMLPart::khtmlMousePressEvent(MousePressEvent *event)
{
    bool singleClick = [_currentEvent clickCount] <= 1;

    // If we got the event back, that must mean it wasn't prevented,
    // so it's allowed to start a drag or selection.
    _mouseDownMayStartSelect = true;
    // Careful that the drag starting logic stays in sync with eventMayStartDrag()
    _mouseDownMayStartDrag = singleClick;

    if (!passWidgetMouseDownEventToWidget(event)) {
        // We don't do this at the start of mouse down handling (before calling into WebCore),
        // because we don't want to do it until we know we didn't hit a widget.
        NSView *view = d->m_view->getDocumentView();

        if (singleClick) {
            KWQ_BLOCK_EXCEPTIONS;
            if ([_bridge firstResponder] != view) {
                [_bridge makeFirstResponder:view];
            }
            KWQ_UNBLOCK_EXCEPTIONS;
        }

        KHTMLPart::khtmlMousePressEvent(event);
    }
}

void KWQKHTMLPart::khtmlMouseDoubleClickEvent(MouseDoubleClickEvent *event)
{
    if (!passWidgetMouseDownEventToWidget(event)) {
        KHTMLPart::khtmlMouseDoubleClickEvent(event);
    }
}

bool KWQKHTMLPart::passWidgetMouseDownEventToWidget(khtml::MouseEvent *event)
{
    // Figure out which view to send the event to.
    RenderObject *target = event->innerNode().handle() ? event->innerNode().handle()->renderer() : 0;
    if (!target)
        return false;

    QWidget* widget = RenderLayer::gScrollBar;
    if (!widget) {
        if (!target->isWidget())
            return false;
        widget = static_cast<RenderWidget *>(target)->widget();
    }

    // Doubleclick events don't exist in Cocoa.  Since passWidgetMouseDownEventToWidget will
    // just pass _currentEvent down to the widget,  we don't want to call it for events that
    // don't correspond to Cocoa events.  The mousedown/ups will have already been passed on as
    // part of the pressed/released handling.
    if (!MouseDoubleClickEvent::test(event))
        return passWidgetMouseDownEventToWidget(widget);
    else
        return true;
}

bool KWQKHTMLPart::passWidgetMouseDownEventToWidget(RenderWidget *renderWidget)
{
    return passWidgetMouseDownEventToWidget(renderWidget->widget());
}

bool KWQKHTMLPart::passWidgetMouseDownEventToWidget(QWidget* widget)
{
    // FIXME: this method always returns true

    if (!widget) {
        ERROR("hit a RenderWidget without a corresponding QWidget, means a frame is half-constructed");
        return true;
    }

    KWQ_BLOCK_EXCEPTIONS;
    
    NSView *nodeView = widget->getView();
    ASSERT(nodeView);
    ASSERT([nodeView superview]);
    NSView *view = [nodeView hitTest:[[nodeView superview] convertPoint:[_currentEvent locationInWindow] fromView:nil]];
    if (view == nil) {
        ERROR("KHTML says we hit a RenderWidget, but AppKit doesn't agree we hit the corresponding NSView");
        return true;
    }
    
    if ([_bridge firstResponder] == view) {
        // In the case where we just became first responder, we should send the mouseDown:
        // to the NSTextField, not the NSTextField's editor. This code makes sure that happens.
        // If we don't do this, we see a flash of selected text when clicking in a text field.
        if (_firstResponderAtMouseDownTime != view && [view isKindOfClass:[NSTextView class]]) {
            NSView *superview = view;
            while (superview != nodeView) {
                superview = [superview superview];
                ASSERT(superview);
                if ([superview isKindOfClass:[NSControl class]]) {
                    NSControl *control = superview;
                    if ([control currentEditor] == view) {
                        view = superview;
                    }
                    break;
                }
            }
        }
    } else {
        // Normally [NSWindow sendEvent:] handles setting the first responder.
        // But in our case, the event was sent to the view representing the entire web page.
        if ([_currentEvent clickCount] <= 1 && [view acceptsFirstResponder] && [view needsPanelToBecomeKey]) {
            [_bridge makeFirstResponder:view];
        }
    }

    // We need to "defer loading" and defer timers while we are tracking the mouse.
    // That's because we don't want the new page to load while the user is holding the mouse down.
    
    BOOL wasDeferringLoading = [_bridge defersLoading];
    if (!wasDeferringLoading) {
        [_bridge setDefersLoading:YES];
    }
    BOOL wasDeferringTimers = QObject::defersTimers();
    if (!wasDeferringTimers) {
        QObject::setDefersTimers(true);
    }

    ASSERT(!_sendingEventToSubview);
    _sendingEventToSubview = true;
    [view mouseDown:_currentEvent];
    _sendingEventToSubview = false;
    
    if (!wasDeferringTimers) {
        QObject::setDefersTimers(false);
    }
    if (!wasDeferringLoading) {
        [_bridge setDefersLoading:NO];
    }

    // Remember which view we sent the event to, so we can direct the release event properly.
    _mouseDownView = view;
    _mouseDownWasInSubframe = false;

    KWQ_UNBLOCK_EXCEPTIONS;

    return true;
}

bool KWQKHTMLPart::lastEventIsMouseUp() const
{
    // Many AK widgets run their own event loops and consume events while the mouse is down.
    // When they finish, currentEvent is the mouseUp that they exited on.  We need to update
    // the khtml state with this mouseUp, which khtml never saw.  This method lets us detect
    // that state.

    KWQ_BLOCK_EXCEPTIONS;
    NSEvent *currentEventAfterHandlingMouseDown = [NSApp currentEvent];
    if (_currentEvent != currentEventAfterHandlingMouseDown) {
        if ([currentEventAfterHandlingMouseDown type] == NSLeftMouseUp) {
            return true;
        }
    }
    KWQ_UNBLOCK_EXCEPTIONS;

    return false;
}
    
// Note that this does the same kind of check as [target isDescendantOf:superview].
// There are two differences: This is a lot slower because it has to walk the whole
// tree, and this works in cases where the target has already been deallocated.
static bool findViewInSubviews(NSView *superview, NSView *target)
{
    KWQ_BLOCK_EXCEPTIONS;
    NSEnumerator *e = [[superview subviews] objectEnumerator];
    NSView *subview;
    while ((subview = [e nextObject])) {
        if (subview == target || findViewInSubviews(subview, target)) {
            return true;
	}
    }
    KWQ_UNBLOCK_EXCEPTIONS;
    
    return false;
}

NSView *KWQKHTMLPart::mouseDownViewIfStillGood()
{
    // Since we have no way of tracking the lifetime of _mouseDownView, we have to assume that
    // it could be deallocated already. We search for it in our subview tree; if we don't find
    // it, we set it to nil.
    NSView *mouseDownView = _mouseDownView;
    if (!mouseDownView) {
        return nil;
    }
    KHTMLView *topKHTMLView = d->m_view;
    NSView *topView = topKHTMLView ? topKHTMLView->getView() : nil;
    if (!topView || !findViewInSubviews(topView, mouseDownView)) {
        _mouseDownView = nil;
        return nil;
    }
    return mouseDownView;
}

// The link drag hysteresis is much larger than the others because there
// needs to be enough space to cancel the link press without starting a link drag,
// and because dragging links is rare.
#define LinkDragHysteresis              40.0
#define ImageDragHysteresis              5.0
#define TextDragHysteresis               3.0
#define GeneralDragHysteresis            3.0

#define TextDragDelay                    0.15

bool KWQKHTMLPart::dragHysteresisExceeded(float dragLocationX, float dragLocationY) const
{
    int dragX, dragY;
    d->m_view->viewportToContents((int)dragLocationX, (int)dragLocationY, dragX, dragY);
    float deltaX = QABS(dragX - _mouseDownX);
    float deltaY = QABS(dragY - _mouseDownY);

    float threshold = GeneralDragHysteresis;
    if (_dragSrcIsImage) {
        threshold = ImageDragHysteresis;
    } else if (_dragSrcIsLink) {
        threshold = LinkDragHysteresis;
    } else if (_dragSrcInSelection) {
        threshold = TextDragHysteresis;
    }
    return deltaX >= threshold || deltaY >= threshold;
}

// returns if we should continue "default processing", i.e., whether eventhandler canceled
bool KWQKHTMLPart::dispatchDragSrcEvent(int eventId, const QPoint &loc) const
{
    bool noDefaultProc = d->m_view->dispatchDragEvent(eventId, _dragSrc.handle(), loc, _dragClipboard);
    return !noDefaultProc;
}

bool KWQKHTMLPart::eventMayStartDrag(NSEvent *event) const
{
    // This is a pre-flight check of whether the event might lead to a drag being started.  Be careful
    // that its logic needs to stay in sync with khtmlMouseMoveEvent() and the way we set
    // _mouseDownMayStartDrag in khtmlMousePressEvent
    
    if ([event type] != NSLeftMouseDown || [event clickCount] != 1) {
        return false;
    }
    
    BOOL DHTMLFlag, UAFlag;
    [_bridge allowDHTMLDrag:&DHTMLFlag UADrag:&UAFlag];
    if (!DHTMLFlag && !UAFlag) {
        return false;
    }

    NSPoint loc = [event locationInWindow];
    int mouseDownX, mouseDownY;
    d->m_view->viewportToContents((int)loc.x, (int)loc.y, mouseDownX, mouseDownY);
    RenderObject::NodeInfo nodeInfo(true, false);
    renderer()->layer()->nodeAtPoint(nodeInfo, mouseDownX, mouseDownY);
    bool srcIsDHTML;
    Node possibleSrc = nodeInfo.innerNode()->renderer()->draggableNode(DHTMLFlag, UAFlag, mouseDownX, mouseDownY, srcIsDHTML);
    return !possibleSrc.isNull();
}

void KWQKHTMLPart::khtmlMouseMoveEvent(MouseMoveEvent *event)
{
    KWQ_BLOCK_EXCEPTIONS;

    if ([_currentEvent type] == NSLeftMouseDragged) {
    	NSView *view = mouseDownViewIfStillGood();

        if (view) {
            _sendingEventToSubview = true;
            [view mouseDragged:_currentEvent];
            _sendingEventToSubview = false;
            return;
        }

        // Careful that the drag starting logic stays in sync with eventMayStartDrag()
    
	if (_mouseDownMayStartDrag && _dragSrc.isNull()) {
            BOOL tempFlag1, tempFlag2;
            [_bridge allowDHTMLDrag:&tempFlag1 UADrag:&tempFlag2];
            _dragSrcMayBeDHTML = tempFlag1;
            _dragSrcMayBeUA = tempFlag2;
            if (!_dragSrcMayBeDHTML && !_dragSrcMayBeUA) {
                _mouseDownMayStartDrag = false;     // no element is draggable
            }
        }
        
        if (_mouseDownMayStartDrag && _dragSrc.isNull()) {
            // try to find an element that wants to be dragged
            RenderObject::NodeInfo nodeInfo(true, false);
            renderer()->layer()->nodeAtPoint(nodeInfo, _mouseDownX, _mouseDownY);
            _dragSrc = nodeInfo.innerNode()->renderer()->draggableNode(_dragSrcMayBeDHTML, _dragSrcMayBeUA, _mouseDownX, _mouseDownY, _dragSrcIsDHTML);
            if (_dragSrc.isNull()) {
                _mouseDownMayStartDrag = false;     // no element is draggable
            } else {
                // remember some facts about this source, while we have a NodeInfo handy
                NodeImpl *node = nodeInfo.URLElement();
                _dragSrcIsLink = node ? node->hasAnchor() : false;

                node = nodeInfo.innerNonSharedNode();
                _dragSrcIsImage = (node && node->renderer() && node->renderer()->isImage());

                _dragSrcInSelection = isPointInsideSelection(_mouseDownX, _mouseDownY);
            }                
        }
        
        // For drags starting in the selection, the user must wait between the mousedown and mousedrag,
        // or else we bail on the dragging stuff and allow selection to occur
        if (_mouseDownMayStartDrag && _dragSrcInSelection && [_currentEvent timestamp] - _mouseDownTimestamp < TextDragDelay) {
            _mouseDownMayStartDrag = false;
            // ...but if this was the first click in the window, we don't even want to start selection
            if (_activationEventNumber == [_currentEvent eventNumber]) {
                _mouseDownMayStartSelect = false;
            }
        }

        if (_mouseDownMayStartDrag) {
            // We are starting a text/image/url drag, so the cursor should be an arrow
            d->m_view->resetCursor();
            
            NSPoint dragLocation = [_currentEvent locationInWindow];
            if (dragHysteresisExceeded(dragLocation.x, dragLocation.y)) {
                
                // Once we're past the hysteresis point, we don't want to treat this gesture as a click
                d->m_view->invalidateClick();

                NSImage *dragImage = nil;       // we use these values if WC is out of the loop
                NSPoint dragLoc = NSZeroPoint;
                NSDragOperation srcOp = NSDragOperationNone;                
                BOOL wcWrotePasteboard = NO;
                if (_dragSrcMayBeDHTML) {
                    NSPasteboard *pasteboard = [NSPasteboard pasteboardWithName:NSDragPboard];
                    // Must be done before ondragstart adds types and data to the pboard,
                    // also done for security, as it erases data from the last drag
                    [pasteboard declareTypes:[NSArray array] owner:nil];
                    
                    freeClipboard();    // would only happen if we missed a dragEnd.  Do it anyway, just
                                        // to make sure it gets numbified
                    _dragClipboard = new KWQClipboard(true, pasteboard, KWQClipboard::Writable, this);
                    _dragClipboard->ref();
                    
                    // If this is drag of an element, get set up to generate a default image.  Otherwise
                    // WebKit will generate the default, the element doesn't override.
                    if (_dragSrcIsDHTML) {
                        int srcX, srcY;
                        _dragSrc.handle()->renderer()->absolutePosition(srcX, srcY);
                        _dragClipboard->setDragImageElement(_dragSrc, QPoint(_mouseDownX - srcX, _mouseDownY - srcY));
                    }
                    
                    _mouseDownMayStartDrag = dispatchDragSrcEvent(EventImpl::DRAGSTART_EVENT, QPoint(_mouseDownWinX, _mouseDownWinY));
                    // Invalidate clipboard here against anymore pasteboard writing for security.  The drag
                    // image can still be changed as we drag, but not the pasteboard data.
                    _dragClipboard->setAccessPolicy(KWQClipboard::ImageWritable);
                    
                    if (_mouseDownMayStartDrag) {
                        // gather values from DHTML element, if it set any
                        _dragClipboard->sourceOperation(&srcOp);

                        NSArray *types = [pasteboard types];
                        wcWrotePasteboard = types && [types count] > 0;

                        if (_dragSrcMayBeDHTML) {
                            dragImage = _dragClipboard->dragNSImage(&dragLoc);
                        }
                        
                        // Yuck, dragSourceMovedTo() can be called as a result of kicking off the drag with
                        // dragImage!  Because of that dumb reentrancy, we may think we've not started the
                        // drag when that happens.  So we have to assume it's started before we kick it off.
                        _dragClipboard->setDragHasStarted();
                    }
                }
                
                if (_mouseDownMayStartDrag) {
                    BOOL startedDrag = [_bridge startDraggingImage:dragImage at:dragLoc operation:srcOp event:_currentEvent sourceIsDHTML:_dragSrcIsDHTML DHTMLWroteData:wcWrotePasteboard];
                    if (!startedDrag && _dragSrcMayBeDHTML) {
                        // WebKit canned the drag at the last minute - we owe _dragSrc a DRAGEND event
                        dispatchDragSrcEvent(EventImpl::DRAGEND_EVENT, QPoint(dragLocation));
                        _mouseDownMayStartDrag = false;
                    }
                } 

                if (!_mouseDownMayStartDrag) {
                    // something failed to start the drag, cleanup
                    freeClipboard();
                    _dragSrc = 0;
                }
            }

            // No more default handling (like selection), whether we're past the hysteresis bounds or not
            return;
	}
        if (!_mouseDownMayStartSelect) {
            return;
        }

        // Don't allow dragging or click handling after we've started selecting.
        _mouseDownMayStartDrag = false;
        d->m_view->invalidateClick();

        // We use khtml's selection but our own autoscrolling.
        [_bridge handleAutoscrollForMouseDragged:_currentEvent];
    } else {
	// If we allowed the other side of the bridge to handle a drag
	// last time, then m_bMousePressed might still be set. So we
	// clear it now to make sure the next move after a drag
	// doesn't look like a drag.
	d->m_bMousePressed = false;
    }

    KHTMLPart::khtmlMouseMoveEvent(event);

    KWQ_UNBLOCK_EXCEPTIONS;
}

void KWQKHTMLPart::dragSourceMovedTo(const QPoint &loc)
{
    if (!_dragSrc.isNull() && _dragSrcMayBeDHTML) {
        // for now we don't care if event handler cancels default behavior, since there is none
        dispatchDragSrcEvent(EventImpl::DRAG_EVENT, loc);
    }
}

void KWQKHTMLPart::dragSourceEndedAt(const QPoint &loc, NSDragOperation operation)
{
    if (!_dragSrc.isNull() && _dragSrcMayBeDHTML) {
        _dragClipboard->setDestinationOperation(operation);
        // for now we don't care if event handler cancels default behavior, since there is none
        dispatchDragSrcEvent(EventImpl::DRAGEND_EVENT, loc);
    }
    freeClipboard();
    _dragSrc = 0;
}

// Returns whether caller should continue with "the default processing", which is the same as 
// the event handler NOT setting the return value to false
bool KWQKHTMLPart::dispatchCPPEvent(int eventId, KWQClipboard::AccessPolicy policy)
{
    NodeImpl *target = d->m_selection.start().element();
    if (!target && docImpl()) {
        target = docImpl()->body();
    }
    if (!target) {
        return true;
    }

    KWQClipboard *clipboard = new KWQClipboard(false, [NSPasteboard generalPasteboard], (KWQClipboard::AccessPolicy)policy);
    clipboard->ref();

    int exceptioncode = 0;
    EventImpl *evt = new ClipboardEventImpl(static_cast<EventImpl::EventId>(eventId), true, true, clipboard);
    evt->ref();
    target->dispatchEvent(evt, exceptioncode, true);
    bool noDefaultProcessing = evt->defaultPrevented();
    evt->deref();

    // invalidate clipboard here for security
    clipboard->setAccessPolicy(KWQClipboard::Numb);
    clipboard->deref();

    return !noDefaultProcessing;
}

// WinIE uses onbeforecut and onbeforepaste to enables the cut and paste menu items.  They
// also send onbeforecopy, apparently for symmetry, but it doesn't affect the menu items.
// We need to use onbeforecopy as a real menu enabler because we allow elements that are not
// normally selectable to implement copy/paste (like divs, or a document body).

bool KWQKHTMLPart::mayCut()
{
    return !dispatchCPPEvent(EventImpl::BEFORECUT_EVENT, KWQClipboard::Numb);
}

bool KWQKHTMLPart::mayCopy()
{
    return !dispatchCPPEvent(EventImpl::BEFORECOPY_EVENT, KWQClipboard::Numb);
}

bool KWQKHTMLPart::mayPaste()
{
    return !dispatchCPPEvent(EventImpl::BEFOREPASTE_EVENT, KWQClipboard::Numb);
}

bool KWQKHTMLPart::tryCut()
{
    // Must be done before oncut adds types and data to the pboard,
    // also done for security, as it erases data from the last copy/paste.
    [[NSPasteboard generalPasteboard] declareTypes:[NSArray array] owner:nil];

    return !dispatchCPPEvent(EventImpl::CUT_EVENT, KWQClipboard::Writable);
}

bool KWQKHTMLPart::tryCopy()
{
    // Must be done before oncopy adds types and data to the pboard,
    // also done for security, as it erases data from the last copy/paste.
    [[NSPasteboard generalPasteboard] declareTypes:[NSArray array] owner:nil];

    return !dispatchCPPEvent(EventImpl::COPY_EVENT, KWQClipboard::Writable);
}

bool KWQKHTMLPart::tryPaste()
{
    return !dispatchCPPEvent(EventImpl::PASTE_EVENT, KWQClipboard::Readable);
}

void KWQKHTMLPart::khtmlMouseReleaseEvent(MouseReleaseEvent *event)
{
    NSView *view = mouseDownViewIfStillGood();
    if (!view) {
        // If this was the first click in the window, we don't even want to clear the selection.
        // This case occurs when the user clicks on a draggable element, since we have to process
        // the mouse down and drag events to see if we might start a drag.  For other first clicks
        // in a window, we just don't acceptFirstMouse, and the whole down-drag-up sequence gets
        // ignored upstream of this layer.
        if (_activationEventNumber != [_currentEvent eventNumber]) {
            KHTMLPart::khtmlMouseReleaseEvent(event);
        }
        return;
    }
    
    _sendingEventToSubview = true;
    KWQ_BLOCK_EXCEPTIONS;
    [view mouseUp:_currentEvent];
    KWQ_UNBLOCK_EXCEPTIONS;
    _sendingEventToSubview = false;
}

void KWQKHTMLPart::clearTimers(KHTMLView *view)
{
    if (view) {
        view->unscheduleRelayout();
        if (view->part()) {
            DocumentImpl* document = view->part()->xmlDocImpl();
            if (document && document->renderer() && document->renderer()->layer())
                document->renderer()->layer()->suspendMarquees();
        }
    }
}

void KWQKHTMLPart::clearTimers()
{
    clearTimers(d->m_view);
}

bool KWQKHTMLPart::passSubframeEventToSubframe(NodeImpl::MouseEvent &event)
{
    KWQ_BLOCK_EXCEPTIONS;

    switch ([_currentEvent type]) {
    	case NSLeftMouseDown: {
            NodeImpl *node = event.innerNode.handle();
            if (!node) {
                return false;
            }
            RenderPart *renderPart = dynamic_cast<RenderPart *>(node->renderer());
            if (!renderPart) {
                return false;
            }
            if (!passWidgetMouseDownEventToWidget(renderPart)) {
                return false;
            }
            _mouseDownWasInSubframe = true;
            return true;
        }
        case NSLeftMouseUp: {
            if (!_mouseDownWasInSubframe) {
                return false;
            }
            NSView *view = mouseDownViewIfStillGood();
            if (!view) {
                return false;
            }
            ASSERT(!_sendingEventToSubview);
            _sendingEventToSubview = true;
	    [view mouseUp:_currentEvent];
	    _sendingEventToSubview = false;
            return true;
        }
        case NSLeftMouseDragged: {
            if (!_mouseDownWasInSubframe) {
                return false;
            }
            NSView *view = mouseDownViewIfStillGood();
            if (!view) {
                return false;
            }
            ASSERT(!_sendingEventToSubview);
            _sendingEventToSubview = true;
	    [view mouseDragged:_currentEvent];
	    _sendingEventToSubview = false;
            return true;
        }
        default:
            return false;
    }
    KWQ_UNBLOCK_EXCEPTIONS;

    return false;
}

void KWQKHTMLPart::mouseDown(NSEvent *event)
{
    KHTMLView *v = d->m_view;
    if (!v || _sendingEventToSubview) {
        return;
    }

    KWQ_BLOCK_EXCEPTIONS;

    prepareForUserAction();

    _mouseDownView = nil;
    _dragSrc = 0;
    
    NSEvent *oldCurrentEvent = _currentEvent;
    _currentEvent = KWQRetain(event);
    NSPoint loc = [event locationInWindow];
    _mouseDownWinX = (int)loc.x;
    _mouseDownWinY = (int)loc.y;
    d->m_view->viewportToContents(_mouseDownWinX, _mouseDownWinY, _mouseDownX, _mouseDownY);
    _mouseDownTimestamp = [event timestamp];

    NSResponder *oldFirstResponderAtMouseDownTime = _firstResponderAtMouseDownTime;
    // Unlike other places in WebCore where we get the first
    // responder, in this case we must be talking about the real first
    // responder, so we could just ask the bridge's window, instead of
    // the bridge. It's unclear which is better.
    _firstResponderAtMouseDownTime = KWQRetain([_bridge firstResponder]);

    _mouseDownMayStartDrag = false;
    _mouseDownMayStartSelect = false;

    QMouseEvent kEvent(QEvent::MouseButtonPress, event);
    v->viewportMousePressEvent(&kEvent);
    
    KWQRelease(_firstResponderAtMouseDownTime);
    _firstResponderAtMouseDownTime = oldFirstResponderAtMouseDownTime;

    ASSERT(_currentEvent == event);
    KWQRelease(event);
    _currentEvent = oldCurrentEvent;

    KWQ_UNBLOCK_EXCEPTIONS;
}

void KWQKHTMLPart::mouseDragged(NSEvent *event)
{
    KHTMLView *v = d->m_view;
    if (!v || _sendingEventToSubview) {
        return;
    }

    KWQ_BLOCK_EXCEPTIONS;

    NSEvent *oldCurrentEvent = _currentEvent;
    _currentEvent = KWQRetain(event);

    QMouseEvent kEvent(QEvent::MouseMove, event);
    v->viewportMouseMoveEvent(&kEvent);
    
    ASSERT(_currentEvent == event);
    KWQRelease(event);
    _currentEvent = oldCurrentEvent;

    KWQ_UNBLOCK_EXCEPTIONS;
}

void KWQKHTMLPart::mouseUp(NSEvent *event)
{
    KHTMLView *v = d->m_view;
    if (!v || _sendingEventToSubview) {
        return;
    }

    KWQ_BLOCK_EXCEPTIONS;

    NSEvent *oldCurrentEvent = _currentEvent;
    _currentEvent = KWQRetain(event);

    // Our behavior here is a little different that Qt. Qt always sends
    // a mouse release event, even for a double click. To correct problems
    // in khtml's DOM click event handling we do not send a release here
    // for a double click. Instead we send that event from KHTMLView's
    // viewportMouseDoubleClickEvent. Note also that the third click of
    // a triple click is treated as a single click, but the fourth is then
    // treated as another double click. Hence the "% 2" below.
    int clickCount = [event clickCount];
    if (clickCount > 0 && clickCount % 2 == 0) {
        QMouseEvent doubleClickEvent(QEvent::MouseButtonDblClick, event);
        v->viewportMouseDoubleClickEvent(&doubleClickEvent);
    } else {
        QMouseEvent releaseEvent(QEvent::MouseButtonRelease, event);
        v->viewportMouseReleaseEvent(&releaseEvent);
    }
    
    ASSERT(_currentEvent == event);
    KWQRelease(event);
    _currentEvent = oldCurrentEvent;
    
    _mouseDownView = nil;

    KWQ_UNBLOCK_EXCEPTIONS;
}

/*
 A hack for the benefit of AK's PopUpButton, which uses the Carbon menu manager, which thus
 eats all subsequent events after it is starts its modal tracking loop.  After the interaction
 is done, this routine is used to fix things up.  When a mouse down started us tracking in
 the widget, we post a fake mouse up to balance the mouse down we started with. When a 
 key down started us tracking in the widget, we post a fake key up to balance things out.
 In addition, we post a fake mouseMoved to get the cursor in sync with whatever we happen to 
 be over after the tracking is done.
 */
void KWQKHTMLPart::sendFakeEventsAfterWidgetTracking(NSEvent *initiatingEvent)
{
    KWQ_BLOCK_EXCEPTIONS;

    _sendingEventToSubview = false;
    int eventType = [initiatingEvent type];
    ASSERT(eventType == NSLeftMouseDown || eventType == NSKeyDown);
    NSEvent *fakeEvent = nil;
    if (eventType == NSLeftMouseDown) {
        fakeEvent = [NSEvent mouseEventWithType:NSLeftMouseUp
                                location:[initiatingEvent locationInWindow]
                            modifierFlags:[initiatingEvent modifierFlags]
                                timestamp:[initiatingEvent timestamp]
                            windowNumber:[initiatingEvent windowNumber]
                                    context:[initiatingEvent context]
                                eventNumber:[initiatingEvent eventNumber]
                                clickCount:[initiatingEvent clickCount]
                                pressure:[initiatingEvent pressure]];
    
        mouseUp(fakeEvent);
    }
    else { // eventType == NSKeyDown
        fakeEvent = [NSEvent keyEventWithType:NSKeyUp
                                location:[initiatingEvent locationInWindow]
                           modifierFlags:[initiatingEvent modifierFlags]
                               timestamp:[initiatingEvent timestamp]
                            windowNumber:[initiatingEvent windowNumber]
                                 context:[initiatingEvent context]
                              characters:[initiatingEvent characters] 
             charactersIgnoringModifiers:[initiatingEvent charactersIgnoringModifiers] 
                               isARepeat:[initiatingEvent isARepeat] 
                                 keyCode:[initiatingEvent keyCode]];
        keyEvent(fakeEvent);
    }
    // FIXME:  We should really get the current modifierFlags here, but there's no way to poll
    // them in Cocoa, and because the event stream was stolen by the Carbon menu code we have
    // no up-to-date cache of them anywhere.
    fakeEvent = [NSEvent mouseEventWithType:NSMouseMoved
                                   location:[[_bridge window] convertScreenToBase:[NSEvent mouseLocation]]
                              modifierFlags:[initiatingEvent modifierFlags]
                                  timestamp:[initiatingEvent timestamp]
                               windowNumber:[initiatingEvent windowNumber]
                                    context:[initiatingEvent context]
                                eventNumber:0
                                 clickCount:0
                                   pressure:0];
    mouseMoved(fakeEvent);

    KWQ_UNBLOCK_EXCEPTIONS;
}

void KWQKHTMLPart::mouseMoved(NSEvent *event)
{
    KHTMLView *v = d->m_view;
    // Reject a mouse moved if the button is down - screws up tracking during autoscroll
    // These happen because WebKit sometimes has to fake up moved events.
    if (!v || d->m_bMousePressed) {
        return;
    }
    
    KWQ_BLOCK_EXCEPTIONS;

    NSEvent *oldCurrentEvent = _currentEvent;
    _currentEvent = KWQRetain(event);
    
    QMouseEvent kEvent(QEvent::MouseMove, event);
    v->viewportMouseMoveEvent(&kEvent);
    
    ASSERT(_currentEvent == event);
    KWQRelease(event);
    _currentEvent = oldCurrentEvent;

    KWQ_UNBLOCK_EXCEPTIONS;
}

// Called as we walk up the element chain for nodes with CSS property -khtml-user-drag == auto
bool KWQKHTMLPart::shouldDragAutoNode(DOM::NodeImpl* node, int x, int y) const
{
    // We assume that WebKit only cares about dragging things that can be leaf nodes (text, images, urls).
    // This saves a bunch of expensive calls (creating WC and WK element dicts) as we walk farther up
    // the node hierarchy, and we also don't have to cook up a way to ask WK about non-leaf nodes
    // (since right now WK just hit-tests using a cached lastMouseDown).
    if (!node->hasChildNodes() && d->m_view) {
        int windowX, windowY;
        d->m_view->contentsToViewport(x, y, windowX, windowY);
        NSPoint eventLoc = {windowX, windowY};
        return [_bridge mayStartDragAtEventLocation:eventLoc];
    } else {
        return NO;
    }
}

bool KWQKHTMLPart::sendContextMenuEvent(NSEvent *event)
{
    DocumentImpl *doc = d->m_doc;
    KHTMLView *v = d->m_view;
    if (!doc || !v) {
        return false;
    }

    KWQ_BLOCK_EXCEPTIONS;

    NSEvent *oldCurrentEvent = _currentEvent;
    _currentEvent = KWQRetain(event);
    
    QMouseEvent qev(QEvent::MouseButtonPress, event);

    int xm, ym;
    v->viewportToContents(qev.x(), qev.y(), xm, ym);

    NodeImpl::MouseEvent mev(qev.stateAfter(), NodeImpl::MousePress);
    doc->prepareMouseEvent(false, xm, ym, &mev);

    bool swallowEvent = v->dispatchMouseEvent(EventImpl::CONTEXTMENU_EVENT,
        mev.innerNode.handle(), true, 0, &qev, true, NodeImpl::MousePress);
    if (!swallowEvent && ([_bridge isEditable] || mev.innerNode.handle()->isContentEditable()) && !isPointInsideSelection(xm, ym)) {
        selectClosestWordFromMouseEvent(&qev, mev.innerNode, xm, ym);
    }

    ASSERT(_currentEvent == event);
    KWQRelease(event);
    _currentEvent = oldCurrentEvent;

    return swallowEvent;

    KWQ_UNBLOCK_EXCEPTIONS;

    return false;
}

struct ListItemInfo {
    unsigned start;
    unsigned end;
};

NSFileWrapper *KWQKHTMLPart::fileWrapperForElement(ElementImpl *e)
{
    KWQ_BLOCK_EXCEPTIONS;
    
    NSFileWrapper *wrapper = nil;

    AtomicString attr = e->getAttribute(ATTR_SRC);
    if (!attr.isEmpty()) {
        NSURL *URL = completeURL(attr.string()).getNSURL();
        wrapper = [_bridge fileWrapperForURL:URL];
    }    
    if (!wrapper) {
        RenderImage *renderer = static_cast<RenderImage *>(e->renderer());
        NSImage *image = renderer->pixmap().image();
        NSData *tiffData = [image TIFFRepresentationUsingCompression:NSTIFFCompressionLZW factor:0.0];
        wrapper = [[NSFileWrapper alloc] initRegularFileWithContents:tiffData];
        [wrapper setPreferredFilename:@"image.tiff"];
        [wrapper autorelease];
    }

    return wrapper;
    
    KWQ_UNBLOCK_EXCEPTIONS;

    return nil;
}

static ElementImpl *listParent(ElementImpl *item)
{
    // Ick!  Avoid use of item->id() which confuses ObjC++.
    unsigned short _id = Node(item).elementId();
    
    while (_id != ID_UL && _id != ID_OL) {
        item = static_cast<ElementImpl *>(item->parentNode());
        if (!item)
            break;
        _id = Node(item).elementId();
    }
    return item;
}

static NodeImpl* isTextFirstInListItem(NodeImpl *e)
{
    if (Node(e).nodeType() != Node::TEXT_NODE)
        return 0;
    NodeImpl* par = e->parentNode();
    while (par) {
        if (par->firstChild() != e)
            return 0;
        if (Node(par).elementId() == ID_LI)
            return par;
        e = par;
        par = par->parentNode();
    }
    return 0;
}

// FIXME: This collosal function needs to be refactored into maintainable smaller bits.

#define BULLET_CHAR 0x2022
#define SQUARE_CHAR 0x25AA
#define CIRCLE_CHAR 0x25E6

NSAttributedString *KWQKHTMLPart::attributedString(NodeImpl *_start, int startOffset, NodeImpl *endNode, int endOffset)
{
    KWQ_BLOCK_EXCEPTIONS;

    NodeImpl * _startNode = _start;

    if (_startNode == nil) {
        return nil;
    }

    NSMutableAttributedString *result = [[[NSMutableAttributedString alloc] init] autorelease];

    bool hasNewLine = true;
    bool addedSpace = true;
    NSAttributedString *pendingStyledSpace = nil;
    bool hasParagraphBreak = true;
    const ElementImpl *linkStartNode = 0;
    unsigned linkStartLocation = 0;
    QPtrList<ElementImpl> listItems;
    QValueList<ListItemInfo> listItemLocations;
    float maxMarkerWidth = 0;
    
    Node n = _startNode;
    
    // If the first item is the entire text of a list item, use the list item node as the start of the 
    // selection, not the text node.  The user's intent was probably to select the list.
    if (n.nodeType() == Node::TEXT_NODE && startOffset == 0) {
        NodeImpl *startListNode = isTextFirstInListItem(_startNode);
        if (startListNode){
            _startNode = startListNode;
            n = _startNode;
        }
    }
    
    while (!n.isNull()) {
        RenderObject *renderer = n.handle()->renderer();
        if (renderer) {
            RenderStyle *style = renderer->style();
            NSFont *font = style->font().getNSFont();
            bool needSpace = pendingStyledSpace != nil;
            if (n.nodeType() == Node::TEXT_NODE) {
                if (hasNewLine) {
                    addedSpace = true;
                    needSpace = false;
                    [pendingStyledSpace release];
                    pendingStyledSpace = nil;
                    hasNewLine = false;
                }
                QString text;
                QString str = n.nodeValue().string();
                int start = (n == _startNode) ? startOffset : -1;
                int end = (n == endNode) ? endOffset : -1;
                if (renderer->isText()) {
                    if (renderer->style()->whiteSpace() == PRE) {
                        if (needSpace && !addedSpace) {
                            if (text.isEmpty() && linkStartLocation == [result length]) {
                                ++linkStartLocation;
                            }
                            [result appendAttributedString:pendingStyledSpace];
                        }
                        int runStart = (start == -1) ? 0 : start;
                        int runEnd = (end == -1) ? str.length() : end;
                        text += str.mid(runStart, runEnd-runStart);
                        [pendingStyledSpace release];
                        pendingStyledSpace = nil;
                        addedSpace = str[runEnd-1].direction() == QChar::DirWS;
                    }
                    else {
                        RenderText* textObj = static_cast<RenderText*>(renderer);
                        if (!textObj->firstTextBox() && str.length() > 0 && !addedSpace) {
                            // We have no runs, but we do have a length.  This means we must be
                            // whitespace that collapsed away at the end of a line.
                            text += ' ';
                            addedSpace = true;
                        }
                        else {
                            addedSpace = false;
                            for (InlineTextBox* box = textObj->firstTextBox(); box; box = box->nextTextBox()) {
                                int runStart = (start == -1) ? box->m_start : start;
                                int runEnd = (end == -1) ? box->m_start + box->m_len : end;
                                runEnd = kMin(runEnd, box->m_start + box->m_len);
                                if (runStart >= box->m_start &&
                                    runStart < box->m_start + box->m_len) {
                                    if (box == textObj->firstTextBox() && box->m_start == runStart && runStart > 0) {
                                        needSpace = true; // collapsed space at the start
                                    }
                                    if (needSpace && !addedSpace) {
                                        if (pendingStyledSpace != nil) {
                                            if (text.isEmpty() && linkStartLocation == [result length]) {
                                                ++linkStartLocation;
                                            }
                                            [result appendAttributedString:pendingStyledSpace];
                                        } else {
                                            text += ' ';
                                        }
                                    }
                                    QString runText = str.mid(runStart, runEnd - runStart);
                                    runText.replace('\n', ' ');
                                    text += runText;
                                    int nextRunStart = box->nextTextBox() ? box->nextTextBox()->m_start : str.length(); // collapsed space between runs or at the end
                                    needSpace = nextRunStart > runEnd;
                                    [pendingStyledSpace release];
                                    pendingStyledSpace = nil;
                                    addedSpace = str[runEnd-1].direction() == QChar::DirWS;
                                    start = -1;
                                }
                                if (end != -1 && runEnd >= end)
                                    break;
                            }
                        }
                    }
                }
                
                text.replace('\\', renderer->backslashAsCurrencySymbol());
    
                if (text.length() > 0 || needSpace) {
                    NSMutableDictionary *attrs = [[NSMutableDictionary alloc] init];
                    [attrs setObject:font forKey:NSFontAttributeName];
                    if (style && style->color().isValid() && qAlpha(style->color().rgb()) != 0)
                        [attrs setObject:style->color().getNSColor() forKey:NSForegroundColorAttributeName];
                    if (style && style->backgroundColor().isValid() && qAlpha(style->backgroundColor().rgb()) != 0)
                        [attrs setObject:style->backgroundColor().getNSColor() forKey:NSBackgroundColorAttributeName];

                    if (text.length() > 0) {
                        hasParagraphBreak = false;
                        NSAttributedString *partialString = [[NSAttributedString alloc] initWithString:text.getNSString() attributes:attrs];
                        [result appendAttributedString: partialString];                
                        [partialString release];
                    }

                    if (needSpace) {
                        [pendingStyledSpace release];
                        pendingStyledSpace = [[NSAttributedString alloc] initWithString:@" " attributes:attrs];
                    }

                    [attrs release];
                }
            } else {
                // This is our simple HTML -> ASCII transformation:
                QString text;
                unsigned short _id = n.elementId();
                switch(_id) {
                    case ID_A:
                        // Note the start of the <a> element.  We will add the NSLinkAttributeName
                        // attribute to the attributed string when navigating to the next sibling 
                        // of this node.
                        linkStartLocation = [result length];
                        linkStartNode = static_cast<ElementImpl*>(n.handle());
                        break;

                    case ID_BR:
                        text += "\n";
                        hasNewLine = true;
                        break;
    
                    case ID_LI:
                        {
                            QString listText;
                            ElementImpl *itemParent = listParent(static_cast<ElementImpl *>(n.handle()));
                            
                            if (!hasNewLine)
                                listText += '\n';
                            hasNewLine = true;
    
                            listItems.append(static_cast<ElementImpl*>(n.handle()));
                            ListItemInfo info;
                            info.start = [result length];
                            info.end = 0;
                            listItemLocations.append (info);
                            
                            listText += '\t';
                            if (itemParent){
                                khtml::RenderListItem *listRenderer = static_cast<khtml::RenderListItem*>(renderer);

                                maxMarkerWidth = MAX([font pointSize], maxMarkerWidth);
                                switch(listRenderer->style()->listStyleType()) {
                                    case khtml::DISC:
                                        listText += ((QChar)BULLET_CHAR);
                                        break;
                                    case khtml::CIRCLE:
                                        listText += ((QChar)CIRCLE_CHAR);
                                        break;
                                    case khtml::SQUARE:
                                        listText += ((QChar)SQUARE_CHAR);
                                        break;
                                    case khtml::LNONE:
                                        break;
                                    default:
                                        QString marker = listRenderer->markerStringValue();
                                        listText += marker;
                                        // Use AppKit metrics.  Will be rendered by AppKit.
                                        float markerWidth = [font widthOfString: marker.getNSString()];
                                        maxMarkerWidth = MAX(markerWidth, maxMarkerWidth);
                                }

                                listText += ' ';
                                listText += '\t';
    
                                NSMutableDictionary *attrs;
            
                                attrs = [[NSMutableDictionary alloc] init];
                                [attrs setObject:font forKey:NSFontAttributeName];
                                if (style && style->color().isValid())
                                    [attrs setObject:style->color().getNSColor() forKey:NSForegroundColorAttributeName];
                                if (style && style->backgroundColor().isValid())
                                    [attrs setObject:style->backgroundColor().getNSColor() forKey:NSBackgroundColorAttributeName];
            
                                NSAttributedString *partialString = [[NSAttributedString alloc] initWithString:listText.getNSString() attributes:attrs];
                                [attrs release];
                                [result appendAttributedString: partialString];                
                                [partialString release];
                            }
                        }
                        break;

                    case ID_OL:
                    case ID_UL:
                        if (!hasNewLine)
                            text += "\n";
                        hasNewLine = true;
                        break;

                    case ID_TD:
                    case ID_TH:
                    case ID_HR:
                    case ID_DD:
                    case ID_DL:
                    case ID_DT:
                    case ID_PRE:
                    case ID_BLOCKQUOTE:
                    case ID_DIV:
                        if (!hasNewLine)
                            text += '\n';
                        hasNewLine = true;
                        break;
                    case ID_P:
                    case ID_TR:
                    case ID_H1:
                    case ID_H2:
                    case ID_H3:
                    case ID_H4:
                    case ID_H5:
                    case ID_H6:
                        if (!hasNewLine)
                            text += '\n';
                        if (!hasParagraphBreak) {
                            text += '\n';
                            hasParagraphBreak = true;
                        }
                        hasNewLine = true;
                        break;
                        
                    case ID_IMG:
                        if (pendingStyledSpace != nil) {
                            if (linkStartLocation == [result length]) {
                                ++linkStartLocation;
                            }
                            [result appendAttributedString:pendingStyledSpace];
                            [pendingStyledSpace release];
                            pendingStyledSpace = nil;
                        }
                        NSFileWrapper *fileWrapper = fileWrapperForElement(static_cast<ElementImpl *>(n.handle()));
                        NSTextAttachment *attachment = [[NSTextAttachment alloc] initWithFileWrapper:fileWrapper];
                        NSAttributedString *iString = [NSAttributedString attributedStringWithAttachment:attachment];
                        [result appendAttributedString: iString];
                        [attachment release];
                        break;
                }
                NSAttributedString *partialString = [[NSAttributedString alloc] initWithString:text.getNSString()];
                [result appendAttributedString: partialString];
                [partialString release];
            }
        }

        if (n == endNode)
            break;

        Node next = n.firstChild();
        if (next.isNull()){
            next = n.nextSibling();
        }

        while (next.isNull() && !n.parentNode().isNull()) {
            QString text;
            n = n.parentNode();
            if (n == endNode)
                break;
            next = n.nextSibling();

            unsigned short _id = n.elementId();
            switch(_id) {
                case ID_A:
                    // End of a <a> element.  Create an attributed string NSLinkAttributeName attribute
                    // for the range of the link.  Note that we create the attributed string from the DOM, which
                    // will have corrected any illegally nested <a> elements.
                    if (linkStartNode && n.handle() == linkStartNode){
                        DOMString href = parseURL(linkStartNode->getAttribute(ATTR_HREF));
                        KURL kURL = KWQ(linkStartNode->getDocument()->part())->completeURL(href.string());
                        
                        NSURL *URL = kURL.getNSURL();
                        [result addAttribute:NSLinkAttributeName value:URL range:NSMakeRange(linkStartLocation, [result length]-linkStartLocation)];
                        linkStartNode = 0;
                    }
                    break;
                
                case ID_OL:
                case ID_UL:
                    if (!hasNewLine)
                        text += '\n';
                    hasNewLine = true;
                    break;

                case ID_LI:
                    {
                        int i, count = listItems.count();
                        for (i = 0; i < count; i++){
                            if (listItems.at(i) == n.handle()){
                                listItemLocations[i].end = [result length];
                                break;
                            }
                        }
                    }
                    if (!hasNewLine)
                        text += '\n';
                    hasNewLine = true;
                    break;

                case ID_TD:
                case ID_TH:
                case ID_HR:
                case ID_DD:
                case ID_DL:
                case ID_DT:
                case ID_PRE:
                case ID_BLOCKQUOTE:
                case ID_DIV:
                    if (!hasNewLine)
                        text += '\n';
                    hasNewLine = true;
                    break;
                case ID_P:
                case ID_TR:
                case ID_H1:
                case ID_H2:
                case ID_H3:
                case ID_H4:
                case ID_H5:
                case ID_H6:
                    if (!hasNewLine)
                        text += '\n';
                    // An extra newline is needed at the start, not the end, of these types of tags,
                    // so don't add another here.
                    hasNewLine = true;
                    break;
            }
            
            NSAttributedString *partialString = [[NSAttributedString alloc] initWithString:text.getNSString()];
            [result appendAttributedString:partialString];
            [partialString release];
        }

        n = next;
    }
    
    [pendingStyledSpace release];
    
    // Apply paragraph styles from outside in.  This ensures that nested lists correctly
    // override their parent's paragraph style.
    {
        unsigned i, count = listItems.count();
        ElementImpl *e;
        ListItemInfo info;

#ifdef POSITION_LIST
        NodeImpl *containingBlock;
        int containingBlockX, containingBlockY;
        
        // Determine the position of the outermost containing block.  All paragraph
        // styles and tabs should be relative to this position.  So, the horizontal position of 
        // each item in the list (in the resulting attributed string) will be relative to position 
        // of the outermost containing block.
        if (count > 0){
            containingBlock = _startNode;
            while (containingBlock->renderer()->isInline()){
                containingBlock = containingBlock->parentNode();
            }
            containingBlock->renderer()->absolutePosition(containingBlockX, containingBlockY);
        }
#endif
        
        for (i = 0; i < count; i++){
            e = listItems.at(i);
            info = listItemLocations[i];
            
            if (info.end < info.start)
                info.end = [result length];
                
            RenderObject *r = e->renderer();
            RenderStyle *style = r->style();

            int rx;
            NSFont *font = style->font().getNSFont();
            float pointSize = [font pointSize];

#ifdef POSITION_LIST
            int ry;
            r->absolutePosition(rx, ry);
            rx -= containingBlockX;
            
            // Ensure that the text is indented at least enough to allow for the markers.
            rx = MAX(rx, (int)maxMarkerWidth);
#else
            rx = (int)MAX(maxMarkerWidth, pointSize);
#endif

            // The bullet text will be right aligned at the first tab marker, followed
            // by a space, followed by the list item text.  The space is arbitrarily
            // picked as pointSize*2/3.  The space on the first line of the text item
            // is established by a left aligned tab, on subsequent lines it's established
            // by the head indent.
            NSMutableParagraphStyle *mps = [[NSMutableParagraphStyle alloc] init];
            [mps setFirstLineHeadIndent: 0];
            [mps setHeadIndent: rx];
            [mps setTabStops:[NSArray arrayWithObjects:
                        [[[NSTextTab alloc] initWithType:NSRightTabStopType location:rx-(pointSize*2/3)] autorelease],
                        [[[NSTextTab alloc] initWithType:NSLeftTabStopType location:rx] autorelease],
                        nil]];
            [result addAttribute:NSParagraphStyleAttributeName value:mps range:NSMakeRange(info.start, info.end-info.start)];
            [mps release];
        }
    }

    return result;

    KWQ_UNBLOCK_EXCEPTIONS;

    return nil;
}

QRect KWQKHTMLPart::selectionRect() const
{
    if(!xmlDocImpl()){
        return QRect();
    }

    RenderCanvas *root = static_cast<RenderCanvas *>(xmlDocImpl()->renderer());
    if (!root) {
        return QRect();
    }

    return root->selectionRect();
}

// returns NSRect because going through QRect would truncate any floats
NSRect KWQKHTMLPart::visibleSelectionRect() const
{
    if (!d->m_view) {
        return NSZeroRect;
    }
    NSView *documentView = d->m_view->getDocumentView();
    if (!documentView) {
        return NSZeroRect;
    }
    return NSIntersectionRect(selectionRect(), [documentView visibleRect]);     
}

void KWQKHTMLPart::centerSelectionInVisibleArea() const
{
    switch (selection().state()) {
        case Selection::NONE:
            break;
        case Selection::CARET: {
            if (view())
                // passing true forces centering even if selection is already exposed
                view()->ensureRectVisibleCentered(selection().caretRect(), true);
            break;
        }
        case Selection::RANGE:
            if (view())
                // passing true forces centering even if selection is already exposed
                view()->ensureRectVisibleCentered(selectionRect(), true);
            break;
    }
}

NSImage *KWQKHTMLPart::imageFromRect(NSRect rect) const
{
    NSView *view = d->m_view->getDocumentView();
    if (!view) {
        return nil;
    }
    
    NSRect bounds = [view bounds];
    NSImage *resultImage = [[[NSImage alloc] initWithSize:rect.size] autorelease];
    
    KWQ_BLOCK_EXCEPTIONS;
    
    if (rect.size.width != 0 && rect.size.height != 0) {
        [resultImage setFlipped:YES];
        [resultImage lockFocus];

        [NSGraphicsContext saveGraphicsState];
        NSPoint translation = { -(NSMinX(rect) - NSMinX(bounds)), -(NSMinY(rect) - NSMinY(bounds)) };
        CGContextTranslateCTM((CGContext *)[[NSGraphicsContext currentContext] graphicsPort], translation.x, translation.y);

        // We change the coord system at the CG level, out from under the AK focus machinery, because it doesn't
        // work to change the coord system of a focused view.  However, WebImageRenderer uses the difference
        // between the focused view's coord system and the window's coord system to adjust the pattern phase, and
        // that calc ignores our translation.  So we must tell it about this extra phase offset.

        // Window is not flipped, we are, so y coord must be inverted when describing phase, which is a
        // window level notion.
        translation.y = -translation.y;
        [[WebCoreGraphicsBridge sharedBridge] setAdditionalPatternPhase:translation];

        [view drawRect:rect];

        [[WebCoreGraphicsBridge sharedBridge] setAdditionalPatternPhase:NSZeroPoint];
        [NSGraphicsContext restoreGraphicsState];

        [resultImage unlockFocus];
        [resultImage setFlipped:NO];
    }
    
    KWQ_UNBLOCK_EXCEPTIONS;
    
    return resultImage;
}

NSImage *KWQKHTMLPart::selectionImage() const
{
    _drawSelectionOnly = true;  // invoke special drawing mode
    NSImage *result = imageFromRect(visibleSelectionRect());
    _drawSelectionOnly = false;
    return result;
}

NSImage *KWQKHTMLPart::snapshotDragImage(DOM::Node node, NSRect *imageRect, NSRect *elementRect) const
{
    RenderObject *renderer = node.handle()->renderer();
    if (!renderer) {
        return nil;
    }
    
    renderer->updateDragState(true);    // mark dragged nodes (so they pick up the right CSS)
    d->m_doc->updateLayout();        // forces style recalc - needed since changing the drag state might
                                        // imply new styles, plus JS could have changed other things
    QRect topLevelRect;
    NSRect paintingRect = renderer->paintingRootRect(topLevelRect);

    _elementToDraw = node;              // invoke special sub-tree drawing mode
    NSImage *result = imageFromRect(paintingRect);
    renderer->updateDragState(false);
    _elementToDraw = 0;

    if (elementRect) {
        *elementRect = topLevelRect;
    }
    if (imageRect) {
        *imageRect = paintingRect;
    }
    return result;
}

NSFont *KWQKHTMLPart::fontForSelection(bool *hasMultipleFonts) const
{
    if (hasMultipleFonts)
        *hasMultipleFonts = false;

    if (!xmlDocImpl())
        return nil;
    if (d->m_selection.isNone())
        return nil;
    
    if (d->m_selection.isCaret()) {
        Position pos = VisiblePosition(d->m_selection.start(), UPSTREAM).deepEquivalent();
        ASSERT(pos.isNotNull());
        if (!pos.inRenderedContent())
            return nil;
        NodeImpl *node = pos.node();
        if (!node)
            return nil;
        
        if (!d->m_typingStyle)
            return node->renderer()->style()->font().getNSFont();

        int exceptionCode = 0;
        ElementImpl *styleElement = xmlDocImpl()->createHTMLElement("span", exceptionCode);
        ASSERT(exceptionCode == 0);
        
        styleElement->setAttribute(ATTR_STYLE, d->m_typingStyle->cssText().implementation(), exceptionCode);
        ASSERT(exceptionCode == 0);
        
        TextImpl *text = xmlDocImpl()->createEditingTextNode("");
        styleElement->appendChild(text, exceptionCode);
        ASSERT(exceptionCode == 0);

        node->parentNode()->insertBefore(styleElement, node, exceptionCode);
        ASSERT(exceptionCode == 0);
        
        RenderObject *renderer = styleElement->renderer();
        ASSERT(renderer);
        NSFont *result = renderer->style()->font().getNSFont();
        
        styleElement->remove(exceptionCode);
        ASSERT(exceptionCode == 0);
        
        return result;
    }

    NSFont *font = nil;

    Range r = d->m_selection.toRange();
    RangeImpl *range = r.handle();
    NodeImpl *pastEnd = range->pastEndNode();
    for (NodeImpl *n = range->startNode(); n != pastEnd; n = n->traverseNextNode()) {
        RenderObject *renderer = n->renderer();
        if (!renderer)
            continue;
        // FIXME: Are there any node types that have renderers, but that we should be skipping?
        NSFont *f = renderer->style()->font().getNSFont();
        if (font == nil) {
            font = f;
            if (!hasMultipleFonts)
                break;
        } else if (font != f) {
            *hasMultipleFonts = false;
            break;
        }
    }

    return font;
}

KWQWindowWidget *KWQKHTMLPart::topLevelWidget()
{
    return _windowWidget;
}

int KWQKHTMLPart::selectionStartOffset() const
{
    return d->m_selection.start().offset();
}

int KWQKHTMLPart::selectionEndOffset() const
{
    return d->m_selection.end().offset();
}

NodeImpl *KWQKHTMLPart::selectionStart() const
{
    return d->m_selection.start().node();
}

NodeImpl *KWQKHTMLPart::selectionEnd() const
{
    return d->m_selection.end().node();
}

void KWQKHTMLPart::setBridge(WebCoreBridge *p)
{ 
    if (_bridge != p) {
	delete _windowWidget;
    }
    _bridge = p;
    _windowWidget = new KWQWindowWidget(_bridge);
}

QString KWQKHTMLPart::overrideMediaType() const
{
    NSString *overrideType = [_bridge overrideMediaType];
    if (overrideType)
        return QString::fromNSString(overrideType);
    return QString();
}

void KWQKHTMLPart::setMediaType(const QString &type)
{
    if (d->m_view) {
        d->m_view->setMediaType(type);
    }
}

void KWQKHTMLPart::setSelectionFromNone()
{
    // Put the caret someplace if the selection is empty and the part is editable.
    // This has the effect of flashing the caret in a contentEditable view automatically 
    // without requiring the programmer to set a selection explicitly.
    DocumentImpl *doc = xmlDocImpl();
    if (doc && selection().isNone() && isContentEditable()) {
        NodeImpl *node = doc->documentElement();
        while (node) {
            // Look for a block flow, but skip over the HTML element, since we really
            // want to get at least as far as the the BODY element in a document.
            if (node->isBlockFlow() && node->identifier() != ID_HTML)
                break;
            node = node->traverseNextNode();
        }
        if (node)
            setSelection(Position(node, 0));
    }
}

void KWQKHTMLPart::setDisplaysWithFocusAttributes(bool flag)
{
    if (d->m_isFocused == flag)
        return;
    d->m_isFocused = flag;
        
    // This method does the job of updating the view based on whether the view is "active".
    // This involves three kinds of drawing updates:

    // 1. The background color used to draw behind selected content (active | inactive color)
    if (d->m_view)
        d->m_view->updateContents(QRect(visibleSelectionRect()));

    // 2. Caret blinking (blinks | does not blink)
    if (flag)
        setSelectionFromNone();
    setCaretVisible(d->m_isFocused);
    
    // 3. The drawing of a focus ring around links in web pages.
    DocumentImpl *doc = xmlDocImpl();
    if (doc) {
        NodeImpl *node = doc->focusNode();
        if (node && node->renderer())
            node->renderer()->repaint();
    }
}

bool KWQKHTMLPart::displaysWithFocusAttributes() const
{
    return d->m_isFocused;
}

QChar KWQKHTMLPart::backslashAsCurrencySymbol() const
{
    DocumentImpl *doc = xmlDocImpl();
    if (!doc) {
        return '\\';
    }
    Decoder *decoder = doc->decoder();
    if (!decoder) {
        return '\\';
    }
    const QTextCodec *codec = decoder->codec();
    if (!codec) {
        return '\\';
    }
    return codec->backslashAsCurrencySymbol();
}

NSColor *KWQKHTMLPart::bodyBackgroundColor() const
{
    if (docImpl() && docImpl()->body() && docImpl()->body()->renderer()) {
        QColor bgColor = docImpl()->body()->renderer()->style()->backgroundColor();
        if (bgColor.isValid()) {
            return bgColor.getNSColor();
        }
    }
    return nil;
}

WebCoreKeyboardUIMode KWQKHTMLPart::keyboardUIMode() const
{
    KWQ_BLOCK_EXCEPTIONS;
    return [_bridge keyboardUIMode];
    KWQ_UNBLOCK_EXCEPTIONS;

    return WebCoreKeyboardAccessDefault;
}

void KWQKHTMLPart::setName(const QString &name)
{
    QString n = name;

    KWQKHTMLPart *parent = KWQ(parentPart());

    if (parent && (name.isEmpty() || parent->frameExists(name))) {
	n = parent->requestFrameName();
    }

    KHTMLPart::setName(n);

    KWQ_BLOCK_EXCEPTIONS;
    [_bridge didSetName:n.getNSString()];
    KWQ_UNBLOCK_EXCEPTIONS;
}

void KWQKHTMLPart::didTellBridgeAboutLoad(const QString &urlString)
{
    static char dummy;
    urlsBridgeKnowsAbout.insert(urlString, &dummy);
}

bool KWQKHTMLPart::haveToldBridgeAboutLoad(const QString &urlString)
{
    return urlsBridgeKnowsAbout.find(urlString) != 0;
}

void KWQKHTMLPart::clear()
{
    urlsBridgeKnowsAbout.clear();
    setMarkedTextRange(0);
    KHTMLPart::clear();
}

void KHTMLPart::print()
{
    [KWQ(this)->_bridge print];
}

KJS::Bindings::Instance *KWQKHTMLPart::getAppletInstanceForView (NSView *aView)
{
    jobject applet;
    
    // Get a pointer to the actual Java applet instance.
    if ([_bridge respondsToSelector:@selector(getAppletInView:)])
        applet = [_bridge getAppletInView:aView];
    else
        applet = [_bridge pollForAppletInView:aView];
    
    if (applet) {
        // Wrap the Java instance in a language neutral binding and hand
        // off ownership to the APPLET element.
        KJS::Bindings::Instance *instance = KJS::Bindings::Instance::createBindingForLanguageInstance (KJS::Bindings::Instance::JavaLanguage, applet);

        KJS::Bindings::RootObject *root = KJS::Bindings::RootObject::findRootObjectForNativeHandleFunction ()(aView);
        instance->setExecutionContext (root);
        
        return instance;
    }
    
    return 0;
}

@interface NSObject (WebPlugIn)
- (id)objectForWebScript;
- (void *)pluginScriptableObject;
@end

KJS::Bindings::Instance *KWQKHTMLPart::getEmbedInstanceForView (NSView *aView)
{
    if ([aView respondsToSelector:@selector(objectForWebScript)]){
        id object = [aView objectForWebScript];
        if (object)
            return KJS::Bindings::Instance::createBindingForLanguageInstance (KJS::Bindings::Instance::ObjectiveCLanguage, object);
    }
    else if ([aView respondsToSelector:@selector(pluginScriptableObject)]){
        void *object = [aView pluginScriptableObject];
        if (object)
            return KJS::Bindings::Instance::createBindingForLanguageInstance (KJS::Bindings::Instance::CLanguage, object);
    }
    return 0;
}

void KWQKHTMLPart::addPluginRootObject(const KJS::Bindings::RootObject *root)
{
    rootObjects.append (root);
}

void KWQKHTMLPart::cleanupPluginRootObjects()
{
    KJS::Bindings::RootObject *root;
    while ((root = rootObjects.getLast())) {
        root->removeAllNativeReferences ();
        rootObjects.removeLast();
    }
}

static NSString *nbspSpaceString = 0;
static NSString *nbspNBSPSpaceString = 0;

DocumentFragmentImpl *KWQKHTMLPart::documentFragmentWithText(NSString *text)
{
    if (!xmlDocImpl())
        return 0;

    DocumentFragmentImpl *fragment = xmlDocImpl()->createDocumentFragment();
    NSMutableString *string = [text mutableCopy];

    // Replace tabs with four plain spaces.
    // These spaces will get converted along with the other existing spaces below.
    [string replaceOccurrencesOfString:@"\t" withString:@"    " options:0 range:NSMakeRange(0, [string length])];

    if (!nbspSpaceString) {
        unichar nbspSpace[] = { 0xa0, ' ' };
        nbspSpaceString = [[NSString alloc] initWithCharacters:nbspSpace length:2];
    }
    
    if (!nbspNBSPSpaceString) {
        unichar nbspNBSPSpace[] = { 0xa0, 0xa0, ' ' };
        nbspNBSPSpaceString = [[NSString alloc] initWithCharacters:nbspNBSPSpace length:3];
    }

    unsigned stringLength = [string length];
    NSRange range = NSMakeRange(0, stringLength);
    while (1) {
        // FIXME: This only converts plain old spaces, and does not
        // deal with more exotic whitespace. Note that we want to 
        // leave newlines and returns alone at this point anyway, 
        // since those are handled specially later.
        NSRange replaceRange = [string rangeOfString:@"  " options:NSLiteralSearch range:range];
        if (replaceRange.location == NSNotFound)
            break;
            
        // Found two adjoining spaces.
        // Now, lookahead to see if these two spaces are followed by:
        //   1. another space and then a non-space
        //   2. another space and then the end of the string
        // If either 1 or 2 is true, replace the three spaces found with nbsp+nbsp+space, 
        // otherwise, replace the first two spaces with nbsp+space.
        unsigned lookahead = replaceRange.location + 2;
        if ((lookahead + 2 < stringLength && [string characterAtIndex:lookahead] == ' ' && [string characterAtIndex:lookahead + 1] != ' ') ||
            (lookahead + 1 == stringLength && [string characterAtIndex:lookahead] == ' ')) {
            replaceRange.length = 3;
            [string replaceCharactersInRange:replaceRange withString:nbspNBSPSpaceString];
        }
        else {
            [string replaceCharactersInRange:replaceRange withString:nbspSpaceString];
        }
        range.location = replaceRange.location + 2;
        range.length = stringLength - range.location;
    }
    
    // Handle line endings, replacing them with BR elements.
    [string replaceOccurrencesOfString:@"\r\n" withString:@"\n" options:0 range:NSMakeRange(0, [string length])];
    [string replaceOccurrencesOfString:@"\r" withString:@"\n" options:0 range:NSMakeRange(0, [string length])];
    NSArray *array = [string componentsSeparatedByString:@"\n"];
    int count = [array count];
    int i;
    for (i = 0; i < count; i++) {
        int exceptionCode = 0;
        if (i != 0) {
            ElementImpl *breakNode = xmlDocImpl()->createHTMLElement("br", exceptionCode);
            ASSERT(exceptionCode == 0);
            fragment->appendChild(breakNode, exceptionCode);
            ASSERT(exceptionCode == 0);
        }
        NSString *component = (NSString *)[array objectAtIndex:i];
        if ([component length] > 0) {
            NodeImpl *textNode = xmlDocImpl()->createTextNode(component);
            fragment->appendChild(textNode, exceptionCode);
        }
    }
    

    return fragment;
}

void KWQKHTMLPart::registerCommandForUndo(const khtml::EditCommandPtr &cmd)
{
    ASSERT(cmd.get());
    KWQEditCommand *kwq = [KWQEditCommand commandWithEditCommandImpl:cmd.get()];
    [[_bridge undoManager] registerUndoWithTarget:_bridge selector:@selector(undoEditing:) object:kwq];
    _haveUndoRedoOperations = YES;
}

void KWQKHTMLPart::registerCommandForRedo(const khtml::EditCommandPtr &cmd)
{
    ASSERT(cmd.get());
    KWQEditCommand *kwq = [KWQEditCommand commandWithEditCommandImpl:cmd.get()];
    [[_bridge undoManager] registerUndoWithTarget:_bridge selector:@selector(redoEditing:) object:kwq];
    _haveUndoRedoOperations = YES;
}

void KWQKHTMLPart::clearUndoRedoOperations()
{
    if (_haveUndoRedoOperations) {
	[[_bridge undoManager] removeAllActionsWithTarget:_bridge];
	_haveUndoRedoOperations = NO;
    }
}

void KWQKHTMLPart::issueUndoCommand()
{
    if (canUndo())
        [[_bridge undoManager] undo];
}

void KWQKHTMLPart::issueRedoCommand()
{
    if (canRedo())
        [[_bridge undoManager] redo];
}

void KWQKHTMLPart::issueCutCommand()
{
    [_bridge issueCutCommand];
}

void KWQKHTMLPart::issueCopyCommand()
{
    [_bridge issueCopyCommand];
}

void KWQKHTMLPart::issuePasteCommand()
{
    [_bridge issuePasteCommand];
}

bool KHTMLPart::canUndo() const
{
    return [[KWQ(this)->_bridge undoManager] canUndo];
}

bool KHTMLPart::canRedo() const
{
    return [[KWQ(this)->_bridge undoManager] canRedo];
}

void KWQKHTMLPart::markMisspellingsInAdjacentWords(const VisiblePosition &p)
{
    if (![_bridge isContinuousSpellCheckingEnabled])
        return;
    markMisspellings(Selection(startOfWord(p, LeftWordIfOnBoundary), endOfWord(p, RightWordIfOnBoundary)));
}

void KWQKHTMLPart::markMisspellings(const Selection &selection)
{
    // This function is called with a selection already expanded to word boundaries.
    // Might be nice to assert that here.

    if (![_bridge isContinuousSpellCheckingEnabled])
        return;

    Range searchRange(selection.toRange());
    if (searchRange.isNull() || searchRange.isDetached())
        return;
    
    // If we're not in an editable node, bail.
    NodeImpl *editableNodeImpl = searchRange.startContainer().handle();
    if (!editableNodeImpl->isContentEditable())
        return;
    
    NSSpellChecker *checker = [NSSpellChecker sharedSpellChecker];
    WordAwareIterator it(searchRange);
    
    while (!it.atEnd()) {      // we may be starting at the end of the doc, and already by atEnd
        const QChar *chars = it.characters();
        long len = it.length();
        if (len > 1 || !chars[0].isSpace()) {
            NSString *chunk = [[NSString alloc] initWithCharactersNoCopy:(unichar *)chars length:len freeWhenDone:NO];
            int startIndex = 0;
            // Loop over the chunk to find each misspelling in it.
            while (startIndex < len) {
                NSRange misspelling = [checker checkSpellingOfString:chunk startingAt:startIndex language:nil wrap:NO inSpellDocumentWithTag:[_bridge spellCheckerDocumentTag] wordCount:NULL];
                if (misspelling.length == 0) {
                    break;
                }
                else {
                    // Build up result range and string.  Note the misspelling may span many text nodes,
                    // but the CharIterator insulates us from this complexity
                    Range misspellingRange(xmlDocImpl());
                    CharacterIterator chars(it.range());
                    chars.advance(misspelling.location);
                    misspellingRange.setStart(chars.range().startContainer(), chars.range().startOffset());
                    chars.advance(misspelling.length);
                    misspellingRange.setEnd(chars.range().startContainer(), chars.range().startOffset());
                    // Mark misspelling in document.
                    xmlDocImpl()->addMarker(misspellingRange, DocumentMarker::Spelling);
                    startIndex = misspelling.location + misspelling.length;
                }
            }
            [chunk release];
        }
    
        it.advance();
    }
}

void KWQKHTMLPart::respondToChangedSelection(const Selection &oldSelection, bool closeTyping)
{
    if (xmlDocImpl()) {
        if ([_bridge isContinuousSpellCheckingEnabled]) {
            VisiblePosition oldStart(oldSelection.start());
            Selection oldAdjacentWords(startOfWord(oldStart, LeftWordIfOnBoundary), endOfWord(oldStart, RightWordIfOnBoundary));

            VisiblePosition newStart(selection().start());
            Selection newAdjacentWords(startOfWord(newStart, LeftWordIfOnBoundary), endOfWord(newStart, RightWordIfOnBoundary));

            if (oldAdjacentWords != newAdjacentWords) {
                // Mark misspellings in the portion that was previously unmarked because of
                // the proximity of the start of the selection. We only spell check words in
                // the vicinity of the start of the old selection because the spelling checker
                // is not fast enough to do a lot of spelling checking implicitly. This matches
                // AppKit. This function is really the only code that knows that rule. The
                // markMisspellings function is prepared to handler larger ranges.

                // When typing we check spelling elsewhere, so don't redo it here.
                if (closeTyping) {
                    markMisspellings(oldAdjacentWords);
                }

                // This only erases a marker in the first word of the selection.
                // Perhaps peculiar, but it matches AppKit.
                xmlDocImpl()->removeMarker(newAdjacentWords.toRange(), DocumentMarker::Spelling);
            }
        } else {
            // When continuous spell checking is off, no markers appear after the selection changes.
            xmlDocImpl()->removeAllMarkers();
        }
    }

    [_bridge respondToChangedSelection];
}

void KWQKHTMLPart::respondToChangedContents()
{
    [_bridge respondToChangedContents];
}

bool KWQKHTMLPart::isContentEditable() const
{
    return [_bridge isEditable];
}

bool KWQKHTMLPart::shouldBeginEditing(const Range &range) const
{
    ASSERT(!range.isNull());
    return [_bridge shouldBeginEditing:[DOMRange _rangeWithImpl:range.handle()]];
}

bool KWQKHTMLPart::shouldEndEditing(const Range &range) const
{
    ASSERT(!range.isNull());
    return [_bridge shouldEndEditing:[DOMRange _rangeWithImpl:range.handle()]];
}

DOM::Range KWQKHTMLPart::markedTextRange() const
{
    return m_markedTextRange;
}

void KWQKHTMLPart::setMarkedTextRange(const DOM::Range &range)
{
    ASSERT(!range.handle() || range.startContainer() == range.endContainer());
    ASSERT(!range.handle() || range.startOffset() == range.endOffset() || range.startContainer().nodeType() == Node::TEXT_NODE);

    if (m_markedTextRange.handle() && xmlDocImpl() 
	&& m_markedTextRange.startContainer().handle()->renderer()) {
	m_markedTextRange.startContainer().handle()->renderer()->repaint();
    }

    m_markedTextRange = range;

    if (m_markedTextRange.handle() && xmlDocImpl() 
	&& m_markedTextRange.startContainer().handle()->renderer()) {
	m_markedTextRange.startContainer().handle()->renderer()->repaint();
    }
}

bool KWQKHTMLPart::canGoBackOrForward(int distance) const
{
    return [_bridge canGoBackOrForward:distance];
}

void KWQKHTMLPart::prepareForUserAction()
{
    // Reset the multiple form submission protection code.
    // We'll let you submit the same form twice if you do two separate user actions.
    _submittedFormURL = KURL();
}

void KWQKHTMLPart::didFirstLayout()
{
    [_bridge didFirstLayout];
}

NSMutableDictionary *KWQKHTMLPart::dashboardRegionsDictionary()
{
    DocumentImpl *doc = xmlDocImpl();
    if (!doc) {
        return nil;
    }

    const QValueList<DashboardRegionValue> regions = doc->dashboardRegions();
    uint i, count = regions.count();

    // Convert the QValueList<DashboardRegionValue> into a NSDictionary of WebDashboardRegions
    NSMutableDictionary *webRegions = [[[NSMutableDictionary alloc] initWithCapacity:count] autorelease];
    for (i = 0; i < count; i++) {
        DashboardRegionValue region = regions[i];
        NSRect clip;
        clip.origin.x = region.clip.x();
        clip.origin.y = region.clip.y();
        clip.size.width = region.clip.width();
        clip.size.height = region.clip.height();
        NSRect rect;
        rect.origin.x = region.bounds.x();
        rect.origin.y = region.bounds.y();
        rect.size.width = region.bounds.width();
        rect.size.height = region.bounds.height();
        NSString *label = region.label.getNSString();
        WebDashboardRegionType type = WebDashboardRegionTypeNone;
        if (region.type == khtml::StyleDashboardRegion::Circle)
            type = WebDashboardRegionTypeCircle;
        else if (region.type == khtml::StyleDashboardRegion::Rectangle)
            type = WebDashboardRegionTypeRectangle;
        NSMutableArray *regionValues = [webRegions objectForKey:label];
        if (!regionValues) {
            regionValues = [NSMutableArray array];
            [webRegions setObject:regionValues forKey:label];
        }
        
        WebDashboardRegion *webRegion = [[[WebDashboardRegion alloc] initWithRect:rect clip:clip type:type] autorelease];
        [regionValues addObject:webRegion];
    }
    
    return webRegions;
}

void KWQKHTMLPart::dashboardRegionsChanged()
{
    NSMutableDictionary *webRegions = dashboardRegionsDictionary();
    [_bridge dashboardRegionsChanged:webRegions];
}
