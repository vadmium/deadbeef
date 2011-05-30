package org.deadbeef.android;

import android.app.ListActivity;
import android.app.ProgressDialog;
import android.content.Intent;
import android.os.Bundle;
import android.os.Environment;
import android.os.RemoteException;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.ListView;

public class FileBrowser extends ListActivity {
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.filelist);

        String path = DeadbeefAPI.conf_get_str("android.lastpath", Environment.getExternalStorageDirectory().getAbsolutePath());
        final FileBrowserAdapter adapter = new FileBrowserAdapter(this, path);
        setListAdapter(adapter);

        Button button = (Button)findViewById(R.id.browse_add_folder);
        button.setOnClickListener(mAddFolderListener);

    }

    @Override
    public void onListItemClick (ListView l, View v, int position, long id) {
   		((FileBrowserAdapter)getListAdapter()).Clicked (position);
    };

    //private ProgressDialog progressDialog; 
    private void AddCurrentFolder() {
    	try {
		if (null != getIntent().getAction () && getIntent().getAction ().toString ().equals ("ADD_FOLDER_AFTER")) {
	        Intent i = getIntent ();
	        String idx_str = getIntent().getData().getFragment ();
	        int idx = Integer.parseInt(idx_str);
	        String plt_str = getIntent().getData().getEncodedSchemeSpecificPart();
	        
	        int plt = Integer.parseInt(plt_str);
	        
    		MusicUtils.sService.pl_insert_dir(((FileBrowserAdapter)getListAdapter()).getPath (), plt, idx);
        }
        else {
    		MusicUtils.sService.pl_add_dir(((FileBrowserAdapter)getListAdapter()).getPath ());
        }
    	}
    	catch (RemoteException ex) {
    	}
		setResult(RESULT_OK);
		DeadbeefAPI.conf_set_str ("android.lastpath", ((FileBrowserAdapter)getListAdapter()).getPath ());
		DeadbeefAPI.conf_save ();
        finish ();

/*    public void AddFolder () {
        // add folder to playlist
        DeadbeefAPI.pl_add_folder (currentPath);
    }
    
    public void AddFolderAfter (int plt, int trk) {
        DeadbeefAPI.pl_insert_dir (plt, trk, currentPath);
    }*/
    	
    	/*
        progressDialog = ProgressDialog.show(this,      
                "Please wait",
                "Adding files to playlist...", true);
        new Thread () {
        	public void run () {
        		if (null != getIntent().getAction () && getIntent().getAction ().toString ().equals ("ADD_FOLDER_AFTER")) {
			        Intent i = getIntent ();
			        String idx_str = getIntent().getData().getFragment ();
			        int idx = Integer.parseInt(idx_str);
			        String plt_str = getIntent().getData().getEncodedSchemeSpecificPart();
			        
			        int plt = Integer.parseInt(plt_str);
	        		((FileBrowserAdapter)getListAdapter()).AddFolderAfter (plt, idx);
		        }
		        else {
	        		((FileBrowserAdapter)getListAdapter()).AddFolder ();
		        }
        		
        		progressDialog.dismiss();
        		
        		setResult(RESULT_OK);
        		finish ();
        	}
        }.start ();
        */
    }

    private OnClickListener mAddFolderListener = new OnClickListener() {
        public void onClick(View v) {
        	AddCurrentFolder ();
        }
    };
    
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
    if (keyCode == KeyEvent.KEYCODE_BACK) {
    	if (((FileBrowserAdapter)getListAdapter()).goBack ()) {
    		return true;
    	}
    }
    return super.onKeyDown(keyCode, event);
}
}
