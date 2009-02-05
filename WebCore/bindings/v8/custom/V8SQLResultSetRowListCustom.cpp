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

#include "v8_binding.h"
#include "v8_custom.h"
#include "v8_proxy.h"

#include "SQLResultSetRowList.h"

namespace WebCore {

CALLBACK_FUNC_DECL(SQLResultSetRowListItem)
{
    INC_STATS("DOM.SQLResultSetRowList.item()");

    if (args.Length() == 0) {
        V8Proxy::ThrowError(V8Proxy::SYNTAX_ERROR, "Item index is required.");
        return v8::Undefined();
    }

    if (!args[0]->IsNumber()) {
        V8Proxy::ThrowError(V8Proxy::TYPE_ERROR, "Item index must be a number.");
        return v8::Undefined();
    }

    SQLResultSetRowList* rowList = V8Proxy::ToNativeObject<SQLResultSetRowList>(V8ClassIndex::SQLRESULTSETROWLIST, args.Holder());

    unsigned long index = args[0]->IntegerValue();
    if (index < 0 || index >= rowList->length()) {
        V8Proxy::ThrowError(V8Proxy::RANGE_ERROR, "Item index is out of range.");
        return v8::Undefined();
    }

    v8::Local<v8::Object> item = v8::Object::New();
    unsigned numColumns = rowList->columnNames().size();
    unsigned valuesIndex = index * numColumns;

    for (unsigned i = 0; i < numColumns; ++i) {
        const SQLValue& sqlValue = rowList->values()[valuesIndex + i];
        v8::Handle<v8::Value> value;
        switch(sqlValue.type()) {
            case SQLValue::StringValue:
                value = v8String(sqlValue.string());
                break;
            case SQLValue::NullValue:
                value = v8::Null();
                break;
            case SQLValue::NumberValue:
                value = v8::Number::New(sqlValue.number());
                break;
            default:
                ASSERT_NOT_REACHED();
        }

        item->Set(v8String(rowList->columnNames()[i]), value, static_cast<v8::PropertyAttribute>(v8::DontDelete|v8::ReadOnly));
    }

    return item;
}

} // namespace WebCore
