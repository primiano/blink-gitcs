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
#include "V8HTMLSelectElementCustom.h"

#include "HTMLSelectElement.h"
#include "HTMLOptionElement.h"
#include "HTMLOptionsCollection.h"

#include "V8Binding.h"
#include "V8Collection.h"
#include "V8CustomBinding.h"
#include "V8HTMLOptionElement.h"
#include "V8HTMLSelectElement.h"
#include "V8NamedNodesCollection.h"
#include "V8Proxy.h"

namespace WebCore {

NAMED_PROPERTY_GETTER(HTMLSelectElement)
{
    INC_STATS("DOM.HTMLSelectElement.NamedPropertySetter");
    HTMLSelectElement* select = V8DOMWrapper::convertDOMWrapperToNode<HTMLSelectElement>(info.Holder());
    v8::Handle<v8::Value> value = info.Holder()->GetRealNamedPropertyInPrototypeChain(name);

    if (!value.IsEmpty())
        return value;

    // Search local callback properties next to find IDL defined properties.
    if (info.Holder()->HasRealNamedCallbackProperty(name))
        return notHandledByInterceptor();

    PassRefPtr<HTMLOptionsCollection> collection = select->options();

    Vector<RefPtr<Node> > items;
    collection->namedItems(v8StringToAtomicWebCoreString(name), items);

    if (!items.size())
        return notHandledByInterceptor();

    if (items.size() == 1)
        return V8DOMWrapper::convertNodeToV8Object(items.at(0).release());

    NodeList* list = new V8NamedNodesCollection(items);
    return V8DOMWrapper::convertToV8Object(V8ClassIndex::NODELIST, list);
}

INDEXED_PROPERTY_GETTER(HTMLSelectElement)
{
    ASSERT(V8DOMWrapper::maybeDOMWrapper(info.Holder()));
    ASSERT(V8DOMWrapper::domWrapperType(info.Holder()) == V8ClassIndex::NODE);
    RefPtr<Node> result = V8DOMWrapper::convertDOMWrapperToNode<HTMLSelectElement>(info.Holder())->item(index);
    if (!result)
        return notHandledByInterceptor();

    return V8DOMWrapper::convertNodeToV8Object(result.release());
}

INDEXED_PROPERTY_SETTER(HTMLSelectElement)
{
    INC_STATS("DOM.HTMLSelectElement.IndexedPropertySetter");
    HTMLSelectElement* select = V8DOMWrapper::convertDOMWrapperToNode<HTMLSelectElement>(info.Holder());
    return toOptionsCollectionSetter(index, value, select);
}

v8::Handle<v8::Value> V8HTMLSelectElement::removeCallback(const v8::Arguments& args)
{
    INC_STATS("DOM.HTMLSelectElement.remove");
    HTMLSelectElement* imp = V8DOMWrapper::convertDOMWrapperToNode<HTMLSelectElement>(args.Holder());
    return removeElement(imp, args);
}

v8::Handle<v8::Value> removeElement(HTMLSelectElement* imp, const v8::Arguments& args) 
{
    if (V8HTMLOptionElement::HasInstance(args[0])) {
        HTMLOptionElement* element = V8DOMWrapper::convertDOMWrapperToNode<HTMLOptionElement>(v8::Handle<v8::Object>::Cast(args[0]));
        imp->remove(element->index());
        return v8::Undefined();
    }

    imp->remove(toInt32(args[0]));
    return v8::Undefined();
}

} // namespace WebCore
