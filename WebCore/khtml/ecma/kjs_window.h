// -*- c-basic-offset: 2 -*-
/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003 Apple Computer, Inc.
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

#ifndef _KJS_WINDOW_H_
#define _KJS_WINDOW_H_

#include <qobject.h>
#include <qguardedptr.h>
#include <qmap.h>
#include <qptrlist.h>

#include "kjs_binding.h"

class QTimer;
class KHTMLView;
class KHTMLPart;

namespace KJS {

  class WindowFunc;
  class WindowQObject;
  class Location;
  class History;
  class FrameArray;
  class JSEventListener;
  class JSLazyEventListener;

  class Screen : public ObjectImp {
  public:
    Screen(ExecState *exec);
    enum {
      Height, Width, ColorDepth, PixelDepth, AvailLeft, AvailTop, AvailHeight,
      AvailWidth
    };
    virtual Value get(ExecState *exec, const Identifier &propertyName) const;
    Value getValueProperty(ExecState *exec, int token) const;
  private:
    KHTMLView *view;
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
  };

  class Window : public ObjectImp {
    friend QGuardedPtr<KHTMLPart> getInstance();
    friend class Location;
    friend class WindowFunc;
    friend class WindowQObject;
    friend class ScheduledAction;
  public:
    Window(KHTMLPart *p);
  public:
    ~Window();
    /**
     * Returns and registers a window object. In case there's already a Window
     * for the specified part p this will be returned in order to have unique
     * bindings.
     */
    static Value retrieve(KHTMLPart *p);
    /**
     * Returns the Window object for a given HTML part
     */
    static Window *retrieveWindow(KHTMLPart *p);
    /**
     * returns a pointer to the Window object this javascript interpreting instance
     * was called from.
     */
    static Window *retrieveActive(ExecState *exec);
    QGuardedPtr<KHTMLPart> part() const { return m_part; }
    virtual void mark();
    virtual bool hasProperty(ExecState *exec, const Identifier &p) const;
    virtual Value get(ExecState *exec, const Identifier &propertyName) const;
    virtual void put(ExecState *exec, const Identifier &propertyName, const Value &value, int attr = None);
    virtual bool toBoolean(ExecState *exec) const;
    int installTimeout(const UString &handler, int t, bool singleShot);
    int installTimeout(const Value &function, List &args, int t, bool singleShot);
    void clearTimeout(int timerId);
#ifdef APPLE_CHANGES
    bool hasTimeouts();
    QMap<int, ScheduledAction*> *pauseTimeouts(const void *key);
    void resumeTimeouts(QMap<int, ScheduledAction*>*sa, const void *key);
#endif
    void scheduleClose();
    bool isSafeScript(ExecState *exec) const;
    Location *location() const;
    JSEventListener *getJSEventListener(const Value &val, bool html = false);
    JSLazyEventListener *getJSLazyEventListener(const QString &code, bool html = false);
    void clear( ExecState *exec );
    virtual UString toString(ExecState *exec) const;

    // Set the current "event" object
    void setCurrentEvent( DOM::Event *evt );

    QPtrDict<JSEventListener> jsEventListeners;
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { Closed, Crypto, DefaultStatus, Status, Document, Node, EventCtor, Range,
           NodeFilter, DOMException, CSSRule, Frames, _History, Event, InnerHeight,
           InnerWidth, Length, _Location, Name, _Navigator, _Konqueror, ClientInformation,
           OffscreenBuffering, Opener, OuterHeight, OuterWidth, PageXOffset, PageYOffset,
           Parent, Personalbar, ScreenX, ScreenY, Scrollbars, Scroll, ScrollBy,
           ScreenTop, ScreenLeft,
           ScrollTo, ScrollX, ScrollY, MoveBy, MoveTo, ResizeBy, ResizeTo, Self, _Window, Top, _Screen,
           Image, Option, Alert, Confirm, Prompt, Open, Print, SetTimeout, ClearTimeout,
           Focus, GetSelection, Blur, Close, SetInterval, ClearInterval, CaptureEvents, 
           ReleaseEvents, AddEventListener, RemoveEventListener, XMLHttpRequest, XMLSerializer,
	   Onabort, Onblur, Onchange, Onclick, Ondblclick, Ondragdrop, Onerror, 
	   Onfocus, Onkeydown, Onkeypress, Onkeyup, Onload, Onmousedown, Onmousemove,
           Onmouseout, Onmouseover, Onmouseup, Onmove, Onreset, Onresize, Onsearch,
           Onselect, Onsubmit, Onunload };
  protected:
    Value getListener(ExecState *exec, int eventId) const;
    void setListener(ExecState *exec, int eventId, Value func);
  private:
    void updateLayout() const;

    QGuardedPtr<KHTMLPart> m_part;
    Screen *screen;
    History *history;
    FrameArray *frames;
    Location *loc;
    WindowQObject *winq;
    DOM::Event *m_evt;
  };

  /**
   * An action (either function or string) to be executed after a specified
   * time interval, either once or repeatedly. Used for window.setTimeout()
   * and window.setInterval()
   */
  class ScheduledAction {
  public:
    ScheduledAction(Object _func, List _args, bool _singleShot);
    ScheduledAction(const QString &_code, bool _singleShot);
    ~ScheduledAction();
    void execute(Window *window);

    Object func;
    List args;
    QString code;
    bool isFunction;
    bool singleShot;
  };

  class WindowQObject : public QObject {
    Q_OBJECT
  public:
    WindowQObject(Window *w);
    ~WindowQObject();
    int installTimeout(const UString &handler, int t, bool singleShot);
    int installTimeout(const Value &func, List args, int t, bool singleShot);
    void clearTimeout(int timerId, bool delAction = true);
#ifdef APPLE_CHANGES
    bool hasTimeouts();
    QMap<int, ScheduledAction*> *WindowQObject::pauseTimeouts(const void *key);
    void WindowQObject::resumeTimeouts(QMap<int, ScheduledAction*> *sa, const void *key);
#endif

  public slots:
    void timeoutClose();
  protected slots:
    void parentDestroyed();
  protected:
    void timerEvent(QTimerEvent *e);
  private:
    Window *parent;
    KHTMLPart *part;   		// not guarded, may be dangling
    QMap<int, ScheduledAction*> scheduledActions;
  };

  class Location : public ObjectImp {
  public:
    ~Location();
    virtual Value get(ExecState *exec, const Identifier &propertyName) const;
    virtual void put(ExecState *exec, const Identifier &propertyName, const Value &value, int attr = None);
    virtual Value toPrimitive(ExecState *exec, Type preferred) const;
    virtual UString toString(ExecState *exec) const;
    enum { Hash, Href, Hostname, Host, Pathname, Port, Protocol, Search, EqualEqual,
           Replace, Reload, ToString };
    KHTMLPart *part() const { return m_part; }
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
  private:
    friend class Window;
    Location(KHTMLPart *p);
    QGuardedPtr<KHTMLPart> m_part;
  };

#ifdef Q_WS_QWS
  class Konqueror : public ObjectImp {
    friend class KonquerorFunc;
  public:
    Konqueror(KHTMLPart *p) : part(p) { }
    virtual Value get(ExecState *exec, const Identifier &propertyName) const;
    virtual bool hasProperty(ExecState *exec, const Identifier &p) const;
    virtual UString toString(ExecState *exec) const;
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
  private:
    KHTMLPart *part;
  };
#endif

}; // namespace

#endif
