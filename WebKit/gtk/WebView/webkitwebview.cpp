/*
 *  Copyright (C) 2007 Holger Hans Peter Freyther
 *  Copyright (C) 2007 Christian Dywan <christian@twotoasts.de>
 *  Copyright (C) 2007 Xan Lopez <xan@gnome.org>
 *  Copyright (C) 2007 Alp Toker <alp@atoker.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"

#include "webkitwebview.h"
#include "webkit-marshal.h"
#include "webkitprivate.h"

#include "NotImplemented.h"
#include "CString.h"
#include "ChromeClientGtk.h"
#include "ContextMenu.h"
#include "ContextMenuClientGtk.h"
#include "ContextMenuController.h"
#include "Cursor.h"
#include "DragClientGtk.h"
#include "Editor.h"
#include "EditorClientGtk.h"
#include "EventHandler.h"
#include "FocusController.h"
#include "HitTestRequest.h"
#include "HitTestResult.h"
#include "GraphicsContext.h"
#include "InspectorClientGtk.h"
#include "FrameLoader.h"
#include "FrameView.h"
#include "Editor.h"
#include "PasteboardHelper.h"
#include "PlatformKeyboardEvent.h"
#include "PlatformWheelEvent.h"
#include "SubstituteData.h"

#include <gdk/gdkkeysyms.h>

using namespace WebKit;
using namespace WebCore;

extern "C" {

enum {
    /* normal signals */
    NAVIGATION_REQUESTED,
    WINDOW_OBJECT_CLEARED,
    LOAD_STARTED,
    LOAD_COMMITTED,
    LOAD_PROGRESS_CHANGED,
    LOAD_FINISHED,
    TITLE_CHANGED,
    HOVERING_OVER_LINK,
    STATUS_BAR_TEXT_CHANGED,
    ICOND_LOADED,
    SELECTION_CHANGED,
    CONSOLE_MESSAGE,
    SCRIPT_ALERT,
    SCRIPT_CONFIRM,
    SCRIPT_PROMPT,
    SELECT_ALL,
    COPY_CLIPBOARD,
    PASTE_CLIPBOARD,
    CUT_CLIPBOARD,
    LAST_SIGNAL
};

enum {
    PROP_0,

    PROP_COPY_TARGET_LIST,
    PROP_PASTE_TARGET_LIST
};

static guint webkit_web_view_signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE(WebKitWebView, webkit_web_view, GTK_TYPE_CONTAINER)

static void webkit_web_view_context_menu_position_func(GtkMenu*, gint* x, gint* y, gboolean* pushIn, WebKitWebViewPrivate* data)
{
    *pushIn = FALSE;
    *x = data->lastPopupXPosition;
    *y = data->lastPopupYPosition;
}

static gboolean webkit_web_view_forward_context_menu_event(WebKitWebView* webView, const PlatformMouseEvent& event)
{
    Page* page = core(webView);
    page->contextMenuController()->clearContextMenu();
    Frame* focusedFrame = page->focusController()->focusedOrMainFrame();
    focusedFrame->view()->setCursor(pointerCursor());
    bool handledEvent = focusedFrame->eventHandler()->sendContextMenuEvent(event);
    if (!handledEvent)
        return FALSE;

    ContextMenu* coreMenu = page->contextMenuController()->contextMenu();
    if (!coreMenu)
        return FALSE;

    if (!coreMenu->platformDescription())
        return FALSE;


    WebKitWebViewPrivate* pageData = WEBKIT_WEB_VIEW_GET_PRIVATE(webView);
    pageData->lastPopupXPosition = event.globalX();
    pageData->lastPopupYPosition = event.globalY();
    gtk_menu_popup(GTK_MENU(coreMenu->platformDescription()), NULL, NULL,
                   reinterpret_cast<GtkMenuPositionFunc>(webkit_web_view_context_menu_position_func), pageData,
                   event.button() + 1, gtk_get_current_event_time());
    return TRUE;
}

static gboolean webkit_web_view_popup_menu_handler(GtkWidget* widget)
{
    static const int contextMenuMargin = 1;

    // The context menu event was generated from the keyboard, so show the context menu by the current selection.
    Page* page = core(WEBKIT_WEB_VIEW(widget));
    FrameView* view = page->mainFrame()->view();
    Position start = page->mainFrame()->selectionController()->selection().start();
    Position end = page->mainFrame()->selectionController()->selection().end();

    int rightAligned = FALSE;
    IntPoint location;

    if (!start.node() || !end.node())
        location = IntPoint(rightAligned ? view->contentsWidth() - contextMenuMargin : contextMenuMargin, contextMenuMargin);
    else {
        RenderObject* renderer = start.node()->renderer();
        if (!renderer)
            return FALSE;

        // Calculate the rect of the first line of the selection (cribbed from -[WebCoreFrameBridge firstRectForDOMRange:]).
        int extraWidthToEndOfLine = 0;
        IntRect startCaretRect = renderer->caretRect(start.offset(), DOWNSTREAM, &extraWidthToEndOfLine);
        IntRect endCaretRect = renderer->caretRect(end.offset(), UPSTREAM);

        IntRect firstRect;
        if (startCaretRect.y() == endCaretRect.y())
            firstRect = IntRect(MIN(startCaretRect.x(), endCaretRect.x()),
                                startCaretRect.y(),
                                abs(endCaretRect.x() - startCaretRect.x()),
                                MAX(startCaretRect.height(), endCaretRect.height()));
        else
            firstRect = IntRect(startCaretRect.x(),
                                startCaretRect.y(),
                                startCaretRect.width() + extraWidthToEndOfLine,
                                startCaretRect.height());

        location = IntPoint(rightAligned ? firstRect.right() : firstRect.x(), firstRect.bottom());
    }

    int x, y;
    gdk_window_get_origin(GTK_WIDGET(view->containingWindow())->window, &x, &y);

    // FIXME: The IntSize(0, -1) is a hack to get the hit-testing to result in the selected element.
    // Ideally we'd have the position of a context menu event be separate from its target node.
    location = view->contentsToWindow(location) + IntSize(0, -1);
    IntPoint global = location + IntSize(x, y);
    PlatformMouseEvent event(location, global, NoButton, MouseEventPressed, 0, false, false, false, false, gtk_get_current_event_time());

    return webkit_web_view_forward_context_menu_event(WEBKIT_WEB_VIEW(widget), event);
}

static void webkit_web_view_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec) {
    WebKitWebView* webView = WEBKIT_WEB_VIEW(object);

    switch(prop_id) {
    case PROP_COPY_TARGET_LIST:
        g_value_set_boxed(value, webkit_web_view_get_copy_target_list(webView));
        break;
    case PROP_PASTE_TARGET_LIST:
        g_value_set_boxed(value, webkit_web_view_get_paste_target_list(webView));
        break;
    default:
        break;
    }
}

