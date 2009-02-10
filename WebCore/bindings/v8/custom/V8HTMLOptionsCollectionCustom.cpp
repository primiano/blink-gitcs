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
#include "HTMLOptionsCollection.h"

#include "ExceptionCode.h"

#include "V8Binding.h"
#include "V8CustomBinding.h"
#include "V8Proxy.h"

namespace WebCore {

ACCESSOR_GETTER(HTMLOptionsCollectionLength)
{
    INC_STATS("DOM.HTMLOptionsCollection.length._get");
    HTMLOptionsCollection* imp = V8Proxy::ToNativeObject<HTMLOptionsCollection>(V8ClassIndex::HTMLOPTIONSCOLLECTION, info.Holder());
    int v = imp->length();
    return v8::Integer::New(v);
}

ACCESSOR_SETTER(HTMLOptionsCollectionLength)
{
    INC_STATS("DOM.HTMLOptionsCollection.length._set");
    HTMLOptionsCollection* imp = V8Proxy::ToNativeObject<HTMLOptionsCollection>(V8ClassIndex::HTMLOPTIONSCOLLECTION, info.Holder());
    double v = value->NumberValue();
    unsigned newLength = 0;
    ExceptionCode ec = 0;
    if (!isnan(v) && !isinf(v)) {
        if (v < 0.0)
            ec = INDEX_SIZE_ERR;
        else if (v > static_cast<double>(UINT_MAX))
            newLength = UINT_MAX;
        else
            newLength = static_cast<unsigned>(v);
    }
    if (!ec)
        imp->setLength(value->Uint32Value(), ec);
    V8Proxy::SetDOMException(ec);
}

} // namespace WebCore
