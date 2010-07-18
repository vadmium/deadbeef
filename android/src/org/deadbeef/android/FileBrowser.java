package org.deadbeef.android;

import android.app.ListActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.ListView;

public class FileBrowser extends ListActivity {
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.filelist);

        // FIXME: get default path from savedInstanceState
        final FileBrowserAdapter adapter = new FileBrowserAdapter(this, "/sdcard");
        setListAdapter(adapter);
    }

    @Override
    public void onListItemClick (ListView l, View v, int position, long id) {
   		((FileBrowserAdapter)getListAdapter()).Clicked (position);
    };
}
