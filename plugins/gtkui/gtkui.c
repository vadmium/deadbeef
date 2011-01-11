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
#include "../../deadbeef.h"
#include <gtk/gtk.h>
#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <unistd.h>
#include "../../gettext.h"
#include "gtkui.h"
#include "ddblistview.h"
#include "mainplaylist.h"
#include "search.h"
#include "progress.h"
#include "interface.h"
#include "callbacks.h"
#include "support.h"
#include "parser.h"
#include "drawing.h"
#include "trkproperties.h"
#include "../artwork/artwork.h"
#include "coverart.h"
#include "plcommon.h"
#include "ddbtabstrip.h"
#include "eq.h"
#include "actions.h"
#include "pluginconf.h"
#include "gtkui_api.h"

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

static ddb_gtkui_t plugin;
DB_functions_t *deadbeef;

static intptr_t gtk_tid;

// cover art loading plugin
DB_artwork_plugin_t *coverart_plugin = NULL;

// main widgets
GtkWidget *mainwin;
GtkWidget *searchwin;
GtkStatusIcon *trayicon;
GtkWidget *traymenu;

// playlist theming
GtkWidget *theme_treeview;
GtkWidget *theme_button;

int gtkui_embolden_current_track;

#define TRAY_ICON "deadbeef-tray-icon"

// that must be called before gtk_init
void
gtkpl_init (void) {
    theme_treeview = gtk_tree_view_new ();
    GTK_WIDGET_UNSET_FLAGS (theme_treeview, GTK_CAN_FOCUS);
    gtk_widget_show (theme_treeview);
    GtkWidget *vbox1 = lookup_widget (mainwin, "vbox1");
    gtk_box_pack_start (GTK_BOX (vbox1), theme_treeview, FALSE, FALSE, 0);
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (theme_treeview), TRUE);

    theme_button = lookup_widget (mainwin, "stopbtn");
}

void
gtkpl_free (DdbListview *pl) {
#if 0
    if (colhdr_anim.timeline) {
        timeline_free (colhdr_anim.timeline, 1);
        colhdr_anim.timeline = 0;
    }
#endif
}

struct fromto_t {
    DB_playItem_t *from;
    DB_playItem_t *to;
};

static gboolean
update_win_title_idle (gpointer data);
static gboolean
redraw_seekbar_cb (gpointer nothing);

// update status bar and window title
static int sb_context_id = -1;
static char sb_text[512];
static float last_songpos = -1;
static char sbitrate[20] = "";
static struct timeval last_br_update;

static gboolean
update_songinfo (gpointer ctx) {
    int iconified = gdk_window_get_state(mainwin->window) & GDK_WINDOW_STATE_ICONIFIED;
    if (!gtk_widget_get_visible (mainwin) || iconified) {
        return FALSE;
    }
    char sbtext_new[512] = "-";
    float songpos = last_songpos;

    float pl_totaltime = deadbeef->pl_get_totaltime ();
    int daystotal = (int)pl_totaltime / (3600*24);
    int hourtotal = ((int)pl_totaltime / 3600) % 24;
    int mintotal = ((int)pl_totaltime/60) % 60;
    int sectotal = ((int)pl_totaltime) % 60;

    char totaltime_str[512] = "";
    if (daystotal == 0) {
        snprintf (totaltime_str, sizeof (totaltime_str), "%d:%02d:%02d", hourtotal, mintotal, sectotal);
    }
    else if (daystotal == 1) {
        snprintf (totaltime_str, sizeof (totaltime_str), _("1 day %d:%02d:%02d"), hourtotal, mintotal, sectotal);
    }
    else {
        snprintf (totaltime_str, sizeof (totaltime_str), _("%d days %d:%02d:%02d"), daystotal, hourtotal, mintotal, sectotal);
    }

    DB_playItem_t *track = deadbeef->streamer_get_playing_track ();
    DB_fileinfo_t *c = deadbeef->streamer_get_current_fileinfo (); // FIXME: might crash streamer

    float duration = track ? deadbeef->pl_get_item_duration (track) : -1;

    if (deadbeef->get_output ()->state () == OUTPUT_STATE_STOPPED || !track || !c) {
        snprintf (sbtext_new, sizeof (sbtext_new), _("Stopped | %d tracks | %s total playtime"), deadbeef->pl_getcount (PL_MAIN), totaltime_str);
        songpos = 0;
    }
    else {
        float playpos = deadbeef->streamer_get_playpos ();
        int minpos = playpos / 60;
        int secpos = playpos - minpos * 60;
        int mindur = duration / 60;
        int secdur = duration - mindur * 60;

        const char *mode;
        char temp[20];
        if (c->fmt.channels <= 2) {
            mode = c->fmt.channels == 1 ? _("Mono") : _("Stereo");
        }
        else {
            snprintf (temp, sizeof (temp), "%dch Multichannel", c->fmt.channels);
            mode = temp;
        }
        int samplerate = c->fmt.samplerate;
        int bitspersample = c->fmt.bps;
        songpos = playpos;
        //        codec_unlock ();

        char t[100];
        if (duration >= 0) {
            snprintf (t, sizeof (t), "%d:%02d", mindur, secdur);
        }
        else {
            strcpy (t, "-:--");
        }

        struct timeval tm;
        gettimeofday (&tm, NULL);
        if (tm.tv_sec - last_br_update.tv_sec + (tm.tv_usec - last_br_update.tv_usec) / 1000000.0 >= 0.3) {
            memcpy (&last_br_update, &tm, sizeof (tm));
            int bitrate = deadbeef->streamer_get_apx_bitrate ();
            if (bitrate > 0) {
                snprintf (sbitrate, sizeof (sbitrate), _("| %4d kbps "), bitrate);
            }
            else {
                sbitrate[0] = 0;
            }
        }
        const char *spaused = deadbeef->get_output ()->state () == OUTPUT_STATE_PAUSED ? _("Paused | ") : "";
        snprintf (sbtext_new, sizeof (sbtext_new), _("%s%s %s| %dHz | %d bit | %s | %d:%02d / %s | %d tracks | %s total playtime"), spaused, track->filetype ? track->filetype:"-", sbitrate, samplerate, bitspersample, mode, minpos, secpos, t, deadbeef->pl_getcount (PL_MAIN), totaltime_str);
    }

    if (strcmp (sbtext_new, sb_text)) {
        strcpy (sb_text, sbtext_new);

        // form statusline
        // FIXME: don't update if window is not visible
        GtkStatusbar *sb = GTK_STATUSBAR (lookup_widget (mainwin, "statusbar"));
        if (sb_context_id == -1) {
            sb_context_id = gtk_statusbar_get_context_id (sb, "msg");
        }

        gtk_statusbar_pop (sb, sb_context_id);
        gtk_statusbar_push (sb, sb_context_id, sb_text);
    }

    if (mainwin) {
        GtkWidget *widget = lookup_widget (mainwin, "seekbar");
        // translate volume to seekbar pixels
        songpos /= duration;
        songpos *= widget->allocation.width;
        if (fabs (songpos - last_songpos) > 0.01) {
            gtk_widget_queue_draw (widget);
            last_songpos = songpos;
        }
    }
    if (track) {
        deadbeef->pl_item_unref (track);
    }
    return FALSE;
}

