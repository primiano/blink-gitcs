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
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef WebKitDOMTestObj_h
#define WebKitDOMTestObj_h

#include "webkit/webkitdomdefines.h"
#include <glib-object.h>
#include <webkit/webkitdefines.h>
#include "webkit/WebKitDOMObject.h"


G_BEGIN_DECLS
#define WEBKIT_TYPE_DOM_TEST_OBJ            (webkit_dom_test_obj_get_type())
#define WEBKIT_DOM_TEST_OBJ(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), WEBKIT_TYPE_DOM_TEST_OBJ, WebKitDOMTestObj))
#define WEBKIT_DOM_TEST_OBJ_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  WEBKIT_TYPE_DOM_TEST_OBJ, WebKitDOMTestObjClass)
#define WEBKIT_DOM_IS_TEST_OBJ(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), WEBKIT_TYPE_DOM_TEST_OBJ))
#define WEBKIT_DOM_IS_TEST_OBJ_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  WEBKIT_TYPE_DOM_TEST_OBJ))
#define WEBKIT_DOM_TEST_OBJ_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  WEBKIT_TYPE_DOM_TEST_OBJ, WebKitDOMTestObjClass))

struct _WebKitDOMTestObj {
    WebKitDOMObject parent_instance;
};

struct _WebKitDOMTestObjClass {
    WebKitDOMObjectClass parent_class;
};

WEBKIT_API GType
webkit_dom_test_obj_get_type (void);

WEBKIT_API void
webkit_dom_test_obj_void_method (WebKitDOMTestObj *self);

WEBKIT_API void
webkit_dom_test_obj_void_method_with_args (WebKitDOMTestObj *self, glong int_arg, gchar*  str_arg, WebKitDOMTestObj*  obj_arg);

WEBKIT_API glong
webkit_dom_test_obj_int_method (WebKitDOMTestObj *self);

WEBKIT_API glong
webkit_dom_test_obj_int_method_with_args (WebKitDOMTestObj *self, glong int_arg, gchar*  str_arg, WebKitDOMTestObj*  obj_arg);

WEBKIT_API WebKitDOMTestObj* 
webkit_dom_test_obj_obj_method (WebKitDOMTestObj *self);

WEBKIT_API WebKitDOMTestObj* 
webkit_dom_test_obj_obj_method_with_args (WebKitDOMTestObj *self, glong int_arg, gchar*  str_arg, WebKitDOMTestObj*  obj_arg);

WEBKIT_API void
webkit_dom_test_obj_method_with_exception (WebKitDOMTestObj *self, GError **error);


/* TODO: custom function webkit_dom_test_obj_custom_args_and_exception */


/* TODO: event function webkit_dom_test_obj_add_event_listener */


/* TODO: event function webkit_dom_test_obj_remove_event_listener */

WEBKIT_API void
webkit_dom_test_obj_with_dynamic_frame (WebKitDOMTestObj *self);

WEBKIT_API void
webkit_dom_test_obj_with_dynamic_frame_and_arg (WebKitDOMTestObj *self, glong int_arg);

WEBKIT_API void
webkit_dom_test_obj_with_dynamic_frame_and_optional_arg (WebKitDOMTestObj *self, glong int_arg, glong optional_arg);

WEBKIT_API void
webkit_dom_test_obj_with_dynamic_frame_and_user_gesture (WebKitDOMTestObj *self, glong int_arg);

WEBKIT_API void
webkit_dom_test_obj_with_dynamic_frame_and_user_gesture_asad (WebKitDOMTestObj *self, glong int_arg, glong optional_arg);

WEBKIT_API void
webkit_dom_test_obj_with_script_state_void (WebKitDOMTestObj *self);

WEBKIT_API WebKitDOMTestObj* 
webkit_dom_test_obj_with_script_state_obj (WebKitDOMTestObj *self);

WEBKIT_API void
webkit_dom_test_obj_method_with_optional_arg (WebKitDOMTestObj *self, glong opt);

WEBKIT_API void
webkit_dom_test_obj_method_with_non_optional_arg_and_optional_arg (WebKitDOMTestObj *self, glong non_opt, glong opt);

WEBKIT_API void
webkit_dom_test_obj_method_with_non_optional_arg_and_two_optional_args (WebKitDOMTestObj *self, glong non_opt, glong opt1, glong opt2);

WEBKIT_API glong
webkit_dom_test_obj_get_read_only_int_attr (WebKitDOMTestObj *self);

WEBKIT_API gchar* 
webkit_dom_test_obj_get_read_only_string_attr (WebKitDOMTestObj *self);

WEBKIT_API WebKitDOMTestObj* 
webkit_dom_test_obj_get_read_only_test_obj_attr (WebKitDOMTestObj *self);

WEBKIT_API glong
webkit_dom_test_obj_get_int_attr (WebKitDOMTestObj *self);

WEBKIT_API void
webkit_dom_test_obj_set_int_attr (WebKitDOMTestObj *self, glong value);

WEBKIT_API gchar* 
webkit_dom_test_obj_get_string_attr (WebKitDOMTestObj *self);

WEBKIT_API void
webkit_dom_test_obj_set_string_attr (WebKitDOMTestObj *self, gchar*  value);

WEBKIT_API WebKitDOMTestObj* 
webkit_dom_test_obj_get_test_obj_attr (WebKitDOMTestObj *self);

WEBKIT_API void
webkit_dom_test_obj_set_test_obj_attr (WebKitDOMTestObj *self, WebKitDOMTestObj*  value);

WEBKIT_API glong
webkit_dom_test_obj_get_attr_with_exception (WebKitDOMTestObj *self);

WEBKIT_API void
webkit_dom_test_obj_set_attr_with_exception (WebKitDOMTestObj *self, glong value);

WEBKIT_API glong
webkit_dom_test_obj_get_attr_with_setter_exception (WebKitDOMTestObj *self);

WEBKIT_API void
webkit_dom_test_obj_set_attr_with_setter_exception (WebKitDOMTestObj *self, glong value);

WEBKIT_API glong
webkit_dom_test_obj_get_attr_with_getter_exception (WebKitDOMTestObj *self);

WEBKIT_API void
webkit_dom_test_obj_set_attr_with_getter_exception (WebKitDOMTestObj *self, glong value);

G_END_DECLS

#endif /* WebKitDOMTestObj_h */
