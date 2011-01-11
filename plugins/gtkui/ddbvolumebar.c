/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>

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
#include <string.h>
#include "ddbvolumebar.h"
#include "drawing.h"
#include "gtkui.h"
#include "interface.h"
#include "support.h"

#define min(x,y) ((x)<(y)?(x):(y))

G_DEFINE_TYPE (DdbVolumeBar, ddb_volumebar, GTK_TYPE_WIDGET);

static void
ddb_volumebar_send_configure (DdbVolumeBar *darea)
{
  GtkWidget *widget;
  GdkEvent *event = gdk_event_new (GDK_CONFIGURE);

  widget = GTK_WIDGET (darea);

  event->configure.window = g_object_ref (widget->window);
  event->configure.send_event = TRUE;
  event->configure.x = widget->allocation.x;
  event->configure.y = widget->allocation.y;
  event->configure.width = widget->allocation.width;
  event->configure.height = widget->allocation.height;
  
  gtk_widget_event (widget, event);
  gdk_event_free (event);
}

static void
ddb_volumebar_realize (GtkWidget *widget) {
  DdbVolumeBar *darea = DDB_VOLUMEBAR (widget);

  gtk_widget_set_has_window (widget, FALSE);
  GTK_WIDGET_CLASS (ddb_volumebar_parent_class)->realize (widget);

#if 0
  if (GTK_WIDGET_NO_WINDOW (widget))
    {
      GTK_WIDGET_CLASS (ddb_volumebar_parent_class)->realize (widget);
    }
  else
    {
      GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
      gint attributes_mask;
      GdkWindowAttr attributes;

      attributes.window_type = GDK_WINDOW_CHILD;
      attributes.x = widget->allocation.x;
      attributes.y = widget->allocation.y;
      attributes.width = widget->allocation.width;
      attributes.height = widget->allocation.height;
      attributes.wclass = GDK_INPUT_OUTPUT;
      attributes.visual = gtk_widget_get_visual (widget);
      attributes.colormap = gtk_widget_get_colormap (widget);
      attributes.event_mask = gtk_widget_get_events (widget);
      attributes.event_mask |= GDK_EXPOSURE_MASK | GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK;

      attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

      widget->window = gdk_window_new (gtk_widget_get_parent_window (widget),
                                       &attributes, attributes_mask);
      gdk_window_set_user_data (widget->window, darea);

      widget->style = gtk_style_attach (widget->style, widget->window);
      gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);
    }

  ddb_volumebar_send_configure (DDB_VOLUMEBAR (widget));
#endif
}

static void
ddb_volumebar_size_allocate (GtkWidget     *widget,
				GtkAllocation *allocation)
{
  g_return_if_fail (DDB_IS_VOLUMEBAR (widget));
  g_return_if_fail (allocation != NULL);

  widget->allocation = *allocation;

  if (gtk_widget_get_realized (widget))
    {
      if (gtk_widget_get_has_window (widget))
        gdk_window_move_resize (widget->window,
                                allocation->x, allocation->y,
                                allocation->width, allocation->height);

      ddb_volumebar_send_configure (DDB_VOLUMEBAR (widget));
    }
}

gboolean
on_volumebar_expose_event                 (GtkWidget       *widget,
                                        GdkEventExpose  *event);

gboolean
on_volumebar_motion_notify_event       (GtkWidget       *widget,
                                        GdkEventMotion  *event);

gboolean
on_volumebar_button_press_event        (GtkWidget       *widget,
                                        GdkEventButton  *event);

gboolean
on_volumebar_button_release_event      (GtkWidget       *widget,
                                        GdkEventButton  *event);

gboolean
on_volumebar_scroll_event              (GtkWidget       *widget,
                                        GdkEventScroll        *event);

gboolean
on_volumebar_configure_event (GtkWidget *widget, GdkEventConfigure *event);

static void
ddb_volumebar_class_init(DdbVolumeBarClass *class)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  widget_class->realize = ddb_volumebar_realize;
  widget_class->size_allocate = ddb_volumebar_size_allocate;
  widget_class->expose_event = on_volumebar_expose_event;
  widget_class->button_press_event = on_volumebar_button_press_event;
  widget_class->button_release_event = on_volumebar_button_release_event;
  widget_class->motion_notify_event = on_volumebar_motion_notify_event;
  widget_class->scroll_event = on_volumebar_scroll_event;
  widget_class->configure_event = on_volumebar_configure_event;
}

GtkWidget * ddb_volumebar_new() {
    return g_object_new (DDB_TYPE_VOLUMEBAR, NULL);
}

static void
ddb_volumebar_init(DdbVolumeBar *volumebar)
{
    char s[100];
    int db = deadbeef->volume_get_db ();
    snprintf (s, sizeof (s), "%s%ddB", db < 0 ? "" : "+", db);
    gtk_widget_set_tooltip_text (GTK_WIDGET (volumebar), s);
}

