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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
//#include <gdk/gdkkeysyms.h>
#include <string.h>
#include "drawing.h"
#include "support.h"
#include "gtkui.h"

static GdkDrawable *drawable;
static GdkGC *gc;
static GdkColor clrfg;
static GdkColor clrbg;
static int pango_ready;
static PangoContext *pangoctx;
static PangoLayout *pangolayout;

void
draw_begin (uintptr_t canvas) {
    drawable = GDK_DRAWABLE (canvas);
    gc = gdk_gc_new (drawable);
}

void
draw_end (void) {
    drawable = NULL;
    if (gc) {
        g_object_unref (gc);
        gc = NULL;
    }
}

void
draw_copy (uintptr_t dest_canvas, uintptr_t src_canvas, int dx, int dy, int sx, int sy, int w, int h) {
    gdk_draw_drawable (GDK_DRAWABLE (dest_canvas), gc, GDK_DRAWABLE (src_canvas), dx, dy, sx, sy, w, h);
}

void
draw_pixbuf (uintptr_t dest_canvas, uintptr_t pixbuf, int dx, int dy, int sx, int sy, int w, int h) {
    gdk_draw_pixbuf (GDK_DRAWABLE (dest_canvas), gc, GDK_PIXBUF (pixbuf), sx, sy, dx, dy, w, h, GDK_RGB_DITHER_NONE, 0, 0);
}

void
draw_get_canvas_size (uintptr_t canvas, int *w, int *h) {
	gdk_drawable_get_size (GDK_DRAWABLE (canvas), w, h);
}

void
draw_set_fg_color (float *rgb) {
    clrfg.red = rgb[0] * 0xffff;
    clrfg.green = rgb[1] * 0xffff;
    clrfg.blue = rgb[2] * 0xffff;
    gdk_gc_set_rgb_fg_color (gc, &clrfg);
}

void
draw_set_bg_color (float *rgb) {
    clrbg.red = rgb[0] * 0xffff;
    clrbg.green = rgb[1] * 0xffff;
    clrbg.blue = rgb[2] * 0xffff;
    gdk_gc_set_rgb_bg_color (gc, &clrbg);
}

void
draw_line (float x1, float y1, float x2, float y2) {
    gdk_draw_line (drawable, gc, x1, y1, x2, y2);
}

void
draw_rect (float x, float y, float w, float h, int fill) {
    gdk_draw_rectangle (drawable, gc, fill, x, y, w, h);
}

static GtkStyle *font_style = NULL;
static PangoWeight font_weight = PANGO_WEIGHT_NORMAL;

void
draw_free (void) {
    draw_end ();
    if (pangoctx) {
        g_object_unref (pangoctx);
        pangoctx = NULL;
    }
    if (pangolayout) {
        g_object_unref (pangolayout);
        pangolayout = NULL;
    }
}

void
draw_init_font (GtkStyle *new_font_style) {
    if (!pango_ready || (new_font_style && font_style != new_font_style)) {
        if (pangoctx) {
            g_object_unref (pangoctx);
            pangoctx = NULL;
        }
        if (pangolayout) {
            g_object_unref (pangolayout);
            pangolayout = NULL;
        }

        font_style = new_font_style ? new_font_style : gtk_widget_get_default_style ();

        pangoctx = gdk_pango_context_get ();
        pangolayout = pango_layout_new (pangoctx);
        pango_layout_set_ellipsize (pangolayout, PANGO_ELLIPSIZE_END);
        PangoFontDescription *desc = font_style->font_desc;
        font_weight = pango_font_description_get_weight (desc);
        pango_layout_set_font_description (pangolayout, desc);
        pango_ready = 1;
    }
    else if (new_font_style) {
        PangoFontDescription *desc = font_style->font_desc;
        pango_layout_set_font_description (pangolayout, desc);
    }
}

void
draw_init_font_bold (void) {
    PangoFontDescription *desc = pango_font_description_copy (font_style->font_desc);
    pango_font_description_set_weight (desc, PANGO_WEIGHT_BOLD);
    pango_layout_set_font_description (pangolayout, desc);
    pango_font_description_free(desc);
}

void
draw_init_font_normal (void) {
    pango_font_description_set_weight (font_style->font_desc, font_weight);
    pango_layout_set_font_description (pangolayout, font_style->font_desc);
}


