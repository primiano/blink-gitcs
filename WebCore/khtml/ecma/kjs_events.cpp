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

#include "CachedImage.h"
#include "Document.h"
#include "EventNames.h"
#include "Frame.h"
#include "JSMutationEvent.h"
#include "JSWheelEvent.h"
#include "dom2_eventsimpl.h"
#include "AbstractView.h"
#include "html_imageimpl.h"
#include "HTMLNames.h"
#include "kjs_proxy.h"
#include "kjs_views.h"
#include "kjs_window.h"
#include "rendering/RenderObject.h"

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
    Frame *frame = window->frame();
    if (!frame)
        return;
    KJSProxy* proxy = frame->jScript();
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
        if (handleEventFunc)
            retval = handleEventFunc->call(exec, listener, args);
        else {
            JSObject* thisObj;
            if (isWindowEvent)
                thisObj = window;
            else
                thisObj = static_cast<JSObject*>(toJS(exec, event->currentTarget()));
            retval = listener->call(exec, thisObj, args);
        }

        window->setCurrentEvent(0);
        interpreter->setCurrentEvent(0);

        if (exec->hadException()) {
            JSObject* exception = exec->exception()->toObject(exec);
            String message = exception->get(exec, messagePropertyName)->toString(exec).domString();
            int lineNumber = exception->get(exec, "line")->toInt32(exec);
            String sourceURL = exception->get(exec, "sourceURL")->toString(exec).domString();
            if (Interpreter::shouldPrintExceptions())
                printf("(event handler):%s\n", message.deprecatedString().utf8().data());
            frame->addMessageToConsole(message, lineNumber, sourceURL);
            exec->clearException();
        } else {
            if (!retval->isUndefinedOrNull() && event->storesResultAsString())
                event->storeResult(retval->toString(exec).domString());
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
    if (_listener)
        _win->jsUnprotectedEventListeners.set(_listener, this);
}