static gboolean webkit_web_view_expose_event(GtkWidget* widget, GdkEventExpose* event)
{
    Frame* frame = core(webkit_web_view_get_main_frame(WEBKIT_WEB_VIEW(widget)));
    GdkRectangle clip;
    gdk_region_get_clipbox(event->region, &clip);
    cairo_t* cr = gdk_cairo_create(event->window);
    GraphicsContext ctx(cr);
    ctx.setGdkExposeEvent(event);
    if (frame->renderer()) {
        frame->view()->layoutIfNeededRecursive();
        frame->view()->paint(&ctx, clip);
    }
    cairo_destroy(cr);

    return FALSE;
}

static gboolean webkit_web_view_key_press_event(GtkWidget* widget, GdkEventKey* event)
{
    Frame* frame = core(WEBKIT_WEB_VIEW(widget))->focusController()->focusedOrMainFrame();
    PlatformKeyboardEvent keyboardEvent(event);

    if (frame->eventHandler()->keyEvent(keyboardEvent))
        return TRUE;

    FrameView* view = frame->view();
    SelectionController::EAlteration alteration;
    if (event->state & GDK_SHIFT_MASK)
        alteration = SelectionController::EXTEND;
    else
        alteration = SelectionController::MOVE;

    // TODO: We probably want to use GTK+ key bindings here and perhaps take an
    // approach more like the Win and Mac ports for key handling.
    switch (event->keyval) {
    case GDK_Down:
        view->scrollBy(0, LINE_STEP);
        return TRUE;
    case GDK_Up:
        view->scrollBy(0, -LINE_STEP);
        return TRUE;
    case GDK_Right:
        view->scrollBy(LINE_STEP, 0);
        return TRUE;
    case GDK_Left:
        view->scrollBy(-LINE_STEP, 0);
        return TRUE;
    case GDK_Home:
        frame->selectionController()->modify(alteration, SelectionController::BACKWARD, DocumentBoundary, true);
        return TRUE;
    case GDK_End:
        frame->selectionController()->modify(alteration, SelectionController::FORWARD, DocumentBoundary, true);
        return TRUE;
    }

    /* Chain up to our parent class for binding activation */
    return GTK_WIDGET_CLASS(webkit_web_view_parent_class)->key_press_event(widget, event);
}

static gboolean webkit_web_view_key_release_event(GtkWidget* widget, GdkEventKey* event)
{
    Frame* frame = core(WEBKIT_WEB_VIEW(widget))->focusController()->focusedOrMainFrame();
    PlatformKeyboardEvent keyboardEvent(event);

    if (frame->eventHandler()->keyEvent(keyboardEvent))
        return TRUE;

    /* Chain up to our parent class for binding activation */
    return GTK_WIDGET_CLASS(webkit_web_view_parent_class)->key_release_event(widget, event);
}

static gboolean webkit_web_view_button_press_event(GtkWidget* widget, GdkEventButton* event)
{
    Frame* frame = core(webkit_web_view_get_main_frame(WEBKIT_WEB_VIEW(widget)));

    // FIXME: need to keep track of subframe focus for key events
    gtk_widget_grab_focus(GTK_WIDGET(widget));

    if (event->button == 3)
        return webkit_web_view_forward_context_menu_event(WEBKIT_WEB_VIEW(widget), PlatformMouseEvent(event));

    return frame->eventHandler()->handleMousePressEvent(PlatformMouseEvent(event));
}

static gboolean webkit_web_view_button_release_event(GtkWidget* widget, GdkEventButton* event)
{
    Frame* frame = core(webkit_web_view_get_main_frame(WEBKIT_WEB_VIEW(widget)));

    WebKitWebView* web_view = WEBKIT_WEB_VIEW(widget);
    WebKitWebViewPrivate* webViewData = WEBKIT_WEB_VIEW_GET_PRIVATE(web_view);
    Frame* focusedFrame = webViewData->corePage->focusController()->focusedFrame();

    if (focusedFrame->editor()->canEdit()) {
        GdkWindow* window = gtk_widget_get_parent_window(widget);
        gtk_im_context_set_client_window(webViewData->imContext, window);
#ifdef MAEMO_CHANGES
        hildon_gtk_im_context_filter_event(webViewData->imContext, (GdkEvent*)event);
        hildon_gtk_im_context_show(webViewData->imContext);
#endif
    }

    return frame->eventHandler()->handleMouseReleaseEvent(PlatformMouseEvent(event));
}

static gboolean webkit_web_view_motion_event(GtkWidget* widget, GdkEventMotion* event)
{
    Frame* frame = core(webkit_web_view_get_main_frame(WEBKIT_WEB_VIEW(widget)));
    return frame->eventHandler()->mouseMoved(PlatformMouseEvent(event));
}

static gboolean webkit_web_view_scroll_event(GtkWidget* widget, GdkEventScroll* event)
{
    Frame* frame = core(webkit_web_view_get_main_frame(WEBKIT_WEB_VIEW(widget)));
    PlatformWheelEvent wheelEvent(event);
    return frame->eventHandler()->handleWheelEvent(wheelEvent);
}

static void webkit_web_view_size_allocate(GtkWidget* widget, GtkAllocation* allocation)
{
    GTK_WIDGET_CLASS(webkit_web_view_parent_class)->size_allocate(widget,allocation);

    Frame* frame = core(webkit_web_view_get_main_frame(WEBKIT_WEB_VIEW(widget)));
    frame->view()->resize(allocation->width, allocation->height);
    frame->forceLayout();
    frame->view()->adjustViewSize();
}

static void webkit_web_view_realize(GtkWidget* widget)
{
    GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);

    GdkWindowAttr attributes;
    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.x = widget->allocation.x;
    attributes.y = widget->allocation.y;
    attributes.width = widget->allocation.width;
    attributes.height = widget->allocation.height;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.visual = gtk_widget_get_visual (widget);
    attributes.colormap = gtk_widget_get_colormap (widget);
    attributes.event_mask = GDK_VISIBILITY_NOTIFY_MASK
                            | GDK_EXPOSURE_MASK
                            | GDK_BUTTON_PRESS_MASK
                            | GDK_BUTTON_RELEASE_MASK
                            | GDK_POINTER_MOTION_MASK
                            | GDK_KEY_PRESS_MASK
                            | GDK_KEY_RELEASE_MASK
                            | GDK_BUTTON_MOTION_MASK
                            | GDK_BUTTON1_MOTION_MASK
                            | GDK_BUTTON2_MOTION_MASK
                            | GDK_BUTTON3_MOTION_MASK;

    gint attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
    widget->window = gdk_window_new(gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
    gdk_window_set_user_data(widget->window, widget);
}

