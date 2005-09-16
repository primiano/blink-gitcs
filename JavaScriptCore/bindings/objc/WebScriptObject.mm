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

#import "WebScriptObjectPrivate.h"

#import "internal.h"
#import "list.h"
#import "value.h"

#import "objc_jsobject.h"
#import "objc_instance.h"
#import "objc_utility.h"

#import "runtime_object.h"
#import "runtime_root.h"

#if MAC_OS_X_VERSION_MAX_ALLOWED <= MAC_OS_X_VERSION_10_3

@interface NSObject (WebExtras)
- (void)finalize;
@end

#endif

using namespace KJS;
using namespace KJS::Bindings;

#define LOG_EXCEPTION(exec) \
    if (Interpreter::shouldPrintExceptions()) \
        printf("%s:%d:[%d]  JavaScript exception:  %s\n", __FILE__, __LINE__, getpid(), exec->exception()->toObject(exec)->get(exec, messagePropertyName)->toString(exec).ascii());

@implementation WebScriptObjectPrivate

@end

@implementation WebScriptObject

static void _didExecute(WebScriptObject *obj)
{
    ExecState *exec = [obj _executionContext]->interpreter()->globalExec();
    KJSDidExecuteFunctionPtr func = Instance::didExecuteFunction();
    if (func)
        func (exec, static_cast<ObjectImp*>([obj _executionContext]->rootObjectImp()));
}

- (void)_initializeWithObjectImp:(ObjectImp *)imp originExecutionContext:(const RootObject *)originExecutionContext executionContext:(const RootObject *)executionContext
{
    _private->imp = imp;
    _private->executionContext = executionContext;    
    _private->originExecutionContext = originExecutionContext;    

    addNativeReference (executionContext, imp);
}

- _initWithObjectImp:(ObjectImp *)imp originExecutionContext:(const RootObject *)originExecutionContext executionContext:(const RootObject *)executionContext
{
    assert (imp != 0);
    //assert (root != 0);

    self = [super init];

    _private = [[WebScriptObjectPrivate alloc] init];

    [self _initializeWithObjectImp:imp originExecutionContext:originExecutionContext executionContext:executionContext];
    
    return self;
}

- (ObjectImp *)_imp
{
    if (!_private->imp && _private->isCreatedByDOMWrapper) {
        // Associate the WebScriptObject with the JS wrapper for the ObjC DOM
        // wrapper.  This is done on lazily, on demand.
        [self _initializeScriptDOMNodeImp];
    }
    return _private->imp;
}

- (const RootObject *)_executionContext
{
    return _private->executionContext;
}

- (void)_setExecutionContext:(const RootObject *)context
{
    _private->executionContext = context;
}


- (const RootObject *)_originExecutionContext
{
    return _private->originExecutionContext;
}

- (void)_setOriginExecutionContext:(const RootObject *)originExecutionContext
{
    _private->originExecutionContext = originExecutionContext;
}

- (BOOL)_isSafeScript
{
    if ([self _originExecutionContext]) {
	Interpreter *originInterpreter = [self _originExecutionContext]->interpreter();
	if (originInterpreter) {
	    return originInterpreter->isSafeScript ([self _executionContext]->interpreter());
	}
    }
    return true;
}

- (void)dealloc
{
    removeNativeReference(_private->imp);
    [_private release];
        
    [super dealloc];
}

- (void)finalize
{
    removeNativeReference(_private->imp);
        
    [super finalize];
}

+ (BOOL)throwException:(NSString *)exceptionMessage
{
    InterpreterImp *first, *interp = InterpreterImp::firstInterpreter();

    // This code assumes that we only ever have one running interpreter.  A
    // good assumption for now, as we depend on that elsewhere.  However,
    // in the future we may have the ability to run multiple interpreters,
    // in which case this will have to change.
    first = interp;
    do {
        ExecState *exec = interp->globalExec();
        // If the interpreter has a context, we set the exception.
        if (interp->context()) {
            throwError(exec, GeneralError, exceptionMessage);
            return YES;
        }
        interp = interp->nextInterpreter();
    } while (interp != first);
    
    return NO;
}

static List listFromNSArray(ExecState *exec, NSArray *array)
{
    int i, numObjects = array ? [array count] : 0;
    List aList;
    
    for (i = 0; i < numObjects; i++) {
        id anObject = [array objectAtIndex:i];
        aList.append (convertObjcValueToValue(exec, &anObject, ObjcObjectType));
    }
    return aList;
}

