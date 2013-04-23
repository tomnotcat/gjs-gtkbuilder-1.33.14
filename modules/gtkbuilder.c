/*
 * Copyright (c) 2013  TinySoft, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#include <config.h>

#include <sys/types.h>
#include <unistd.h>

#include <gjs/gjs-module.h>
#include <gi/object.h>
#include <gi/closure.h>
#include <gi/value.h>
#include <jsapi.h>
#include <gtk/gtk.h>

#include "gtkbuilder.h"

typedef struct _builder_ud {
    JSContext *ctx;
    JSObject *obj;
} builder_ud;

static void _gjs_builder_connect_func (GtkBuilder *builder,
                                       GObject *object,
                                       const gchar *signal_name,
                                       const gchar *handler_name,
                                       GObject *connect_object,
                                       GConnectFlags flags,
                                       gpointer user_data)
{
    builder_ud *priv = (builder_ud *)user_data;
    JSContext *ctx = priv->ctx;
    JSObject *obj = priv->obj;
    GClosure *closure;
    JSObject *callable;
    jsval func;

    if (!gjs_object_get_property (ctx, obj, handler_name, &func))
        return;

    if (!JSVAL_IS_OBJECT(func))
        return;

    callable = JSVAL_TO_OBJECT (func);
    if (!JS_ObjectIsFunction(ctx, callable))
        return;

    closure = gjs_closure_new_for_signal (ctx,
                                          callable,
                                          "signal handler (GtkBuilder)",
                                          0);
    if (connect_object != NULL)
        g_object_watch_closure (connect_object, closure);

    g_signal_connect_closure (object, signal_name, closure, FALSE);
}

static JSBool gjs_gtkbuilder_connect_signals (JSContext *context,
                                              uintN      argc,
                                              jsval     *vp)
{
    jsval *argv = JS_ARGV (context, vp);
    JSObject *obj = JS_THIS_OBJECT (context, vp);
    GtkBuilder *builder;
    builder_ud ud;

    builder = GTK_BUILDER (gjs_g_object_from_object (context, obj));
    if (NULL == builder) {
        gjs_throw (context, "Gtk.Builder.connect_signals () invalid this");
        return JS_FALSE;
    }

    if (argc < 1) {
        gjs_throw (context, "Gtk.Builder.connect_signals () takes arguments");
        return JS_FALSE;
    }

    ud.ctx = context;
    ud.obj = JSVAL_TO_OBJECT (argv[0]);

    gtk_builder_connect_signals_full (builder,
                                      _gjs_builder_connect_func,
                                      &ud);

    JS_SET_RVAL(context, vp, JSVAL_VOID);

    return JS_TRUE;
}

JSBool gjs_js_define_gtkbuilder_stuff (JSContext *context,
                                       JSObject  *module)
{
    if (!JS_DefineFunction (context,
                            module,
                            "connect_signals",
                            (JSNative) gjs_gtkbuilder_connect_signals,
                            1,
                            GJS_MODULE_PROP_FLAGS))
    {
        return JS_FALSE;
    }

    return JS_TRUE;
}

GJS_REGISTER_NATIVE_MODULE ("gtkbuilder", gjs_js_define_gtkbuilder_stuff)
