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
    	return DeadbeefAPI.pl_get_count ();
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
	        String o = DeadbeefAPI.pl_get_item_text (position);

	        if (o != null && v != null) {                                
	            TextView tt = (TextView) v.findViewById(R.id.title);
	            if (tt != null) {
	                tt.setText(o);
	            }
	        }

	        return v;
	}

}