float
draw_get_font_size (void) {
    draw_init_font (NULL);
    GdkScreen *screen = gdk_screen_get_default ();
    float dpi = gdk_screen_get_resolution (screen);
    PangoFontDescription *desc = font_style->font_desc;
    return (float)(pango_font_description_get_size (desc) / PANGO_SCALE * dpi / 72);
}

void
draw_text (float x, float y, int width, int align, const char *text) {
    draw_init_font (NULL);
    pango_layout_set_width (pangolayout, width*PANGO_SCALE);
    pango_layout_set_alignment (pangolayout, align ? PANGO_ALIGN_RIGHT : PANGO_ALIGN_LEFT);
    pango_layout_set_text (pangolayout, text, -1);
    gdk_draw_layout (drawable, gc, x, y, pangolayout);
}

void
draw_text_with_colors (float x, float y, int width, int align, const char *text) {
    draw_init_font (NULL);
    pango_layout_set_width (pangolayout, width*PANGO_SCALE);
    pango_layout_set_alignment (pangolayout, align ? PANGO_ALIGN_RIGHT : PANGO_ALIGN_LEFT);
    pango_layout_set_text (pangolayout, text, -1);
    gdk_draw_layout_with_colors (drawable, gc, x, y, pangolayout, &clrfg, &clrbg);
}

void
draw_get_text_extents (const char *text, int len, int *w, int *h) {
    draw_init_font (NULL);
    pango_layout_set_width (pangolayout, 1000 * PANGO_SCALE);
    pango_layout_set_alignment (pangolayout, PANGO_ALIGN_LEFT);
    pango_layout_set_text (pangolayout, text, len);
    PangoRectangle ink;
    PangoRectangle log;
    pango_layout_get_pixel_extents (pangolayout, &ink, &log);
    *w = ink.width;
    *h = ink.height;
}

static GdkColor gtkui_bar_foreground_color;
static GdkColor gtkui_bar_background_color;

static GdkColor gtkui_tabstrip_dark_color;
static GdkColor gtkui_tabstrip_mid_color;
static GdkColor gtkui_tabstrip_light_color;
static GdkColor gtkui_tabstrip_base_color;

static GdkColor gtkui_listview_even_row_color;
static GdkColor gtkui_listview_odd_row_color;
static GdkColor gtkui_listview_selection_color;
static GdkColor gtkui_listview_text_color;
static GdkColor gtkui_listview_selected_text_color;
static GdkColor gtkui_listview_cursor_color;

static int override_listview_colors = 0;
static int override_bar_colors = 0;
static int override_tabstrip_colors = 0;

int
gtkui_override_listview_colors (void) {
    return override_listview_colors;
}

int
gtkui_override_bar_colors (void) {
    return override_bar_colors;
}

int
gtkui_override_tabstrip_colors (void) {
    return override_tabstrip_colors;
}

