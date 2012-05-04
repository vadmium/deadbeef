/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <drawing.h>
#include <gtkui.h>
#include "support.h"

#define DDB_TYPE_SEEKBAR (ddb_seekbar_get_type ())
#define DDB_SEEKBAR(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), DDB_TYPE_SEEKBAR, DdbSeekbar))
#define DDB_SEEKBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), DDB_TYPE_SEEKBAR, DdbSeekbarClass))
#define DDB_IS_SEEKBAR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DDB_TYPE_SEEKBAR))
#define DDB_IS_SEEKBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), DDB_TYPE_SEEKBAR))
#define DDB_SEEKBAR_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DDB_TYPE_SEEKBAR, DdbSeekbarClass))

typedef struct _DdbSeekbar DdbSeekbar;
typedef struct _DdbSeekbarClass DdbSeekbarClass;
typedef struct _DdbSeekbarPrivate DdbSeekbarPrivate;

struct _DdbSeekbar {
	GtkWidget parent_instance;
	DdbSeekbarPrivate * priv;
};

struct _DdbSeekbarClass {
	GtkWidgetClass parent_class;
};


static gpointer ddb_seekbar_parent_class = NULL;

GType ddb_seekbar_get_type (void) G_GNUC_CONST;
enum  {
	DDB_SEEKBAR_DUMMY_PROPERTY
};
#if GTK_CHECK_VERSION(3,0,0)
static void ddb_seekbar_get_preferred_width (GtkWidget* base, gint *minimal_width, gint *natural_width);
static void ddb_seekbar_get_preferred_height (GtkWidget* base, gint *minimal_height, gint *natural_height);
#else
static gboolean ddb_seekbar_real_expose_event (GtkWidget* base, GdkEventExpose* event);
#endif
static void ddb_seekbar_real_size_request (GtkWidget* base, GtkRequisition* requisition);
static gboolean ddb_seekbar_real_draw (GtkWidget* base, cairo_t *cr);
static gboolean ddb_seekbar_real_button_press_event (GtkWidget* base, GdkEventButton* event);
static gboolean ddb_seekbar_real_button_release_event (GtkWidget* base, GdkEventButton* event);
static gboolean ddb_seekbar_real_motion_notify_event (GtkWidget* base, GdkEventMotion* event);
static gboolean ddb_seekbar_real_configure_event (GtkWidget* base, GdkEventConfigure* event);
DdbSeekbar* ddb_seekbar_new (void);
DdbSeekbar* ddb_seekbar_construct (GType object_type);
static GObject * ddb_seekbar_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);


#if GTK_CHECK_VERSION(3,0,0)
static void ddb_seekbar_get_preferred_width (GtkWidget* widget, gint *minimal_width, gint *natural_width) {
    GtkRequisition requisition;

    ddb_seekbar_real_size_request (widget, &requisition);

    *minimal_width = *natural_width = requisition.width;
}

static void ddb_seekbar_get_preferred_height (GtkWidget* widget, gint *minimal_height, gint *natural_height) {
    GtkRequisition requisition;

    ddb_seekbar_real_size_request (widget, &requisition);

    *minimal_height = *natural_height = requisition.height;
}
#endif

static void ddb_seekbar_real_size_request (GtkWidget* base, GtkRequisition* requisition) {
	DdbSeekbar * self;
	GtkRequisition _vala_requisition = {0};
	self = (DdbSeekbar*) base;
	if (requisition) {
		*requisition = _vala_requisition;
	}
}

static gboolean ddb_seekbar_real_draw (GtkWidget* base, cairo_t *cr) {
	seekbar_draw (base, cr);
	return FALSE;
}

#if !GTK_CHECK_VERSION(3,0,0)
static gboolean ddb_seekbar_real_expose_event (GtkWidget* base, GdkEventExpose* event) {
    cairo_t *cr = gdk_cairo_create (gtk_widget_get_window (base));
    ddb_seekbar_real_draw (base, cr);
    cairo_destroy (cr);
	return FALSE;
}
#endif

static gboolean ddb_seekbar_real_button_press_event (GtkWidget* base, GdkEventButton* event) {
	DdbSeekbar * self;
	gboolean result = FALSE;
	GdkEventButton _tmp0_;
	gboolean _tmp1_ = FALSE;
	self = (DdbSeekbar*) base;
	g_return_val_if_fail (event != NULL, FALSE);
	_tmp0_ = *event;
	_tmp1_ = on_seekbar_button_press_event ((GtkWidget*) self, &_tmp0_);
	result = _tmp1_;
	return result;
}