JSUnprotectedEventListener::~JSUnprotectedEventListener()
{
    if (listener && win)
        win->jsUnprotectedEventListeners.remove(listener);
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

// -------------------------------------------------------------------------

JSEventListener::JSEventListener(JSObject* _listener, Window* _win, bool _html)
    : JSAbstractEventListener(_html)
    , listener(_listener)
    , win(_win)
{
    if (_listener)
        _win->jsEventListeners.set(_listener, this);
}

JSEventListener::~JSEventListener()
{
    if (listener && win)
        win->jsEventListeners.remove(listener);
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

JSLazyEventListener::JSLazyEventListener(const String& _code, Window* _win, Node* _originalNode, int lineno)
  : JSEventListener(0, _win, true),
    code(_code),
    parsed(false)
{
    lineNumber = lineno;

    // We don't retain the original node, because we assume it
    // will stay alive as long as this handler object is around
    // and we need to avoid a reference cycle. If JS transfers
    // this handler to another node, parseCode will be called and
    // then originalNode is no longer needed.
    
    originalNode = _originalNode;
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
        proxy = frame->jScript();

    if (proxy) {
        ScriptInterpreter* interpreter = proxy->interpreter();
        ExecState* exec = interpreter->globalExec();

        JSLock lock;
        JSObject* constr = interpreter->builtinFunction();
        List args;

        UString sourceURL(frame->url().url());
        args.append(eventParameterName());
        args.append(jsString(code));
        listener = constr->construct(exec, args, sourceURL, lineNumber); // ### is globalExec ok ?

        if (exec->hadException()) {
            exec->clearException();

            // failed to parse, so let's just make this listener a no-op
            listener = 0;
        } else if (originalNode) {
            // Add the event's home element to the scope
            // (and the document, and the form - see JSHTMLElement::eventHandlerScope)
            ScopeChain scope = listener->scope();

            JSValue* thisObj = toJS(exec, originalNode);
            if (thisObj->isObject()) {
                static_cast<DOMEventTargetNode*>(thisObj)->pushEventHandlerScope(exec, scope);
                listener->setScope(scope);
            }
        }
    }

    // no more need to keep the unparsed code around
    code = String();

    if (listener)
        windowObj()->jsEventListeners.set(listener, const_cast<JSLazyEventListener*>(this));
}

JSValue* getNodeEventListener(EventTargetNode* n, const AtomicString& eventType)
{
    if (JSAbstractEventListener* listener = static_cast<JSAbstractEventListener*>(n->getHTMLEventListener(eventType)))
        if (JSValue* obj = listener->listenerObj())
            return obj;
    return jsNull();
}

// -------------------------------------------------------------------------

const ClassInfo EventConstructor::info = { "EventConstructor", 0, &EventConstructorTable, 0 };
/*
@begin EventConstructorTable 3
  CAPTURING_PHASE       WebCore::Event::CAPTURING_PHASE     DontDelete|ReadOnly
  AT_TARGET             WebCore::Event::AT_TARGET           DontDelete|ReadOnly
  BUBBLING_PHASE        WebCore::Event::BUBBLING_PHASE      DontDelete|ReadOnly
# Reverse-engineered from Netscape
  MOUSEDOWN             1                               DontDelete|ReadOnly
  MOUSEUP               2                               DontDelete|ReadOnly
  MOUSEOVER             4                               DontDelete|ReadOnly
  MOUSEOUT              8                               DontDelete|ReadOnly
  MOUSEMOVE             16                              DontDelete|ReadOnly
  MOUSEDRAG             32                              DontDelete|ReadOnly
  CLICK                 64                              DontDelete|ReadOnly
  DBLCLICK              128                             DontDelete|ReadOnly
  KEYDOWN               256                             DontDelete|ReadOnly
  KEYUP                 512                             DontDelete|ReadOnly
  KEYPRESS              1024                            DontDelete|ReadOnly
  DRAGDROP              2048                            DontDelete|ReadOnly
  FOCUS                 4096                            DontDelete|ReadOnly
  BLUR                  8192                            DontDelete|ReadOnly
  SELECT                16384                           DontDelete|ReadOnly
  CHANGE                32768                           DontDelete|ReadOnly
@end
*/

bool EventConstructor::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
  return getStaticValueSlot<EventConstructor, DOMObject>(exec, &EventConstructorTable, this, propertyName, slot);
}

JSValue *EventConstructor::getValueProperty(ExecState *, int token) const
{
  // We use the token as the value to return directly
  return jsNumber(token);
}

JSValue *getEventConstructor(ExecState *exec)
{
  return cacheGlobalObject<EventConstructor>(exec, "[[event.constructor]]");
}

// -------------------------------------------------------------------------

const ClassInfo DOMEvent::info = { "Event", 0, &DOMEventTable, 0 };
/*
@begin DOMEventTable 12
  type          DOMEvent::Type          DontDelete|ReadOnly
  target        DOMEvent::Target        DontDelete|ReadOnly
  currentTarget DOMEvent::CurrentTarget DontDelete|ReadOnly
  srcElement    DOMEvent::SrcElement    DontDelete|ReadOnly
  eventPhase    DOMEvent::EventPhase    DontDelete|ReadOnly
  bubbles       DOMEvent::Bubbles       DontDelete|ReadOnly
  cancelable    DOMEvent::Cancelable    DontDelete|ReadOnly
  timeStamp     DOMEvent::TimeStamp     DontDelete|ReadOnly
  returnValue   DOMEvent::ReturnValue   DontDelete
  cancelBubble  DOMEvent::CancelBubble  DontDelete
  dataTransfer  DOMEvent::DataTransfer  DontDelete|ReadOnly
  clipboardData  DOMEvent::ClipboardData  DontDelete|ReadOnly
@end
@begin DOMEventProtoTable 3
  stopPropagation       DOMEvent::StopPropagation       DontDelete|Function 0
  preventDefault        DOMEvent::PreventDefault        DontDelete|Function 0
  initEvent             DOMEvent::InitEvent             DontDelete|Function 3
@end
*/
KJS_IMPLEMENT_PROTOFUNC(DOMEventProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("DOMEvent", DOMEventProto, DOMEventProtoFunc)

DOMEvent::DOMEvent(ExecState *exec, Event *e)
  : m_impl(e), clipboard(0) 
{
  setPrototype(DOMEventProto::self(exec));
}

DOMEvent::~DOMEvent()
{
  ScriptInterpreter::forgetDOMObject(m_impl.get());
}

// pass marks through to JS objects we hold during garbage collection
void DOMMouseEvent::mark()
{
    JSObject::mark();
    if (clipboard && !clipboard->marked())
        clipboard->mark();
}

bool DOMEvent::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
  return getStaticValueSlot<DOMEvent, DOMObject>(exec, &DOMEventTable, this, propertyName, slot);
}

JSValue *DOMEvent::getValueProperty(ExecState *exec, int token) const
{
  Event &event = *m_impl;
  switch (token) {
  case Type:
    return jsString(event.type().domString());
  case Target:
  case SrcElement: /*MSIE extension - "the object that fired the event"*/
    return toJS(exec, event.target());
  case CurrentTarget:
    return toJS(exec, event.currentTarget());
  case EventPhase:
    return jsNumber(event.eventPhase());
  case Bubbles:
    return jsBoolean(event.bubbles());
  case CancelBubble:
    return jsBoolean(event.getCancelBubble());
  case ReturnValue:
    return jsBoolean(!event.defaultPrevented());
  case Cancelable:
    return jsBoolean(event.cancelable());
  case TimeStamp:
    return jsNumber(event.timeStamp());
  case ClipboardData:
  {
    if (event.isClipboardEvent()) {
      ClipboardEvent *impl = static_cast<ClipboardEvent *>(&event);
      if (!clipboard) {
        clipboard = new Clipboard(exec, impl->clipboard());
      }
      return clipboard;
    } else {
      return jsUndefined();
    }
  }
  case DataTransfer:
  {
    if (event.isDragEvent()) {
      MouseEvent *impl = static_cast<MouseEvent *>(&event);
      if (!clipboard) {
        clipboard = new Clipboard(exec, impl->clipboard());
      }
      return clipboard;
    } else {
      return jsUndefined();
    }
  }
  default:
    return NULL;
  }
}

void DOMEvent::put(ExecState *exec, const Identifier &propertyName,
                      JSValue *value, int attr)
{
  lookupPut<DOMEvent, DOMObject>(exec, propertyName, value, attr,
                                          &DOMEventTable, this);
}

void DOMEvent::putValueProperty(ExecState *exec, int token, JSValue *value, int)
{
  Event &event = *m_impl;
  switch (token) {
  case ReturnValue:
    event.setDefaultPrevented(!value->toBoolean(exec));
    break;
  case CancelBubble:
    event.setCancelBubble(value->toBoolean(exec));
    break;
  default:
    break;
  }
}

JSValue *DOMEventProtoFunc::callAsFunction(ExecState *exec, JSObject * thisObj, const List &args)
{
  if (!thisObj->inherits(&DOMEvent::info))
    return throwError(exec, TypeError);
  Event &event = *static_cast<DOMEvent *>( thisObj )->impl();
  switch (id) {
    case DOMEvent::StopPropagation:
      event.stopPropagation();
      return jsUndefined();
    case DOMEvent::PreventDefault:
      event.preventDefault();
      return jsUndefined();
    case DOMEvent::InitEvent:
      event.initEvent(AtomicString(args[0]->toString(exec).domString()), args[1]->toBoolean(exec), args[2]->toBoolean(exec));
      return jsUndefined();
  };
  return jsUndefined();
}

JSValue *toJS(ExecState *exec, Event *e)
{
  if (!e)
    return jsNull();
  ScriptInterpreter* interp = static_cast<ScriptInterpreter *>(exec->dynamicInterpreter());

  JSLock lock;

  DOMObject *ret = interp->getDOMObject(e);
  if (!ret) {
    if (e->isKeyboardEvent())
      ret = new DOMKeyboardEvent(exec, static_cast<KeyboardEvent *>(e));
    else if (e->isMouseEvent())
      ret = new DOMMouseEvent(exec, static_cast<MouseEvent *>(e));
    else if (e->isWheelEvent())
      ret = new JSWheelEvent(exec, static_cast<WheelEvent *>(e));
    else if (e->isUIEvent())
      ret = new DOMUIEvent(exec, static_cast<UIEvent *>(e));
    else if (e->isMutationEvent())
      ret = new JSMutationEvent(exec, static_cast<MutationEvent *>(e));
    else
      ret = new DOMEvent(exec, e);

    interp->putDOMObject(e, ret);
  }

  return ret;
}

Event *toEvent(JSValue *val)
{
    if (!val || !val->isObject(&DOMEvent::info))
        return 0;
    return static_cast<DOMEvent *>(val)->impl();
}

// -------------------------------------------------------------------------


const ClassInfo EventExceptionConstructor::info = { "EventExceptionConstructor", 0, &EventExceptionConstructorTable, 0 };
/*
@begin EventExceptionConstructorTable 1
  UNSPECIFIED_EVENT_TYPE_ERR    WebCore::UNSPECIFIED_EVENT_TYPE_ERR-WebCore::EventExceptionOffset DontDelete|ReadOnly
@end
*/
bool EventExceptionConstructor::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
  return getStaticValueSlot<EventExceptionConstructor, DOMObject>(exec, &EventExceptionConstructorTable, this, propertyName, slot);
}