void
volumebar_draw (GtkWidget *widget) {
    if (!widget) {
        return;
    }
    GdkDrawable *volumebar_backbuf = GDK_DRAWABLE (widget->window);
    float range = -deadbeef->volume_get_min_db ();
    int n = widget->allocation.width / 4;
    float vol = (range + deadbeef->volume_get_db ()) / range * n;
    float h = 17;

    GdkGC *back_gc = gdk_gc_new (widget->window);
    GdkColor clr;
    gdk_gc_set_rgb_fg_color (back_gc, (gtkui_get_bar_background_color (&clr), &clr));

    GdkGC *front_gc = gdk_gc_new (widget->window);
    gdk_gc_set_rgb_fg_color (front_gc, (gtkui_get_bar_foreground_color (&clr), &clr));

    for (int i = 0; i < n; i++) {
        float iy = (float)i + 3;
        int _x = i * 4;
        int _h = h * iy / n;
        int _y = widget->allocation.height/2-h/2;
        _y += (h - _h);
        int _w = 3;
        if (i < vol) {
            gdk_draw_rectangle (volumebar_backbuf, front_gc, TRUE, _x + widget->allocation.x, _y + widget->allocation.y, _w, _h);
        }
        else {
            gdk_draw_rectangle (volumebar_backbuf, back_gc, TRUE, _x + widget->allocation.x, _y + widget->allocation.y, _w, _h);
        }
    }
#if 0
    if (DDB_VOLUMEBAR (widget)->show_dbs) {
        draw_begin ((uintptr_t)widget->window);
        draw_init_font (widget->style);
        char s[100];
        int db = deadbeef->volume_get_db ();
        snprintf (s, sizeof (s), "%s%ddB", db < 0 ? "" : "+", db);
        draw_text (widget->allocation.x, widget->allocation.y, widget->allocation.width, 0, s);
        gtk_widget_set_tooltip_text (widget, s);
        gtk_widget_trigger_tooltip_query (widget);
        draw_end ();
    }
#endif
    g_object_unref (back_gc);
    g_object_unref (front_gc);
}

gboolean
on_volumebar_expose_event                 (GtkWidget       *widget,
                                        GdkEventExpose  *event)
{
    volumebar_draw (widget);
    return FALSE;
}

gboolean
on_volumebar_motion_notify_event       (GtkWidget       *widget,
                                        GdkEventMotion  *event)
{
    if (event->state & GDK_BUTTON1_MASK) {
        float range = -deadbeef->volume_get_min_db ();
        float volume = (event->x - widget->allocation.x) / widget->allocation.width * range - range;
        if (volume > 0) {
            volume = 0;
        }
        if (volume < -range) {
            volume = -range;
        }
        deadbeef->volume_set_db (volume);
        char s[100];
        int db = volume;
        snprintf (s, sizeof (s), "%s%ddB", db < 0 ? "" : "+", db);
        gtk_widget_set_tooltip_text (widget, s);
        gtk_widget_trigger_tooltip_query (widget);
        gtk_widget_queue_draw (widget);
    }
    return FALSE;
}

gboolean
on_volumebar_button_press_event        (GtkWidget       *widget,
                                        GdkEventButton  *event)
{
    if (event->button == 1) {
        float range = -deadbeef->volume_get_min_db ();
        float volume = (event->x - widget->allocation.x)/ widget->allocation.width * range - range;
        if (volume < -range) {
            volume = -range;
        }
        if (volume > 0) {
            volume = 0;
        }
        deadbeef->volume_set_db (volume);
        char s[100];
        int db = volume;
        snprintf (s, sizeof (s), "%s%ddB", db < 0 ? "" : "+", db);
        gtk_widget_set_tooltip_text (widget, s);
        gtk_widget_trigger_tooltip_query (widget);
//        DDB_VOLUMEBAR (widget)->show_dbs = 1;
        gtk_widget_queue_draw (widget);
    }
    return FALSE;
}


gboolean
on_volumebar_button_release_event      (GtkWidget       *widget,
                                        GdkEventButton  *event)
{
    if (event->button == 1) {
//        DDB_VOLUMEBAR (widget)->show_dbs = 0;
        gtk_widget_queue_draw (widget);
    }
  return FALSE;
}

void
volumebar_notify_changed (void) {
    GtkWidget *widget = lookup_widget (mainwin, "volumebar");
    gtk_widget_queue_draw (widget);
    char s[100];
    int db = deadbeef->volume_get_db ();
    snprintf (s, sizeof (s), "%s%ddB", db < 0 ? "" : "+", db);
    gtk_widget_set_tooltip_text (widget, s);
    gtk_widget_trigger_tooltip_query (widget);
}

gboolean
on_volumebar_scroll_event              (GtkWidget       *widget,
                                        GdkEventScroll        *event)
{
    float range = -deadbeef->volume_get_min_db ();
    float vol = deadbeef->volume_get_db ();
    if (event->direction == GDK_SCROLL_UP || event->direction == GDK_SCROLL_RIGHT) {
        vol += 1;
    }
    else if (event->direction == GDK_SCROLL_DOWN || event->direction == GDK_SCROLL_LEFT) {
        vol -= 1;
    }
    if (vol > 0) {
        vol = 0;
    }
    else if (vol < -range) {
        vol = -range;
    }
    deadbeef->volume_set_db (vol);
    GtkWidget *volumebar = lookup_widget (mainwin, "volumebar");
    gtk_widget_queue_draw (widget);
    char s[100];
    int db = deadbeef->volume_get_db ();
    snprintf (s, sizeof (s), "%s%ddB", db < 0 ? "" : "+", db);
    gtk_widget_set_tooltip_text (widget, s);
    gtk_widget_trigger_tooltip_query (widget);
    return FALSE;
}

gboolean
on_volumebar_configure_event (GtkWidget *widget, GdkEventConfigure *event) {
    gtkui_init_theme_colors ();
    return FALSE;
}