static void webkit_web_view_set_scroll_adjustments(WebKitWebView* webView, GtkAdjustment* hadj, GtkAdjustment* vadj)
{
    FrameView* view = core(webkit_web_view_get_main_frame(webView))->view();
    view->setGtkAdjustments(hadj, vadj);
}

static void webkit_web_view_container_add(GtkContainer* container, GtkWidget* widget)
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(container);
    WebKitWebViewPrivate* private_data = WEBKIT_WEB_VIEW_GET_PRIVATE(webView);

    private_data->children.add(widget);
    if (GTK_WIDGET_REALIZED(container))
        gtk_widget_set_parent_window(widget, GTK_WIDGET(webView)->window);
    gtk_widget_set_parent(widget, GTK_WIDGET(container));
}

static void webkit_web_view_container_remove(GtkContainer* container, GtkWidget* widget)
{
    WebKitWebViewPrivate* private_data = WEBKIT_WEB_VIEW_GET_PRIVATE(WEBKIT_WEB_VIEW(container));

    if (private_data->children.contains(widget)) {
        gtk_widget_unparent(widget);
        private_data->children.remove(widget);
    }
}

static void webkit_web_view_container_forall(GtkContainer* container, gboolean, GtkCallback callback, gpointer callbackData)
{
    WebKitWebViewPrivate* privateData = WEBKIT_WEB_VIEW_GET_PRIVATE(WEBKIT_WEB_VIEW(container));

    HashSet<GtkWidget*> children = privateData->children;
    HashSet<GtkWidget*>::const_iterator end = children.end();
    for (HashSet<GtkWidget*>::const_iterator current = children.begin(); current != end; ++current)
        (*callback)(*current, callbackData);
}

static WebKitWebView* webkit_web_view_real_create_web_view(WebKitWebView*)
{
    notImplemented();
    return 0;
}

static WebKitNavigationResponse webkit_web_view_real_navigation_requested(WebKitWebView*, WebKitWebFrame* frame, WebKitNetworkRequest*)
{
    notImplemented();
    return WEBKIT_NAVIGATION_RESPONSE_ACCEPT;
}

static void webkit_web_view_real_window_object_cleared(WebKitWebView*, WebKitWebFrame*, JSGlobalContextRef context, JSObjectRef window_object)
{
    notImplemented();
}

static gchar* webkit_web_view_real_choose_file(WebKitWebView*, WebKitWebFrame*, const gchar* old_name)
{
    notImplemented();
    return g_strdup(old_name);
}

typedef enum {
    WEBKIT_SCRIPT_DIALOG_ALERT,
    WEBKIT_SCRIPT_DIALOG_CONFIRM,
    WEBKIT_SCRIPT_DIALOG_PROMPT
 } WebKitScriptDialogType;

static gboolean webkit_web_view_script_dialog(WebKitWebView* webView, WebKitWebFrame* frame, const gchar* message, WebKitScriptDialogType type, const gchar* defaultValue, gchar** value)
{
    GtkMessageType messageType;
    GtkButtonsType buttons;
    gint defaultResponse;
    GtkWidget* window;
    GtkWidget* dialog;
    GtkWidget* entry = 0;
    gboolean didConfirm = FALSE;

    switch (type) {
    case WEBKIT_SCRIPT_DIALOG_ALERT:
        messageType = GTK_MESSAGE_WARNING;
        buttons = GTK_BUTTONS_CLOSE;
        defaultResponse = GTK_RESPONSE_CLOSE;
        break;
    case WEBKIT_SCRIPT_DIALOG_CONFIRM:
        messageType = GTK_MESSAGE_QUESTION;
        buttons = GTK_BUTTONS_YES_NO;
        defaultResponse = GTK_RESPONSE_YES;
        break;
    case WEBKIT_SCRIPT_DIALOG_PROMPT:
        messageType = GTK_MESSAGE_QUESTION;
        buttons = GTK_BUTTONS_OK_CANCEL;
        defaultResponse = GTK_RESPONSE_OK;
        break;
    default:
        g_warning("Unknown value for WebKitScriptDialogType.");
        return FALSE;
    }

    window = gtk_widget_get_toplevel(GTK_WIDGET(webView));
    dialog = gtk_message_dialog_new(GTK_WIDGET_TOPLEVEL(window) ? GTK_WINDOW(window) : 0, GTK_DIALOG_DESTROY_WITH_PARENT, messageType, buttons, "%s", message);
    gchar* title = g_strconcat("JavaScript - ", webkit_web_frame_get_uri(frame), NULL);
    gtk_window_set_title(GTK_WINDOW(dialog), title);
    g_free(title);

    if (type == WEBKIT_SCRIPT_DIALOG_PROMPT) {
        entry = gtk_entry_new();
        gtk_entry_set_text(GTK_ENTRY(entry), defaultValue);
        gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), entry);
        gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
        gtk_widget_show(entry);
    }

    gtk_dialog_set_default_response(GTK_DIALOG(dialog), defaultResponse);
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));

    switch (response) {
    case GTK_RESPONSE_YES:
        didConfirm = TRUE;
        break;
    case GTK_RESPONSE_OK:
        didConfirm = TRUE;
        if (entry)
            *value = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
        else
            *value = 0;
        break;
    case GTK_RESPONSE_NO:
    case GTK_RESPONSE_CANCEL:
        didConfirm = FALSE;
        break;

    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
    return didConfirm;
}

static gboolean webkit_web_view_real_script_alert(WebKitWebView* webView, WebKitWebFrame* frame, const gchar* message)
{
    webkit_web_view_script_dialog(webView, frame, message, WEBKIT_SCRIPT_DIALOG_ALERT, 0, 0);
    return TRUE;
}

static gboolean webkit_web_view_real_script_confirm(WebKitWebView* webView, WebKitWebFrame* frame, const gchar* message, gboolean* didConfirm)
{
    *didConfirm = webkit_web_view_script_dialog(webView, frame, message, WEBKIT_SCRIPT_DIALOG_CONFIRM, 0, 0);
    return TRUE;
}

static gboolean webkit_web_view_real_script_prompt(WebKitWebView* webView, WebKitWebFrame* frame, const gchar* message, const gchar* defaultValue, gchar** value)
{
    if (!webkit_web_view_script_dialog(webView, frame, message, WEBKIT_SCRIPT_DIALOG_PROMPT, defaultValue, value))
        *value = NULL;
    return TRUE;
}

