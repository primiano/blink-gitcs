/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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
 *
 */

#include "config.h"
#include "FunctionPrototype.h"

#include "JSGlobalObject.h"
#include "Parser.h"
#include "ArrayPrototype.h"
#include "debugger.h"
#include "JSFunction.h"
#include "JSString.h"
#include "lexer.h"
#include "nodes.h"
#include "JSObject.h"
#include <stdio.h>
#include <string.h>
#include <wtf/Assertions.h>

namespace KJS {

// ------------------------------ FunctionPrototype -------------------------

static JSValue* functionProtoFuncToString(ExecState*, JSObject*, const ArgList&);
static JSValue* functionProtoFuncApply(ExecState*, JSObject*, const ArgList&);
static JSValue* functionProtoFuncCall(ExecState*, JSObject*, const ArgList&);

FunctionPrototype::FunctionPrototype(ExecState* exec)
{
    putDirect(exec->propertyNames().length, jsNumber(0), DontDelete | ReadOnly | DontEnum);

    putDirectFunction(new PrototypeFunction(exec, this, 0, exec->propertyNames().toString, functionProtoFuncToString), DontEnum);
    putDirectFunction(new PrototypeFunction(exec, this, 2, exec->propertyNames().apply, functionProtoFuncApply), DontEnum);
    putDirectFunction(new PrototypeFunction(exec, this, 1, exec->propertyNames().call, functionProtoFuncCall), DontEnum);
}

// ECMA 15.3.4
JSValue* FunctionPrototype::callAsFunction(ExecState*, JSObject*, const ArgList&)
{
    return jsUndefined();
}

// Functions

JSValue* functionProtoFuncToString(ExecState* exec, JSObject* thisObj, const ArgList&)
{
    if (!thisObj || !thisObj->inherits(&InternalFunction::info)) {
#ifndef NDEBUG
        fprintf(stderr,"attempted toString() call on null or non-function object\n");
#endif
        return throwError(exec, TypeError);
    }

    if (thisObj->inherits(&JSFunction::info)) {
        JSFunction* fi = static_cast<JSFunction*>(thisObj);
        return jsString("function " + fi->functionName().ustring() + "(" + fi->body->paramString() + ") " + fi->body->toSourceString());
    }

    return jsString("function " + static_cast<InternalFunction*>(thisObj)->functionName().ustring() + "() {\n    [native code]\n}");
}

JSValue* functionProtoFuncApply(ExecState* exec, JSObject* thisObj, const ArgList& args)
{
    if (!thisObj->implementsCall())
        return throwError(exec, TypeError);

    JSValue* thisArg = args[0];
    JSValue* argArray = args[1];

    JSObject* applyThis;
    if (thisArg->isUndefinedOrNull())
        applyThis = exec->globalThisValue();
    else
        applyThis = thisArg->toObject(exec);

    ArgList applyArgs;
    if (!argArray->isUndefinedOrNull()) {
        if (argArray->isObject() &&
            (static_cast<JSObject*>(argArray)->inherits(&JSArray::info) ||
             static_cast<JSObject*>(argArray)->inherits(&Arguments::info))) {

            JSObject* argArrayObj = static_cast<JSObject*>(argArray);
            unsigned int length = argArrayObj->get(exec, exec->propertyNames().length)->toUInt32(exec);
            for (unsigned int i = 0; i < length; i++)
                applyArgs.append(argArrayObj->get(exec, i));
        } else
            return throwError(exec, TypeError);
    }

    return thisObj->callAsFunction(exec, applyThis, applyArgs);
}

JSValue* functionProtoFuncCall(ExecState* exec, JSObject* thisObj, const ArgList& args)
{
    if (!thisObj->implementsCall())
        return throwError(exec, TypeError);

    JSValue* thisArg = args[0];

    JSObject* callThis;
    if (thisArg->isUndefinedOrNull())
        callThis = exec->globalThisValue();
    else
        callThis = thisArg->toObject(exec);

    ArgList argsTail;
    args.getSlice(1, argsTail);
    return thisObj->callAsFunction(exec, callThis, argsTail);
}

// ------------------------------ FunctionConstructor ----------------------------

FunctionConstructor::FunctionConstructor(ExecState* exec, FunctionPrototype* functionPrototype)
    : InternalFunction(functionPrototype, Identifier(exec, functionPrototype->classInfo()->className))
{
    putDirect(exec->propertyNames().prototype, functionPrototype, DontEnum | DontDelete | ReadOnly);

    // Number of arguments for constructor
    putDirect(exec->propertyNames().length, jsNumber(1), ReadOnly | DontDelete | DontEnum);
}

ConstructType FunctionConstructor::getConstructData(ConstructData&)
{
    return ConstructTypeNative;
}

// ECMA 15.3.2 The Function Constructor
JSObject* FunctionConstructor::construct(ExecState* exec, const ArgList& args, const Identifier& functionName, const UString& sourceURL, int lineNumber)
{
    UString p("");
    UString body;
    int argsSize = args.size();
    if (argsSize == 0)
        body = "";
    else if (argsSize == 1)
        body = args[0]->toString(exec);
    else {
        p = args[0]->toString(exec);
        for (int k = 1; k < argsSize - 1; k++)
            p += "," + args[k]->toString(exec);
        body = args[argsSize - 1]->toString(exec);
    }

    // parse the source code
    int sourceId;
    int errLine;
    UString errMsg;
    RefPtr<SourceProvider> source = UStringSourceProvider::create(body);
    RefPtr<FunctionBodyNode> functionBody = exec->parser()->parse<FunctionBodyNode>(exec, sourceURL, lineNumber, source, &sourceId, &errLine, &errMsg);

    // No program node == syntax error - throw a syntax error
    if (!functionBody)
        // We can't return a Completion(Throw) here, so just set the exception
        // and return it
        return throwError(exec, SyntaxError, errMsg, errLine, sourceId, sourceURL);
    
    functionBody->setSource(SourceRange(source, 0, source->length()));
    ScopeChain scopeChain(exec->lexicalGlobalObject(), exec->globalThisValue());

    JSFunction* fimp = new JSFunction(exec, functionName, functionBody.get(), scopeChain.node());

    // parse parameter list. throw syntax error on illegal identifiers
    int len = p.size();
    const UChar* c = p.data();
    int i = 0, params = 0;
    UString param;
    while (i < len) {
        while (*c == ' ' && i < len)
            c++, i++;
        if (Lexer::isIdentStart(c[0])) {  // else error
            param = UString(c, 1);
            c++, i++;
            while (i < len && (Lexer::isIdentPart(c[0]))) {
                param += UString(c, 1);
                c++, i++;
            }
            while (i < len && *c == ' ')
                c++, i++;
            if (i == len) {
                functionBody->parameters().append(Identifier(exec, param));
                params++;
                break;
            } else if (*c == ',') {
                functionBody->parameters().append(Identifier(exec, param));
                params++;
                c++, i++;
                continue;
            } // else error
        }
        return throwError(exec, SyntaxError, "Syntax error in parameter list");
    }
  
    JSObject* objCons = exec->lexicalGlobalObject()->objectConstructor();
    JSObject* prototype = objCons->construct(exec, exec->emptyList());
    prototype->putDirect(exec->propertyNames().constructor, fimp, DontEnum);
    fimp->putDirect(exec->propertyNames().prototype, prototype, DontDelete);
    return fimp;
}

// ECMA 15.3.2 The Function Constructor
JSObject* FunctionConstructor::construct(ExecState* exec, const ArgList& args)
{
    return construct(exec, args, Identifier(exec, "anonymous"), UString(), 1);
}

// ECMA 15.3.1 The Function Constructor Called as a Function
JSValue* FunctionConstructor::callAsFunction(ExecState* exec, JSObject*, const ArgList& args)
{
    return construct(exec, args);
}

} // namespace KJS
