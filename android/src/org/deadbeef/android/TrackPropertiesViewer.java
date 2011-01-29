package org.deadbeef.android;

import android.app.ListActivity;
import android.app.ProgressDialog;
import android.os.Bundle;
import android.util.Pair;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ListView;

public class TrackPropertiesViewer extends ListActivity {
	
	@Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.trackproperties);

        final ArrayAdapter<String> mAdapter = new ArrayAdapter<String>(this,R.layout.trackproperty,R.id.property);
        
        int trk = DeadbeefAPI.pl_get_for_idx (0);
        if (0 != trk) {
        	int meta = DeadbeefAPI.pl_get_meta (trk);
        	while (0 != meta) {
        		String key = DeadbeefAPI.meta_get_key (meta); 
        		String val = DeadbeefAPI.meta_get_value (meta);
        		if (key.startsWith(":")) {
        			key = "<" + key.substring(1) + ">";
        		}
       			mAdapter.add(key + ": " + val);
       			meta = DeadbeefAPI.meta_get_next (meta);
        	}
        	DeadbeefAPI.pl_item_unref (trk);
        }
        setListAdapter(mAdapter);

        ((Button)findViewById(R.id.close)).setOnClickListener(mCloseListener);
    }

    private OnClickListener mCloseListener = new OnClickListener() {
        public void onClick(View v) {
            finish ();
        }
    };
}
