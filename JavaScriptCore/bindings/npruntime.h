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
 *
 * Revision 1 (March 4, 2004):
 * Initial proposal.
 *
 * Revision 2 (March 10, 2004):
 * All calls into JavaScript were made asynchronous.  Results are
 * provided via the NP_JavaScriptResultInterface callback.
 *
 * Revision 3 (March 10, 2004):
 * Corrected comments to not refer to class retain/release interfaces.
 *
 * Revision 4 (March 11, 2004):
 * Added additional convenience NP_SetExceptionWithUTF8().
 * Changed NP_HasPropertyInterface and NP_HasMethodInterface to take NP_Class
 * pointers instead of objects.
 * Added NP_IsValidIdentifier().
 *
 */
#ifndef _NP_RUNTIME_H_
#define _NP_RUNTIME_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
    This API is used to facilitate binding code written in 'C' to JavaScript
    objects.  In particular it is used to support the extended Netscape 
    script-ability API for plugins (NP-SAP).  NP-SAP is an extension of the 
    Netscape plugin API.  As such we have adopted the use of the "NP_" prefix
    for this API.  
    
    The following NP-SAP entry points were added to the Netscape plugin API:

    typedef NP_Object *(*NPP_GetNativeObjectForJavaScript) (NPP instance);
    typedef NP_JavaScriptObject *(*NPN_GetWindowJavaScriptObject) (NPP instance);
    typedef NP_JavaScriptObject *(*NPN_GetInstanceJavaScriptObject) (NPP instance);
    
    See NP_SAP.h for more details regarding the above Interfaces.
*/


/*
    Data passed between 'C' and JavaScript is always wrapped in an NP_Object.
    The interface on an NP_Object is described by an NP_Class.
*/
typedef struct NP_Object NP_Object;
typedef struct NP_Class NP_Class;

/*
    A NP_JavaScriptObject wraps a JavaScript Object in an NP_Object.
*/
typedef NP_Object NP_JavaScriptObject;

/*
	Type mappings:

	JavaScript		to			C
	Boolean						NP_Boolean	
	Number						NP_Number
	String						NP_String
	Undefined					NP_Undefined
	Null						NP_Null
	Object (including Array)	NP_JavaScriptObject
	Object wrapper              NP_Object


	C				to			JavaScript
	NP_Boolean					Boolean	
	NP_Number					Number
	NP_String					String
	NP_Undefined				Undefined
	NP_Null						Null
	NP_Array					Array (restricted)
	NP_JavaScriptObject			Object
	other NP_Object             Object wrapper

*/

typedef uint32_t NP_Identifier;
typedef char NP_UTF8;
typedef uint16_t NP_UTF16;

/*
    NP_Objects have methods and properties.  Methods and properties are named with NP_Identifiers.
    These identifiers may be reflected in JavaScript.  NP_Identifiers can be compared using ==.
    
    NP_IsValidIdentifier will return true if an identifier for the name has already been
    assigned with either NP_IdentifierFromUTF8() or NP_GetIdentifiers();
*/
NP_Identifier NP_IdentifierFromUTF8 (const NP_UTF8 *name);
bool NP_IsValidIdentifier (const NP_UTF8 *name);
void NP_GetIdentifiers (const NP_UTF8 **names, int nameCount, NP_Identifier *identifiers);

/*
    The returned NP_UTF8 should NOT be freed.
*/
const NP_UTF8 *NP_UTF8FromIdentifier (NP_Identifier identifier);

/*
    NP_Object behavior is implemented using the following set of callback interfaces.
*/
typedef NP_Object *(*NP_AllocateInterface)();
typedef void (*NP_DeallocateInterface)(NP_Object *obj);
typedef void (*NP_InvalidateInterface)();
typedef bool (*NP_HasMethodInterface)(NP_Class *theClass, NP_Identifier name);
typedef NP_Object *(*NP_InvokeInterface)(NP_Object *obj, NP_Identifier name, NP_Object **args, unsigned argCount);
typedef bool (*NP_HasPropertyInterface)(NP_Class *theClass, NP_Identifier name);
typedef NP_Object *(*NP_GetPropertyInterface)(NP_Object *obj, NP_Identifier name);
typedef void (*NP_SetPropertyInterface)(NP_Object *obj, NP_Identifier name, NP_Object *value);

