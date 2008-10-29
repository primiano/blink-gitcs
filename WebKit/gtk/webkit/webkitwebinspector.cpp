/*
 * Copyright (C) 2008 Gustavo Noronha Silva
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"

#include "webkitwebinspector.h"
#include "webkitmarshal.h"
#include "InspectorClientGtk.h"
#include "webkitprivate.h"

using namespace WebKit;

extern "C" {

enum {
    INSPECT_WEB_VIEW,
    SHOW_WINDOW,
    ATTACH_WINDOW,
    DETTACH_WINDOW,
    CLOSE_WINDOW,
    FINISHED,
    LAST_SIGNAL
};

static guint webkit_web_inspector_signals[LAST_SIGNAL] = { 0, };

enum {
    PROP_0,

    PROP_WEB_VIEW,
    PROP_INSPECTED_URI,
};

G_DEFINE_TYPE(WebKitWebInspector, webkit_web_inspector, G_TYPE_OBJECT)

struct _WebKitWebInspectorPrivate {
    InspectorClient* inspectorClient;
    WebKitWebView* inspector_view;
    gchar* inspected_uri;
};

#define WEBKIT_WEB_INSPECTOR_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), WEBKIT_TYPE_WEB_INSPECTOR, WebKitWebInspectorPrivate))

static void webkit_web_inspector_finalize(GObject* object);

static void webkit_web_inspector_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec);

static void webkit_web_inspector_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec);

static gboolean webkit_inspect_web_view_request_handled(GSignalInvocationHint* ihint, GValue* returnAccu, const GValue* handlerReturn, gpointer dummy)
{
    gboolean continueEmission = TRUE;
    gpointer newWebView = g_value_get_object(handlerReturn);
    g_value_set_object(returnAccu, newWebView);

    if (newWebView)
        continueEmission = FALSE;

    return continueEmission;
}

static void webkit_web_inspector_class_init(WebKitWebInspectorClass* klass)
{
    GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->finalize = webkit_web_inspector_finalize;
    gobject_class->set_property = webkit_web_inspector_set_property;
    gobject_class->get_property = webkit_web_inspector_get_property;

    /**
     * WebKitWebInspector::inspect-web-view:
     * @web_inspector: the object on which the signal is emitted
     * @web_view: the #WebKitWeb which will be inspected
     * @return: a newly allocated #WebKitWebView or %NULL
     *
     * Emitted when the user activates the 'inspect' context menu item
     * to inspect a web view. The application which is interested in
     * the inspector should create a window, or otherwise add the
     * #WebKitWebView it creates to an existing window.
     *
     * You don't need to handle the reference count of the
     * #WebKitWebView instance you create; the widget to which you add
     * it will do that.
     *
     * Since: 1.0.2
     */
    webkit_web_inspector_signals[INSPECT_WEB_VIEW] = g_signal_new("inspect-web-view",
            G_TYPE_FROM_CLASS(klass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            0,
            webkit_inspect_web_view_request_handled,
            NULL,
            webkit_marshal_OBJECT__OBJECT,
            WEBKIT_TYPE_WEB_VIEW , 1,
            WEBKIT_TYPE_WEB_VIEW);

    /**
     * WebKitWebInspector::show-window:
     * @web_inspector: the object on which the signal is emitted
     * @return: %TRUE if the signal has been handled
     *
     * Emitted when the inspector window should be displayed. Notice
     * that the window must have been created already by handling
     * ::inspect-web-view.
     *
     * Since: 1.0.2
     */
    webkit_web_inspector_signals[SHOW_WINDOW] = g_signal_new("show-window",
            G_TYPE_FROM_CLASS(klass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            0,
            g_signal_accumulator_true_handled,
            NULL,
            webkit_marshal_BOOLEAN__VOID,
            G_TYPE_BOOLEAN , 0);

    /**
     * WebKitWebInspector::attach-window:
     * @web_inspector: the object on which the signal is emitted
     * @return: %TRUE if the signal has been handled
     *
     * Emitted when the inspector should appear at the same window as
     * the #WebKitWebView being inspected.
     *
     * Since: 1.0.2
     */
    webkit_web_inspector_signals[ATTACH_WINDOW] = g_signal_new("attach-window",
            G_TYPE_FROM_CLASS(klass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            0,
            g_signal_accumulator_true_handled,
            NULL,
            webkit_marshal_BOOLEAN__VOID,
            G_TYPE_BOOLEAN , 0);

    /**
     * WebKitWebInspector::dettach-window:
     * @web_inspector: the object on which the signal is emitted
     * @return: %TRUE if the signal has been handled
     *
     * Emitted when the inspector should appear in a separate window.
     *
     * Since: 1.0.2
     */
    webkit_web_inspector_signals[DETTACH_WINDOW] = g_signal_new("dettach-window",
            G_TYPE_FROM_CLASS(klass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            0,
            g_signal_accumulator_true_handled,
            NULL,
            webkit_marshal_BOOLEAN__VOID,
            G_TYPE_BOOLEAN , 0);

    /**
     * WebKitWebInspector::close-window:
     * @web_inspector: the object on which the signal is emitted
     * @return: %TRUE if the signal has been handled
     *
     * Emitted when the inspector window should be closed. You can
     * destroy the window or hide it so that it can be displayed again
     * by handling ::show-window later on.
     *
     * Notice that the inspected #WebKitWebView may no longer exist
     * when this signal is emitted.
     *
     * Notice, too, that if you decide to destroy the window,
     * ::inspect-web-view will be emmited again, when the user
     * inspects an element.
     *
     * Since: 1.0.2
     */
    webkit_web_inspector_signals[CLOSE_WINDOW] = g_signal_new("close-window",
            G_TYPE_FROM_CLASS(klass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            0,
            g_signal_accumulator_true_handled,
            NULL,
            webkit_marshal_BOOLEAN__VOID,
            G_TYPE_BOOLEAN , 0);

    /**
     * WebKitWebInspector::finished:
     * @web_inspector: the object on which the signal is emitted
     *
     * Emitted when the inspection is done. You should release your
     * references on the inspector at this time. The inspected
     * #WebKitWebView may no longer exist when this signal is emitted.
     *
     * Since: 1.0.2
     */
    webkit_web_inspector_signals[FINISHED] = g_signal_new("finished",
            G_TYPE_FROM_CLASS(klass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            0,
            NULL,
            NULL,
            g_cclosure_marshal_VOID__VOID,
            G_TYPE_NONE , 0);

    /*
     * properties
     */

    /**
     * WebKitWebInspector:web-view:
     *
     * The Web View that renders the Web Inspector itself.
     *
     * Since: 1.0.2
     */
    g_object_class_install_property(gobject_class, PROP_WEB_VIEW,
                                    g_param_spec_object("web-view",
                                                        "Web View",
                                                        "The Web View that renders the Web Inspector itself",
                                                        WEBKIT_TYPE_WEB_VIEW,
                                                        WEBKIT_PARAM_READABLE));

    /**
     * WebKitWebInspector:inspected-uri:
     *
     * The URI that is currently being inspected.
     *
     * Since: 1.0.2
     */
    g_object_class_install_property(gobject_class, PROP_INSPECTED_URI,
                                    g_param_spec_string("inspected-uri",
                                                        "Inspected URI",
                                                        "The URI that is currently being inspected",
                                                        NULL,
                                                        WEBKIT_PARAM_READABLE));

    g_type_class_add_private(klass, sizeof(WebKitWebInspectorPrivate));
}

static void webkit_web_inspector_init(WebKitWebInspector* web_inspector)
{
    web_inspector->priv = WEBKIT_WEB_INSPECTOR_GET_PRIVATE(web_inspector);
}

static void webkit_web_inspector_finalize(GObject* object)
{
    WebKitWebInspector* web_inspector = WEBKIT_WEB_INSPECTOR(object);
    WebKitWebInspectorPrivate* priv = web_inspector->priv;

    if (priv->inspector_view)
        g_object_unref(priv->inspector_view);

    if (priv->inspected_uri)
        g_free(priv->inspected_uri);

    G_OBJECT_CLASS(webkit_web_inspector_parent_class)->finalize(object);
}

static void webkit_web_inspector_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    switch(prop_id) {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void webkit_web_inspector_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec)
{
    WebKitWebInspector* web_inspector = WEBKIT_WEB_INSPECTOR(object);
    WebKitWebInspectorPrivate* priv = web_inspector->priv;

    switch (prop_id) {
    case PROP_WEB_VIEW:
        g_value_set_object(value, priv->inspector_view);
        break;
    case PROP_INSPECTED_URI:
        g_value_set_string(value, priv->inspected_uri);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

// internal use only
void webkit_web_inspector_set_web_view(WebKitWebInspector *web_inspector, WebKitWebView *web_view)
{
    g_return_if_fail(WEBKIT_IS_WEB_INSPECTOR(web_inspector));
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(web_view));

    WebKitWebInspectorPrivate* priv = web_inspector->priv;

    if (priv->inspector_view)
        g_object_unref(priv->inspector_view);

    g_object_ref(web_view);
    priv->inspector_view = web_view;
}

/**
 * webkit_web_inspector_get_web_view:
 *
 * Obtains the #WebKitWebView that is used to render the
 * inspector. The #WebKitWebView instance is created by the
 * application, by handling the ::inspect-web-view signal. This means
 * that this method may return %NULL if the user hasn't inspected
 * anything.
 *
 * Returns: the #WebKitWebView instance that is used to render the
 * inspector or %NULL if it is not yet created.
 *
 * Since: 1.0.2
 **/
WebKitWebView* webkit_web_inspector_get_web_view(WebKitWebInspector *web_inspector)
{
    WebKitWebInspectorPrivate* priv = web_inspector->priv;

    return priv->inspector_view;
}

// internal use only
void webkit_web_inspector_set_inspected_uri(WebKitWebInspector* web_inspector, const gchar* inspected_uri)
{
    g_return_if_fail(WEBKIT_IS_WEB_INSPECTOR(web_inspector));

    WebKitWebInspectorPrivate* priv = web_inspector->priv;

    g_free(priv->inspected_uri);
    priv->inspected_uri = g_strdup(inspected_uri);
}

/**
 * webkit_web_inspector_get_inspected_uri:
 *
 * Obtains the URI that is currently being inspected.
 *
 * Returns: a pointer to the URI as an internally allocated string; it
 * should not be freed, modified or stored.
 *
 * Since: 1.0.2
 **/
const gchar* webkit_web_inspector_get_inspected_uri(WebKitWebInspector *web_inspector)
{
    WebKitWebInspectorPrivate* priv = web_inspector->priv;

    return priv->inspected_uri;
}

void
webkit_web_inspector_set_inspector_client(WebKitWebInspector* web_inspector, WebKit::InspectorClient* inspectorClient)
{
    WebKitWebInspectorPrivate* priv = web_inspector->priv;

    priv->inspectorClient = inspectorClient;
}

}