void
set_tray_tooltip (const char *text) {
    if (trayicon) {
#if !GTK_CHECK_VERSION(2,16,0) || defined(ULTRA_COMPATIBLE)
        gtk_status_icon_set_tooltip (trayicon, text);
#else
        gtk_status_icon_set_tooltip_text (trayicon, text);
#endif
    }
}

gboolean
on_trayicon_scroll_event               (GtkWidget       *widget,
                                        GdkEventScroll  *event,
                                        gpointer         user_data)
{
    float vol = deadbeef->volume_get_db ();
    int sens = deadbeef->conf_get_int ("gtkui.tray_volume_sensitivity", 1);
    if (event->direction == GDK_SCROLL_UP || event->direction == GDK_SCROLL_RIGHT) {
        vol += sens;
    }
    else if (event->direction == GDK_SCROLL_DOWN || event->direction == GDK_SCROLL_LEFT) {
        vol -= sens;
    }
    if (vol > 0) {
        vol = 0;
    }
    else if (vol < deadbeef->volume_get_min_db ()) {
        vol = deadbeef->volume_get_min_db ();
    }
    deadbeef->volume_set_db (vol);
    volumebar_redraw ();

#if 0
    char str[100];
    if (deadbeef->conf_get_int ("gtkui.show_gain_in_db", 1)) {
        snprintf (str, sizeof (str), "Gain: %s%d dB", vol == 0 ? "+" : "", (int)vol);
    }
    else {
        snprintf (str, sizeof (str), "Gain: %d%%", (int)(deadbeef->volume_get_amp () * 100));
    }
    set_tray_tooltip (str);
#endif

    return FALSE;
}

void
mainwin_toggle_visible (void) {
    int iconified = gdk_window_get_state(mainwin->window) & GDK_WINDOW_STATE_ICONIFIED;
    if (gtk_widget_get_visible (mainwin) && !iconified) {
        gtk_widget_hide (mainwin);
    }
    else {
        int x = deadbeef->conf_get_int ("mainwin.geometry.x", 40);
        int y = deadbeef->conf_get_int ("mainwin.geometry.y", 40);
        int w = deadbeef->conf_get_int ("mainwin.geometry.w", 500);
        int h = deadbeef->conf_get_int ("mainwin.geometry.h", 300);
        gtk_window_move (GTK_WINDOW (mainwin), x, y);
        gtk_window_resize (GTK_WINDOW (mainwin), w, h);
        if (deadbeef->conf_get_int ("mainwin.geometry.maximized", 0)) {
            gtk_window_maximize (GTK_WINDOW (mainwin));
        }
        if (iconified) {
            gtk_window_deiconify (GTK_WINDOW(mainwin));
        }
        else {
            gtk_window_present (GTK_WINDOW (mainwin));
        }
    }
}

#if !GTK_CHECK_VERSION(2,14,0) || defined(ULTRA_COMPATIBLE)

gboolean
on_trayicon_activate (GtkWidget       *widget,
                                        GdkEvent  *event,
                                        gpointer         user_data)
{
    mainwin_toggle_visible ();
    return FALSE;
}

#else

gboolean
on_trayicon_button_press_event (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    if (event->button == 1) {
        mainwin_toggle_visible ();
    }
    else if (event->button == 2) {
        deadbeef->sendmessage (M_TOGGLE_PAUSE, 0, 0, 0);
    }
    return FALSE;
}
#endif

gboolean
on_trayicon_popup_menu (GtkWidget       *widget,
        guint button,
        guint time,
                                        gpointer         user_data)
{
    gtk_menu_popup (GTK_MENU (traymenu), NULL, NULL, gtk_status_icon_position_menu, trayicon, button, time);
    return FALSE;
}

static gboolean
activate_cb (gpointer nothing) {
    gtk_widget_show (mainwin);
    gtk_window_present (GTK_WINDOW (mainwin));
    return FALSE;
}

static int
gtkui_on_activate (DB_event_t *ev, uintptr_t data) {
    g_idle_add (activate_cb, NULL);
    return 0;
}