/*
    NP_Objects returned by create, retain, invoke, and getProperty 
    pass a reference count to the caller.  That is, the callee adds a reference
    count which passes to the caller.  It is the caller's responsibility
    to release the returned object.

    NP_InvokeInterface Interfaces may return 0 to indicate a void result.
    
    NP_InvalidateInterface is called by the plugin container when the plugin is
    shutdown.  Any attempt to message a NP_JavaScriptObject instance after the invalidate
    interface has been called will result in undefined behavior, even if the
    plugin is still retaining those NP_JavaScriptObject instances.
    (The runtime will typically return immediately, with 0 or NULL, from an attempt to
    dispatch to a NP_JavaScriptObject, but this behavior should not be depended upon.)
*/
struct NP_Class
{
    int32_t structVersion;
    NP_AllocateInterface allocate;
    NP_DeallocateInterface deallocate;
    NP_InvalidateInterface invalidate;
    NP_HasMethodInterface hasMethod;
    NP_InvokeInterface invoke;
    NP_HasPropertyInterface hasProperty;
    NP_GetPropertyInterface getProperty;
    NP_SetPropertyInterface setProperty;
};
typedef struct NP_Class NP_Class;

#define kNP_ClassStructVersion1 1
#define kNP_ClassStructVersionCurrent kNP_ClassStructVersion1

struct NP_Object {
    NP_Class *_class;
    uint32_t referenceCount;
    // Additional space may be allocated here by types of NP_Objects
};

/*
    If the class has a create interface this function invokes that interface,
    otherwise a NP_Object is allocated and returned.
*/
NP_Object *NP_CreateObject (NP_Class *aClass);

/*
    Increment the NP_Object's reference count.
*/
NP_Object *NP_RetainObject (NP_Object *obj);

/*
    Decremented the NP_Object's reference count.  If the reference
    count goes to zero, the class's destroy interface is invoke if
    specified, otherwise the object is free()ed directly.
*/
void NP_ReleaseObject (NP_Object *obj);

/*
    Built-in data types.  These classes can be passed to NP_IsKindOfClass().
*/
extern NP_Class *NP_BooleanClass;
extern NP_Class *NP_NullClass;
extern NP_Class *NP_UndefinedClass;
extern NP_Class *NP_ArrayClass;
extern NP_Class *NP_NumberClass;
extern NP_Class *NP_StringClass;
extern NP_Class *NP_JavaScriptObjectClass;

typedef NP_Object NP_Boolean;
typedef NP_Object NP_Null;
typedef NP_Object NP_Undefined;
typedef NP_Object NP_Array;
typedef NP_Object NP_Number;
typedef NP_Object NP_String;

/*
    Functions to access JavaScript Objects represented by NP_JavaScriptObject.
    
    Calls to JavaScript objects are asynchronous.  If a function returns a value, it
    will be supplied via the NP_JavaScriptResultInterface callback.
    
    Calls made from plugin code to JavaScript may be made from any thread.
    
    Calls made from JavaScript to the plugin will always be made on the main
    user agent thread, this include calls to NP_JavaScriptResultInterface callbacks.
*/
typedef void (*NP_JavaScriptResultInterface)(NP_Object *obj);

