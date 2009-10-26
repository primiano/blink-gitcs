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
#include "HTMLAllCollection.h"

#include "V8Binding.h"
#include "V8Collection.h"
#include "V8CustomBinding.h"
#include "V8Proxy.h"

namespace WebCore {

NAMED_PROPERTY_GETTER(HTMLAllCollection)
{
    INC_STATS("DOM.HTMLAllCollection.NamedPropertyGetter");
    // Search the prototype chain first.
    v8::Handle<v8::Value> value = info.Holder()->GetRealNamedPropertyInPrototypeChain(name);

    if (!value.IsEmpty())
        return value;

    // Search local callback properties next to find IDL defined
    // properties.
    if (info.Holder()->HasRealNamedCallbackProperty(name))
        return v8::Handle<v8::Value>();

    // Finally, search the DOM structure.
    HTMLAllCollection* imp = V8DOMWrapper::convertToNativeObject<HTMLAllCollection>(V8ClassIndex::HTMLALLCOLLECTION, info.Holder());
    return getNamedItemsFromCollection(imp, v8StringToAtomicWebCoreString(name));
}

CALLBACK_FUNC_DECL(HTMLAllCollectionItem)
{
    INC_STATS("DOM.HTMLAllCollection.item()");
    HTMLAllCollection* imp = V8DOMWrapper::convertToNativeObject<HTMLAllCollection>(V8ClassIndex::HTMLALLCOLLECTION, args.Holder());
    return getItemFromCollection(imp, args[0]);
}

CALLBACK_FUNC_DECL(HTMLAllCollectionNamedItem)
{
    INC_STATS("DOM.HTMLAllCollection.namedItem()");
    HTMLAllCollection* imp = V8DOMWrapper::convertToNativeObject<HTMLAllCollection>(V8ClassIndex::HTMLALLCOLLECTION, args.Holder());
    v8::Handle<v8::Value> result = getNamedItemsFromCollection(imp, toWebCoreString(args[0]));

    if (result.IsEmpty())
        return v8::Undefined();

    return result;
}

CALLBACK_FUNC_DECL(HTMLAllCollectionCallAsFunction)
{
    INC_STATS("DOM.HTMLAllCollection.callAsFunction()");
    if (args.Length() < 1)
        return v8::Undefined();

    HTMLAllCollection* imp = V8DOMWrapper::convertToNativeObject<HTMLAllCollection>(V8ClassIndex::HTMLALLCOLLECTION, args.Holder());

    if (args.Length() == 1)
        return getItemFromCollection(imp, args[0]);

    // If there is a second argument it is the index of the item we want.
    String name = toWebCoreString(args[0]);
    v8::Local<v8::Uint32> index = args[1]->ToArrayIndex();
    if (index.IsEmpty())
        return v8::Undefined();

    unsigned current = index->Uint32Value();
    Node* node = imp->namedItem(name);
    while (node) {
        if (!current)
            return V8DOMWrapper::convertNodeToV8Object(node);

        node = imp->nextNamedItem(name);
        current--;
    }

    return v8::Undefined();
}

} // namespace WebCore
