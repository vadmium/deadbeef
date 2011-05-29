package org.deadbeef.android;

import android.app.ListActivity;
import android.app.ProgressDialog;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;
import android.view.ContextMenu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.AdapterView.AdapterContextMenuInfo;
import android.widget.Button;
import android.widget.ListView;

import com.google.ads.AdRequest;
import com.google.ads.AdView;

public class PlaylistViewer extends ListActivity {
    private ProgressDialog progressDialog;
    private BroadcastReceiver mMediaServiceReceiver;
   	Handler handler = new Handler();
    private int mSelected; // track selected for ctx menu
    
    public static final int MENU_ACT_ADD_FILES = 0;
    public static final int MENU_ACT_ADD_FOLDER = 1;
    public static final int MENU_ACT_REMOVE = 2;
    public static final int MENU_ACT_MOVE_TO_PLAYLIST = 3;
    public static final int MENU_ACT_PROPERTIES = 4;

    void startMediaServiceListener() {
	    mMediaServiceReceiver = new BroadcastReceiver() {
	        @Override
	        public void onReceive(Context context, Intent intent) {
	        	if (null == progressDialog && intent.getAction().toString().equals ("org.deadbeef.android.ADD_FILES_START")) {
					Log.w("DDB", "received ADD_FILES_START");
    				progressDialog = ProgressDialog.show(PlaylistViewer.this,      
    					"Please wait",
    					"Adding files to playlist...", true);
    			}
	        	else if (null != progressDialog && intent.getAction().toString().equals ("org.deadbeef.android.ADD_FILES_FINISH")) {
					Log.w("DDB", "received ADD_FILES_END");
    				progressDialog.dismiss();
    				progressDialog = null;
			        final FileListAdapter adapter = new FileListAdapter(PlaylistViewer.this, R.layout.plitem, R.id.title); 
			        handler.post(new Runnable() {
			            public void run() {
			                setListAdapter(adapter);
			            }
			        });
    			}
	        }
	    };
	    IntentFilter filter = new IntentFilter();
	    filter.addAction("org.deadbeef.android.ADD_FILES_START");
	    filter.addAction("org.deadbeef.android.ADD_FILES_FINISH");
	    registerReceiver(mMediaServiceReceiver, filter);
    }
    
    void stopMediaServiceListener () {
    	unregisterReceiver(mMediaServiceReceiver);
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
		Log.e("DDB", "Deadbeef.onCreate");
        super.onCreate(savedInstanceState);
        setContentView(R.layout.playlist);
        MusicUtils.bindToService(this, new ServiceConnection() {
	        public void onServiceConnected(ComponentName className, IBinder obj) {
	        	Log.e("DDB", "Deadbeef.onCreate connected");
	        	MusicUtils.sService = IMediaPlaybackService.Stub.asInterface(obj);
		        final FileListAdapter adapter = new FileListAdapter(PlaylistViewer.this, R.layout.plitem, R.id.title); 
		        handler.post(new Runnable() {
		            public void run() {
		                setListAdapter(adapter);
		            }
		        });
		        startMediaServiceListener ();
	        }
	
	        public void onServiceDisconnected(ComponentName className) {
		        stopMediaServiceListener ();
	        	MusicUtils.sService = null;
	        }
	    });

        startMediaServiceListener ();
        registerForContextMenu(findViewById(android.R.id.list));
        
        ((Button)findViewById(R.id.add)).setOnClickListener(mAddListener);
        ((Button)findViewById(R.id.clear)).setOnClickListener(mClearListener);
        
        
        AdView adView = (AdView)this.findViewById(R.id.adView);
        AdRequest req = new AdRequest();
        req.addTestDevice("047F1C49C21BD737CFA3DD834B2BC416");
        adView.loadAd(req);
    }
    @Override
    public void onDestroy () {
    	stopMediaServiceListener ();
    	super.onDestroy();
    }