void NP_Call (NP_JavaScriptObject *obj, NP_Identifier methodName, NP_Object **args, unsigned argCount, NP_JavaScriptResultInterface result);
void NP_Evaluate (NP_JavaScriptObject *obj, NP_String *script, NP_JavaScriptResultInterface result);
void NP_GetProperty (NP_JavaScriptObject *obj, NP_Identifier  propertyName, NP_JavaScriptResultInterface result);
void NP_SetProperty (NP_JavaScriptObject *obj, NP_Identifier  propertyName, NP_Object *value);
void NP_RemoveProperty (NP_JavaScriptObject *obj, NP_Identifier propertyName);
void NP_ToString (NP_JavaScriptObject *obj, NP_JavaScriptResultInterface result);
void NP_GetPropertyAtIndex (NP_JavaScriptObject *obj, int32_t index, NP_JavaScriptResultInterface result);
void NP_SetPropertyAtIndex (NP_JavaScriptObject *obj, unsigned index, NP_Object *value);

/*
    Functions for dealing with data types.
*/
NP_Number *NP_CreateNumberWithInt (int i);
NP_Number *NP_CreateNumberWithFloat (float f);
NP_Number *NP_CreateNumberWithDouble (double d);
int NP_IntFromNumber (NP_Number *obj);
float NP_FloatFromNumber (NP_Number *obj);
double NP_DoubleFromNumber (NP_Number *obj);

NP_String *NP_CreateStringWithUTF8 (const NP_UTF8 *utf8String);
NP_String *NP_CreateStringWithUTF16 (const NP_UTF16 *utf16String, unsigned int len);

/*
    Memory returned from NP_UTF8FromString and NP_UTF16FromString must be freed by the caller.
*/
NP_UTF8 *NP_UTF8FromString (NP_String *obj);
NP_UTF16 *NP_UTF16FromString (NP_String *obj);
int32_t NP_StringLength (NP_String *obj);

NP_Boolean *NP_CreateBoolean (bool f);
bool NP_BoolFromBoolean (NP_Boolean *aBool);

/*
    NP_Null returns a NP_Null singleton.
*/
NP_Null *NP_GetNull();

/*
    NP_Undefined returns a NP_Undefined singleton.
*/
NP_Undefined *NP_GetUndefined();

/*
    NP_Arrays are immutable.  They are used to pass arguments to 
    JavaScription Interfaces that expect arrays, or to export 
    arrays of properties.  NP_Array is represented in JavaScript
    by a restricted Array.  The Array in JavaScript is read-only,
    only has index accessors, and may not be resized.
    
    Objects added to arrays are retained by the array.
*/
NP_Array *NP_CreateArray (NP_Object **, int32_t count);
NP_Array *NP_CreateArrayV (int32_t count, ...);

/*
    Objects returned by NP_ObjectAtIndex pass a reference count
    to the caller.  The caller must release the object.
*/
NP_Object *NP_ObjectAtIndex (NP_Array *array, int32_t index);

/*
    Returns true if the object is a kind of class as specified by
    aClass.
*/
bool NP_IsKindOfClass (NP_Object *obj, NP_Class *aClass);

/*
    NP_SetException may be called to trigger a JavaScript exception upon return
    from entry points into NP_Objects.  A reference count of the message passes
    to the callee.  Typical usage:

    NP_String *message = NP_CreateStringWithUTF8("invalid type");
    NP_SetException (obj, mesage);
    NP_ReleaseObject (message);
*/
void NP_SetExceptionWithUTF8 (NP_Object *obj, const NP_UTF8 *message);
void NP_SetException (NP_Object *obj, NP_String *message);