void
redraw_queued_tracks (DdbListview *pl, int list) {
    DB_playItem_t *it;
    int idx = 0;
    for (it = deadbeef->pl_get_first (PL_MAIN); it; idx++) {
        if (deadbeef->pl_playqueue_test (it) != -1) {
            ddb_listview_draw_row (pl, idx, (DdbListviewIter)it);
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }
}

static gboolean
redraw_queued_tracks_cb (gpointer nothing) {
    int iconified = gdk_window_get_state(mainwin->window) & GDK_WINDOW_STATE_ICONIFIED;
    if (!gtk_widget_get_visible (mainwin) || iconified) {
        return FALSE;
    }
    redraw_queued_tracks (DDB_LISTVIEW (lookup_widget (mainwin, "playlist")), PL_MAIN);
    redraw_queued_tracks (DDB_LISTVIEW (lookup_widget (searchwin, "searchlist")), PL_SEARCH);
    return FALSE;
}

void
gtkpl_songchanged_wrapper (DB_playItem_t *from, DB_playItem_t *to) {
    struct fromto_t *ft = malloc (sizeof (struct fromto_t));
    ft->from = from;
    ft->to = to;
    if (from) {
        deadbeef->pl_item_ref (from);
    }
    if (to) {
        deadbeef->pl_item_ref (to);
    }
    g_idle_add (update_win_title_idle, ft);
    g_idle_add (redraw_seekbar_cb, NULL);
    g_idle_add (redraw_queued_tracks_cb, NULL);
}

static int
gtkui_on_songchanged (DB_event_trackchange_t *ev, uintptr_t data) {
    gtkpl_songchanged_wrapper (ev->from, ev->to);
    return 0;
}

void
gtkui_set_titlebar (DB_playItem_t *it) {
    if (!it) {
        it = deadbeef->streamer_get_playing_track ();
    }
    else {
        deadbeef->pl_item_ref (it);
    }
    char str[600];
    const char *fmt;
    if (it) {
        fmt = deadbeef->conf_get_str ("gtkui.titlebar_playing", "%a - %t - DeaDBeeF-%V");
    }
    else {
        fmt = deadbeef->conf_get_str ("gtkui.titlebar_stopped", "DeaDBeeF-%V");
    }
    deadbeef->pl_format_title (it, -1, str, sizeof (str), -1, fmt);
    gtk_window_set_title (GTK_WINDOW (mainwin), str);
    if (it) {
        deadbeef->pl_item_unref (it);
    }
    set_tray_tooltip (str);
}

static void
trackinfochanged_wrapper (DdbListview *playlist, DB_playItem_t *track, int iter) {
    if (track) {
        int idx = deadbeef->pl_get_idx_of_iter (track, iter);
        if (idx != -1) {
            ddb_listview_draw_row (playlist, idx, (DdbListviewIter)track);
        }
    }
}

static gboolean
trackinfochanged_cb (gpointer data) {
    DB_playItem_t *track = (DB_playItem_t *)data;
    GtkWidget *playlist = lookup_widget (mainwin, "playlist");
    trackinfochanged_wrapper (DDB_LISTVIEW (playlist), track, PL_MAIN);

    if (searchwin && gtk_widget_get_visible (searchwin)) {
        GtkWidget *search = lookup_widget (searchwin, "searchlist");
        trackinfochanged_wrapper (DDB_LISTVIEW (search), track, PL_SEARCH);
    }

    DB_playItem_t *curr = deadbeef->streamer_get_playing_track ();
    if (track == curr) {
        gtkui_set_titlebar (track);
    }
    if (curr) {
        deadbeef->pl_item_unref (curr);
    }
    if (track) {
        deadbeef->pl_item_unref (track);
    }
    return FALSE;
}

static int
gtkui_on_trackinfochanged (DB_event_track_t *ev, uintptr_t data) {
    if (ev->track) {
        deadbeef->pl_item_ref (ev->track);
    }
    g_idle_add (trackinfochanged_cb, ev->track);
    return 0;
}

static gboolean
paused_cb (gpointer nothing) {
    DB_playItem_t *curr = deadbeef->streamer_get_playing_track ();
    if (curr) {
        int idx = deadbeef->pl_get_idx_of (curr);
        GtkWidget *playlist = lookup_widget (mainwin, "playlist");
        ddb_listview_draw_row (DDB_LISTVIEW (playlist), idx, (DdbListviewIter)curr);
        deadbeef->pl_item_unref (curr);
    }
    return FALSE;
}

static int
gtkui_on_paused (DB_event_state_t *ev, uintptr_t data) {
    g_idle_add (paused_cb, NULL);
    return 0;
}

void
playlist_refresh (void) {
    DdbListview *ps = DDB_LISTVIEW (lookup_widget (mainwin, "playlist"));
    ddb_listview_refresh (ps, DDB_REFRESH_LIST | DDB_REFRESH_VSCROLL | DDB_EXPOSE_LIST);
    search_refresh ();
}

static gboolean
playlistchanged_cb (gpointer none) {
    playlist_refresh ();
    return FALSE;
}

void
gtkui_playlist_changed (void) {
    g_idle_add (playlistchanged_cb, NULL);
}

static int
gtkui_on_playlistchanged (DB_event_t *ev, uintptr_t data) {
    gtkui_playlist_changed ();
    return 0;
}

static gboolean
playlistswitch_cb (gpointer none) {
    GtkWidget *tabstrip = lookup_widget (mainwin, "tabstrip");
    int curr = deadbeef->plt_get_curr ();
    char conf[100];
    snprintf (conf, sizeof (conf), "playlist.scroll.%d", curr);
    int scroll = deadbeef->conf_get_int (conf, 0);
    snprintf (conf, sizeof (conf), "playlist.cursor.%d", curr);
    int cursor = deadbeef->conf_get_int (conf, -1);
//    gdk_window_invalidate_rect (tabstrip->window, NULL, FALSE);
    ddb_tabstrip_refresh (DDB_TABSTRIP (tabstrip));
    DdbListview *listview = DDB_LISTVIEW (lookup_widget (mainwin, "playlist"));
    deadbeef->pl_set_cursor (PL_MAIN, cursor);
    if (cursor != -1) {
        DB_playItem_t *it = deadbeef->pl_get_for_idx_and_iter (cursor, PL_MAIN);
        if (it) {
            deadbeef->pl_set_selected (it, 1);
            deadbeef->pl_item_unref (it);
        }
    }
    ddb_listview_clear_sort (listview);
    playlist_refresh ();
    ddb_listview_set_vscroll (listview, scroll);
    search_refresh ();
    return FALSE;
}

static int
gtkui_on_playlistswitch (DB_event_t *ev, uintptr_t data) {
    g_idle_add (playlistswitch_cb, NULL);
    return 0;
}

static int
gtkui_on_frameupdate (DB_event_t *ev, uintptr_t data) {
    g_idle_add (update_songinfo, NULL);

    return 0;
}

static gboolean
gtkui_volumechanged_cb (gpointer ctx) {
    GtkWidget *volumebar = lookup_widget (mainwin, "volumebar");
    gdk_window_invalidate_rect (volumebar->window, NULL, FALSE);
    return FALSE;
}

static int
gtkui_on_volumechanged (DB_event_t *ev, uintptr_t data) {
    g_idle_add (gtkui_volumechanged_cb, NULL);

    return 0;
}

static gboolean
gtkui_update_status_icon (gpointer unused) {
    int hide_tray_icon = deadbeef->conf_get_int ("gtkui.hide_tray_icon", 0);
    if (hide_tray_icon && !trayicon) {
        return FALSE;
    }
    if (trayicon) {
        if (hide_tray_icon) {
            g_object_set (trayicon, "visible", FALSE, NULL);
        }
        else {
            g_object_set (trayicon, "visible", TRUE, NULL);
        }
        return FALSE;
    }
    // system tray icon
    traymenu = create_traymenu ();

    const char *icon_name = deadbeef->conf_get_str ("gtkui.custom_tray_icon", TRAY_ICON);
    GtkIconTheme *theme = gtk_icon_theme_get_default();

    if (!gtk_icon_theme_has_icon(theme, icon_name))
        icon_name = "deadbeef";
    else {
        GtkIconInfo *icon_info = gtk_icon_theme_lookup_icon(theme, icon_name, 48, GTK_ICON_LOOKUP_USE_BUILTIN);
        const gboolean icon_is_builtin = gtk_icon_info_get_filename(icon_info) == NULL;
        gtk_icon_info_free(icon_info);
        icon_name = icon_is_builtin ? "deadbeef" : icon_name;
    }

    if (!gtk_icon_theme_has_icon(theme, icon_name)) {
        char iconpath[1024];
        snprintf (iconpath, sizeof (iconpath), "%s/deadbeef.png", deadbeef->get_prefix ());
        trayicon = gtk_status_icon_new_from_file(iconpath);
    }
    else {
        trayicon = gtk_status_icon_new_from_icon_name(icon_name);
    }
    if (hide_tray_icon) {
        g_object_set (trayicon, "visible", FALSE, NULL);
    }

#if !GTK_CHECK_VERSION(2,14,0) || defined(ULTRA_COMPATIBLE)
    g_signal_connect ((gpointer)trayicon, "activate", G_CALLBACK (on_trayicon_activate), NULL);
#else
    g_signal_connect ((gpointer)trayicon, "scroll_event", G_CALLBACK (on_trayicon_scroll_event), NULL);
    g_signal_connect ((gpointer)trayicon, "button_press_event", G_CALLBACK (on_trayicon_button_press_event), NULL);
#endif
    g_signal_connect ((gpointer)trayicon, "popup_menu", G_CALLBACK (on_trayicon_popup_menu), NULL);

    gtkui_set_titlebar (NULL);

    return FALSE;
}

static void
gtkui_hide_status_icon () {
    if (trayicon) {
        g_object_set (trayicon, "visible", FALSE, NULL);
    }
}

static int
gtkui_on_configchanged (DB_event_t *ev, uintptr_t data) {
    // order and looping
    const char *w;

    // order
    const char *orderwidgets[4] = { "order_linear", "order_shuffle", "order_random", "order_shuffle_albums" };
    w = orderwidgets[deadbeef->conf_get_int ("playback.order", PLAYBACK_ORDER_LINEAR)];
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, w)), TRUE);

    // looping
    const char *loopingwidgets[3] = { "loop_all", "loop_disable", "loop_single" };
    w = loopingwidgets[deadbeef->conf_get_int ("playback.loop", PLAYBACK_MODE_LOOP_ALL)];
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, w)), TRUE);

    // scroll follows playback
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, "scroll_follows_playback")), deadbeef->conf_get_int ("playlist.scroll.followplayback", 0) ? TRUE : FALSE);

    // cursor follows playback
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, "cursor_follows_playback")), deadbeef->conf_get_int ("playlist.scroll.cursorfollowplayback", 0) ? TRUE : FALSE);

    // stop after current
    int stop_after_current = deadbeef->conf_get_int ("playlist.stop_after_current", 0);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, "stop_after_current")), stop_after_current ? TRUE : FALSE);

    // embolden current track
    gtkui_embolden_current_track = deadbeef->conf_get_int ("gtkui.embolden_current_track", 0);

    // tray icon
    g_idle_add (gtkui_update_status_icon, NULL);

    return 0;
}

