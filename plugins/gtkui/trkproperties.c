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
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include <math.h>
#include "../../gettext.h"
#include "ddblistview.h"
#include "trkproperties.h"
#include "interface.h"
#include "support.h"
#include "../../deadbeef.h"
#include "gtkui.h"
#include "mainplaylist.h"
#include "search.h"
#include "ddbcellrenderertextmultiline.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

static GtkWidget *trackproperties;
static DB_playItem_t *track;
static GtkCellRenderer *rend_text2;
static GtkListStore *store;
static GtkListStore *propstore;
static int trkproperties_modified;

gboolean
on_trackproperties_delete_event        (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    if (trkproperties_modified) {
        GtkWidget *dlg = gtk_message_dialog_new (GTK_WINDOW (mainwin), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, _("You've modified data for this track."));
        gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (trackproperties));
        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg), _("Really close the window?"));
        gtk_window_set_title (GTK_WINDOW (dlg), _("Warning"));

        int response = gtk_dialog_run (GTK_DIALOG (dlg));
        gtk_widget_destroy (dlg);
        if (response != GTK_RESPONSE_YES) {
            return TRUE;
        }
    }
    gtk_widget_destroy (widget);
    rend_text2 = NULL;
    trackproperties = NULL;
    if (track) {
        deadbeef->pl_item_unref (track);
        track = NULL;
    }
    return TRUE;
}

gboolean
on_trackproperties_key_press_event     (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data)
{
    if (event->keyval == GDK_Escape) {
        on_trackproperties_delete_event (trackproperties, NULL, NULL);
        return TRUE;
    }
    return FALSE;
}

void
trkproperties_destroy (void) {
    if (trackproperties) {
        on_trackproperties_delete_event (trackproperties, NULL, NULL);
    }
}

void
on_closebtn_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
    trkproperties_destroy ();
}

void
on_metadata_edited (GtkCellRendererText *renderer, gchar *path, gchar *new_text, gpointer user_data) {
    GtkListStore *store = GTK_LIST_STORE (user_data);
    GtkTreePath *treepath = gtk_tree_path_new_from_string (path);
    GtkTreeIter iter;
    gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, treepath);
    gtk_tree_path_free (treepath);
    GValue value = {0,};
    gtk_tree_model_get_value (GTK_TREE_MODEL (store), &iter, 1, &value);
    const char *svalue = g_value_get_string (&value);
    if (strcmp (svalue, new_text)) {
        gtk_list_store_set (store, &iter, 1, new_text, -1);
        trkproperties_modified = 1;
    }
}

// full metadata
static const char *types[] = {
    "artist", "Artist",
    "title", "Track Title",
    "performer", "Performer",
    "album", "Album",
    "year", "Date",
    "track", "Track Number",
    "numtracks", "Total Tracks",
    "genre", "Genre",
    "composer", "Composer",
    "disc", "Disc Number",
    "comment", "Comment",
    NULL
};

static inline float
amp_to_db (float amp) {
    return 20*log10 (amp);
}

