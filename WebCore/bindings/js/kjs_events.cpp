/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2004, 2005, 2006 Apple Computer, Inc.
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

#include "config.h"
#include "kjs_events.h"

#include "CString.h"
#include "Chrome.h"
#include "Clipboard.h"
#include "ClipboardEvent.h"
#include "Document.h"
#include "Event.h"
#include "EventNames.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "HTMLImageElement.h"
#include "HTMLNames.h"
#include "JSEvent.h"
#include "JSEventTargetNode.h"
#include "JSKeyboardEvent.h"
#include "JSMouseEvent.h"
#include "JSMutationEvent.h"
#include "JSOverflowEvent.h"
#include "JSTextEvent.h"
#include "JSWheelEvent.h"
#include "KURL.h"
#include "KeyboardEvent.h"
#include "MouseEvent.h"
#include "MutationEvent.h"
#include "OverflowEvent.h"
#include "Page.h"
#include "TextEvent.h"
#include "UIEvent.h"
#include "WheelEvent.h"
#include "kjs_proxy.h"
#include "kjs_window.h"

#include "kjs_events.lut.h"

using namespace WebCore;
using namespace EventNames;
using namespace HTMLNames;

namespace KJS {

JSAbstractEventListener::JSAbstractEventListener(bool _html)
    : html(_html)
{
}

void JSAbstractEventListener::handleEvent(Event* ele, bool isWindowEvent)
{
#ifdef KJS_DEBUGGER
    if (KJSDebugWin::instance() && KJSDebugWin::instance()->inSession())
        return;
#endif

    Event *event = ele;

    JSObject* listener = listenerObj();
    if (!listener)
        return;

    Window* window = windowObj();
    // Null check as clearWindowObj() can clear this and we still get called back by
    // xmlhttprequest objects. See http://bugs.webkit.org/show_bug.cgi?id=13275
    if (!window)
        return;
    Frame *frame = window->frame();
    if (!frame)
        return;
    KJSProxy* proxy = frame->scriptProxy();
    if (!proxy)
        return;

    JSLock lock;
  
    ScriptInterpreter* interpreter = proxy->interpreter();
    ExecState* exec = interpreter->globalExec();
  
    JSValue* handleEventFuncValue = listener->get(exec, "handleEvent");
    JSObject* handleEventFunc = 0;
    if (handleEventFuncValue->isObject()) {      
        handleEventFunc = static_cast<JSObject*>(handleEventFuncValue);
        if (!handleEventFunc->implementsCall())
            handleEventFunc = 0;
    }
  
    if (handleEventFunc || listener->implementsCall()) {
        ref();
      
        List args;
        args.append(toJS(exec, event));
      
        // Set the event we're handling in the Window object
        window->setCurrentEvent(event);
        // ... and in the interpreter
        interpreter->setCurrentEvent(event);
      
        JSValue* retval;
        if (handleEventFunc) {
            interpreter->startTimeoutCheck();
            retval = handleEventFunc->call(exec, listener, args);
        } else {
            JSObject* thisObj;
            if (isWindowEvent)
                thisObj = window;
            else
                thisObj = static_cast<JSObject*>(toJS(exec, event->currentTarget()));
            interpreter->startTimeoutCheck();
            retval = listener->call(exec, thisObj, args);
        }
        interpreter->stopTimeoutCheck();

        window->setCurrentEvent(0);
        interpreter->setCurrentEvent(0);

        if (exec->hadException()) {
            JSObject* exception = exec->exception()->toObject(exec);
            String message = exception->get(exec, exec->propertyNames().message)->toString(exec);
            int lineNumber = exception->get(exec, "line")->toInt32(exec);
            String sourceURL = exception->get(exec, "sourceURL")->toString(exec);
            if (Interpreter::shouldPrintExceptions())
                printf("(event handler):%s\n", message.utf8().data());
            if (Page* page = frame->page())
                page->chrome()->addMessageToConsole(JSMessageSource, ErrorMessageLevel, message, lineNumber, sourceURL);
            exec->clearException();
        } else {
            if (!retval->isUndefinedOrNull() && event->storesResultAsString())
                event->storeResult(retval->toString(exec));
            if (html) {
                bool retvalbool;
                if (retval->getBoolean(retvalbool) && !retvalbool)
                    event->preventDefault();
            }
        }

        Document::updateDocumentsRendering();
        deref();
    }
}

bool JSAbstractEventListener::isHTMLEventListener() const
{
    return html;
}

// -------------------------------------------------------------------------

JSUnprotectedEventListener::JSUnprotectedEventListener(JSObject* _listener, Window* _win, bool _html)
  : JSAbstractEventListener(_html)
  , listener(_listener)
  , win(_win)
{
    if (_listener) {
        Window::UnprotectedListenersMap& listeners = _html 
            ? _win->jsUnprotectedHTMLEventListeners() : _win->jsUnprotectedEventListeners();
        listeners.set(_listener, this);
    }
}

JSUnprotectedEventListener::~JSUnprotectedEventListener()
{
    if (listener && win) {
        Window::UnprotectedListenersMap& listeners = isHTMLEventListener()
            ? win->jsUnprotectedHTMLEventListeners() : win->jsUnprotectedEventListeners();
        listeners.remove(listener);
    }
}

JSObject* JSUnprotectedEventListener::listenerObj() const
{ 
    return listener; 
}

Window* JSUnprotectedEventListener::windowObj() const
{
    return win;
}

void JSUnprotectedEventListener::clearWindowObj()
{
    win = 0;
}

void JSUnprotectedEventListener::mark()
{
    if (listener && !listener->marked())
        listener->mark();
}

#ifndef NDEBUG
#ifndef LOG_CHANNEL_PREFIX
#define LOG_CHANNEL_PREFIX Log
#endif
WTFLogChannel LogWebCoreEventListenerLeaks = { 0x00000000, "", WTFLogChannelOn };

struct EventListenerCounter {
    static unsigned count;
    ~EventListenerCounter()
    {
        if (count)
            LOG(WebCoreEventListenerLeaks, "LEAK: %u EventListeners\n", count);
    }
};
unsigned EventListenerCounter::count = 0;
static EventListenerCounter eventListenerCounter;
#endif

// -------------------------------------------------------------------------

JSEventListener::JSEventListener(JSObject* _listener, Window* _win, bool _html)
    : JSAbstractEventListener(_html)
    , listener(_listener)
    , win(_win)
{
    if (_listener) {
        Window::ListenersMap& listeners = _html
            ? _win->jsHTMLEventListeners() : _win->jsEventListeners();
        listeners.set(_listener, this);
    }
#ifndef NDEBUG
    ++eventListenerCounter.count;
#endif
}

JSEventListener::~JSEventListener()
{
    if (listener && win) {
        Window::ListenersMap& listeners = isHTMLEventListener()
            ? win->jsHTMLEventListeners() : win->jsEventListeners();
        listeners.remove(listener);
    }
#ifndef NDEBUG
    --eventListenerCounter.count;
#endif
}

JSObject* JSEventListener::listenerObj() const
{ 
    return listener; 
}

Window* JSEventListener::windowObj() const
{
    return win;
}

void JSEventListener::clearWindowObj()
{
    win = 0;
}

// -------------------------------------------------------------------------

JSLazyEventListener::JSLazyEventListener(const String& functionName, const String& code, Window* win, Node* node, int lineno)
  : JSEventListener(0, win, true)
  , m_functionName(functionName)
  , code(code)
  , parsed(false)
  , lineNumber(lineno)
  , originalNode(node)
{
    // We don't retain the original node because we assume it
    // will stay alive as long as this handler object is around
    // and we need to avoid a reference cycle. If JS transfers
    // this handler to another node, parseCode will be called and
    // then originalNode is no longer needed.
}

JSObject* JSLazyEventListener::listenerObj() const
{
    parseCode();
    return listener;
}

JSValue* JSLazyEventListener::eventParameterName() const
{
    static ProtectedPtr<JSValue> eventString = jsString("event");
    return eventString.get();
}

void JSLazyEventListener::parseCode() const
{
    if (parsed)
        return;
    parsed = true;

    Frame *frame = windowObj()->frame();
    KJSProxy *proxy = 0;
    if (frame)
        proxy = frame->scriptProxy();

    if (proxy) {
        ScriptInterpreter* interpreter = proxy->interpreter();
        ExecState* exec = interpreter->globalExec();

        JSLock lock;
        JSObject* constr = interpreter->builtinFunction();
        List args;

        UString sourceURL(frame->loader()->url().url());
        args.append(eventParameterName());
        args.append(jsString(code));
        listener = constr->construct(exec, args, m_functionName, sourceURL, lineNumber); // ### is globalExec ok ?

        FunctionImp* listenerAsFunction = static_cast<FunctionImp*>(listener.get());

        if (exec->hadException()) {
            exec->clearException();

            // failed to parse, so let's just make this listener a no-op
            listener = 0;
        } else if (originalNode) {
            // Add the event's home element to the scope
            // (and the document, and the form - see JSHTMLElement::eventHandlerScope)
            ScopeChain scope = listenerAsFunction->scope();

            JSValue* thisObj = toJS(exec, originalNode);
            if (thisObj->isObject()) {
                static_cast<JSEventTargetNode*>(thisObj)->pushEventHandlerScope(exec, scope);
                listenerAsFunction->setScope(scope);
            }
        }
    }

    // no more need to keep the unparsed code around
    m_functionName = String();
    code = String();

    if (listener) {
        Window::ListenersMap& listeners = isHTMLEventListener()
            ? windowObj()->jsHTMLEventListeners() : windowObj()->jsEventListeners();
        listeners.set(listener, const_cast<JSLazyEventListener*>(this));
    }
}

JSValue* getNodeEventListener(EventTargetNode* n, const AtomicString& eventType)
{
    if (JSAbstractEventListener* listener = static_cast<JSAbstractEventListener*>(n->getHTMLEventListener(eventType)))
        if (JSValue* obj = listener->listenerObj())
            return obj;
    return jsNull();
}

// -------------------------------------------------------------------------

const ClassInfo JSClipboard::info = { "Clipboard", 0, &JSClipboardTable, 0 };

/* Source for JSClipboardTable. Use "make hashtables" to regenerate.
@begin JSClipboardTable 3
  dropEffect    JSClipboard::DropEffect   DontDelete
  effectAllowed JSClipboard::EffectAllowed        DontDelete
  types         JSClipboard::Types        DontDelete|ReadOnly
@end
@begin JSClipboardPrototypeTable 4
  clearData     JSClipboard::ClearData    DontDelete|Function 0
  getData       JSClipboard::GetData      DontDelete|Function 1
  setData       JSClipboard::SetData      DontDelete|Function 2
  setDragImage  JSClipboard::SetDragImage DontDelete|Function 3
@end
*/

KJS_DEFINE_PROTOTYPE(JSClipboardPrototype)
KJS_IMPLEMENT_PROTOTYPE_FUNCTION(JSClipboardPrototypeFunction)
KJS_IMPLEMENT_PROTOTYPE("Clipboard", JSClipboardPrototype, JSClipboardPrototypeFunction)

JSClipboard::JSClipboard(ExecState* exec, WebCore::Clipboard* cb)
    : m_impl(cb)
{
    setPrototype(JSClipboardPrototype::self(exec));
}

JSClipboard::~JSClipboard()
{
    ScriptInterpreter::forgetDOMObject(m_impl.get());
}

bool JSClipboard::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    return getStaticValueSlot<JSClipboard, DOMObject>(exec, &JSClipboardTable, this, propertyName, slot);
}