static gboolean webkit_web_view_real_console_message(WebKitWebView* webView, const gchar* message, unsigned int line, const gchar* sourceId)
{
    g_print("console message: %s @%d: %s\n", sourceId, line, message);
    return TRUE;
}

static void webkit_web_view_real_select_all(WebKitWebView* webView)
{
    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    frame->editor()->command("SelectAll").execute();
}

static void webkit_web_view_real_cut_clipboard(WebKitWebView* webView)
{
    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    frame->editor()->command("Cut").execute();
}

static void webkit_web_view_real_copy_clipboard(WebKitWebView* webView)
{
    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    frame->editor()->command("Copy").execute();
}

static void webkit_web_view_real_paste_clipboard(WebKitWebView* webView)
{
    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    frame->editor()->command("Paste").execute();
}

static void webkit_web_view_finalize(GObject* object)
{
    webkit_web_view_stop_loading(WEBKIT_WEB_VIEW(object));

    WebKitWebViewPrivate* webViewData = WEBKIT_WEB_VIEW_GET_PRIVATE(WEBKIT_WEB_VIEW(object));
    delete webViewData->corePage;
    delete webViewData->settings;
    g_object_unref(webViewData->mainFrame);
    g_object_unref(webViewData->imContext);
    gtk_target_list_unref(webViewData->copy_target_list);
    gtk_target_list_unref(webViewData->paste_target_list);
    delete webViewData->userAgent;

    G_OBJECT_CLASS(webkit_web_view_parent_class)->finalize(object);
}

static gboolean webkit_navigation_request_handled(GSignalInvocationHint* ihint, GValue* returnAccu, const GValue* handlerReturn, gpointer dummy)
{
  gboolean continueEmission = TRUE;
  int signalHandled = g_value_get_int(handlerReturn);
  g_value_set_int(returnAccu, signalHandled);

  if (signalHandled != WEBKIT_NAVIGATION_RESPONSE_ACCEPT)
      continueEmission = FALSE;

  return continueEmission;
}