JSValue *EventExceptionConstructor::getValueProperty(ExecState *, int token) const
{
  // We use the token as the value to return directly
  return jsNumber(token);
}

JSValue *getEventExceptionConstructor(ExecState *exec)
{
  return cacheGlobalObject<EventExceptionConstructor>(exec, "[[eventException.constructor]]");
}

// -------------------------------------------------------------------------

const ClassInfo DOMUIEvent::info = { "UIEvent", &DOMEvent::info, &DOMUIEventTable, 0 };
/*
@begin DOMUIEventTable 8
  view          DOMUIEvent::View        DontDelete|ReadOnly
  detail        DOMUIEvent::Detail      DontDelete|ReadOnly
  keyCode       DOMUIEvent::KeyCode     DontDelete|ReadOnly
  charCode      DOMUIEvent::CharCode    DontDelete|ReadOnly
  layerX        DOMUIEvent::LayerX      DontDelete|ReadOnly
  layerY        DOMUIEvent::LayerY      DontDelete|ReadOnly
  pageX         DOMUIEvent::PageX       DontDelete|ReadOnly
  pageY         DOMUIEvent::PageY       DontDelete|ReadOnly
  which         DOMUIEvent::Which       DontDelete|ReadOnly
@end
@begin DOMUIEventProtoTable 1
  initUIEvent   DOMUIEvent::InitUIEvent DontDelete|Function 5
@end
*/
KJS_DEFINE_PROTOTYPE_WITH_PROTOTYPE(DOMUIEventProto, DOMEventProto)
KJS_IMPLEMENT_PROTOFUNC(DOMUIEventProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("DOMUIEvent", DOMUIEventProto, DOMUIEventProtoFunc)

DOMUIEvent::DOMUIEvent(ExecState *exec, UIEvent *e)
  : DOMEvent(exec, e)
{
  setPrototype(DOMUIEventProto::self(exec));
}

bool DOMUIEvent::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
  return getStaticValueSlot<DOMUIEvent, DOMEvent>(exec, &DOMUIEventTable, this, propertyName, slot);
}

