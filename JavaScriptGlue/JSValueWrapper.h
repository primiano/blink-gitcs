#ifndef JSValueWrapper_h
#define JSValueWrapper_h

/*
    JSValueWrapper.h
*/

#include "JSUtils.h"
#include "JSBase.h"
#include "JSObject.h"

class JSValueWrapper {
public:
    JSValueWrapper(JSValue *inValue, ExecState *inExec);
    virtual ~JSValueWrapper();

    JSValue *GetValue();
    ExecState *GetExecState() const;

    ProtectedPtr<JSValue> fValue;
    ExecState *fExec;

    static void GetJSObectCallBacks(JSObjectCallBacks& callBacks);

private:
    static void JSObjectDispose(void *data);
    static CFArrayRef JSObjectCopyPropertyNames(void *data);
    static JSObjectRef JSObjectCopyProperty(void *data, CFStringRef propertyName);
    static void JSObjectSetProperty(void *data, CFStringRef propertyName, JSObjectRef jsValue);
    static JSObjectRef JSObjectCallFunction(void *data, JSObjectRef thisObj, CFArrayRef args);
    static CFTypeRef JSObjectCopyCFValue(void *data);
    static void JSObjectMark(void *data);
};

#endif
