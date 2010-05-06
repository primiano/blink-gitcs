/*
    This file is part of the WebKit open source project.
    This file has been generated by generate-bindings.pl. DO NOT MODIFY!

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include "config.h"
#include "V8TestInterface.h"

#include "RuntimeEnabledFeatures.h"
#include "V8Binding.h"
#include "V8BindingState.h"
#include "V8DOMWrapper.h"
#include "V8IsolatedContext.h"
#include "V8Proxy.h"

namespace WebCore {

WrapperTypeInfo V8TestInterface::info = { V8TestInterface::GetTemplate, V8TestInterface::derefObject, 0 };

namespace TestInterfaceInternal {

template <typename T> void V8_USE(T) { }

} // namespace TestInterfaceInternal

v8::Handle<v8::Value> V8TestInterface::constructorCallback(const v8::Arguments& args)
{
    INC_STATS("DOM.TestInterface.Contructor");
    return V8Proxy::constructDOMObjectWithScriptExecutionContext<TestInterface>(args, &info);
}
static v8::Persistent<v8::FunctionTemplate> ConfigureV8TestInterfaceTemplate(v8::Persistent<v8::FunctionTemplate> desc)
{
    v8::Local<v8::Signature> defaultSignature = configureTemplate(desc, "TestInterface", v8::Persistent<v8::FunctionTemplate>(), V8TestInterface::internalFieldCount,
        0, 0,
        0, 0);
        desc->SetCallHandler(V8TestInterface::constructorCallback);
    

    // Custom toString template
    desc->Set(getToStringName(), getToStringTemplate());
    return desc;
}

v8::Persistent<v8::FunctionTemplate> V8TestInterface::GetRawTemplate()
{
    static v8::Persistent<v8::FunctionTemplate> V8TestInterfaceRawCache = createRawTemplate();
    return V8TestInterfaceRawCache;
}

v8::Persistent<v8::FunctionTemplate> V8TestInterface::GetTemplate()
{
    static v8::Persistent<v8::FunctionTemplate> V8TestInterfaceCache = ConfigureV8TestInterfaceTemplate(GetRawTemplate());
    return V8TestInterfaceCache;
}

TestInterface* V8TestInterface::toNative(v8::Handle<v8::Object> object)
{
    return reinterpret_cast<TestInterface*>(object->GetPointerFromInternalField(v8DOMWrapperObjectIndex));
}

bool V8TestInterface::HasInstance(v8::Handle<v8::Value> value)
{
    return GetRawTemplate()->HasInstance(value);
}


v8::Handle<v8::Object> V8TestInterface::wrap(TestInterface* impl)
{
    v8::Handle<v8::Object> wrapper;
    V8Proxy* proxy = 0;
        wrapper = getDOMObjectMap().get(impl);
        if (!wrapper.IsEmpty())
            return wrapper;
    wrapper = V8DOMWrapper::instantiateV8Object(proxy, &info, impl);
    if (wrapper.IsEmpty())
        return wrapper;

    impl->ref();
    getDOMObjectMap().set(impl, v8::Persistent<v8::Object>::New(wrapper));
    return wrapper;
}

v8::Handle<v8::Value> toV8(PassRefPtr<TestInterface > impl)
{
    return toV8(impl.get());
}

v8::Handle<v8::Value> toV8(TestInterface* impl)
{
    if (!impl)
        return v8::Null();
    return V8TestInterface::wrap(impl);
}

void V8TestInterface::derefObject(void* object)
{
    static_cast<TestInterface*>(object)->deref();
}

} // namespace WebCore