JSValue *DOMUIEvent::getValueProperty(ExecState *exec, int token) const
{
  UIEvent &event = *static_cast<UIEvent *>(impl());
  switch (token) {
  case View:
    return toJS(exec, event.view());
  case Detail:
    return jsNumber(event.detail());
  case KeyCode:
    return jsNumber(event.keyCode());
  case CharCode:
    return jsNumber(event.charCode());
  case LayerX:
    return jsNumber(event.layerX());
  case LayerY:
    return jsNumber(event.layerY());
  case PageX:
    return jsNumber(event.pageX());
  case PageY:
    return jsNumber(event.pageY());
  case Which:
    return jsNumber(event.which());
  default:
    return jsUndefined();
  }
}

JSValue *DOMUIEventProtoFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  if (!thisObj->inherits(&DOMUIEvent::info))
    return throwError(exec, TypeError);
  UIEvent &uiEvent = *static_cast<UIEvent *>(static_cast<DOMUIEvent *>(thisObj)->impl());
  switch (id) {
    case DOMUIEvent::InitUIEvent:
      uiEvent.initUIEvent(AtomicString(args[0]->toString(exec).domString()),
                          args[1]->toBoolean(exec),
                          args[2]->toBoolean(exec),
                          toAbstractView(args[3]),
                          args[4]->toInt32(exec));
      return jsUndefined();
  }
  return jsUndefined();
}