static void webkit_web_view_class_init(WebKitWebViewClass* webViewClass)
{
    GtkBindingSet* binding_set;

    webkit_init();

    g_type_class_add_private(webViewClass, sizeof(WebKitWebViewPrivate));

    /*
     * Signals
     */

    webkit_web_view_signals[NAVIGATION_REQUESTED] = g_signal_new("navigation-requested",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            G_STRUCT_OFFSET (WebKitWebViewClass, navigation_requested),
            webkit_navigation_request_handled,
            NULL,
            webkit_marshal_INT__OBJECT_OBJECT,
            G_TYPE_INT, 2,
            G_TYPE_OBJECT,
            G_TYPE_OBJECT);

    /**
     * WebKitWebView::window-object-cleared:
     * @web_view: the object on which the signal is emitted
     * @frame: the #WebKitWebFrame to which @window_object belongs
     * @context: the #JSGlobalContextRef holding the global object and other
     * execution state; equivalent to the return value of
     * webkit_web_frame_get_global_context(@frame)
     *
     * @window_object: the #JSObjectRef representing the frame's JavaScript
     * window object
     *
     * Emitted when the JavaScript window object in a #WebKitWebFrame has been
     * cleared in preparation for a new load. This is the preferred place to
     * set custom properties on the window object using the JavaScriptCore API.
     */
    webkit_web_view_signals[WINDOW_OBJECT_CLEARED] = g_signal_new("window-object-cleared",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            G_STRUCT_OFFSET (WebKitWebViewClass, window_object_cleared),
            NULL,
            NULL,
            webkit_marshal_VOID__OBJECT_POINTER_POINTER,
            G_TYPE_NONE, 3,
            WEBKIT_TYPE_WEB_FRAME,
            G_TYPE_POINTER,
            G_TYPE_POINTER);

    /**
     * WebKitWebView::load-started:
     * @web_view: the object on which the signal is emitted
     * @frame: the frame going to do the load
     *
     * When a #WebKitWebFrame begins to load this signal is emitted.
     */
    webkit_web_view_signals[LOAD_STARTED] = g_signal_new("load-started",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            0,
            NULL,
            NULL,
            g_cclosure_marshal_VOID__OBJECT,
            G_TYPE_NONE, 1,
            WEBKIT_TYPE_WEB_FRAME);

    /**
     * WebKitWebView::load-committed:
     * @web_view: the object on which the signal is emitted
     * @frame: the main frame that received the first data
     *
     * When a #WebKitWebFrame loaded the first data this signal is emitted.
     */
    webkit_web_view_signals[LOAD_COMMITTED] = g_signal_new("load-committed",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            0,
            NULL,
            NULL,
            g_cclosure_marshal_VOID__OBJECT,
            G_TYPE_NONE, 1,
            WEBKIT_TYPE_WEB_FRAME);


    /**
     * WebKitWebView::load-progress-changed:
     * @web_view: the #WebKitWebView
     * @progress: the global progress
     */
    webkit_web_view_signals[LOAD_PROGRESS_CHANGED] = g_signal_new("load-progress-changed",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            0,
            NULL,
            NULL,
            g_cclosure_marshal_VOID__INT,
            G_TYPE_NONE, 1,
            G_TYPE_INT);

    webkit_web_view_signals[LOAD_FINISHED] = g_signal_new("load-finished",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            0,
            NULL,
            NULL,
            g_cclosure_marshal_VOID__OBJECT,
            G_TYPE_NONE, 1,
            WEBKIT_TYPE_WEB_FRAME);

    /**
     * WebKitWebView::title-changed:
     * @web_view: the object on which the signal is emitted
     * @frame: the main frame
     * @title: the new title
     *
     * When a #WebKitWebFrame changes the document title this signal is emitted.
     */
    webkit_web_view_signals[TITLE_CHANGED] = g_signal_new("title-changed",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            0,
            NULL,
            NULL,
            webkit_marshal_VOID__OBJECT_STRING,
            G_TYPE_NONE, 2,
            WEBKIT_TYPE_WEB_FRAME,
            G_TYPE_STRING);

    webkit_web_view_signals[HOVERING_OVER_LINK] = g_signal_new("hovering-over-link",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            0,
            NULL,
            NULL,
            webkit_marshal_VOID__STRING_STRING,
            G_TYPE_NONE, 2,
            G_TYPE_STRING,
            G_TYPE_STRING);

    webkit_web_view_signals[STATUS_BAR_TEXT_CHANGED] = g_signal_new("status-bar-text-changed",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            0,
            NULL,
            NULL,
            g_cclosure_marshal_VOID__STRING,
            G_TYPE_NONE, 1,
            G_TYPE_STRING);

    webkit_web_view_signals[ICOND_LOADED] = g_signal_new("icon-loaded",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            0,
            NULL,
            NULL,
            g_cclosure_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    webkit_web_view_signals[SELECTION_CHANGED] = g_signal_new("selection-changed",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            0,
            NULL,
            NULL,
            g_cclosure_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * WebKitWebView::console-message:
     * @web_view: the object on which the signal is emitted
     * @message: the message text
     * @line: the line where the error occured
     * @source_id: the source id
     * @return: TRUE to stop other handlers from being invoked for the event. FALSE to propagate the event further.
     *
     * A JavaScript console message was created.
     */
    webkit_web_view_signals[CONSOLE_MESSAGE] = g_signal_new("console-message",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            G_STRUCT_OFFSET(WebKitWebViewClass, console_message),
            g_signal_accumulator_true_handled,
            NULL,
            webkit_marshal_BOOLEAN__STRING_INT_STRING,
            G_TYPE_BOOLEAN, 3,
            G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING);

    /**
     * WebKitWebView::script-alert:
     * @web_view: the object on which the signal is emitted
     * @frame: the relevant frame
     * @message: the message text
     * @return: TRUE to stop other handlers from being invoked for the event. FALSE to propagate the event further.
     *
     * A JavaScript alert dialog was created.
     */
    webkit_web_view_signals[SCRIPT_ALERT] = g_signal_new("script-alert",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            G_STRUCT_OFFSET(WebKitWebViewClass, script_alert),
            g_signal_accumulator_true_handled,
            NULL,
            webkit_marshal_BOOLEAN__OBJECT_STRING,
            G_TYPE_BOOLEAN, 2,
            G_TYPE_OBJECT, G_TYPE_STRING);

    /**
     * WebKitWebView::script-confirm:
     * @web_view: the object on which the signal is emitted
     * @frame: the relevant frame
     * @message: the message text
     * @confirmed: whether the dialog has been confirmed
     * @return: TRUE to stop other handlers from being invoked for the event. FALSE to propagate the event further.
     *
     * A JavaScript confirm dialog was created, providing Yes and No buttons.
     */
    webkit_web_view_signals[SCRIPT_CONFIRM] = g_signal_new("script-confirm",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            G_STRUCT_OFFSET(WebKitWebViewClass, script_confirm),
            g_signal_accumulator_true_handled,
            NULL,
            webkit_marshal_BOOLEAN__OBJECT_STRING_BOOLEAN,
            G_TYPE_BOOLEAN, 3,
            G_TYPE_OBJECT, G_TYPE_STRING, G_TYPE_BOOLEAN);

    /**
     * WebKitWebView::script-prompt:
     * @web_view: the object on which the signal is emitted
     * @frame: the relevant frame
     * @message: the message text
     * @default: the default value
     * @text: To be filled with the return value or NULL if the dialog was cancelled.
     * @return: TRUE to stop other handlers from being invoked for the event. FALSE to propagate the event further.
     *
     * A JavaScript prompt dialog was created, providing an entry to input text.
     */
    webkit_web_view_signals[SCRIPT_PROMPT] = g_signal_new("script-prompt",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            G_STRUCT_OFFSET(WebKitWebViewClass, script_prompt),
            g_signal_accumulator_true_handled,
            NULL,
            webkit_marshal_BOOLEAN__OBJECT_STRING_STRING_STRING,
            G_TYPE_BOOLEAN, 4,
            G_TYPE_OBJECT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);

    /**
     * WebKitWebView::select-all:
     * @web_view: the object which received the signal
     *
     * The ::select-all signal is a keybinding signal which gets emitted to
     * select the complete contents of the text view.
     *
     * The default bindings for this signal is Ctrl-a.
     */
    webkit_web_view_signals[SELECT_ALL] = g_signal_new("select-all",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            G_STRUCT_OFFSET(WebKitWebViewClass, select_all),
            NULL, NULL,
            g_cclosure_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * WebKitWebView::cut-clipboard:
     * @web_view: the object which received the signal
     *
     * The ::cut-clipboard signal is a keybinding signal which gets emitted to
     * cut the selection to the clipboard.
     *
     * The default bindings for this signal are Ctrl-x and Shift-Delete.
     */
    webkit_web_view_signals[CUT_CLIPBOARD] = g_signal_new("cut-clipboard",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            G_STRUCT_OFFSET(WebKitWebViewClass, cut_clipboard),
            NULL, NULL,
            g_cclosure_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * WebKitWebView::copy-clipboard:
     * @web_view: the object which received the signal
     *
     * The ::copy-clipboard signal is a keybinding signal which gets emitted to
     * copy the selection to the clipboard.
     *
     * The default bindings for this signal are Ctrl-c and Ctrl-Insert.
     */
    webkit_web_view_signals[COPY_CLIPBOARD] = g_signal_new("copy-clipboard",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            G_STRUCT_OFFSET(WebKitWebViewClass, copy_clipboard),
            NULL, NULL,
            g_cclosure_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * WebKitWebView::paste-clipboard:
     * @web_view: the object which received the signal
     *
     * The ::paste-clipboard signal is a keybinding signal which gets emitted to
     * paste the contents of the clipboard into the Web view.
     *
     * The default bindings for this signal are Ctrl-v and Shift-Insert.
     */
    webkit_web_view_signals[PASTE_CLIPBOARD] = g_signal_new("paste-clipboard",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            G_STRUCT_OFFSET(WebKitWebViewClass, paste_clipboard),
            NULL, NULL,
            g_cclosure_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /*
     * implementations of virtual methods
     */
    webViewClass->create_web_view = webkit_web_view_real_create_web_view;
    webViewClass->navigation_requested = webkit_web_view_real_navigation_requested;
    webViewClass->window_object_cleared = webkit_web_view_real_window_object_cleared;
    webViewClass->choose_file = webkit_web_view_real_choose_file;
    webViewClass->script_alert = webkit_web_view_real_script_alert;
    webViewClass->script_confirm = webkit_web_view_real_script_confirm;
    webViewClass->script_prompt = webkit_web_view_real_script_prompt;
    webViewClass->console_message = webkit_web_view_real_console_message;
    webViewClass->select_all = webkit_web_view_real_select_all;
    webViewClass->cut_clipboard = webkit_web_view_real_cut_clipboard;
    webViewClass->copy_clipboard = webkit_web_view_real_copy_clipboard;
    webViewClass->paste_clipboard = webkit_web_view_real_paste_clipboard;

    G_OBJECT_CLASS(webViewClass)->finalize = webkit_web_view_finalize;

    GObjectClass* objectClass = G_OBJECT_CLASS(webViewClass);
    objectClass->get_property = webkit_web_view_get_property;

    GtkWidgetClass* widgetClass = GTK_WIDGET_CLASS(webViewClass);
    widgetClass->realize = webkit_web_view_realize;
    widgetClass->expose_event = webkit_web_view_expose_event;
    widgetClass->key_press_event = webkit_web_view_key_press_event;
    widgetClass->key_release_event = webkit_web_view_key_release_event;
    widgetClass->button_press_event = webkit_web_view_button_press_event;
    widgetClass->button_release_event = webkit_web_view_button_release_event;
    widgetClass->motion_notify_event = webkit_web_view_motion_event;
    widgetClass->scroll_event = webkit_web_view_scroll_event;
    widgetClass->size_allocate = webkit_web_view_size_allocate;
    widgetClass->popup_menu = webkit_web_view_popup_menu_handler;

    GtkContainerClass* containerClass = GTK_CONTAINER_CLASS(webViewClass);
    containerClass->add = webkit_web_view_container_add;
    containerClass->remove = webkit_web_view_container_remove;
    containerClass->forall = webkit_web_view_container_forall;

    /*
     * make us scrollable (e.g. addable to a GtkScrolledWindow)
     */
    webViewClass->set_scroll_adjustments = webkit_web_view_set_scroll_adjustments;
    GTK_WIDGET_CLASS(webViewClass)->set_scroll_adjustments_signal = g_signal_new("set-scroll-adjustments",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            G_STRUCT_OFFSET(WebKitWebViewClass, set_scroll_adjustments),
            NULL, NULL,
            webkit_marshal_VOID__OBJECT_OBJECT,
            G_TYPE_NONE, 2,
            GTK_TYPE_ADJUSTMENT, GTK_TYPE_ADJUSTMENT);

    /*
     * Key bindings
     */

    binding_set = gtk_binding_set_by_class(webViewClass);

    gtk_binding_entry_add_signal(binding_set, GDK_a, GDK_CONTROL_MASK,
                                 "select_all", 0);

    /* Cut/copy/paste */

    gtk_binding_entry_add_signal(binding_set, GDK_x, GDK_CONTROL_MASK,
                                 "cut_clipboard", 0);
    gtk_binding_entry_add_signal(binding_set, GDK_c, GDK_CONTROL_MASK,
                                 "copy_clipboard", 0);
    gtk_binding_entry_add_signal(binding_set, GDK_v, GDK_CONTROL_MASK,
                                 "paste_clipboard", 0);

    gtk_binding_entry_add_signal(binding_set, GDK_Delete, GDK_SHIFT_MASK,
                                 "cut_clipboard", 0);
    gtk_binding_entry_add_signal(binding_set, GDK_Insert, GDK_CONTROL_MASK,
                                 "copy_clipboard", 0);
    gtk_binding_entry_add_signal(binding_set, GDK_Insert, GDK_SHIFT_MASK,
                                 "paste_clipboard", 0);

    /* Properties */
    GParamFlags flags = (GParamFlags)(G_PARAM_READABLE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB);
    g_object_class_install_property(objectClass, PROP_COPY_TARGET_LIST,
                                    g_param_spec_boxed("copy-target-list",
                                                       "Target list",
                                                       "The list of targets this Web view supports for copying to the clipboard",
                                                       GTK_TYPE_TARGET_LIST,
                                                       flags));

    g_object_class_install_property(objectClass, PROP_PASTE_TARGET_LIST,
                                    g_param_spec_boxed("paste-target-list",
                                                       "Target list",
                                                       "The list of targets this Web view supports for pasting to the clipboard",
                                                       GTK_TYPE_TARGET_LIST,
                                                       flags));
}

static void webkit_web_view_init(WebKitWebView* webView)
{
    WebKitWebViewPrivate* webViewData = WEBKIT_WEB_VIEW_GET_PRIVATE(WEBKIT_WEB_VIEW(webView));
    webViewData->imContext = gtk_im_multicontext_new();
    webViewData->corePage = new Page(new WebKit::ChromeClient(webView), new WebKit::ContextMenuClient, new WebKit::EditorClient(webView), new WebKit::DragClient, new WebKit::InspectorClient);

    Settings* settings = webViewData->corePage->settings();
    settings->setLoadsImagesAutomatically(true);
    settings->setMinimumFontSize(5);
    settings->setDOMPasteAllowed(true);
    settings->setMinimumLogicalFontSize(5);
    settings->setShouldPrintBackgrounds(true);
    settings->setJavaScriptEnabled(true);
    settings->setDefaultFixedFontSize(14);
    settings->setDefaultFontSize(14);
    settings->setSerifFontFamily("Times New Roman");
    settings->setSansSerifFontFamily("Arial");
    settings->setFixedFontFamily("Courier New");
    settings->setStandardFontFamily("Arial");

    GTK_WIDGET_SET_FLAGS(webView, GTK_CAN_FOCUS);
    webViewData->mainFrame = WEBKIT_WEB_FRAME(webkit_web_frame_new(webView));
    webViewData->lastPopupXPosition = webViewData->lastPopupYPosition = -1;
    webViewData->editable = false;

#if GTK_CHECK_VERSION(2,10,0)
    GdkAtom textHtml = gdk_atom_intern_static_string("text/html");
#else
    GdkAtom textHtml = gdk_atom_intern("text/html", false);
#endif
    /* Targets for copy */
    webViewData->copy_target_list = gtk_target_list_new(NULL, 0);
    gtk_target_list_add(webViewData->copy_target_list, textHtml, 0, WEBKIT_WEB_VIEW_TARGET_INFO_HTML);
    gtk_target_list_add_text_targets(webViewData->copy_target_list, WEBKIT_WEB_VIEW_TARGET_INFO_TEXT);

    /* Targets for pasting */
    webViewData->paste_target_list = gtk_target_list_new(NULL, 0);
    gtk_target_list_add(webViewData->paste_target_list, textHtml, 0, WEBKIT_WEB_VIEW_TARGET_INFO_HTML);
    gtk_target_list_add_text_targets(webViewData->paste_target_list, WEBKIT_WEB_VIEW_TARGET_INFO_TEXT);

}

GtkWidget* webkit_web_view_new(void)
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(g_object_new(WEBKIT_TYPE_WEB_VIEW, NULL));

    return GTK_WIDGET(webView);
}

void webkit_web_view_set_settings(WebKitWebView* webView, WebKitSettings* settings)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));
    g_return_if_fail(settings);

    notImplemented();
}

WebKitSettings* webkit_web_view_get_settings(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), NULL);

    notImplemented();
    return NULL;
}

