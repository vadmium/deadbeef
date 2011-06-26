package org.deadbeef.android;
import org.deadbeefpro.android.R;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;

public class PlaylistListAdapter extends BaseAdapter
 {
    private Context myContext;

    public PlaylistListAdapter(Context context) {
        super();
        this.myContext = context;
    }

    public int getCount() {
     return DeadbeefAPI.plt_get_count()+1;
    }

    public Object getItem(int position) {
     return position-1;
    }

    public long getItemId(int position) {
     return position-1;
    }

    public boolean hasStableIds() {
     return false;
    }

   public View getView(int position, View convertView, ViewGroup parent) {
      View v = convertView;
         if (v == null) {
             LayoutInflater vi = (LayoutInflater)myContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
             v = vi.inflate(R.layout.plt, null);
         }
         
         if (v != null) {
             TextView t1 = (TextView) v.findViewById(R.id.property);
             TextView t2 = (TextView) v.findViewById(R.id.count);
	         if (position == 0) {
	        	 t1.setText ("Add new playlist");
	         }
	         else {
                 t1.setText(DeadbeefAPI.plt_get_title(position-1));
                 int cnt = DeadbeefAPI.plt_get_item_count (position-1);
                 t2.setText(String.format("%d tracks", cnt));
	         }
         }

         return v;
 }

}