- (id)callWebScriptMethod:(NSString *)name withArguments:(NSArray *)args
{
    if (![self _executionContext])
        return nil;

    if (![self _isSafeScript])
	return nil;

    // Lookup the function object.
    ExecState *exec = [self _executionContext]->interpreter()->globalExec();

    Interpreter::lock();
    
    ValueImp *v = convertObjcValueToValue(exec, &name, ObjcObjectType);
    Identifier identifier(v->toString(exec));
    ValueImp *func = [self _imp]->get (exec, identifier);
    Interpreter::unlock();
    if (!func || func->isUndefined()) {
        // Maybe throw an exception here?
        return 0;
    }

    // Call the function object.    
    Interpreter::lock();
    ObjectImp *funcImp = static_cast<ObjectImp*>(func);
    ObjectImp *thisObj = const_cast<ObjectImp*>([self _imp]);
    List argList = listFromNSArray(exec, args);
    ValueImp *result = funcImp->call (exec, thisObj, argList);
    Interpreter::unlock();

    if (exec->hadException()) {
        LOG_EXCEPTION (exec);
        result = Undefined();
    }

    // Convert and return the result of the function call.
    id resultObj = [WebScriptObject _convertValueToObjcValue:result originExecutionContext:[self _originExecutionContext] executionContext:[self _executionContext]];

    _didExecute(self);
        
    return resultObj;
}

- (id)evaluateWebScript:(NSString *)script
{
    if (![self _executionContext])
        return nil;

    if (![self _isSafeScript])
	return nil;

    ExecState *exec = [self _executionContext]->interpreter()->globalExec();

    ValueImp *result;
    
    Interpreter::lock();
    
    ValueImp *v = convertObjcValueToValue(exec, &script, ObjcObjectType);
    Completion completion = [self _executionContext]->interpreter()->evaluate(UString(), 0, v->toString(exec));
    ComplType type = completion.complType();
    
    if (type == Normal) {
        result = completion.value();
        if (!result) {
            result = Undefined();
        }
    }
    else
        result = Undefined();

    Interpreter::unlock();
    
    if (exec->hadException()) {
        LOG_EXCEPTION (exec);
        result = Undefined();
    }

    id resultObj = [WebScriptObject _convertValueToObjcValue:result originExecutionContext:[self _originExecutionContext] executionContext:[self _executionContext]];

    _didExecute(self);
    
    return resultObj;
}

- (void)setValue:(id)value forKey:(NSString *)key
{
    if (![self _executionContext])
        return;

    if (![self _isSafeScript])
	return;

    ExecState *exec = [self _executionContext]->interpreter()->globalExec();

    Interpreter::lock();
    ValueImp *v = convertObjcValueToValue(exec, &key, ObjcObjectType);
    [self _imp]->put (exec, Identifier (v->toString(exec)), (convertObjcValueToValue(exec, &value, ObjcObjectType)));
    Interpreter::unlock();

    if (exec->hadException()) {
        LOG_EXCEPTION (exec);
    }

    _didExecute(self);
}

- (id)valueForKey:(NSString *)key
{
    if (![self _executionContext])
        return nil;
        
    if (![self _isSafeScript])
	return nil;

    ExecState *exec = [self _executionContext]->interpreter()->globalExec();

    Interpreter::lock();
    ValueImp *v = convertObjcValueToValue(exec, &key, ObjcObjectType);
    ValueImp *result = [self _imp]->get (exec, Identifier (v->toString(exec)));
    Interpreter::unlock();
    
    if (exec->hadException()) {
        LOG_EXCEPTION (exec);
        result = Undefined();
    }

    id resultObj = [WebScriptObject _convertValueToObjcValue:result originExecutionContext:[self _originExecutionContext] executionContext:[self _executionContext]];

    _didExecute(self);
    
    return resultObj;
}

- (void)removeWebScriptKey:(NSString *)key
{
    if (![self _executionContext])
        return;
        
    if (![self _isSafeScript])
	return;

    ExecState *exec = [self _executionContext]->interpreter()->globalExec();

    Interpreter::lock();
    ValueImp *v = convertObjcValueToValue(exec, &key, ObjcObjectType);
    [self _imp]->deleteProperty (exec, Identifier (v->toString(exec)));
    Interpreter::unlock();

    if (exec->hadException()) {
        LOG_EXCEPTION (exec);
    }

    _didExecute(self);
}

- (NSString *)stringRepresentation
{
    if (![self _isSafeScript])
        // This is a workaround for a gcc 3.3 internal compiler error.
	return @"Undefined";

    Interpreter::lock();
    ObjectImp *thisObj = const_cast<ObjectImp*>([self _imp]);
    ExecState *exec = [self _executionContext]->interpreter()->globalExec();
    
    id result = convertValueToObjcValue(exec, thisObj, ObjcObjectType).objectValue;

    Interpreter::unlock();
    
    id resultObj = [result description];

    _didExecute(self);

    return resultObj;
}

