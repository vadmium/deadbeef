package org.deadbeef_common.android;

import android.app.ListActivity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ArrayAdapter;
import android.widget.Button;

public class TrackPropertiesViewer extends ListActivity {
	
	@Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        Intent i = getIntent ();
        int idx = Integer.parseInt(getIntent().getData().getEncodedFragment ().toString());

        setContentView(R.layout.trackproperties);

        final ArrayAdapter<String> adapter = new ArrayAdapter<String>(this,R.layout.trackproperty,R.id.property);
        
        int trk = DeadbeefAPI.pl_get_for_idx (idx);
        if (0 != trk) {
        	int meta = DeadbeefAPI.pl_get_meta (trk);
        	while (0 != meta) {
        		String key = DeadbeefAPI.meta_get_key (meta); 
        		String val = DeadbeefAPI.meta_get_value (meta);
        		if (key.startsWith(":")) {
        			key = key.substring(1);
        		}
       			adapter.add(key + ": " + val);
       			meta = DeadbeefAPI.meta_get_next (meta);
        	}
        	DeadbeefAPI.pl_item_unref (trk);
        }
        setListAdapter(adapter);
    }
}