JSValue* JSClipboard::getValueProperty(ExecState* exec, int token) const
{
    WebCore::Clipboard* clipboard = impl();
    switch (token) {
        case DropEffect:
            ASSERT(clipboard->isForDragging() || clipboard->dropEffect().isNull());
            return jsStringOrUndefined(clipboard->dropEffect());
        case EffectAllowed:
            ASSERT(clipboard->isForDragging() || clipboard->effectAllowed().isNull());
            return jsStringOrUndefined(clipboard->effectAllowed());
        case Types:
        {
            HashSet<String> types = clipboard->types();
            if (types.isEmpty())
                return jsNull();
            else {
                List list;
                HashSet<String>::const_iterator end = types.end();
                for (HashSet<String>::const_iterator it = types.begin(); it != end; ++it)
                    list.append(jsString(UString(*it)));
                return exec->lexicalInterpreter()->builtinArray()->construct(exec, list);
            }
        }
        default:
            return 0;
    }
}

void JSClipboard::put(ExecState* exec, const Identifier& propertyName, JSValue* value, int attr)
{
    lookupPut<JSClipboard, DOMObject>(exec, propertyName, value, attr, &JSClipboardTable, this );
}

void JSClipboard::putValueProperty(ExecState* exec, int token, JSValue* value, int /*attr*/)
{
    WebCore::Clipboard* clipboard = impl();
    switch (token) {
        case DropEffect:
            // can never set this when not for dragging, thus getting always returns NULL string
            if (clipboard->isForDragging())
                clipboard->setDropEffect(value->toString(exec));
            break;
        case EffectAllowed:
            // can never set this when not for dragging, thus getting always returns NULL string
            if (clipboard->isForDragging())
                clipboard->setEffectAllowed(value->toString(exec));
            break;
    }
}