- (id)webScriptValueAtIndex:(unsigned int)index
{
    if (![self _executionContext])
        return nil;

    if (![self _isSafeScript])
	return nil;

    ExecState *exec = [self _executionContext]->interpreter()->globalExec();
    Interpreter::lock();
    ValueImp *result = [self _imp]->get (exec, (unsigned)index);
    Interpreter::unlock();

    if (exec->hadException()) {
        LOG_EXCEPTION (exec);
        result = Undefined();
    }

    id resultObj = [WebScriptObject _convertValueToObjcValue:result originExecutionContext:[self _originExecutionContext] executionContext:[self _executionContext]];

    _didExecute(self);

    return resultObj;
}

- (void)setWebScriptValueAtIndex:(unsigned int)index value:(id)value
{
    if (![self _executionContext])
        return;

    if (![self _isSafeScript])
	return;

    ExecState *exec = [self _executionContext]->interpreter()->globalExec();
    Interpreter::lock();
    [self _imp]->put (exec, (unsigned)index, (convertObjcValueToValue(exec, &value, ObjcObjectType)));
    Interpreter::unlock();

    if (exec->hadException()) {
        LOG_EXCEPTION (exec);
    }

    _didExecute(self);
}

- (void)setException:(NSString *)description
{
    if (const RootObject *root = [self _executionContext])
        throwError(root->interpreter()->globalExec(), GeneralError, description);
}

+ (id)_convertValueToObjcValue:(ValueImp *)value originExecutionContext:(const RootObject *)originExecutionContext executionContext:(const RootObject *)executionContext
{
    id result = 0;

    // First see if we have a ObjC instance.
    if (value->isObject()) {
        ObjectImp *objectImp = static_cast<ObjectImp*>(value);
	Interpreter *intepreter = executionContext->interpreter();
	ExecState *exec = intepreter->globalExec();
        Interpreter::lock();
	
        if (objectImp->classInfo() != &RuntimeObjectImp::info) {
	    ValueImp *runtimeObject = objectImp->get(exec, "__apple_runtime_object");
	    if (runtimeObject && runtimeObject->isObject())
		objectImp = static_cast<RuntimeObjectImp*>(runtimeObject);
	}
        
        Interpreter::unlock();

        if (objectImp->classInfo() == &RuntimeObjectImp::info) {
            RuntimeObjectImp *imp = static_cast<RuntimeObjectImp *>(objectImp);
            ObjcInstance *instance = static_cast<ObjcInstance*>(imp->getInternalInstance());
            if (instance)
                result = instance->getObject();
        }
        // Convert to a WebScriptObject
        else {
	    result = (id)intepreter->createLanguageInstanceForValue (exec, Instance::ObjectiveCLanguage, value->toObject(exec), originExecutionContext, executionContext);
        }
    }
    
    // Convert JavaScript String value to NSString?
    else if (value->isString()) {
        StringImp *s = static_cast<StringImp*>(value);
        UString u = s->value();
        
        NSString *string = [NSString stringWithCharacters:(const unichar*)u.data() length:u.size()];
        result = string;
    }
    
    // Convert JavaScript Number value to NSNumber?
    else if (value->isNumber()) {
        result = [NSNumber numberWithDouble:value->getNumber()];
    }
    
    else if (value->isBoolean()) {
        BooleanImp *b = static_cast<BooleanImp*>(value);
        result = [NSNumber numberWithBool:b->value()];
    }
    
    // Convert JavaScript Undefined types to WebUndefined
    else if (value->isUndefined()) {
        result = [WebUndefined undefined];
    }
    
    // Other types (UnspecifiedType and NullType) converted to 0.
    
    return result;
}

@end

@implementation WebUndefined

+ (id)allocWithZone:(NSZone *)zone
{
    static WebUndefined *sharedUndefined = 0;
    if (!sharedUndefined)
        sharedUndefined = [super allocWithZone:NULL];
    return sharedUndefined;
}

- (id)initWithCoder:(NSCoder *)coder
{
    return self;
}

- (void)encodeWithCoder:(NSCoder *)encoder
{
}

- (id)copyWithZone:(NSZone *)zone
{
    return self;
}

- (id)retain
{
    return self;
}

- (void)release
{
}

- (unsigned)retainCount
{
    return UINT_MAX;
}

- (id)autorelease
{
    return self;
}

- (void)dealloc
{
    assert(false);
    return;
    [super dealloc]; // make -Wdealloc-check happy
}

+ (WebUndefined *)undefined
{
    return [WebUndefined allocWithZone:NULL];
}

@end
