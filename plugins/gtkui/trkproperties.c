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
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include "trkproperties.h"
#include "interface.h"
#include "support.h"
#include "../../deadbeef.h"
#include "gtkui.h"

static GtkWidget *trackproperties;
static DB_playItem_t *track;

gboolean
on_trackproperties_delete_event        (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    trackproperties = NULL;
    if (track) {
        deadbeef->pl_item_unref (track);
        track = NULL;
    }
    return FALSE;
}

gboolean
on_trackproperties_key_press_event     (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data)
{
    if (event->keyval == GDK_Escape) {
        trackproperties = NULL;
        if (track) {
            deadbeef->pl_item_unref (track);
            track = NULL;
        }
        gtk_widget_destroy (widget);
    }
    return FALSE;
}

void
on_closebtn_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
    if (trackproperties) {
        if (track) {
            deadbeef->pl_item_unref (track);
            track = NULL;
        }
        gtk_widget_destroy (trackproperties);
        trackproperties = NULL;
    }
}

void
on_metadata_edited (GtkCellRendererText *renderer, gchar *path, gchar *new_text, gpointer user_data) {
    GtkListStore *store = GTK_LIST_STORE (user_data);
    GtkTreePath *treepath = gtk_tree_path_new_from_string (path);
    GtkTreeIter iter;
    gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, treepath);
    gtk_tree_path_free (treepath);
    gtk_list_store_set (store, &iter, 1, new_text, -1);
}

// full metadata
static const char *types[] = {
    "artist", "Artist",
    "band", "Band / Album Artist",
    "title", "Track Title",
    "track", "Track Number",
    "album", "Album",
    "genre", "Genre",
    "year", "Date",
    "performer", "Performer",
    "composer", "Composer",
    "numtracks", "Total Tracks",
    "disc", "Disc Number",
    "comment", "Comment",
    "vendor", "Encoded by",
    NULL
};

void
show_track_properties_dlg (DB_playItem_t *it) {
    if (it) {
        deadbeef->pl_item_ref (it);
    }
    track = it;

    if (!trackproperties) {
        trackproperties = create_trackproperties ();
        gtk_window_set_transient_for (GTK_WINDOW (trackproperties), GTK_WINDOW (mainwin));
    }
    GtkWidget *widget = trackproperties;
    GtkWidget *w;
    const char *meta;
    // location
    w = lookup_widget (widget, "location");
    gtk_entry_set_text (GTK_ENTRY (w), it->fname);

    GtkTreeView *tree = GTK_TREE_VIEW (lookup_widget (widget, "metalist"));
    GtkListStore *store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
    GtkCellRenderer *rend_text = gtk_cell_renderer_text_new ();
    GtkCellRenderer *rend_text2 = gtk_cell_renderer_text_new ();
    g_object_set (G_OBJECT (rend_text2), "editable", TRUE, NULL);
    g_signal_connect ((gpointer)rend_text2, "edited",
            G_CALLBACK (on_metadata_edited),
            store);
    GtkTreeViewColumn *col1 = gtk_tree_view_column_new_with_attributes ("Key", rend_text, "text", 0, NULL);
    GtkTreeViewColumn *col2 = gtk_tree_view_column_new_with_attributes ("Value", rend_text2, "text", 1, NULL);
    gtk_tree_view_append_column (tree, col1);
    gtk_tree_view_append_column (tree, col2);

    deadbeef->pl_lock ();
    int i = 0;
    while (types[i]) {
        GtkTreeIter iter;
        gtk_list_store_append (store, &iter);
        const char *value = deadbeef->pl_find_meta (it, types[i]);
        if (!value) {
            value = "";
        }
        gtk_list_store_set (store, &iter, 0, types[i+1], 1, value, -1);
        gtk_tree_view_set_model (tree, GTK_TREE_MODEL (store));
        i += 2;
    }
    deadbeef->pl_unlock ();

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

    if (dec && dec->write_metadata && deadbeef->conf_get_int ("enable_tag_writing", 0)) {
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
    gtk_tree_model_get_value (model, iter, 0, &key);
    gtk_tree_model_get_value (model, iter, 1, &value);
    const char *skey = g_value_get_string (&key);
    const char *svalue = g_value_get_string (&value);


    for (int i = 0; types[i]; i += 2) {
        if (!strcmp (skey, types[i+1])) {
            printf ("setting %s = %s\n", types[i], svalue);
            deadbeef->pl_replace_meta (DB_PLAYITEM (data), types[i], svalue);
        }
    }

    return FALSE;
}

void
on_write_tags_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
    fprintf (stderr, "on_write_tags_clicked\n");
    if (!deadbeef->conf_get_int ("enable_tag_writing", 0)) {
        fprintf (stderr, "tag writing disabled\n");
        return;
    }
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
            }
            break;
        }
    }
