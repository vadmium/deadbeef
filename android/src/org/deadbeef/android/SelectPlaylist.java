package org.deadbeef.android;

import android.app.ListActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.ListView;

public class SelectPlaylist extends ListActivity {
	@Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.playlists);
        final ArrayAdapter<String> adapter = new ArrayAdapter<String>(this,R.layout.trackproperty,R.id.property);
        
        int n = DeadbeefAPI.plt_get_count ();
        for (int i = 0; i < n ; i++) {
        	String nm = DeadbeefAPI.plt_get_title (i);
        	adapter.add(nm);
        }
        
        setListAdapter(adapter);
	}
	
	@Override
    public void onListItemClick (ListView l, View v, int position, long id) {
		DeadbeefAPI.plt_set_curr (position);
		setResult(RESULT_OK);
        finish ();
    };
}