// -------------------------------------------------------------------------

const ClassInfo DOMMouseEvent::info = { "MouseEvent", &DOMUIEvent::info, &DOMMouseEventTable, 0 };

/*
@begin DOMMouseEventTable 16
  screenX       DOMMouseEvent::ScreenX  DontDelete|ReadOnly
  screenY       DOMMouseEvent::ScreenY  DontDelete|ReadOnly
  clientX       DOMMouseEvent::ClientX  DontDelete|ReadOnly
  x             DOMMouseEvent::X        DontDelete|ReadOnly
  clientY       DOMMouseEvent::ClientY  DontDelete|ReadOnly
  y             DOMMouseEvent::Y        DontDelete|ReadOnly
  offsetX       DOMMouseEvent::OffsetX  DontDelete|ReadOnly
  offsetY       DOMMouseEvent::OffsetY  DontDelete|ReadOnly
  ctrlKey       DOMMouseEvent::CtrlKey  DontDelete|ReadOnly
  shiftKey      DOMMouseEvent::ShiftKey DontDelete|ReadOnly
  altKey        DOMMouseEvent::AltKey   DontDelete|ReadOnly
  metaKey       DOMMouseEvent::MetaKey  DontDelete|ReadOnly
  button        DOMMouseEvent::Button   DontDelete|ReadOnly
  relatedTarget DOMMouseEvent::RelatedTarget DontDelete|ReadOnly
  fromElement   DOMMouseEvent::FromElement DontDelete|ReadOnly
  toElement     DOMMouseEvent::ToElement        DontDelete|ReadOnly
@end
@begin DOMMouseEventProtoTable 1
  initMouseEvent        DOMMouseEvent::InitMouseEvent   DontDelete|Function 15
@end
*/
KJS_DEFINE_PROTOTYPE_WITH_PROTOTYPE(DOMMouseEventProto, DOMUIEventProto)
KJS_IMPLEMENT_PROTOFUNC(DOMMouseEventProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("DOMMouseEvent", DOMMouseEventProto, DOMMouseEventProtoFunc)

DOMMouseEvent::DOMMouseEvent(ExecState *exec, MouseEvent *e)
  : DOMUIEvent(exec, e)
{
  setPrototype(DOMMouseEventProto::self(exec));
}

bool DOMMouseEvent::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
  return getStaticValueSlot<DOMMouseEvent, DOMUIEvent>(exec, &DOMMouseEventTable, this, propertyName, slot);
}