void
trkproperties_fill_metadata (void) {
    if (!trackproperties) {
        return;
    }
    trkproperties_modified = 0;
    gtk_list_store_clear (store);
    deadbeef->pl_lock ();

    // add "standard" fields
    int i = 0;
    while (types[i]) {
        const char *value = deadbeef->pl_find_meta (track, types[i]);
        if (!value) {
            value = "";
        }
        GtkTreeIter iter;
        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter, 0, _(types[i+1]), 1, value, 2, types[i], -1);
        i += 2;
    }

    // properties
    char temp[200];
    GtkTreeIter iter;
    gtk_list_store_clear (propstore);
    gtk_list_store_append (propstore, &iter);
    gtk_list_store_set (propstore, &iter, 0, _("Location"), 1, track->fname, -1);
    gtk_list_store_append (propstore, &iter);
    snprintf (temp, sizeof (temp), "%d", track->tracknum);
    gtk_list_store_set (propstore, &iter, 0, _("Subtrack Index"), 1, temp, -1);
    gtk_list_store_append (propstore, &iter);
    deadbeef->pl_format_time (deadbeef->pl_get_item_duration (track), temp, sizeof (temp));
    gtk_list_store_set (propstore, &iter, 0, _("Duration"), 1, temp, -1);
    gtk_list_store_append (propstore, &iter);
    deadbeef->pl_format_title (track, -1, temp, sizeof (temp), -1, "%T");
    gtk_list_store_set (propstore, &iter, 0, _("Tag Type(s)"), 1, temp, -1);
    gtk_list_store_append (propstore, &iter);
    gtk_list_store_set (propstore, &iter, 0, _("Embedded Cuesheet"), 1, (deadbeef->pl_get_item_flags (track) & DDB_HAS_EMBEDDED_CUESHEET) ? _("Yes") : _("No"), -1);
    gtk_list_store_append (propstore, &iter);
    gtk_list_store_set (propstore, &iter, 0, _("Codec"), 1, track->decoder_id, -1);

    gtk_list_store_append (propstore, &iter);
    snprintf (temp, sizeof (temp), "%0.2f dB", track->replaygain_album_gain);
    gtk_list_store_set (propstore, &iter, 0, "ReplayGain Album Gain", 1, temp, -1);
    gtk_list_store_append (propstore, &iter);
    snprintf (temp, sizeof (temp), "%0.6f", track->replaygain_album_peak);
    gtk_list_store_set (propstore, &iter, 0, "ReplayGain Album Peak", 1, temp, -1);

    gtk_list_store_append (propstore, &iter);
    snprintf (temp, sizeof (temp), "%0.2f dB", track->replaygain_track_gain);
    gtk_list_store_set (propstore, &iter, 0, "ReplayGain Track Gain", 1, temp, -1);
    gtk_list_store_append (propstore, &iter);
    snprintf (temp, sizeof (temp), "%0.6f", track->replaygain_track_peak);
    gtk_list_store_set (propstore, &iter, 0, "ReplayGain Track Peak", 1, temp, -1);

    // unknown fields
    DB_metaInfo_t *meta = deadbeef->pl_get_metadata (track);
    while (meta) {
        if (meta->key[0] == ':') {
            int l = strlen (meta->key)-1;
            char title[l+3];
            snprintf (title, sizeof (title), "<%s>", meta->key+1);
            const char *value = meta->value;

            GtkTreeIter iter;
            gtk_list_store_append (propstore, &iter);
            gtk_list_store_set (propstore, &iter, 0, title, 1, value, -1);
            meta = meta->next;
            continue;
        }
        int i = 0;
        while (types[i]) {
            if (!strcmp (types[i], meta->key)) {
                break;
            }
            i += 2;
        }
        if (types[i]) {
            meta = meta->next;
            continue;
        }

        int l = strlen (meta->key);
        char title[l+3];
        snprintf (title, sizeof (title), "<%s>", meta->key);
        const char *value = meta->value;
        const char *key = meta->key;
        meta = meta->next;

        if (!value) {
            value = "";
        }

        GtkTreeIter iter;
        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter, 0, _(title), 1, value, 2, key, -1);
    }

    deadbeef->pl_unlock ();
}