static gboolean
outputchanged_cb (gpointer nothing) {
    preferences_fill_soundcards ();
    return FALSE;
}

static int
gtkui_on_outputchanged (DB_event_t *ev, uintptr_t nothing) {
    g_idle_add (outputchanged_cb, NULL);
    return 0;
}

char last_playlist_save_name[1024] = "";

void
save_playlist_as (void) {
    GtkWidget *dlg = gtk_file_chooser_dialog_new (_("Save Playlist As"), GTK_WINDOW (mainwin), GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_OK, NULL);

    gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dlg), TRUE);
    gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dlg), "untitled.dbpl");
    // restore folder
    gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (dlg), deadbeef->conf_get_str ("filechooser.playlist.lastdir", ""));

    GtkFileFilter* flt;
    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, _("DeaDBeeF playlist files (*.dbpl)"));
    gtk_file_filter_add_pattern (flt, "*.dbpl");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);

    int res = gtk_dialog_run (GTK_DIALOG (dlg));
    // store folder
    gchar *folder = gtk_file_chooser_get_current_folder_uri (GTK_FILE_CHOOSER (dlg));
    if (folder) {
        deadbeef->conf_set_str ("filechooser.playlist.lastdir", folder);
        g_free (folder);
    }
    if (res == GTK_RESPONSE_OK)
    {
        gchar *fname = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dlg));
        gtk_widget_destroy (dlg);

        if (fname) {
            int res = deadbeef->pl_save (fname);
            if (res >= 0 && strlen (fname) < 1024) {
                strcpy (last_playlist_save_name, fname);
            }
            g_free (fname);
        }
    }
    else {
        gtk_widget_destroy (dlg);
    }
}