JSValue *DOMMouseEvent::getValueProperty(ExecState *exec, int token) const
{
  MouseEvent &event = *static_cast<MouseEvent *>(impl());
  switch (token) {
  case ScreenX:
    return jsNumber(event.screenX());
  case ScreenY:
    return jsNumber(event.screenY());
  case ClientX:
    return jsNumber(event.clientX());
  case ClientY:
    return jsNumber(event.clientY());
  case OffsetX:
    return jsNumber(event.offsetX());
  case OffsetY:
    return jsNumber(event.offsetY());
  case CtrlKey:
    return jsBoolean(event.ctrlKey());
  case ShiftKey:
    return jsBoolean(event.shiftKey());
  case AltKey:
    return jsBoolean(event.altKey());
  case MetaKey:
    return jsBoolean(event.metaKey());
  case Button:
    return jsNumber(event.button());
  case ToElement:
    return toJS(exec, event.toElement());
  case FromElement:
    return toJS(exec, event.fromElement());
  case RelatedTarget:
    return toJS(exec, event.relatedTarget());
  case X:
    return jsNumber(event.x());
  case Y:
    return jsNumber(event.y());
  default:
    return NULL;
  }
}

JSValue *DOMMouseEventProtoFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  if (!thisObj->inherits(&DOMMouseEvent::info))
    return throwError(exec, TypeError);
  MouseEvent &mouseEvent = *static_cast<MouseEvent *>(static_cast<DOMMouseEvent *>(thisObj)->impl());
  switch (id) {
    case DOMMouseEvent::InitMouseEvent:
      mouseEvent.initMouseEvent(AtomicString(args[0]->toString(exec).domString()), // typeArg
                                args[1]->toBoolean(exec), // canBubbleArg
                                args[2]->toBoolean(exec), // cancelableArg
                                toAbstractView(args[3]), // viewArg
                                args[4]->toInt32(exec), // detailArg
                                args[5]->toInt32(exec), // screenXArg
                                args[6]->toInt32(exec), // screenYArg
                                args[7]->toInt32(exec), // clientXArg
                                args[8]->toInt32(exec), // clientYArg
                                args[9]->toBoolean(exec), // ctrlKeyArg
                                args[10]->toBoolean(exec), // altKeyArg
                                args[11]->toBoolean(exec), // shiftKeyArg
                                args[12]->toBoolean(exec), // metaKeyArg
                                args[13]->toInt32(exec), // buttonArg
                                toNode(args[14])); // relatedTargetArg
      return jsUndefined();
  }
  return jsUndefined();
}

// -------------------------------------------------------------------------

const ClassInfo DOMKeyboardEvent::info = { "KeyboardEvent", &DOMUIEvent::info, &DOMKeyboardEventTable, 0 };

/*
@begin DOMKeyboardEventTable 5
  keyIdentifier DOMKeyboardEvent::KeyIdentifier DontDelete|ReadOnly
  keyLocation   DOMKeyboardEvent::KeyLocation   DontDelete|ReadOnly
  ctrlKey       DOMKeyboardEvent::CtrlKey       DontDelete|ReadOnly
  shiftKey      DOMKeyboardEvent::ShiftKey      DontDelete|ReadOnly
  altKey        DOMKeyboardEvent::AltKey        DontDelete|ReadOnly
  metaKey       DOMKeyboardEvent::MetaKey       DontDelete|ReadOnly
  altGraphKey   DOMKeyboardEvent::AltGraphKey   DontDelete|ReadOnly
@end
@begin DOMKeyboardEventProtoTable 1
  initKeyboardEvent     DOMKeyboardEvent::InitKeyboardEvent     DontDelete|Function 11
@end
*/
KJS_DEFINE_PROTOTYPE_WITH_PROTOTYPE(DOMKeyboardEventProto, DOMUIEventProto)
KJS_IMPLEMENT_PROTOFUNC(DOMKeyboardEventProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("DOMKeyboardEvent", DOMKeyboardEventProto, DOMKeyboardEventProtoFunc)

DOMKeyboardEvent::DOMKeyboardEvent(ExecState *exec, KeyboardEvent *e)
  : DOMUIEvent(exec, e)
{
  setPrototype(DOMKeyboardEventProto::self(exec));
}

bool DOMKeyboardEvent::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
  return getStaticValueSlot<DOMKeyboardEvent, DOMUIEvent>(exec, &DOMKeyboardEventTable, this, propertyName, slot);
}

