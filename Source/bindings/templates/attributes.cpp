{##############################################################################}
{% macro attribute_getter(attribute) %}
static void {{attribute.name}}AttributeGetter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    {% if not attribute.is_static %}
    {{cpp_class_name}}* imp = {{v8_class_name}}::toNative(info.Holder());
    {% endif %}
    {% if attribute.is_nullable %}
    bool isNull = false;
    {{attribute.cpp_type}} {{attribute.cpp_value}} = {{attribute.cpp_value_original}};
    if (isNull) {
        v8SetReturnValueNull(info);
        return;
    }
    {% endif %}
    {% if attribute.idl_type == 'EventHandler' %}
    {{attribute.cpp_type}} {{attribute.cpp_value}} = {{attribute.cpp_value_original}};
    {% endif %}
    {% if attribute.is_keep_alive_for_gc %}
    {{attribute.cpp_type}} result = {{attribute.cpp_value}};
    if (result.get() && DOMDataStore::setReturnValueFromWrapper<{{attribute.v8_type}}>(info.GetReturnValue(), result.get()))
        return;
    v8::Handle<v8::Value> wrapper = toV8(result.get(), info.Holder(), info.GetIsolate());
    if (!wrapper.IsEmpty()) {
        V8HiddenPropertyName::setNamedHiddenReference(info.Holder(), "{{attribute.name}}", wrapper);
        v8SetReturnValue(info, wrapper);
    }
    {% else %}
    {{attribute.return_v8_value_statement}}
    {% endif %}
}
{% endmacro %}


{##############################################################################}
{% macro attribute_getter_callback(attribute) %}
static void {{attribute.name}}AttributeGetterCallback(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    {% if attribute.is_activity_logging_getter %}
    V8PerContextData* contextData = V8PerContextData::from(info.GetIsolate()->GetCurrentContext());
    if (contextData && contextData->activityLogger())
        contextData->activityLogger()->log("{{interface_name}}.{{attribute.name}}", 0, 0, "Getter");
    {% endif %}
    {{cpp_class_name}}V8Internal::{{attribute.name}}AttributeGetter(name, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}
{% endmacro %}