void
on_playlist_save_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    if (!last_playlist_save_name[0]) {
        save_playlist_as ();
    }
    else {
        /*int res = */deadbeef->pl_save (last_playlist_save_name);
    }
}


void
on_playlist_save_as_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    save_playlist_as ();
}

static gboolean
playlist_filter_func (const GtkFileFilterInfo *filter_info, gpointer data) {
    // get ext
    const char *p = strrchr (filter_info->filename, '.');
    if (!p) {
        return FALSE;
    }
    p++;
    DB_playlist_t **plug = deadbeef->plug_get_playlist_list ();
    for (int i = 0; plug[i]; i++) {
        if (plug[i]->extensions && plug[i]->load) {
            const char **exts = plug[i]->extensions;
            if (exts) {
                for (int e = 0; exts[e]; e++) {
                    if (!strcasecmp (exts[e], p)) {
                        return TRUE;
                    }
                }
            }
        }
    }
    return FALSE;
}

void
on_playlist_load_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkWidget *dlg = gtk_file_chooser_dialog_new (_("Load Playlist"), GTK_WINDOW (mainwin), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

    // restore folder
    gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (dlg), deadbeef->conf_get_str ("filechooser.playlist.lastdir", ""));

    GtkFileFilter* flt;
    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, "Supported playlist formats");
    gtk_file_filter_add_custom (flt, GTK_FILE_FILTER_FILENAME, playlist_filter_func, NULL, NULL);
    gtk_file_filter_add_pattern (flt, "*.dbpl");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);
    gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dlg), flt);
    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, _("Other files (*)"));
    gtk_file_filter_add_pattern (flt, "*");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);
    
    int res = gtk_dialog_run (GTK_DIALOG (dlg));
    // store folder
    gchar *folder = gtk_file_chooser_get_current_folder_uri (GTK_FILE_CHOOSER (dlg));
    if (folder) {
        deadbeef->conf_set_str ("filechooser.playlist.lastdir", folder);
        g_free (folder);
    }
    if (res == GTK_RESPONSE_OK)
    {
        gchar *fname = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dlg));
        gtk_widget_destroy (dlg);
        if (fname) {
            /*int res = */deadbeef->pl_load (fname);
            g_free (fname);
            main_refresh ();
            search_refresh ();
        }
    }
    else {
        gtk_widget_destroy (dlg);
    }
}

void
on_add_location_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkWidget *dlg = create_addlocationdlg ();
    gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);
    int res = gtk_dialog_run (GTK_DIALOG (dlg));
    if (res == GTK_RESPONSE_OK) {
        GtkEntry *entry = GTK_ENTRY (lookup_widget (dlg, "addlocation_entry"));
        if (entry) {
            const char *text = gtk_entry_get_text (entry);
            if (text) {
                deadbeef->pl_add_file (text, NULL, NULL);
                playlist_refresh ();
            }
        }
    }
    gtk_widget_destroy (dlg);
}

static void
songchanged (DdbListview *ps, DB_playItem_t *from, DB_playItem_t *to) {
    int plt = deadbeef->plt_get_curr ();
    int to_idx = -1;
    if (!ddb_listview_is_scrolling (ps) && to) {
        int cursor_follows_playback = deadbeef->conf_get_int ("playlist.scroll.cursorfollowplayback", 0);
        int scroll_follows_playback = deadbeef->conf_get_int ("playlist.scroll.followplayback", 0);
        int plt = deadbeef->streamer_get_current_playlist ();
        if (plt != -1) {
            if (cursor_follows_playback && plt != deadbeef->plt_get_curr ()) {
                deadbeef->plt_set_curr (plt);
            }
            to_idx = deadbeef->pl_get_idx_of (to);
            if (to_idx != -1) {
                if (cursor_follows_playback) {
                    ddb_listview_set_cursor_noscroll (ps, to_idx);
                }
                if (scroll_follows_playback && plt == deadbeef->plt_get_curr ()) {
                    ddb_listview_scroll_to (ps, to_idx);
                }
            }
        }
    }

    if (from) {
        int idx = deadbeef->pl_get_idx_of (from);
        if (idx != -1) {
            ddb_listview_draw_row (ps, idx, from);
        }
    }
    if (to && to_idx != -1) {
        ddb_listview_draw_row (ps, to_idx, to);
    }
}