#if 0
    DB_id3v2_tag_t tag;
    memset (&tag, 0, sizeof (tag));
    DB_FILE *fp = deadbeef->fopen (track->fname);
    if (fp) {
        if (deadbeef->junk_read_id3v2_full (NULL, &tag, fp) < 0) {
            fprintf (stderr, "failed to read tags from %s\n", track->fname);
            goto error;
        }
        fprintf (stderr, "writing id3v2.%d.%d\n", tag.version[0], tag.version[1]);

#if 0
        // 2.3 editing test
        if (tag.version[0] == 3) {
            // remove frames
            deadbeef->junk_id3v2_remove_frames (&tag, "TPE1");
            deadbeef->junk_id3v2_remove_frames (&tag, "TIT2");

            // add frames
            deadbeef->junk_id3v2_add_text_frame_23 (&tag, "TPE1", "test title");
            deadbeef->junk_id3v2_add_text_frame_23 (&tag, "TIT2", "название на русском");
            if (deadbeef->junk_write_id3v2 (track->fname, &tag) < 0) {
                fprintf (stderr, "failed to write tags to %s\n", track->fname);
                goto error;
            }
        }
#endif
        // 2.4 -> 2.3 conversion test
        if (tag.version[0] == 4) {
            DB_id3v2_tag_t tag23;
            memset (&tag23, 0, sizeof (tag23));
            int res = deadbeef->junk_id3v2_convert_24_to_23 (&tag, &tag23);
            if (res == -1) {
                deadbeef->junk_free_id3v2 (&tag23);
                goto error;
            }
            if (deadbeef->junk_write_id3v2 (track->fname, &tag23) < 0) {
                fprintf (stderr, "failed to write 2.3 tag to %s\n", track->fname);
                deadbeef->junk_free_id3v2 (&tag23);
                goto error;
            }
            deadbeef->junk_free_id3v2 (&tag23);
        }
        else if (tag.version[0] == 3) {
            DB_id3v2_tag_t tag24;
            memset (&tag24, 0, sizeof (tag24));
            int res = deadbeef->junk_id3v2_convert_23_to_24 (&tag, &tag24);
            if (res == -1) {
                deadbeef->junk_free_id3v2 (&tag24);
                goto error;
            }
            if (deadbeef->junk_write_id3v2 (track->fname, &tag24) < 0) {
                fprintf (stderr, "failed to write 2.4 tag to %s\n", track->fname);
                deadbeef->junk_free_id3v2 (&tag24);
                goto error;
            }
            deadbeef->junk_free_id3v2 (&tag24);
        }
        else if (tag.version[0] == 2) {
            DB_id3v2_tag_t tag24;
            memset (&tag24, 0, sizeof (tag24));
            int res = deadbeef->junk_id3v2_convert_22_to_24 (&tag, &tag24);
            if (res == -1) {
                deadbeef->junk_free_id3v2 (&tag24);
                goto error;
            }
            if (deadbeef->junk_write_id3v2 (track->fname, &tag24) < 0) {
                fprintf (stderr, "failed to write 2.4 tag to %s\n", track->fname);
                deadbeef->junk_free_id3v2 (&tag24);
                goto error;
            }
            deadbeef->junk_free_id3v2 (&tag24);
        }

    }
    else {
        fprintf (stderr, "failed to open %s\n", track->fname);
    }
error:
    if (fp) {
        deadbeef->fclose (fp);
    }
    deadbeef->junk_free_id3v2 (&tag);
#endif
}