JSValue *DOMKeyboardEvent::getValueProperty(ExecState *exec, int token) const
{
  KeyboardEvent &event = *static_cast<KeyboardEvent *>(impl());
  switch (token) {
  case KeyIdentifier:
    return jsString(event.keyIdentifier());
  case KeyLocation:
    return jsNumber(event.keyLocation());
  case CtrlKey:
    return jsBoolean(event.ctrlKey());
  case ShiftKey:
    return jsBoolean(event.shiftKey());
  case AltKey:
    return jsBoolean(event.altKey());
  case MetaKey:
    return jsBoolean(event.metaKey());
  case AltGraphKey:
    return jsBoolean(event.altGraphKey());
  default:
    return NULL;
  }
}

JSValue *DOMKeyboardEventProtoFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  if (!thisObj->inherits(&DOMKeyboardEvent::info))
    return throwError(exec, TypeError);
  KeyboardEvent &event = *static_cast<KeyboardEvent *>(static_cast<DOMUIEvent *>(thisObj)->impl());
  switch (id) {
    case DOMKeyboardEvent::InitKeyboardEvent:
      event.initKeyboardEvent(AtomicString(args[0]->toString(exec).domString()), // typeArg
                              args[1]->toBoolean(exec), // canBubbleArg
                              args[2]->toBoolean(exec), // cancelableArg
                              toAbstractView(args[3]), // viewArg
                              args[4]->toString(exec).domString(), // keyIdentifier
                              args[5]->toInt32(exec), // keyLocationArg
                              args[6]->toBoolean(exec), // ctrlKeyArg
                              args[7]->toBoolean(exec), // altKeyArg
                              args[8]->toBoolean(exec), // shiftKeyArg
                              args[9]->toBoolean(exec), // metaKeyArg
                              args[10]->toBoolean(exec)); // altGraphKeyArg
      return jsUndefined();
  }
  return jsUndefined();
}

// -------------------------------------------------------------------------

const ClassInfo Clipboard::info = { "Clipboard", 0, &ClipboardTable, 0 };

/* Source for ClipboardTable. Use "make hashtables" to regenerate.
@begin ClipboardTable 3
  dropEffect    Clipboard::DropEffect   DontDelete
  effectAllowed Clipboard::EffectAllowed        DontDelete
  types         Clipboard::Types        DontDelete|ReadOnly
@end
@begin ClipboardProtoTable 4
  clearData     Clipboard::ClearData    DontDelete|Function 0
  getData       Clipboard::GetData      DontDelete|Function 1
  setData       Clipboard::SetData      DontDelete|Function 2
  setDragImage  Clipboard::SetDragImage DontDelete|Function 3
@end
*/

KJS_DEFINE_PROTOTYPE(ClipboardProto)
KJS_IMPLEMENT_PROTOFUNC(ClipboardProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("Clipboard", ClipboardProto, ClipboardProtoFunc)

Clipboard::Clipboard(ExecState *exec, WebCore::Clipboard *cb)
  : clipboard(cb)
{
    setPrototype(ClipboardProto::self(exec));
}

bool Clipboard::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
    return getStaticValueSlot<Clipboard, DOMObject>(exec, &ClipboardTable, this, propertyName, slot);
}

