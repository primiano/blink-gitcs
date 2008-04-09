/*
 *  Copyright (C) 2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reseved.
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

#ifndef JSDOMWindowBase_h
#define JSDOMWindowBase_h

#include "PlatformString.h"
#include "SecurityOrigin.h"
#include "kjs_binding.h"
#include <kjs/protect.h>
#include <wtf/HashMap.h>
#include <wtf/OwnPtr.h>

namespace WebCore {

    class AtomicString;
    class DOMWindow;
    class DOMWindowTimer;
    class Frame;
    class JSDOMWindow;
    class JSDOMWindowWrapper;
    class JSEventListener;
    class JSLocation;
    class JSUnprotectedEventListener;
    class PausedTimeouts;
    class ScheduledAction;

    class JSDOMWindowBasePrivate;

    // This is the only WebCore JS binding which does not inherit from DOMObject
    class JSDOMWindowBase : public KJS::JSGlobalObject {
        typedef KJS::JSGlobalObject Base;

        friend class ScheduledAction;
    protected:
        JSDOMWindowBase(KJS::JSObject* prototype, DOMWindow*);

    public:
        virtual ~JSDOMWindowBase();

        DOMWindow* impl() const { return m_impl.get(); }

        void disconnectFrame();

        virtual bool getOwnPropertySlot(KJS::ExecState*, const KJS::Identifier&, KJS::PropertySlot&);
        KJS::JSValue* getValueProperty(KJS::ExecState*, int token) const;
        virtual void put(KJS::ExecState*, const KJS::Identifier& propertyName, KJS::JSValue*);

        int installTimeout(const KJS::UString& handler, int t, bool singleShot);
        int installTimeout(KJS::JSValue* function, const KJS::List& args, int t, bool singleShot);
        void clearTimeout(int timerId, bool delAction = true);
        PausedTimeouts* pauseTimeouts();
        void resumeTimeouts(PausedTimeouts*);

        void timerFired(DOMWindowTimer*);

        // Finds a wrapper of a JS EventListener, returns 0 if no existing one.
        JSEventListener* findJSEventListener(KJS::JSValue*, bool html = false);

        // Finds or creates a wrapper of a JS EventListener. JS EventListener object is GC-protected.
        JSEventListener* findOrCreateJSEventListener(KJS::JSValue*, bool html = false);

        // Finds a wrapper of a GC-unprotected JS EventListener, returns 0 if no existing one.
        JSUnprotectedEventListener* findJSUnprotectedEventListener(KJS::JSValue*, bool html = false);

        // Finds or creates a wrapper of a JS EventListener. JS EventListener object is *NOT* GC-protected.
        JSUnprotectedEventListener* findOrCreateJSUnprotectedEventListener(KJS::JSValue*, bool html = false);

        void clear();

        void setCurrentEvent(Event*);
        Event* currentEvent();

        // Set a place to put a dialog return value when the window is cleared.
        void setReturnValueSlot(KJS::JSValue** slot);

        typedef HashMap<KJS::JSObject*, JSEventListener*> ListenersMap;
        typedef HashMap<KJS::JSObject*, JSUnprotectedEventListener*> UnprotectedListenersMap;

        ListenersMap& jsEventListeners();
        ListenersMap& jsHTMLEventListeners();
        UnprotectedListenersMap& jsUnprotectedEventListeners();
        UnprotectedListenersMap& jsUnprotectedHTMLEventListeners();

        virtual const KJS::ClassInfo* classInfo() const { return &s_info; }
        static const KJS::ClassInfo s_info;

        virtual KJS::ExecState* globalExec();

        virtual bool shouldInterruptScript() const;

        bool allowsAccessFrom(KJS::ExecState*) const;
        bool allowsAccessFromNoErrorMessage(KJS::ExecState*) const;
        bool allowsAccessFrom(KJS::ExecState*, String& message) const;

        void printErrorMessage(const String&) const;

        // Don't call this version of allowsAccessFrom -- it's a slightly incorrect implementation used only by WebScriptObject
        virtual bool allowsAccessFrom(const KJS::JSGlobalObject*) const;

        virtual KJS::JSObject* toThisObject(KJS::ExecState*) const;
        JSDOMWindowWrapper* wrapper() const;

        enum {
            // Attributes
            Crypto, Event_, Location_, Navigator_,
            ClientInformation,

            // Event Listeners
            Onabort, Onblur, Onchange, Onclick,
            Ondblclick, Onerror, Onfocus, Onkeydown,
            Onkeypress, Onkeyup, Onload, Onmousedown,
            Onmousemove, Onmouseout, Onmouseover, Onmouseup,
            OnWindowMouseWheel, Onreset, Onresize, Onscroll,
            Onsearch, Onselect, Onsubmit, Onunload,
            Onbeforeunload,

            // Constructors
            DOMException, Audio, Image, Option, XMLHttpRequest,
            XSLTProcessor_
        };

    private:
        KJS::JSValue* getListener(KJS::ExecState*, const AtomicString& eventType) const;
        void setListener(KJS::ExecState*, const AtomicString& eventType, KJS::JSValue* function);

        static KJS::JSValue* childFrameGetter(KJS::ExecState*, KJS::JSObject*, const KJS::Identifier&, const KJS::PropertySlot&);
        static KJS::JSValue* indexGetter(KJS::ExecState*, KJS::JSObject*, const KJS::Identifier&, const KJS::PropertySlot&);
        static KJS::JSValue* namedItemGetter(KJS::ExecState*, KJS::JSObject*, const KJS::Identifier&, const KJS::PropertySlot&);

        void clearHelperObjectProperties();
        void clearAllTimeouts();
        int installTimeout(ScheduledAction*, int interval, bool singleShot);

        bool allowsAccessFromPrivate(const KJS::JSGlobalObject*, SecurityOrigin::Reason&) const;
        bool allowsAccessFromPrivate(const KJS::ExecState*, SecurityOrigin::Reason&) const;
        String crossDomainAccessErrorMessage(const KJS::JSGlobalObject*, SecurityOrigin::Reason) const;

        RefPtr<DOMWindow> m_impl;
        OwnPtr<JSDOMWindowBasePrivate> d;
    };

    // Functions
    KJS::JSValue* windowProtoFuncAToB(KJS::ExecState*, KJS::JSObject*, const KJS::List&);
    KJS::JSValue* windowProtoFuncBToA(KJS::ExecState*, KJS::JSObject*, const KJS::List&);
    KJS::JSValue* windowProtoFuncOpen(KJS::ExecState*, KJS::JSObject*, const KJS::List&);
    KJS::JSValue* windowProtoFuncSetTimeout(KJS::ExecState*, KJS::JSObject*, const KJS::List&);
    KJS::JSValue* windowProtoFuncClearTimeout(KJS::ExecState*, KJS::JSObject*, const KJS::List&);
    KJS::JSValue* windowProtoFuncSetInterval(KJS::ExecState*, KJS::JSObject*, const KJS::List&);
    KJS::JSValue* windowProtoFuncAddEventListener(KJS::ExecState*, KJS::JSObject*, const KJS::List&);
    KJS::JSValue* windowProtoFuncRemoveEventListener(KJS::ExecState*, KJS::JSObject*, const KJS::List&);
    KJS::JSValue* windowProtoFuncShowModalDialog(KJS::ExecState*, KJS::JSObject*, const KJS::List&);
    KJS::JSValue* windowProtoFuncNotImplemented(KJS::ExecState*, KJS::JSObject*, const KJS::List&);

    // Returns a JSDOMWindow or jsNull()
    KJS::JSValue* toJS(KJS::ExecState*, DOMWindow*);

    // Returns JSDOMWindow or 0
    JSDOMWindow* toJSDOMWindow(Frame*);
    JSDOMWindow* toJSDOMWindow(KJS::JSGlobalObject*);

} // namespace WebCore

#endif // JSDOMWindowBase_h