/*
    Example usage:
    
    typedef NP_Object MyInterfaceObject;

    typedef struct
    {
        NP_Object object;
        // Properties needed by MyInterfaceObject are added here.
        int numChapters;
        ...
    } MyInterfaceObject

    void stop(MyInterfaceObject *obj)
    {
        ...
    }

    void start(MyInterfaceObject *obj)
    {
        ...
    }

    void setChapter(MyInterfaceObject *obj, int chapter)
    {
        ...
    }

    int getChapter(MyInterfaceObject *obj)
    {
        ...
    }

    static NP_Identifier stopIdentifier;
    static NP_Identifier startIdentifier;
    static NP_Identifier setChapterIdentifier;
    static NP_Identifier getChapterIdentifier;
    static NP_Identifier numChaptersIdentifier;

    static void initializeIdentifiers()
    {
        stopIdentifier = NP_IdentifierFromUTF8 ("stop");
        startIdentifier = NP_IdentifierFromUTF8 ("start");
        setChapterIdentifier = NP_IdentifierFromUTF8 ("setChapter");
        getChapterIdentifier = NP_IdentifierFromUTF8 ("getChapter");
        numChaptersIdentifier = NP_IdentifierFromUTF8 ("numChapters");
    }

    bool myInterfaceHasProperty (MyInterfaceObject *obj, NP_Identifier name)
    {
        if (name == numChaptersIdentifier){
            return true;
        }
        return false;
    }

    bool myInterfaceHasMethod (MyInterfaceObject *obj, NP_Identifier name)
    {
        if (name == stopIdentifier ||
            name == startIdentifier ||
            name == setChapterIdentifier ||
            name == getChapterIdentifier) {
            return true;
        }
        return false;
    }

    NP_Object *myInterfaceGetProperty (MyInterfaceObject *obj, NP_Identifier name)
    {
        if (name == numChaptersIdentifier){
            return NP_CreateNumberWithInt(obj->numChapters); 
        }
        return 0;
    }

    void myInterfaceSetProperty (MyInterfaceObject *obj, NP_Identifier name, NP_Object *value)
    {
        if (name == numChaptersIdentifier){
            obj->numChapters = NP_IntFromNumber(obj)
        }
    }

    NP_Object *myInterfaceInvoke (MyInterfaceObject *obj, NP_Identifier name, NP_Object **args, unsigned argCount)
    {

        if (name == stopIdentifier){
            stop(obj);
        }
        else if (name == startIdentifier){
            start(obj);
        }
        else if (name == setChapterIdentifier){
            if (NP_IsKindOfClass (args[0], NP_NumberClass)) {
                setChapter (obj, NP_IntFromNumber(args[0]));
            }
            else {
                NP_SetException (obj, NP_CreateStringWithUTF8 ("invalid type"));
            }
        }
        else if (name == getChapterIdentifier){
            return NP_CreateNumberWithInt (getChapter (obj));
        }
        return 0;
    }

    NP_Object *myInterfaceAllocate ()
    {
        MyInterfaceObject *newInstance = (MyInterfaceObject *)malloc (sizeof(MyInterfaceObject));
        
        if (stopIdentifier == 0)
            initializeIdentifiers();
            
        return (NP_Object *)newInstance;
    }

    void myInterfaceInvalidate ()
    {
        // Make sure we've released any remainging references to JavaScript
        // objects.
    }
    
    void myInterfaceDeallocate (MyInterfaceObject *obj) 
    {
        free ((void *)obj);
    }
    
    static NP_Class _myInterface = { 
        (NP_AllocateInterface) myInterfaceAllocate, 
        (NP_DeallocateInterface) myInterfaceDeallocate, 
        (NP_InvalidateInterface) myInterfaceInvalidate,
        (NP_HasMethodInterface) myInterfaceHasMethod,
        (NP_InvokeInterface) myInterfaceInvoke,
        (NP_HasPropertyInterface) myInterfaceHasProperty,
        (NP_GetPropertyInterface) myInterfaceGetProperty,
        (NP_SetPropertyInterface) myInterfaceSetProperty,
    };
    static NP_Class *myInterface = &_myInterface;

    // myGetNativeObjectForJavaScript would be set as the entry point for
    // the plugin's NPP_GetNativeObjectForJavaScript function.
    // It is invoked by the plugin container, i.e. the browser.
    NP_Object *myGetNativeObjectForJavaScript(NPP_Instance instance)
    {
        NP_Object *myInterfaceObject = NP_CreateObject (myInterface);
        return myInterfaceObject;
    }

*/


#ifdef __cplusplus
}
#endif

#endif