void webkit_web_view_go_backward(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    Frame* frame = core(webkit_web_view_get_main_frame(webView));
    frame->loader()->goBackOrForward(-1);
}

void webkit_web_view_go_forward(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    Frame* frame = core(webkit_web_view_get_main_frame(webView));
    frame->loader()->goBackOrForward(1);
}

gboolean webkit_web_view_can_go_backward(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);

    Frame* frame = core(webkit_web_view_get_main_frame(webView));
    return frame->loader()->canGoBackOrForward(-1);
}

gboolean webkit_web_view_can_go_forward(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);

    Frame* frame = core(webkit_web_view_get_main_frame(webView));
    return frame->loader()->canGoBackOrForward(1);
}

void webkit_web_view_open(WebKitWebView* webView, const gchar* uri)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));
    g_return_if_fail(uri);

    Frame* frame = core(webkit_web_view_get_main_frame(webView));
    DeprecatedString string = DeprecatedString::fromUtf8(uri);
    frame->loader()->load(ResourceRequest(KURL(string)));
}

void webkit_web_view_reload(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    Frame* frame = core(webkit_web_view_get_main_frame(webView));
    frame->loader()->reload();
}

void webkit_web_view_load_string(WebKitWebView* webView, const gchar* content, const gchar* contentMimeType, const gchar* contentEncoding, const gchar* baseUri)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));
    g_return_if_fail(content);

    Frame* frame = core(webkit_web_view_get_main_frame(webView));

    KURL url(baseUri ? DeprecatedString::fromUtf8(baseUri) : "");
    RefPtr<SharedBuffer> sharedBuffer = new SharedBuffer(strdup(content), strlen(content));
    SubstituteData substituteData(sharedBuffer.release(), contentMimeType ? String(contentMimeType) : "text/html", contentEncoding ? String(contentEncoding) : "UTF-8", KURL("about:blank"), url);

    frame->loader()->load(ResourceRequest(url), substituteData);
}

