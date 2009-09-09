/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(DATABASE)

#include "Database.h"
#include "V8Binding.h"
#include "V8CustomBinding.h"
#include "V8CustomSQLTransactionCallback.h"
#include "V8CustomSQLTransactionErrorCallback.h"
#include "V8CustomVoidCallback.h"
#include "V8Proxy.h"

namespace WebCore {

CALLBACK_FUNC_DECL(DatabaseChangeVersion)
{
    INC_STATS("DOM.Database.changeVersion()");
    return v8::Undefined();
}

static v8::Handle<v8::Value> createTransaction(const v8::Arguments& args, bool readOnly)
{
    if (!args.Length())
        return throwError("Transaction callback is required.", V8Proxy::SyntaxError);

    if (!args[0]->IsObject())
        return throwError("Transaction callback must be of valid type.");

    Database* database = V8DOMWrapper::convertToNativeObject<Database>(V8ClassIndex::DATABASE, args.Holder());

    Frame* frame = V8Proxy::retrieveFrameForCurrentContext();
    if (!frame)
        return v8::Undefined();

    RefPtr<V8CustomSQLTransactionCallback> callback = V8CustomSQLTransactionCallback::create(args[0], frame);

    RefPtr<V8CustomSQLTransactionErrorCallback> errorCallback;
    if (args.Length() > 1) {
        if (!args[1]->IsObject())
            return throwError("Transaction error callback must be of valid type.");

        errorCallback = V8CustomSQLTransactionErrorCallback::create(args[1], frame);
    }

    RefPtr<V8CustomVoidCallback> successCallback;
    if (args.Length() > 2) {
        if (!args[1]->IsObject())
            return throwError("Transaction success callback must be of valid type.");

        successCallback = V8CustomVoidCallback::create(args[2], frame);
    }

    database->transaction(callback.release(), errorCallback.release(), successCallback.release(), readOnly);
    return v8::Undefined();
}

CALLBACK_FUNC_DECL(DatabaseTransaction)
{
    INC_STATS("DOM.Database.transaction()");
    return createTransaction(args, false);
}

CALLBACK_FUNC_DECL(DatabaseReadTransaction)
{
    INC_STATS("DOM.Database.readTransaction()");
    return createTransaction(args, true);
}

} // namespace WebCore

#endif