void
show_track_properties_dlg (DB_playItem_t *it) {
    if (track) {
        deadbeef->pl_item_unref (track);
        track = NULL;
    }
    if (it) {
        deadbeef->pl_item_ref (it);
    }
    track = it;


    int allow_editing = 0;

    int is_subtrack = deadbeef->pl_get_item_flags (it) & DDB_IS_SUBTRACK;

    if (!is_subtrack && deadbeef->is_local_file (it->fname)) {
        // get decoder plugin by id
        DB_decoder_t *dec = NULL;
        if (it->decoder_id) {
            DB_decoder_t **decoders = deadbeef->plug_get_decoder_list ();
            for (int i = 0; decoders[i]; i++) {
                if (!strcmp (decoders[i]->plugin.id, it->decoder_id)) {
                    dec = decoders[i];
                    break;
                }
            }
        }

        if (dec && dec->write_metadata) {
            allow_editing = 1;
        }
    }

    GtkTreeView *tree;
    GtkTreeView *proptree;
    if (!trackproperties) {
        trackproperties = create_trackproperties ();
        gtk_window_set_transient_for (GTK_WINDOW (trackproperties), GTK_WINDOW (mainwin));

        // metadata tree
        tree = GTK_TREE_VIEW (lookup_widget (trackproperties, "metalist"));
        store = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
        gtk_tree_view_set_model (tree, GTK_TREE_MODEL (store));
        GtkCellRenderer *rend_text = gtk_cell_renderer_text_new ();
        rend_text2 = GTK_CELL_RENDERER (ddb_cell_renderer_text_multiline_new ());//gtk_cell_renderer_text_new ();
        if (allow_editing) {
            g_signal_connect ((gpointer)rend_text2, "edited",
                    G_CALLBACK (on_metadata_edited),
                    store);
        }
        GtkTreeViewColumn *col1 = gtk_tree_view_column_new_with_attributes (_("Key"), rend_text, "text", 0, NULL);
        GtkTreeViewColumn *col2 = gtk_tree_view_column_new_with_attributes (_("Value"), rend_text2, "text", 1, NULL);
        gtk_tree_view_append_column (tree, col1);
        gtk_tree_view_append_column (tree, col2);

        // properties tree
        proptree = GTK_TREE_VIEW (lookup_widget (trackproperties, "properties"));
        propstore = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
        gtk_tree_view_set_model (proptree, GTK_TREE_MODEL (propstore));
        GtkCellRenderer *rend_propkey = gtk_cell_renderer_text_new ();
        GtkCellRenderer *rend_propvalue = gtk_cell_renderer_text_new ();
        g_object_set (G_OBJECT (rend_propvalue), "editable", TRUE, NULL);
        col1 = gtk_tree_view_column_new_with_attributes (_("Key"), rend_propkey, "text", 0, NULL);
        col2 = gtk_tree_view_column_new_with_attributes (_("Value"), rend_propvalue, "text", 1, NULL);
        gtk_tree_view_append_column (proptree, col1);
        gtk_tree_view_append_column (proptree, col2);
    }
    else {
        tree = GTK_TREE_VIEW (lookup_widget (trackproperties, "metalist"));
        store = GTK_LIST_STORE (gtk_tree_view_get_model (tree));
        gtk_list_store_clear (store);
        proptree = GTK_TREE_VIEW (lookup_widget (trackproperties, "properties"));
        propstore = GTK_LIST_STORE (gtk_tree_view_get_model (proptree));
        gtk_list_store_clear (propstore);
    }

//    if (allow_editing) {
        g_object_set (G_OBJECT (rend_text2), "editable", TRUE, NULL);
//    }
//    else {
//        g_object_set (G_OBJECT (rend_text2), "editable", FALSE, NULL);
//    }

    GtkWidget *widget = trackproperties;
    GtkWidget *w;
    const char *meta;

    trkproperties_fill_metadata ();

    if (allow_editing) {
        gtk_widget_set_sensitive (lookup_widget (widget, "write_tags"), TRUE);
    }
    else {
        gtk_widget_set_sensitive (lookup_widget (widget, "write_tags"), FALSE);
    }

    gtk_widget_show (widget);
    gtk_window_present (GTK_WINDOW (widget));
}

static gboolean
set_metadata_cb (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data) {
    GValue key = {0,}, value = {0,};
    gtk_tree_model_get_value (model, iter, 2, &key);
    gtk_tree_model_get_value (model, iter, 1, &value);
    const char *skey = g_value_get_string (&key);
    const char *svalue = g_value_get_string (&value);

    deadbeef->pl_replace_meta (DB_PLAYITEM (data), skey, svalue);

    return FALSE;
}

void
on_write_tags_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
    if (!track || !track->decoder_id) {
        return;
    }
    // find decoder
    DB_decoder_t *dec = NULL;
    DB_decoder_t **decoders = deadbeef->plug_get_decoder_list ();
    for (int i = 0; decoders[i]; i++) {
        if (!strcmp (decoders[i]->plugin.id, track->decoder_id)) {
            dec = decoders[i];
            if (dec->write_metadata) {
                // put all metainfo into track
                GtkTreeView *tree = GTK_TREE_VIEW (lookup_widget (trackproperties, "metalist"));
                GtkTreeModel *model = GTK_TREE_MODEL (gtk_tree_view_get_model (tree));
                gtk_tree_model_foreach (model, set_metadata_cb, track);
                dec->write_metadata (track);
                main_refresh ();
                search_refresh ();
            }
            break;
        }
    }
    trkproperties_modified = 0;
}
