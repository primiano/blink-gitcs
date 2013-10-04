# Copyright (C) 2013 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Generate template values for attributes.

FIXME: Not currently used in build.
This is a rewrite of the Perl IDL compiler in Python, but is not complete.
Once it is complete, we will switch all IDL files over to Python at once.
Until then, please work on the Perl IDL compiler.
For details, see bug http://crbug.com/239771
"""

import v8_types
import v8_utilities
from v8_utilities import uncapitalize


def generate_attributes(interface):
    def generate_attribute(attribute):
        attribute_contents, attribute_includes = generate_attribute_and_includes(interface, attribute)
        includes.update(attribute_includes)
        return attribute_contents

    includes = set()
    contents = generate_attributes_common(interface)
    contents['attributes'] = [generate_attribute(attribute) for attribute in interface.attributes]
    return contents, includes


def generate_attributes_common(interface):
    attributes = interface.attributes
    v8_class_name = v8_utilities.v8_class_name(interface)
    return {
        # Size 0 constant array is not allowed in VC++
        'number_of_attributes': 'WTF_ARRAY_LENGTH(%sAttributes)' % v8_class_name if attributes else '0',
        'attribute_templates': '%sAttributes' % v8_class_name if attributes else '0',
    }


def generate_attribute_and_includes(interface, attribute):
    idl_type = attribute.data_type
    extended_attributes = attribute.extended_attributes
    this_is_keep_alive_for_gc = is_keep_alive_for_gc(attribute)
    contents = {
        'cached_attribute_validation_method': extended_attributes.get('CachedAttribute'),
        'conditional_string': v8_utilities.generate_conditional_string(attribute),
        'conditional_string': v8_utilities.generate_conditional_string(attribute),
        'cpp_type': v8_types.cpp_type(idl_type),
        'idl_type': idl_type,
        'is_keep_alive_for_gc': this_is_keep_alive_for_gc,
        'is_nullable': attribute.is_nullable,
        'is_static': attribute.is_static,
        'name': attribute.name,
        'v8_type': v8_types.v8_type(idl_type),
    }

    cpp_value = getter_expression(interface, attribute)
    # Normally we can inline the function call into the return statement to
    # avoid the overhead of using a Ref<> temporary, but for some cases
    # (nullable types, EventHandler, CachedAttribute, or if there are
    # exceptions), we need to use a local variable.
    # FIXME: check if compilers are smart enough to inline this, and if so,
    # always use a local variable (for readability and CG simplicity).
    if (attribute.is_nullable or
        idl_type == 'EventHandler' or
        'CachedAttribute' in extended_attributes):
        contents['cpp_value_original'] = cpp_value
        cpp_value = 'value'
    contents['cpp_value'] = cpp_value

    if this_is_keep_alive_for_gc:
        return_v8_value_statement = 'v8SetReturnValue(info, wrapper);'
        includes = v8_types.includes_for_type(idl_type)
        includes.add('bindings/v8/V8HiddenPropertyName.h')
    else:
        return_v8_value_statement, includes = v8_types.v8_set_return_value(idl_type, cpp_value, callback_info='info', isolate='info.GetIsolate()', extended_attributes=extended_attributes, script_wrappable='imp')
    contents['return_v8_value_statement'] = return_v8_value_statement

    if (idl_type == 'EventHandler' and
        interface.name in ['Window', 'WorkerGlobalScope'] and
        attribute.name == 'onerror'):
            includes.add('bindings/v8/V8ErrorHandler.h')

    # ActivityLog
    contents['is_activity_logging_getter'] = v8_utilities.has_activity_logging(attribute, includes, 'Getter')

    return contents, includes


def getter_expression(interface, attribute):
    this_getter_name = getter_name(interface, attribute)
    arguments = []
    if attribute.is_nullable:
        arguments.append('isNull')
    if attribute.data_type == 'EventHandler':
        arguments.append('isolatedWorldForIsolate(info.GetIsolate())')
    return '%s(%s)' % (this_getter_name, ', '.join(arguments))


def getter_name(interface, attribute):
    getter_method_name = uncapitalize(attribute.name)
    if attribute.is_static:
        return '%s::%s' % (interface.name, getter_method_name)
    return 'imp->%s' % getter_method_name


def is_keep_alive_for_gc(attribute):
    idl_type = attribute.data_type
    extended_attributes = attribute.extended_attributes
    return (
        'KeepAttributeAliveForGC' in extended_attributes or
        # For readonly attributes, for performance reasons we keep the attribute
        # wrapper alive while the owner wrapper is alive, because the attribute
        # never changes.
        (attribute.is_read_only and
         v8_types.wrapper_type(idl_type) and
         # There are some exceptions, however:
         not(
             # Node lifetime is managed by object grouping.
             v8_types.dom_node_type(idl_type) or
             # A self-reference is unnecessary.
             attribute.name == 'self' or
             # FIXME: Remove these hard-coded hacks.
             idl_type in ['EventHandler', 'Promise', 'Window'] or
             idl_type.startswith('HTML'))))