JSValue* JSClipboardPrototypeFunction::callAsFunction(ExecState* exec, JSObject* thisObj, const List& args)
{
    if (!thisObj->inherits(&JSClipboard::info))
        return throwError(exec, TypeError);

    WebCore::Clipboard* clipboard = static_cast<JSClipboard*>(thisObj)->impl();
    switch (id) {
        case JSClipboard::ClearData:
            if (args.size() == 0) {
                clipboard->clearAllData();
                return jsUndefined();
            } else if (args.size() == 1) {
                clipboard->clearData(args[0]->toString(exec));
                return jsUndefined();
            } else
                return throwError(exec, SyntaxError, "clearData: Invalid number of arguments");
        case JSClipboard::GetData:
        {
            if (args.size() == 1) {
                bool success;
                String result = clipboard->getData(args[0]->toString(exec), success);
                if (success)
                    return jsString(result);
                return jsUndefined();
            } else
                return throwError(exec, SyntaxError, "getData: Invalid number of arguments");
        }
        case JSClipboard::SetData:
            if (args.size() == 2)
                return jsBoolean(clipboard->setData(args[0]->toString(exec), args[1]->toString(exec)));
            return throwError(exec, SyntaxError, "setData: Invalid number of arguments");
        case JSClipboard::SetDragImage:
        {
            if (!clipboard->isForDragging())
                return jsUndefined();

            if (args.size() != 3)
                return throwError(exec, SyntaxError, "setDragImage: Invalid number of arguments");

            int x = static_cast<int>(args[1]->toNumber(exec));
            int y = static_cast<int>(args[2]->toNumber(exec));

            // See if they passed us a node
            Node* node = toNode(args[0]);
            if (!node)
                return throwError(exec, TypeError);
            
            if (!node->isElementNode())
                return throwError(exec, SyntaxError, "setDragImageFromElement: Invalid first argument");

            if (static_cast<Element*>(node)->hasLocalName(imgTag) && 
                !node->inDocument())
                clipboard->setDragImage(static_cast<HTMLImageElement*>(node)->cachedImage(), IntPoint(x, y));
            else
                clipboard->setDragImageElement(node, IntPoint(x, y));                    
                
            return jsUndefined();
        }
    }
    return jsUndefined();
}

JSValue* toJS(ExecState* exec, WebCore::Clipboard* obj)
{
    return cacheDOMObject<WebCore::Clipboard, JSClipboard>(exec, obj);
}

WebCore::Clipboard* toClipboard(JSValue* val)
{
    return val->isObject(&JSClipboard::info) ? static_cast<JSClipboard*>(val)->impl() : 0;
}

}