void webkit_web_view_load_html_string(WebKitWebView* webView, const gchar* content, const gchar* baseUri)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));
    g_return_if_fail(content);

    webkit_web_view_load_string(webView, content, NULL, NULL, baseUri);
}

void webkit_web_view_stop_loading(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    Frame* frame = core(webkit_web_view_get_main_frame(webView));

    if (FrameLoader* loader = frame->loader())
        loader->stopAllLoaders();
}

/**
 * webkit_web_view_search_text:
 * @web_view: a #WebKitWebView
 * @text: a string to look for
 * @forward: whether to find forward or not
 * @case_sensitive: whether to respect the case of text
 * @wrap: whether to continue looking at the beginning after reaching the end
 *
 * Looks for a specified string inside #web_view.
 *
 * Return value: %TRUE on success or %FALSE on failure
 */
gboolean webkit_web_view_search_text(WebKitWebView* webView, const gchar* string, gboolean caseSensitive, gboolean forward, gboolean shouldWrap)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);
    g_return_val_if_fail(string, FALSE);

    TextCaseSensitivity caseSensitivity = caseSensitive ? TextCaseSensitive : TextCaseInsensitive;
    FindDirection direction = forward ? FindDirectionForward : FindDirectionBackward;

    WebKitWebViewPrivate* webViewData = WEBKIT_WEB_VIEW_GET_PRIVATE(webView);
    return webViewData->corePage->findString(String::fromUTF8(string), caseSensitivity, direction, shouldWrap);
}

/**
 * webkit_web_view_mark_text_matches:
 * @web_view: a #WebKitWebView
 * @string: a string to look for
 * @case_sensitive: whether to respect the case of text
 * @limit: the maximum number of strings to look for or %0 for all
 *
 * Attempts to highlight all occurances of #string inside #web_view.
 *
 * Return value: the number of strings highlighted
 */
guint webkit_web_view_mark_text_matches(WebKitWebView* webView, const gchar* string, gboolean caseSensitive, guint limit)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), 0);
    g_return_val_if_fail(string, 0);

    TextCaseSensitivity caseSensitivity = caseSensitive ? TextCaseSensitive : TextCaseInsensitive;

    WebKitWebViewPrivate* webViewData = WEBKIT_WEB_VIEW_GET_PRIVATE(webView);
    return webViewData->corePage->markAllMatchesForText(String::fromUTF8(string), caseSensitivity, false, limit);
}

/**
 * webkit_web_view_set_highlight_text_matches:
 * @web_view: a #WebKitWebView
 * @highlight: whether to highlight text matches
 *
 * Highlights text matches previously marked by webkit_web_view_mark_text_matches.
 */
void webkit_web_view_set_highlight_text_matches(WebKitWebView* webView, gboolean shouldHighlight)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    WebKitWebViewPrivate* webViewData = WEBKIT_WEB_VIEW_GET_PRIVATE(webView);
    WebKitWebFramePrivate* frameData = WEBKIT_WEB_FRAME_GET_PRIVATE(webViewData->mainFrame);
    frameData->frame->setMarkedTextMatchesAreHighlighted(shouldHighlight);
}

/**
 * webkit_web_view_unmark_text_matches:
 * @web_view: a #WebKitWebView
 *
 * Removes highlighting previously set by webkit_web_view_mark_text_matches.
 */
void webkit_web_view_unmark_text_matches(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    WebKitWebViewPrivate* webViewData = WEBKIT_WEB_VIEW_GET_PRIVATE(webView);
    return webViewData->corePage->unmarkAllTextMatches();
}

WebKitWebFrame* webkit_web_view_get_main_frame(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), NULL);

    WebKitWebViewPrivate* webViewData = WEBKIT_WEB_VIEW_GET_PRIVATE(webView);
    return webViewData->mainFrame;
}

void webkit_web_view_execute_script(WebKitWebView* webView, const gchar* script)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));
    g_return_if_fail(script);

    Frame* frame = core(webkit_web_view_get_main_frame(webView));
    if (FrameLoader* loader = frame->loader())
        loader->executeScript(String::fromUTF8(script), true);
}

/**
 * webkit_web_view_cut_clipboard:
 * @web_view: a #WebKitWebView
 *
 * Determines whether or not it is currently possible to cut to the clipboard.
 *
 * Return value: %TRUE if a selection can be cut, %FALSE if not
 */
gboolean webkit_web_view_can_cut_clipboard(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);

    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    return frame->editor()->canCut() || frame->editor()->canDHTMLCut();
}

/**
 * webkit_web_view_copy_clipboard:
 * @web_view: a #WebKitWebView
 *
 * Determines whether or not it is currently possible to copy to the clipboard.
 *
 * Return value: %TRUE if a selection can be copied, %FALSE if not
 */
