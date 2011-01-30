package org.deadbeef.android;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;

public class FileListAdapter extends BaseAdapter
 {
    private Context myContext;

    public FileListAdapter(Context context, int resource, int textViewResourceId) {
        super();
        this.myContext = context;
    }
    
    public int getCount() {
    	return DeadbeefAPI.pl_getcount (0);
    }
    
    public Object	 getItem(int position) {
    	return DeadbeefAPI.pl_get_item_text (position);
    }
    
    public long	 getItemId(int position) {
    	return position;
    }
 
    public boolean	 hasStableIds() {
    	return false;
    }

   public View	 getView(int position, View convertView, ViewGroup parent) {
	    	View v = convertView;
	        if (v == null) {
	            LayoutInflater vi = (LayoutInflater)myContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
	            v = vi.inflate(R.layout.plitem, null);
	        }
	        String title = DeadbeefAPI.pl_get_metadata (position, "title");
	        String artist = DeadbeefAPI.pl_get_metadata (position, "artist");
	        String dur = DeadbeefAPI.pl_get_duration_formatted (position);
	        if (title == null) {
	        	title = "No Title";
	        }
	        if (artist == null) {
	        	artist = "Unknown Artist";
	        }
	        if (dur == null) {
	        	dur = "--:-";
	        }

	        if (v != null) {                                
	            TextView tt = (TextView) v.findViewById(R.id.title);
	            if (tt != null) {
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
	        }

	        return v;
	}

}