static gboolean ddb_seekbar_real_button_release_event (GtkWidget* base, GdkEventButton* event) {
	DdbSeekbar * self;
	gboolean result = FALSE;
	GdkEventButton _tmp0_;
	gboolean _tmp1_ = FALSE;
	self = (DdbSeekbar*) base;
	g_return_val_if_fail (event != NULL, FALSE);
	_tmp0_ = *event;
	_tmp1_ = on_seekbar_button_release_event ((GtkWidget*) self, &_tmp0_);
	result = _tmp1_;
	return result;
}


static gboolean ddb_seekbar_real_motion_notify_event (GtkWidget* base, GdkEventMotion* event) {
	DdbSeekbar * self;
	gboolean result = FALSE;
	GdkEventMotion _tmp0_;
	gboolean _tmp1_ = FALSE;
	self = (DdbSeekbar*) base;
	g_return_val_if_fail (event != NULL, FALSE);
	_tmp0_ = *event;
	_tmp1_ = on_seekbar_motion_notify_event ((GtkWidget*) self, &_tmp0_);
	result = _tmp1_;
	return result;
}


static gboolean ddb_seekbar_real_configure_event (GtkWidget* base, GdkEventConfigure* event) {
	DdbSeekbar * self;
	gboolean result = FALSE;
	self = (DdbSeekbar*) base;
	g_return_val_if_fail (event != NULL, FALSE);
	gtkui_init_theme_colors ();
	result = FALSE;
	return result;
}


DdbSeekbar* ddb_seekbar_construct (GType object_type) {
	DdbSeekbar * self = NULL;
	self = (DdbSeekbar*) gtk_widget_new (object_type, NULL);
	return self;
}


DdbSeekbar* ddb_seekbar_new (void) {
	return ddb_seekbar_construct (DDB_TYPE_SEEKBAR);
}


static GObject * ddb_seekbar_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties) {
	GObject * obj;
	GObjectClass * parent_class;
	DdbSeekbar * self;
	parent_class = G_OBJECT_CLASS (ddb_seekbar_parent_class);
	obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	self = DDB_SEEKBAR (obj);
	return obj;
}


static void ddb_seekbar_class_init (DdbSeekbarClass * klass) {
	ddb_seekbar_parent_class = g_type_class_peek_parent (klass);
#if GTK_CHECK_VERSION(3,0,0)
	GTK_WIDGET_CLASS (klass)->get_preferred_width = ddb_seekbar_get_preferred_width;
	GTK_WIDGET_CLASS (klass)->get_preferred_height = ddb_seekbar_get_preferred_height;
	GTK_WIDGET_CLASS (klass)->draw = ddb_seekbar_real_draw;
#else
	GTK_WIDGET_CLASS (klass)->size_request = ddb_seekbar_real_size_request;
	GTK_WIDGET_CLASS (klass)->expose_event = ddb_seekbar_real_expose_event;
#endif
	GTK_WIDGET_CLASS (klass)->button_press_event = ddb_seekbar_real_button_press_event;
	GTK_WIDGET_CLASS (klass)->button_release_event = ddb_seekbar_real_button_release_event;
	GTK_WIDGET_CLASS (klass)->motion_notify_event = ddb_seekbar_real_motion_notify_event;
	GTK_WIDGET_CLASS (klass)->configure_event = ddb_seekbar_real_configure_event;
	G_OBJECT_CLASS (klass)->constructor = ddb_seekbar_constructor;
}


static void ddb_seekbar_instance_init (DdbSeekbar * self) {
	gtk_widget_set_has_window ((GtkWidget*) self, FALSE);
}


GType ddb_seekbar_get_type (void) {
	static volatile gsize ddb_seekbar_type_id__volatile = 0;
	if (g_once_init_enter (&ddb_seekbar_type_id__volatile)) {
		static const GTypeInfo g_define_type_info = { sizeof (DdbSeekbarClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) ddb_seekbar_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (DdbSeekbar), 0, (GInstanceInitFunc) ddb_seekbar_instance_init, NULL };
		GType ddb_seekbar_type_id;
		ddb_seekbar_type_id = g_type_register_static (GTK_TYPE_WIDGET, "DdbSeekbar", &g_define_type_info, 0);
		g_once_init_leave (&ddb_seekbar_type_id__volatile, ddb_seekbar_type_id);
	}
	return ddb_seekbar_type_id__volatile;
}



