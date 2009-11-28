/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "JSGlobalData.h"

#include "ArgList.h"
#include "Collector.h"
#include "CommonIdentifiers.h"
#include "FunctionConstructor.h"
#include "GetterSetter.h"
#include "Interpreter.h"
#include "JSActivation.h"
#include "JSAPIValueWrapper.h"
#include "JSArray.h"
#include "JSByteArray.h"
#include "JSClassRef.h"
#include "JSFunction.h"
#include "JSLock.h"
#include "JSNotAnObject.h"
#include "JSPropertyNameIterator.h"
#include "JSStaticScopeObject.h"
#include "Parser.h"
#include "Lexer.h"
#include "Lookup.h"
#include "Nodes.h"

#if ENABLE(JSC_MULTIPLE_THREADS)
#include <wtf/Threading.h>
#endif

#if PLATFORM(MAC)
#include "ProfilerServer.h"
#endif

using namespace WTF;

namespace JSC {

extern JSC_CONST_HASHTABLE HashTable arrayTable;
extern JSC_CONST_HASHTABLE HashTable jsonTable;
extern JSC_CONST_HASHTABLE HashTable dateTable;
extern JSC_CONST_HASHTABLE HashTable mathTable;
extern JSC_CONST_HASHTABLE HashTable numberTable;
extern JSC_CONST_HASHTABLE HashTable regExpTable;
extern JSC_CONST_HASHTABLE HashTable regExpConstructorTable;
extern JSC_CONST_HASHTABLE HashTable stringTable;

void JSGlobalData::ClientData::willExecute(ExecState*)
{
}

void JSGlobalData::ClientData::didExecute(ExecState*)
{
}

struct VPtrSet {
    VPtrSet();