static gboolean
update_win_title_idle (gpointer data) {
    struct fromto_t *ft = (struct fromto_t *)data;
    DB_playItem_t *from = ft->from;
    DB_playItem_t *to = ft->to;
    free (ft);

    // update window title
    if (from || to) {
        if (to) {
            DB_playItem_t *it = deadbeef->streamer_get_playing_track ();
            if (it) { // it might have been deleted after event was sent
                gtkui_set_titlebar (it);
                deadbeef->pl_item_unref (it);
            }
            else {
                gtkui_set_titlebar (NULL);
            }
        }
        else {
            gtkui_set_titlebar (NULL);
        }
    }
    // update playlist view
    songchanged (DDB_LISTVIEW (lookup_widget (mainwin, "playlist")), from, to);
    if (from) {
        deadbeef->pl_item_unref (from);
    }
    if (to) {
        deadbeef->pl_item_unref (to);
    }
    return FALSE;
}

static gboolean
redraw_seekbar_cb (gpointer nothing) {
    int iconified = gdk_window_get_state(mainwin->window) & GDK_WINDOW_STATE_ICONIFIED;
    if (!gtk_widget_get_visible (mainwin) || iconified) {
        return FALSE;
    }
    seekbar_redraw ();
    return FALSE;
}

int
gtkui_add_new_playlist (void) {
    int cnt = deadbeef->plt_get_count ();
    int i;
    int idx = 0;
    for (;;) {
        char name[100];
        if (!idx) {
            strcpy (name, _("New Playlist"));
        }
        else {
            snprintf (name, sizeof (name), _("New Playlist (%d)"), idx);
        }
        for (i = 0; i < cnt; i++) {
            char t[100];
            deadbeef->plt_get_title (i, t, sizeof (t));
            if (!strcasecmp (t, name)) {
                break;
            }
        }
        if (i == cnt) {
            return deadbeef->plt_add (cnt, name);
        }
        idx++;
    }
    return -1;
}

void
volumebar_redraw (void) {
    GtkWidget *volumebar = lookup_widget (mainwin, "volumebar");
    gdk_window_invalidate_rect (volumebar->window, NULL, FALSE);
}

void
tabstrip_redraw (void) {
    GtkWidget *ts = lookup_widget (mainwin, "tabstrip");
    ddb_tabstrip_refresh (DDB_TABSTRIP (ts));
}

static int gtk_initialized = 0;