void
gtkui_init_theme_colors (void) {
    override_listview_colors= deadbeef->conf_get_int ("gtkui.override_listview_colors", 0);
    override_bar_colors = deadbeef->conf_get_int ("gtkui.override_bar_colors", 0);
    override_tabstrip_colors = deadbeef->conf_get_int ("gtkui.override_tabstrip_colors", 0);

    extern GtkWidget *mainwin;
    GtkStyle *style = mainwin->style;
    char color_text[100];
    const char *clr;

    if (!override_bar_colors) {
        memcpy (&gtkui_bar_foreground_color, &style->base[GTK_STATE_SELECTED], sizeof (GdkColor));
        memcpy (&gtkui_bar_background_color, &style->fg[GTK_STATE_NORMAL], sizeof (GdkColor));
    }
    else {
        snprintf (color_text, sizeof (color_text), "%hd %hd %hd", style->base[GTK_STATE_SELECTED].red, style->base[GTK_STATE_SELECTED].green, style->base[GTK_STATE_SELECTED].blue);
        clr = deadbeef->conf_get_str ("gtkui.color.bar_foreground", color_text);
        sscanf (clr, "%hd %hd %hd", &gtkui_bar_foreground_color.red, &gtkui_bar_foreground_color.green, &gtkui_bar_foreground_color.blue);

        snprintf (color_text, sizeof (color_text), "%hd %hd %hd", style->fg[GTK_STATE_NORMAL].red, style->fg[GTK_STATE_NORMAL].green, style->fg[GTK_STATE_NORMAL].blue);
        clr = deadbeef->conf_get_str ("gtkui.color.bar_background", color_text);
        sscanf (clr, "%hd %hd %hd", &gtkui_bar_background_color.red, &gtkui_bar_background_color.green, &gtkui_bar_background_color.blue);

    }

    if (!override_tabstrip_colors) {
        memcpy (&gtkui_tabstrip_dark_color, &style->dark[GTK_STATE_NORMAL], sizeof (GdkColor));
        memcpy (&gtkui_tabstrip_mid_color, &style->mid[GTK_STATE_NORMAL], sizeof (GdkColor));
        memcpy (&gtkui_tabstrip_light_color, &style->light[GTK_STATE_NORMAL], sizeof (GdkColor));
        memcpy (&gtkui_tabstrip_base_color, &style->bg[GTK_STATE_NORMAL], sizeof (GdkColor));
    }
    else {
        snprintf (color_text, sizeof (color_text), "%hd %hd %hd", style->dark[GTK_STATE_NORMAL].red, style->dark[GTK_STATE_NORMAL].green, style->dark[GTK_STATE_NORMAL].blue);
        clr = deadbeef->conf_get_str ("gtkui.color.tabstrip_dark", color_text);
        sscanf (clr, "%hd %hd %hd", &gtkui_tabstrip_dark_color.red, &gtkui_tabstrip_dark_color.green, &gtkui_tabstrip_dark_color.blue);

        snprintf (color_text, sizeof (color_text), "%hd %hd %hd", style->mid[GTK_STATE_NORMAL].red, style->mid[GTK_STATE_NORMAL].green, style->mid[GTK_STATE_NORMAL].blue);
        clr = deadbeef->conf_get_str ("gtkui.color.tabstrip_mid", color_text);
        sscanf (clr, "%hd %hd %hd", &gtkui_tabstrip_mid_color.red, &gtkui_tabstrip_mid_color.green, &gtkui_tabstrip_mid_color.blue);

        snprintf (color_text, sizeof (color_text), "%hd %hd %hd", style->light[GTK_STATE_NORMAL].red, style->light[GTK_STATE_NORMAL].green, style->light[GTK_STATE_NORMAL].blue);
        clr = deadbeef->conf_get_str ("gtkui.color.tabstrip_light", color_text);
        sscanf (clr, "%hd %hd %hd", &gtkui_tabstrip_light_color.red, &gtkui_tabstrip_light_color.green, &gtkui_tabstrip_light_color.blue);

        snprintf (color_text, sizeof (color_text), "%hd %hd %hd", style->bg[GTK_STATE_NORMAL].red, style->bg[GTK_STATE_NORMAL].green, style->bg[GTK_STATE_NORMAL].blue);
        clr = deadbeef->conf_get_str ("gtkui.color.tabstrip_base", color_text);
        sscanf (clr, "%hd %hd %hd", &gtkui_tabstrip_base_color.red, &gtkui_tabstrip_base_color.green, &gtkui_tabstrip_base_color.blue);
    }

    if (!override_listview_colors) {
        memcpy (&gtkui_listview_even_row_color, &style->light[GTK_STATE_NORMAL], sizeof (GdkColor));
        memcpy (&gtkui_listview_odd_row_color, &style->mid[GTK_STATE_NORMAL], sizeof (GdkColor));
        memcpy (&gtkui_listview_selection_color, &style->bg[GTK_STATE_SELECTED], sizeof (GdkColor));
        memcpy (&gtkui_listview_text_color, &style->fg[GTK_STATE_NORMAL], sizeof (GdkColor));
        memcpy (&gtkui_listview_selected_text_color, &style->fg[GTK_STATE_SELECTED], sizeof (GdkColor));
        memcpy (&gtkui_listview_cursor_color, &style->fg[GTK_STATE_NORMAL], sizeof (GdkColor));
    }
    else {
        snprintf (color_text, sizeof (color_text), "%hd %hd %hd", style->light[GTK_STATE_NORMAL].red, style->light[GTK_STATE_NORMAL].green, style->light[GTK_STATE_NORMAL].blue);
        clr = deadbeef->conf_get_str ("gtkui.color.listview_even_row", color_text);
        sscanf (clr, "%hd %hd %hd", &gtkui_listview_even_row_color.red, &gtkui_listview_even_row_color.green, &gtkui_listview_even_row_color.blue);

        snprintf (color_text, sizeof (color_text), "%hd %hd %hd", style->mid[GTK_STATE_NORMAL].red, style->mid[GTK_STATE_NORMAL].green, style->mid[GTK_STATE_NORMAL].blue);
        clr = deadbeef->conf_get_str ("gtkui.color.listview_odd_row", color_text);
        sscanf (clr, "%hd %hd %hd", &gtkui_listview_odd_row_color.red, &gtkui_listview_odd_row_color.green, &gtkui_listview_odd_row_color.blue);

        snprintf (color_text, sizeof (color_text), "%hd %hd %hd", style->mid[GTK_STATE_NORMAL].red, style->mid[GTK_STATE_NORMAL].green, style->mid[GTK_STATE_NORMAL].blue);
        clr = deadbeef->conf_get_str ("gtkui.color.listview_selection", color_text);
        sscanf (clr, "%hd %hd %hd", &gtkui_listview_selection_color.red, &gtkui_listview_selection_color.green, &gtkui_listview_selection_color.blue);

        snprintf (color_text, sizeof (color_text), "%hd %hd %hd", style->fg[GTK_STATE_NORMAL].red, style->fg[GTK_STATE_NORMAL].green, style->fg[GTK_STATE_NORMAL].blue);
        clr = deadbeef->conf_get_str ("gtkui.color.listview_text", color_text);
        sscanf (clr, "%hd %hd %hd", &gtkui_listview_text_color.red, &gtkui_listview_text_color.green, &gtkui_listview_text_color.blue);

        snprintf (color_text, sizeof (color_text), "%hd %hd %hd", style->fg[GTK_STATE_SELECTED].red, style->fg[GTK_STATE_SELECTED].green, style->fg[GTK_STATE_SELECTED].blue);
        clr = deadbeef->conf_get_str ("gtkui.color.listview_selected_text", color_text);
        sscanf (clr, "%hd %hd %hd", &gtkui_listview_selected_text_color.red, &gtkui_listview_selected_text_color.green, &gtkui_listview_selected_text_color.blue);

        snprintf (color_text, sizeof (color_text), "%hd %hd %hd", style->fg[GTK_STATE_SELECTED].red, style->fg[GTK_STATE_SELECTED].green, style->fg[GTK_STATE_SELECTED].blue);
        clr = deadbeef->conf_get_str ("gtkui.color.listview_cursor", color_text);
        sscanf (clr, "%hd %hd %hd", &gtkui_listview_cursor_color.red, &gtkui_listview_cursor_color.green, &gtkui_listview_cursor_color.blue);
    }
}

