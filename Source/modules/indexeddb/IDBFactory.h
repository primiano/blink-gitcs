/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
#ifndef IDBFactory_h
#define IDBFactory_h

#include "bindings/v8/ScriptWrappable.h"
#include "modules/indexeddb/IDBOpenDBRequest.h"
#include "modules/indexeddb/IndexedDBClient.h"
#include "platform/heap/Handle.h"
#include "wtf/PassRefPtr.h"
#include "wtf/RefCounted.h"
#include "wtf/RefPtr.h"
#include "wtf/text/WTFString.h"

namespace WebCore {

class ExceptionState;
class IDBKey;
class IDBKeyRange;
class ExecutionContext;

class IDBFactory : public RefCountedWillBeGarbageCollectedFinalized<IDBFactory>, public ScriptWrappable {
public:
    static PassRefPtrWillBeRawPtr<IDBFactory> create(PassRefPtrWillBeRawPtr<IndexedDBClient> client)
    {
        return adoptRefWillBeNoop(new IDBFactory(client));
    }
    ~IDBFactory();
    void trace(Visitor*);

    PassRefPtrWillBeRawPtr<IDBRequest> getDatabaseNames(ExecutionContext*, ExceptionState&);

    PassRefPtrWillBeRawPtr<IDBOpenDBRequest> open(ExecutionContext*, const String& name, ExceptionState&);
    PassRefPtrWillBeRawPtr<IDBOpenDBRequest> open(ExecutionContext*, const String& name, unsigned long long version, ExceptionState&);
    PassRefPtrWillBeRawPtr<IDBOpenDBRequest> deleteDatabase(ExecutionContext*, const String& name, ExceptionState&);

    short cmp(ExecutionContext*, const ScriptValue& first, const ScriptValue& second, ExceptionState&);

private:
    explicit IDBFactory(PassRefPtrWillBeRawPtr<IndexedDBClient>);

    PassRefPtrWillBeRawPtr<IDBOpenDBRequest> openInternal(ExecutionContext*, const String& name, int64_t version, ExceptionState&);

    RefPtrWillBeMember<IndexedDBClient> m_permissionClient;
};

} // namespace WebCore

#endif // IDBFactory_h