JSValue *Clipboard::getValueProperty(ExecState *exec, int token) const
{
    switch (token) {
        case DropEffect:
            assert(clipboard->isForDragging() || clipboard->dropEffect().isNull());
            return jsStringOrUndefined(clipboard->dropEffect());
        case EffectAllowed:
            assert(clipboard->isForDragging() || clipboard->effectAllowed().isNull());
            return jsStringOrUndefined(clipboard->effectAllowed());
        case Types:
        {
            DeprecatedStringList qTypes = clipboard->types();
            if (qTypes.isEmpty()) {
                return jsNull(); 
            } else {
                List list;
                for (DeprecatedStringList::Iterator it = qTypes.begin(); it != qTypes.end(); ++it) {
                    list.append(jsString(UString(*it)));
                }
                return exec->lexicalInterpreter()->builtinArray()->construct(exec, list);
            }
        }
        default:
            return NULL;
    }
}

void Clipboard::put(ExecState *exec, const Identifier &propertyName, JSValue *value, int attr)
{
    lookupPut<Clipboard,DOMObject>(exec, propertyName, value, attr, &ClipboardTable, this );
}

void Clipboard::putValueProperty(ExecState *exec, int token, JSValue *value, int /*attr*/)
{
    switch (token) {
        case DropEffect:
            // can never set this when not for dragging, thus getting always returns NULL string
            if (clipboard->isForDragging())
                clipboard->setDropEffect(value->toString(exec).domString());
            break;
        case EffectAllowed:
            // can never set this when not for dragging, thus getting always returns NULL string
            if (clipboard->isForDragging())
                clipboard->setEffectAllowed(value->toString(exec).domString());
            break;
    }
}

JSValue *ClipboardProtoFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
    if (!thisObj->inherits(&Clipboard::info))
        return throwError(exec, TypeError);

    Clipboard *cb = static_cast<Clipboard *>(thisObj);
    switch (id) {
        case Clipboard::ClearData:
            if (args.size() == 0) {
                cb->clipboard->clearAllData();
                return jsUndefined();
            } else if (args.size() == 1) {
                cb->clipboard->clearData(args[0]->toString(exec).domString());
                return jsUndefined();
            } else {
                return throwError(exec, SyntaxError, "clearData: Invalid number of arguments");
            }
        case Clipboard::GetData:
        {
            if (args.size() == 1) {
                bool success;
                WebCore::String result = cb->clipboard->getData(args[0]->toString(exec).domString(), success);
                if (success) {
                    return jsString(result);
                } else {
                    return jsUndefined();
                }
            } else {
                return throwError(exec, SyntaxError, "getData: Invalid number of arguments");
            }
        }
        case Clipboard::SetData:
            if (args.size() == 2) {
                return jsBoolean(cb->clipboard->setData(args[0]->toString(exec).domString(), args[1]->toString(exec).domString()));
            } else {
                return throwError(exec, SyntaxError, "setData: Invalid number of arguments");
            }
        case Clipboard::SetDragImage:
        {
            if (!cb->clipboard->isForDragging()) {
                return jsUndefined();
            }

            if (args.size() != 3)
                return throwError(exec, SyntaxError, "setDragImage: Invalid number of arguments");

            int x = (int)args[1]->toNumber(exec);
            int y = (int)args[2]->toNumber(exec);

            // See if they passed us a node
            Node *node = toNode(args[0]);
            if (!node)
                return throwError(exec, TypeError);
            
            if (!node->isElementNode())
                return throwError(exec, SyntaxError, "setDragImageFromElement: Invalid first argument");

            if (static_cast<Element*>(node)->hasLocalName(imgTag) && 
                !node->inDocument())
                cb->clipboard->setDragImage(static_cast<HTMLImageElement*>(node)->cachedImage(), IntPoint(x, y));
            else
                cb->clipboard->setDragImageElement(node, IntPoint(x, y));                    
                
            return jsUndefined();
        }
    }
    return jsUndefined();
}

}