void
gtkui_get_bar_foreground_color (GdkColor *clr) {
    memcpy (clr, &gtkui_bar_foreground_color, sizeof (GdkColor));
}

void
gtkui_get_bar_background_color (GdkColor *clr) {
    memcpy (clr, &gtkui_bar_background_color, sizeof (GdkColor));
}

void
gtkui_get_tabstrip_dark_color (GdkColor *clr) {
    memcpy (clr, &gtkui_tabstrip_dark_color, sizeof (GdkColor));
}

void
gtkui_get_tabstrip_mid_color (GdkColor *clr) {
    memcpy (clr, &gtkui_tabstrip_mid_color, sizeof (GdkColor));
}

void
gtkui_get_tabstrip_light_color (GdkColor *clr) {
    memcpy (clr, &gtkui_tabstrip_light_color, sizeof (GdkColor));
}

void
gtkui_get_tabstrip_base_color (GdkColor *clr) {
    memcpy (clr, &gtkui_tabstrip_base_color, sizeof (GdkColor));
}

void
gtkui_get_listview_even_row_color (GdkColor *clr) {
    memcpy (clr, &gtkui_listview_even_row_color, sizeof (GdkColor));
}

void
gtkui_get_listview_odd_row_color (GdkColor *clr) {
    memcpy (clr, &gtkui_listview_odd_row_color, sizeof (GdkColor));
}

void
gtkui_get_listview_selection_color (GdkColor *clr) {
    memcpy (clr, &gtkui_listview_selection_color, sizeof (GdkColor));
}

void
gtkui_get_listview_text_color (GdkColor *clr) {
    memcpy (clr, &gtkui_listview_text_color, sizeof (GdkColor));
}

void
gtkui_get_listview_selected_text_color (GdkColor *clr) {
    memcpy (clr, &gtkui_listview_selected_text_color, sizeof (GdkColor));
}

void
gtkui_get_listview_cursor_color (GdkColor *clr) {
    memcpy (clr, &gtkui_listview_cursor_color, sizeof (GdkColor));
}
