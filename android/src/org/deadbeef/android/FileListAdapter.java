package org.deadbeef.android;
import org.deadbeef.android.R;

import android.content.Context;
import android.graphics.Typeface;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;
import android.widget.ImageView;
import android.util.Log;

public class FileListAdapter extends BaseAdapter
{
    private Context myContext;
    private int mCurrent = -1;
    private boolean mbPlaying = false;

    public FileListAdapter(Context context, int resource, int textViewResourceId) {
        super();
        this.myContext = context;
    }

    public void updateCurrent (int current, boolean playing) {
     if (mCurrent != current || mbPlaying != playing) {
      mCurrent = current;
      mbPlaying = playing;
         notifyDataSetChanged();
        }
    }

    public int getCount() {
     return DeadbeefAPI.pl_getcount (0);
    }

    public Object getItem(int position) {
     return DeadbeefAPI.pl_get_item_text (position);
    }

    public long getItemId(int position) {
     return position;
    }

    public boolean hasStableIds() {
     return false;
    }

   public View getView(int position, View convertView, ViewGroup parent) {
      View v = convertView;
         if (v == null) {
             LayoutInflater vi = (LayoutInflater)myContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
             v = vi.inflate(R.layout.plitem, null);
         }
         String title = "No title";
         String artist = "";
         String dur = "";
         int trk = DeadbeefAPI.pl_get_for_idx (position);
         if (trk != 0) {
          title = DeadbeefAPI.pl_format_title (trk, position, -1, "%t");
          artist = DeadbeefAPI.pl_format_title (trk, position, -1, "%a");
          dur = DeadbeefAPI.pl_get_duration_formatted (trk);
          DeadbeefAPI.pl_item_unref (trk);
         }

         if (v != null) {
             TextView tt = (TextView) v.findViewById(R.id.title);
             if (tt != null) {
              if (position == mCurrent && mbPlaying) {
               tt.setTypeface (Typeface.DEFAULT, Typeface.BOLD);
              }
              else {
               tt.setTypeface (Typeface.DEFAULT, Typeface.NORMAL);
              }
                 tt.setText(title);
             }
             TextView ta = (TextView) v.findViewById(R.id.artist);
             if (ta != null) {
                 ta.setText(artist);
             }
             TextView td = (TextView) v.findViewById(R.id.duration);
             if (td != null) {
                 td.setText(dur);
             }
             ImageView st = (ImageView)v.findViewById(R.id.status);
             if (st != null) {
              if (position == mCurrent && mbPlaying) {
               st.setImageResource(R.drawable.playing_small);
              }
              else {
               st.setImageResource(-1);
              }
             }
         }

         return v;
 }

}