gboolean webkit_web_view_can_copy_clipboard(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);

    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    return frame->editor()->canCopy() || frame->editor()->canDHTMLCopy();
}

/**
 * webkit_web_view_paste_clipboard:
 * @web_view: a #WebKitWebView
 *
 * Determines whether or not it is currently possible to paste from the clipboard.
 *
 * Return value: %TRUE if a selection can be pasted, %FALSE if not
 */
gboolean webkit_web_view_can_paste_clipboard(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);

    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    return frame->editor()->canPaste() || frame->editor()->canDHTMLPaste();
}

/**
 * webkit_web_view_cut_clipboard:
 * @web_view: a #WebKitWebView
 *
 * Cuts the current selection inside the @web_view to the clipboard.
 */
void webkit_web_view_cut_clipboard(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    if (webkit_web_view_can_cut_clipboard(webView))
        g_signal_emit(webView, webkit_web_view_signals[CUT_CLIPBOARD], 0);
}

/**
 * webkit_web_view_copy_clipboard:
 * @web_view: a #WebKitWebView
 *
 * Copies the current selection inside the @web_view to the clipboard.
 */
void webkit_web_view_copy_clipboard(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    if (webkit_web_view_can_copy_clipboard(webView))
        g_signal_emit(webView, webkit_web_view_signals[COPY_CLIPBOARD], 0);
}

/**
 * webkit_web_view_paste_clipboard:
 * @web_view: a #WebKitWebView
 *
 * Pastes the current contents of the clipboard to the @web_view.
 */
void webkit_web_view_paste_clipboard(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    if (webkit_web_view_can_paste_clipboard(webView))
        g_signal_emit(webView, webkit_web_view_signals[PASTE_CLIPBOARD], 0);
}

/**
 * webkit_web_view_delete_selection:
 * @web_view: a #WebKitWebView
 *
 * Deletes the current selection inside the @web_view.
 */
void webkit_web_view_delete_selection(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    frame->editor()->performDelete();
}

/**
 * webkit_web_view_has_selection:
 * @web_view: a #WebKitWebView
 *
 * Determines whether text was selected.
 *
 * Return value: %TRUE if there is selected text, %FALSE if not
 */
gboolean webkit_web_view_has_selection(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);

    WebKitWebViewPrivate* webViewData = WEBKIT_WEB_VIEW_GET_PRIVATE(webView);
    return !webViewData->corePage->selection().isNone();
}

/**
 * webkit_web_view_get_selected_text:
 * @web_view: a #WebKitWebView
 *
 * Retrieves the selected text if any.
 *
 * Return value: a newly allocated string with the selection or %NULL
 */
gchar* webkit_web_view_get_selected_text(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), 0);

    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    return g_strdup(frame->selectedText().utf8().data());
}

/**
 * webkit_web_view_select_all:
 * @web_view: a #WebKitWebView
 *
 * Attempts to select everything inside the @web_view.
 */
void webkit_web_view_select_all(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    g_signal_emit(webView, webkit_web_view_signals[SELECT_ALL], 0);
}

/**
 * webkit_web_view_get_editable:
 * @web_view: a #WebKitWebView
 *
 * Returns whether the user is allowed to edit the document.
 *
 * Returns %TRUE if @web_view allows the user to edit the HTML document, %FALSE if
 * it doesn't. You can change @web_view's document programmatically regardless of
 * this setting.
 *
 * Return value: a #gboolean indicating the editable state
 */
gboolean webkit_web_view_get_editable(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);

    WebKitWebViewPrivate* webViewData = WEBKIT_WEB_VIEW_GET_PRIVATE(webView);
    ASSERT(webViewData);

    return webViewData->editable;
}

/**
 * webkit_web_view_set_editable:
 * @web_view: a #WebKitWebView
 * @flag: a #gboolean indicating the editable state
 *
 * Sets whether @web_view allows the user to edit its HTML document.
 *
 * If @flag is %TRUE, @web_view allows the user to edit the document. If @flag is
 * %FALSE, an element in @web_view's document can only be edited if the
 * CONTENTEDITABLE attribute has been set on the element or one of its parent
 * elements. You can change @web_view's document programmatically regardless of
 * this setting. By default a #WebKitWebView is not editable.

 * Normally, an HTML document is not editable unless the elements within the
 * document are editable. This function provides a low-level way to make the
 * contents of a #WebKitWebView editable without altering the document or DOM
 * structure.
 */
void webkit_web_view_set_editable(WebKitWebView* webView, gboolean flag)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));
    flag = flag != FALSE;

    WebKitWebViewPrivate* webViewData = WEBKIT_WEB_VIEW_GET_PRIVATE(webView);
    ASSERT(webViewData);

    Frame* mainFrame = core(webViewData->mainFrame);
    g_return_if_fail(mainFrame);

    // TODO: What happens when the frame is replaced?
    if (flag == webViewData->editable)
        return;

    webViewData->editable = flag;

    if (flag) {
        mainFrame->applyEditingStyleToBodyElement();
        // TODO: If the WebKitWebView is made editable and the selection is empty, set it to something.
        //if (!webkit_web_view_get_selected_dom_range(webView))
        //    mainFrame->setSelectionFromNone();
    } else
        mainFrame->removeEditingStyleFromBodyElement();
}

/**
 * webkit_web_view_get_copy_target_list:
 * @web_view: a #WebKitWebView
 *
 * This function returns the list of targets this #WebKitWebView can
 * provide for clipboard copying and as DND source. The targets in the list are
 * added with %info values from the #WebKitWebViewTargetInfo enum,
 * using gtk_target_list_add() and
 * gtk_target_list_add_text_targets().
 *
 * Return value: the #GtkTargetList
 **/
GtkTargetList* webkit_web_view_get_copy_target_list(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), NULL);

    WebKitWebViewPrivate* webViewData = WEBKIT_WEB_VIEW_GET_PRIVATE(webView);
    return webViewData->copy_target_list;
}

/**
 * webkit_web_view_get_paste_target_list:
 * @web_view: a #WebKitWebView
 *
 * This function returns the list of targets this #WebKitWebView can
 * provide for clipboard pasting and as DND destination. The targets in the list are
 * added with %info values from the #WebKitWebViewTargetInfo enum,
 * using gtk_target_list_add() and
 * gtk_target_list_add_text_targets().
 *
 * Return value: the #GtkTargetList
 **/
GtkTargetList* webkit_web_view_get_paste_target_list(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), NULL);

    WebKitWebViewPrivate* webViewData = WEBKIT_WEB_VIEW_GET_PRIVATE(webView);
    return webViewData->paste_target_list;
}

}