    @Override
    public void onListItemClick (ListView l, View v, int position, long id) {
   		try {
   			MusicUtils.sService.playIdx (position);
   		}
   		catch (RemoteException e) {
   		}
    };
    
    
	@Override
	public void onCreateContextMenu (ContextMenu menu, View v, ContextMenu.ContextMenuInfo menuInfo) {
		mSelected = ((AdapterContextMenuInfo)menuInfo).position;
		Log.e("DDB","onCreateContextMenu");
//		menu.add(0, MENU_ACT_ADD_FILES, 0, R.string.ctx_menu_add_files);
		menu.add(0, MENU_ACT_ADD_FOLDER, 1, R.string.ctx_menu_add_folder);
//		menu.add(0, MENU_ACT_REMOVE, 2, R.string.ctx_menu_remove);
//		menu.add(0, MENU_ACT_MOVE_TO_PLAYLIST, 3, R.string.ctx_menu_move_to_playlist);
		
		Intent i = new Intent (this, TrackPropertiesViewer.class);
		i.setData(Uri.fromParts("track", String.valueOf (DeadbeefAPI.plt_get_curr()), String.valueOf(((AdapterContextMenuInfo)menuInfo).position)));
		menu.add(0, MENU_ACT_PROPERTIES, 4, R.string.ctx_menu_properties).setIntent (i);
	}

    @Override
    protected void onActivityResult (int requestCode, int resultCode, Intent data) {
        // add folder to playlist
    	if (requestCode == Deadbeef.REQUEST_ADD_FOLDER && resultCode == RESULT_OK) {
	        final FileListAdapter adapter = new FileListAdapter(this, R.layout.plitem, R.id.title); 
	        handler.post(new Runnable() {
	            public void run() {
	                setListAdapter(adapter);
	            }
	        });
    	}
    	if (requestCode == Deadbeef.REQUEST_ADD_FOLDER_AFTER && resultCode == RESULT_OK) {
	        final FileListAdapter adapter = new FileListAdapter(this, R.layout.plitem, R.id.title); 
	        handler.post(new Runnable() {
	            public void run() {
	                setListAdapter(adapter);
	            }
	        });
    	}
    	else if (requestCode == Deadbeef.REQUEST_SELECT_PLAYLIST && resultCode >= 0) {
    		DeadbeefAPI.plt_set_curr (resultCode);
	        final FileListAdapter adapter = new FileListAdapter(this, R.layout.plitem, R.id.title); 
	        handler.post(new Runnable() {
	            public void run() {
	                setListAdapter(adapter);
	            }
	        });
    	}
    }
	
	@Override
	public boolean onContextItemSelected (MenuItem item) {
		switch (item.getItemId()) {
		case MENU_ACT_ADD_FILES:
			break;
		case MENU_ACT_ADD_FOLDER:
	        Intent i = new Intent (this, FileBrowser.class);
	        i.setAction("ADD_FOLDER_AFTER");
	        i.setData(Uri.fromParts("track", String.valueOf (DeadbeefAPI.plt_get_curr()), String.valueOf(mSelected)));
	    	startActivityForResult(i, Deadbeef.REQUEST_ADD_FOLDER_AFTER);
			break;
		case MENU_ACT_REMOVE:
			break;
		case MENU_ACT_MOVE_TO_PLAYLIST:
			break;
		}
		return false;
	}

	private void AddFolder () {
        Intent i = new Intent (this, FileBrowser.class);
    	startActivity(i);
    }

	private OnClickListener mAddListener = new OnClickListener() {
        public void onClick(View v) {
        	AddFolder ();
        }
    };

    private OnClickListener mClearListener = new OnClickListener() {
        public void onClick(View v) {
        	DeadbeefAPI.pl_clear (); // FIXME: should be called through mediaservice, only when connected
	        final FileListAdapter adapter = new FileListAdapter(PlaylistViewer.this, R.layout.plitem, R.id.title); 
	        handler.post(new Runnable() {
	            public void run() {
	                setListAdapter(adapter);
	            }
	        });
        }
    };

}