void
gtkui_thread (void *ctx) {
    // let's start some gtk
    g_thread_init (NULL);
//    add_pixmap_directory (PREFIX "/share/deadbeef/pixmaps");
    add_pixmap_directory (deadbeef->get_pixmap_dir ());
    gdk_threads_init ();
    gdk_threads_enter ();

    int argc = 2;
    const char **argv = alloca (sizeof (char *) * argc);
    argv[0] = "deadbeef";
    argv[1] = "--sync";
    //argv[1] = "--g-fatal-warnings";
    if (!deadbeef->conf_get_int ("gtkui.sync", 0)) {
        argc = 1;
    }
    gtk_disable_setlocale ();
    gtk_init (&argc, (char ***)&argv);

    mainwin = create_mainwin ();
    gtkpl_init ();

#if PORTABLE
    char iconpath[1024];
    snprintf (iconpath, sizeof (iconpath), "%s/deadbeef.png", deadbeef->get_prefix ());
    gtk_window_set_icon_from_file (GTK_WINDOW (mainwin), iconpath, NULL);
#else
    gtk_window_set_icon_name (GTK_WINDOW (mainwin), "deadbeef");
#endif

    {
        int x = deadbeef->conf_get_int ("mainwin.geometry.x", 40);
        int y = deadbeef->conf_get_int ("mainwin.geometry.y", 40);
        int w = deadbeef->conf_get_int ("mainwin.geometry.w", 500);
        int h = deadbeef->conf_get_int ("mainwin.geometry.h", 300);
        gtk_window_move (GTK_WINDOW (mainwin), x, y);
        gtk_window_resize (GTK_WINDOW (mainwin), w, h);
        if (deadbeef->conf_get_int ("mainwin.geometry.maximized", 0)) {
            gtk_window_maximize (GTK_WINDOW (mainwin));
        }
    }

    gtkui_on_configchanged (NULL, 0);
    gtkui_init_theme_colors ();

    // visibility of statusbar and headers
    GtkWidget *header_mi = lookup_widget (mainwin, "view_headers");
    GtkWidget *sb_mi = lookup_widget (mainwin, "view_status_bar");
    GtkWidget *ts_mi = lookup_widget (mainwin, "view_tabs");
    GtkWidget *sb = lookup_widget (mainwin, "statusbar");
    GtkWidget *ts = lookup_widget (mainwin, "tabstrip");
    if (deadbeef->conf_get_int ("gtkui.statusbar.visible", 1)) {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (sb_mi), TRUE);
    }
    else {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (sb_mi), FALSE);
        gtk_widget_hide (sb);
    }
    if (deadbeef->conf_get_int ("gtkui.tabs.visible", 1)) {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (ts_mi), TRUE);
    }
    else {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (ts_mi), FALSE);
        gtk_widget_hide (ts);
    }
    searchwin = create_searchwin ();
    gtk_window_set_transient_for (GTK_WINDOW (searchwin), GTK_WINDOW (mainwin));

    DdbListview *main_playlist = DDB_LISTVIEW (lookup_widget (mainwin, "playlist"));
    main_playlist_init (GTK_WIDGET (main_playlist));

    if (deadbeef->conf_get_int ("gtkui.headers.visible", 1)) {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (header_mi), TRUE);
        ddb_listview_show_header (main_playlist, 1);
    }
    else {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (header_mi), FALSE);
        ddb_listview_show_header (main_playlist, 0);
    }

    DdbListview *search_playlist = DDB_LISTVIEW (lookup_widget (searchwin, "searchlist"));
    search_playlist_init (GTK_WIDGET (search_playlist));

    progress_init ();
    cover_art_init ();
    add_mainmenu_actions (lookup_widget (mainwin, "menubar1"));

    gtk_widget_show (mainwin);

    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_ACTIVATE, DB_CALLBACK (gtkui_on_activate), 0);
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_SONGCHANGED, DB_CALLBACK (gtkui_on_songchanged), 0);
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_TRACKINFOCHANGED, DB_CALLBACK (gtkui_on_trackinfochanged), 0);
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_PAUSED, DB_CALLBACK (gtkui_on_paused), 0);
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_PLAYLISTCHANGED, DB_CALLBACK (gtkui_on_playlistchanged), 0);
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_FRAMEUPDATE, DB_CALLBACK (gtkui_on_frameupdate), 0);
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_VOLUMECHANGED, DB_CALLBACK (gtkui_on_volumechanged), 0);
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_CONFIGCHANGED, DB_CALLBACK (gtkui_on_configchanged), 0);
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_OUTPUTCHANGED, DB_CALLBACK (gtkui_on_outputchanged), 0);
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_PLAYLISTSWITCH, DB_CALLBACK (gtkui_on_playlistswitch), 0);

    char str[600];
    deadbeef->pl_format_title (NULL, -1, str, sizeof (str), -1, deadbeef->conf_get_str ("gtkui.titlebar_stopped", "DeaDBeeF-%V"));
    gtk_window_set_title (GTK_WINDOW (mainwin), str);
    gtk_initialized = 1;

    gtk_main ();

    cover_art_free ();
    eq_window_destroy ();
    trkproperties_destroy ();
    progress_destroy ();
    gtkui_hide_status_icon ();
    draw_free ();
    gtk_widget_destroy (mainwin);
    gtk_widget_destroy (searchwin);
    gdk_threads_leave ();
}

gboolean
gtkui_progress_show_idle (gpointer data) {
    progress_show ();
    return FALSE;
}

gboolean
gtkui_set_progress_text_idle (gpointer data) {
    const char *text = (const char *)data;
    progress_settext (text);
    return FALSE;
}

gboolean
gtkui_progress_hide_idle (gpointer data) {
    progress_hide ();
    deadbeef->sendmessage (M_PLAYLIST_REFRESH, 0, 0, 0);
    //playlist_refresh ();
    return FALSE;
}

int
gtkui_add_file_info_cb (DB_playItem_t *it, void *data) {
    if (progress_is_aborted ()) {
        return -1;
    }
    g_idle_add (gtkui_set_progress_text_idle, it->fname);
    return 0;
}

int (*gtkui_original_pl_add_dir) (const char *dirname, int (*cb)(DB_playItem_t *it, void *data), void *user_data);
int (*gtkui_original_pl_add_file) (const char *fname, int (*cb)(DB_playItem_t *it, void *data), void *user_data);
void (*gtkui_original_pl_add_files_begin) (void);
void (*gtkui_original_pl_add_files_end) (void);

int
gtkui_pl_add_dir (const char *dirname, int (*cb)(DB_playItem_t *it, void *data), void *user_data) {
    int res = gtkui_original_pl_add_dir (dirname, gtkui_add_file_info_cb, NULL);
    return res;
}

int
gtkui_pl_add_file (const char *filename, int (*cb)(DB_playItem_t *it, void *data), void *user_data) {
    int res = gtkui_original_pl_add_file (filename, gtkui_add_file_info_cb, NULL);
    return res;
}

void
gtkui_pl_add_files_begin (void) {
    g_idle_add (gtkui_progress_show_idle, NULL);
    gtkui_original_pl_add_files_begin ();
}

void
gtkui_pl_add_files_end (void) {
    g_idle_add (gtkui_progress_hide_idle, NULL);
    gtkui_original_pl_add_files_end ();
}

void
gtkui_focus_on_playing_track (void) {
    DB_playItem_t *it = deadbeef->streamer_get_playing_track ();
    if (it) {
        int plt = deadbeef->streamer_get_current_playlist ();
        if (plt != deadbeef->plt_get_curr ()) {
            deadbeef->plt_set_curr (plt);
        }
        int idx = deadbeef->pl_get_idx_of (it);
        if (idx != -1) {
            DdbListview *pl = DDB_LISTVIEW (lookup_widget (mainwin, "playlist"));
            ddb_listview_scroll_to (pl, idx);
            ddb_listview_set_cursor (pl, idx);
        }
        deadbeef->pl_item_unref (it);
    }
}

void
gtkui_playlist_set_curr (int playlist) {
    deadbeef->plt_set_curr (playlist);
    deadbeef->conf_set_int ("playlist.current", playlist);
}