    void* jsArrayVPtr;
    void* jsByteArrayVPtr;
    void* jsStringVPtr;
    void* jsFunctionVPtr;
};

VPtrSet::VPtrSet()
{
    CollectorCell cell;
    void* storage = &cell;

    ASSERT(sizeof(JSArray) <= sizeof(CollectorCell));
    JSCell* jsArray = new (storage) JSArray(JSArray::createStructure(jsNull()));
    jsArrayVPtr = jsArray->vptr();
    jsArray->~JSCell();

    ASSERT(sizeof(JSByteArray) <= sizeof(CollectorCell));
    JSCell* jsByteArray = new (storage) JSByteArray(JSByteArray::VPtrStealingHack);
    jsByteArrayVPtr = jsByteArray->vptr();
    jsByteArray->~JSCell();

    ASSERT(sizeof(JSString) <= sizeof(CollectorCell));
    JSCell* jsString = new (storage) JSString(JSString::VPtrStealingHack);
    jsStringVPtr = jsString->vptr();
    jsString->~JSCell();

    ASSERT(sizeof(JSFunction) <= sizeof(CollectorCell));
    JSCell* jsFunction = new (storage) JSFunction(JSFunction::createStructure(jsNull()));
    jsFunctionVPtr = jsFunction->vptr();
    jsFunction->~JSCell();
}

JSGlobalData::JSGlobalData(bool isShared, const VPtrSet& vptrSet)
    : isSharedInstance(isShared)
    , clientData(0)
    , arrayTable(fastNew<HashTable>(JSC::arrayTable))
    , dateTable(fastNew<HashTable>(JSC::dateTable))
    , jsonTable(fastNew<HashTable>(JSC::jsonTable))
    , mathTable(fastNew<HashTable>(JSC::mathTable))
    , numberTable(fastNew<HashTable>(JSC::numberTable))
    , regExpTable(fastNew<HashTable>(JSC::regExpTable))
    , regExpConstructorTable(fastNew<HashTable>(JSC::regExpConstructorTable))
    , stringTable(fastNew<HashTable>(JSC::stringTable))
    , activationStructure(JSActivation::createStructure(jsNull()))
    , interruptedExecutionErrorStructure(JSObject::createStructure(jsNull()))
    , staticScopeStructure(JSStaticScopeObject::createStructure(jsNull()))
    , stringStructure(JSString::createStructure(jsNull()))
    , notAnObjectErrorStubStructure(JSNotAnObjectErrorStub::createStructure(jsNull()))
    , notAnObjectStructure(JSNotAnObject::createStructure(jsNull()))
    , propertyNameIteratorStructure(JSPropertyNameIterator::createStructure(jsNull()))
    , getterSetterStructure(GetterSetter::createStructure(jsNull()))
    , apiWrapperStructure(JSAPIValueWrapper::createStructure(jsNull()))
#if USE(JSVALUE32)
    , numberStructure(JSNumberCell::createStructure(jsNull()))
#endif
    , jsArrayVPtr(vptrSet.jsArrayVPtr)
    , jsByteArrayVPtr(vptrSet.jsByteArrayVPtr)
    , jsStringVPtr(vptrSet.jsStringVPtr)
    , jsFunctionVPtr(vptrSet.jsFunctionVPtr)
    , identifierTable(createIdentifierTable())
    , propertyNames(new CommonIdentifiers(this))
    , emptyList(new MarkedArgumentBuffer)
    , lexer(new Lexer(this))
    , parser(new Parser)
    , interpreter(new Interpreter)
#if ENABLE(JIT)
    , jitStubs(this)
#endif
    , heap(this)
    , initializingLazyNumericCompareFunction(false)
    , head(0)
    , dynamicGlobalObject(0)
    , functionCodeBlockBeingReparsed(0)
    , firstStringifierToMark(0)
    , markStack(vptrSet.jsArrayVPtr)
    , cachedUTCOffset(NaN)
    , weakRandom(static_cast<int>(currentTime()))
#ifndef NDEBUG
    , mainThreadOnly(false)
#endif
{
#if PLATFORM(MAC)
    startProfilerServerIfNeeded();
#endif
}

JSGlobalData::~JSGlobalData()
{
    // By the time this is destroyed, heap.destroy() must already have been called.

    delete interpreter;
#ifndef NDEBUG
    // Zeroing out to make the behavior more predictable when someone attempts to use a deleted instance.
    interpreter = 0;
#endif

    arrayTable->deleteTable();
    dateTable->deleteTable();
    jsonTable->deleteTable();
    mathTable->deleteTable();
    numberTable->deleteTable();
    regExpTable->deleteTable();
    regExpConstructorTable->deleteTable();
    stringTable->deleteTable();

    fastDelete(const_cast<HashTable*>(arrayTable));
    fastDelete(const_cast<HashTable*>(dateTable));
    fastDelete(const_cast<HashTable*>(jsonTable));
    fastDelete(const_cast<HashTable*>(mathTable));
    fastDelete(const_cast<HashTable*>(numberTable));
    fastDelete(const_cast<HashTable*>(regExpTable));
    fastDelete(const_cast<HashTable*>(regExpConstructorTable));
    fastDelete(const_cast<HashTable*>(stringTable));

    delete parser;
    delete lexer;

    deleteAllValues(opaqueJSClassData);

    delete emptyList;

    delete propertyNames;
    deleteIdentifierTable(identifierTable);

    delete clientData;
}

PassRefPtr<JSGlobalData> JSGlobalData::create(bool isShared)
{
    return adoptRef(new JSGlobalData(isShared, VPtrSet()));
}

PassRefPtr<JSGlobalData> JSGlobalData::createLeaked()
{
    Structure::startIgnoringLeaks();
    RefPtr<JSGlobalData> data = create();
    Structure::stopIgnoringLeaks();
    return data.release();
}

bool JSGlobalData::sharedInstanceExists()
{
    return sharedInstanceInternal();
}

JSGlobalData& JSGlobalData::sharedInstance()
{
    JSGlobalData*& instance = sharedInstanceInternal();
    if (!instance) {
        instance = create(true).releaseRef();
#if ENABLE(JSC_MULTIPLE_THREADS)
        instance->makeUsableFromMultipleThreads();
#endif
    }
    return *instance;
}

JSGlobalData*& JSGlobalData::sharedInstanceInternal()
{
    ASSERT(JSLock::currentThreadIsHoldingLock());
    static JSGlobalData* sharedInstance;
    return sharedInstance;
}

// FIXME: We can also detect forms like v1 < v2 ? -1 : 0, reverse comparison, etc.
const Vector<Instruction>& JSGlobalData::numericCompareFunction(ExecState* exec)
{
    if (!lazyNumericCompareFunction.size() && !initializingLazyNumericCompareFunction) {
        initializingLazyNumericCompareFunction = true;
        RefPtr<FunctionExecutable> function = FunctionExecutable::fromGlobalCode(Identifier(exec, "numericCompare"), exec, 0, makeSource(UString("(function (v1, v2) { return v1 - v2; })")), 0, 0);
        lazyNumericCompareFunction = function->bytecode(exec, exec->scopeChain()).instructions();
        initializingLazyNumericCompareFunction = false;
    }

    return lazyNumericCompareFunction;
}

JSGlobalData::ClientData::~ClientData()
{
}

void JSGlobalData::resetDateCache()
{
    cachedUTCOffset = NaN;
    dstOffsetCache.reset();
    cachedDateString = UString();
    dateInstanceCache.reset();
}

void JSGlobalData::startSampling()
{
    interpreter->startSampling();
}

void JSGlobalData::stopSampling()
{
    interpreter->stopSampling();
}

void JSGlobalData::dumpSampleData(ExecState* exec)
{
    interpreter->dumpSampleData(exec);
}

} // namespace JSC