static int
gtkui_start (void) {
    fprintf (stderr, "gtkui plugin compiled for gtk version: %d.%d.%d\n", GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION);

    // gtk must be running in separate thread
    gtk_initialized = 0;
    gtk_tid = deadbeef->thread_start (gtkui_thread, NULL);
    // wait until gtk finishes initializing
    while (!gtk_initialized) {
        usleep (10000);
    }

    // override default pl_add_dir
    gtkui_original_pl_add_dir = deadbeef->pl_add_dir;
    deadbeef->pl_add_dir = gtkui_pl_add_dir;

    gtkui_original_pl_add_file = deadbeef->pl_add_file;
    deadbeef->pl_add_file = gtkui_pl_add_file;

    gtkui_original_pl_add_files_begin = deadbeef->pl_add_files_begin;
    deadbeef->pl_add_files_begin = gtkui_pl_add_files_begin;

    gtkui_original_pl_add_files_end = deadbeef->pl_add_files_end;
    deadbeef->pl_add_files_end = gtkui_pl_add_files_end;

    return 0;
}

static DB_plugin_t *supereq_plugin;

gboolean
gtkui_connect_cb (void *none) {
    // equalizer
    GtkWidget *eq_mi = lookup_widget (mainwin, "view_eq");
    if (!supereq_plugin) {
        gtk_widget_hide (GTK_WIDGET (eq_mi));
    }
    else {
        if (deadbeef->conf_get_int ("gtkui.eq.visible", 0)) {
            gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (eq_mi), TRUE);
            eq_window_show ();
        }
        else {
            gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (eq_mi), FALSE);
        }
    }

    // cover_art
    DB_plugin_t **plugins = deadbeef->plug_get_list ();
    for (int i = 0; plugins[i]; i++) {
        DB_plugin_t *p = plugins[i];
        if (p->id && !strcmp (p->id, "cover_loader")) {
            trace ("gtkui: found cover-art loader plugin\n");
            coverart_plugin = (DB_artwork_plugin_t *)p;
            break;
        }
    }
    return FALSE;
}

static int
gtkui_connect (void) {
    supereq_plugin = deadbeef->plug_get_for_id ("supereq");
    // need to do it in gtk thread
    g_idle_add (gtkui_connect_cb, NULL);

    return 0;
}


static gboolean
quit_gtk_cb (gpointer nothing) {
    gtk_main_quit ();
    return FALSE;
}

static int
gtkui_stop (void) {
    if (coverart_plugin) {
        coverart_plugin->plugin.plugin.stop ();
        coverart_plugin = NULL;
    }
    trace ("unsubscribing events\n");
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_ACTIVATE, DB_CALLBACK (gtkui_on_activate), 0);
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_SONGCHANGED, DB_CALLBACK (gtkui_on_songchanged), 0);
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_TRACKINFOCHANGED, DB_CALLBACK (gtkui_on_trackinfochanged), 0);
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_PAUSED, DB_CALLBACK (gtkui_on_paused), 0);
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_PLAYLISTCHANGED, DB_CALLBACK (gtkui_on_playlistchanged), 0);
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_FRAMEUPDATE, DB_CALLBACK (gtkui_on_frameupdate), 0);
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_VOLUMECHANGED, DB_CALLBACK (gtkui_on_volumechanged), 0);
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_CONFIGCHANGED, DB_CALLBACK (gtkui_on_configchanged), 0);
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_OUTPUTCHANGED, DB_CALLBACK (gtkui_on_outputchanged), 0);
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_PLAYLISTSWITCH, DB_CALLBACK (gtkui_on_playlistswitch), 0);
    trace ("quitting gtk\n");
    g_idle_add (quit_gtk_cb, NULL);
    trace ("waiting for gtk thread to finish\n");
    deadbeef->thread_join (gtk_tid);
    trace ("gtk thread finished\n");
    gtk_tid = 0;
    main_playlist_free ();
    trace ("gtkui_stop completed\n");
    return 0;
}

GtkWidget *
gtkui_get_mainwin (void) {
    return mainwin;
}

DB_plugin_t *
gtkui_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

static const char settings_dlg[] =
    "property \"Ask confirmation to delete files from disk\" checkbox gtkui.delete_files_ask 1;\n"
    "property \"Status icon volume control sensitivity\" entry gtkui.tray_volume_sensitivity 1;\n"
//    "property \"Show volume in dB (percentage otherwise)\" entry gtkui.show_gain_in_db 1\n"
    "property \"Custom status icon\" entry gtkui.custom_tray_icon \"" TRAY_ICON "\" ;\n"
    "property \"Run gtk_init with --sync (debug mode)\" checkbox gtkui.sync 0;\n"
;

// define plugin interface
static ddb_gtkui_t plugin = {
    .gui.plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .gui.plugin.api_vminor = DB_API_VERSION_MINOR,
    .gui.plugin.version_major = 1,
    .gui.plugin.version_minor = 0,
    .gui.plugin.type = DB_PLUGIN_MISC,
    .gui.plugin.id = "gtkui",
    .gui.plugin.name = "Standard GTK2 user interface",
    .gui.plugin.descr = "Default DeaDBeeF GUI",
    .gui.plugin.author = "Alexey Yakovenko",
    .gui.plugin.email = "waker@users.sourceforge.net",
    .gui.plugin.website = "http://deadbeef.sf.net",
    .gui.plugin.start = gtkui_start,
    .gui.plugin.stop = gtkui_stop,
    .gui.plugin.connect = gtkui_connect,
    .gui.plugin.configdialog = settings_dlg,
    .gui.run_dialog = gtkui_run_dialog_root,
    .get_mainwin = gtkui_get_mainwin,
};
