package org.deadbeef.android;

import java.util.Timer;
import java.util.TimerTask;

import android.app.ListActivity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageButton;
import android.widget.ListView;
import android.widget.SeekBar;
import android.widget.TextView;

public class Deadbeef extends ListActivity {
	String TAG = "DDB";
	Handler handler = new Handler();
	boolean terminate = false;
	
    boolean dontUpdatePlayPos = false;
    
    int curr_track = -1;
    boolean curr_state = false; // false=stopped/paused
    
	private TextView current_pos_tv;
	private TextView duration_tv;
	private String current_pos_text = "-:--";
	private String duration_text = "-:--";
    private SeekBar seekbar;
    private int seekbar_pos = -1;

    private static final int REQUEST_ADD_FOLDER = 1;
    
    private IMediaPlaybackService mPlaybackService;
    private boolean mIsBound = false;
    
    private boolean isVisible = true;
    
    public void onWindowFocusChanged (boolean hasFocus) { 
    	isVisible = hasFocus;
    }
    
    private ServiceConnection mConnection = new ServiceConnection() {
        public void onServiceConnected(ComponentName className, IBinder obj) {
        	mPlaybackService = IMediaPlaybackService.Stub.asInterface(obj);
	        final FileListAdapter adapter = new FileListAdapter(Deadbeef.this, R.layout.plitem, R.id.title); 
	        handler.post(new Runnable() {
	            public void run() {
	                setListAdapter(adapter);
	            }
	        });
        }

        public void onServiceDisconnected(ComponentName className) {
        	mPlaybackService = null;
        }
    };
    
    void doBindService() {
        // Establish a connection with the service.  We use an explicit
        // class name because we want a specific service implementation that
        // we know will be running in our own process (and thus won't be
        // supporting component replacement by other applications).
        bindService(new Intent(Deadbeef.this, 
                MediaPlaybackService.class), mConnection, Context.BIND_AUTO_CREATE);
        mIsBound = true;
    }

    void doUnbindService() {
        if (mIsBound) {
            // Detach our existing connection.
            unbindService(mConnection);
            mIsBound = false;
        }
    }
    
    
    final Runnable UpdateInfoRunnable = new Runnable() {
    	public void run() {
    		if (!isVisible) {
    			return;
    		}
    		try {
	    		TextView tv;
	    		int track = DeadbeefAPI.pl_get_current_idx ();
	    		
	    		if (mPlaybackService == null) {
	    			return;
	    		}
	    		
	    		boolean new_state = mPlaybackService.isPlaying ();
	    		if (new_state != curr_state) {
	    			curr_state = new_state;
	        		ImageButton button = (ImageButton)findViewById(R.id.play);
	        		if (!curr_state) {
	        			button.setImageResource (R.drawable.ic_media_play);
	        		}
	        		else {
	        			button.setImageResource (R.drawable.ic_media_pause);
	        		}
	    		}
	    		
	    		if (track != curr_track) {
	    			curr_track = track;
	    			
	    			if (curr_track >= 0) {
	    	    		// update album/artist/title
	    	    		tv = (TextView)findViewById(R.id.np_album);
	    	    		tv.setText(DeadbeefAPI.pl_get_metadata (curr_track, "album"));
	    	    		tv = (TextView)findViewById(R.id.np_artist);
	    	    		tv.setText(DeadbeefAPI.pl_get_metadata (curr_track, "artist"));
	    	    		tv = (TextView)findViewById(R.id.np_title);
	    	    		tv.setText(DeadbeefAPI.pl_get_metadata (curr_track, "title"));
	    	    		
	    	    		mPlaybackService.refreshStatus();
	    			}
	    		}
		    	// update numbers
	    		String new_pos_text = DeadbeefAPI.play_get_pos_formatted ();
	    		if (!new_pos_text.equals (current_pos_text)) {
	    			current_pos_text = new_pos_text;
		    		current_pos_tv.setText(current_pos_text);
	    		}
	    		String new_duration_text = DeadbeefAPI.play_get_duration_formatted ();
	    		if (!new_duration_text.equals (duration_text)) {
	    			duration_text = new_duration_text;
	    			duration_tv.setText (duration_text);
	    		}
		
		    	// update seekbar
		    	if (dontUpdatePlayPos) {
		    		return;
		    	}
		    	int new_pos = (int)(DeadbeefAPI.play_get_pos_normalized () * 100);
		    	if (new_pos != seekbar_pos) {
		    		seekbar_pos = new_pos;
		        	seekbar.setProgress (seekbar_pos);
		    	}
    		}
    		catch (RemoteException e) {
    			Log.e(TAG, "playback service error");
    		}
    	}
    };
    
    private Timer mTimer;
    private ProgressTask mTimerTask;
    
    private class ProgressTask extends TimerTask {
    	public void run () {
    		handler.post(UpdateInfoRunnable);
    	}
    }
    
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        
        ImageButton button;
        
        button = (ImageButton)findViewById(R.id.prev);
        button.setOnClickListener(mPrevListener);

        button = (ImageButton)findViewById(R.id.next);
        button.setOnClickListener(mNextListener);
        
        button = (ImageButton)findViewById(R.id.play);
        button.setOnClickListener(mPlayPauseListener);
        
        SeekBar sb = (SeekBar)findViewById(R.id.seekbar);
        sb.setMax(100);
        sb.setOnSeekBarChangeListener(sbChangeListener);

		current_pos_tv = (TextView)findViewById(R.id.current_pos_text);
		duration_tv = (TextView)findViewById(R.id.duration_text); 
        seekbar = (SeekBar)findViewById(R.id.seekbar);

        doBindService();

        final FileListAdapter adapter = new FileListAdapter(this, R.layout.plitem, R.id.title); 
        setListAdapter(adapter);
        
        mTimer = new Timer();
        mTimerTask = new ProgressTask();
        mTimer.schedule (mTimerTask, 0, 250);
    }
    
    @Override
    public void onDestroy() {
    	mTimerTask.cancel ();
    	mTimer.cancel ();
    	mTimerTask = null;
    	mTimer = null;
        super.onDestroy();
    }

    
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
    	MenuInflater inflater = getMenuInflater();
    	inflater.inflate(R.menu.optionsmenu, menu);
    	return true;
    }
             
    private OnClickListener mPrevListener = new OnClickListener() {
        public void onClick(View v) {
        	try {
        		mPlaybackService.prev();
        	}
        	catch (RemoteException e) {
        	}
        }
    };
    
    private OnClickListener mNextListener = new OnClickListener() {
        public void onClick(View v) {
        	try {
        		mPlaybackService.next();
        	}
        	catch (RemoteException e) {
        	}
        }
    };
    
    private void AddFolder () {
        Log.i(TAG,"AddFolder ()");
        Intent i = new Intent (this, FileBrowser.class);
    	startActivityForResult(
        		i,
                REQUEST_ADD_FOLDER);
    }

    @Override
    protected void onActivityResult (int requestCode, int resultCode, Intent data) {
        // add folder to playlist
    	if (requestCode == REQUEST_ADD_FOLDER && resultCode == RESULT_OK) {
	        final FileListAdapter adapter = new FileListAdapter(this, R.layout.plitem, R.id.title); 
	        handler.post(new Runnable() {
	            public void run() {
	                setListAdapter(adapter);
	            }
	        });
    	}
    }

    @Override
    public boolean onOptionsItemSelected (MenuItem item) {
        int id = item.getItemId ();
        if (id == R.id.menu_add_folder) {
            AddFolder ();
        }
        else if (id == R.id.menu_clear_playlist) {
        	DeadbeefAPI.pl_clear ();
	        final FileListAdapter adapter = new FileListAdapter(this, R.layout.plitem, R.id.title); 
	        handler.post(new Runnable() {
	            public void run() {
	                setListAdapter(adapter);
	            }
	        });
        }
        else if (id == R.id.menu_quit) {
        	mTimerTask.cancel ();
        	mTimer.cancel ();
        	mTimerTask = null;
        	mTimer = null;
/*	        try {
	        	mPlaybackService.stop();
	        }
	        catch (RemoteException e) {
	        	Log.e(TAG, "remote exception on quit");
	        }*/
            finish ();
        }
        return true;
    };
    
    private void PlayPause () {
    	try {
    		if (!mPlaybackService.isPlaying()) {
    			mPlaybackService.play ();
    		}
    		else {
    			mPlaybackService.pause ();
    		}
    	}
    	catch (RemoteException e) {
    		Log.e(TAG,"remote exception on PlayPause");
    	}
    }

    private OnClickListener mPlayPauseListener = new OnClickListener() {
        public void onClick(View v) {
        	PlayPause();
        }
    };

    @Override
    public void onListItemClick (ListView l, View v, int position, long id) {
   		try {
   			mPlaybackService.playIdx (position);
   		}
   		catch (RemoteException e) {
   			Log.e(TAG,"remote exception in onListItemClick");
   		}
    };
    
	void PlayerSeek (float value) {
		try {
	   		mPlaybackService.seek(value);
   		}
   		catch (RemoteException e) {
   			Log.e(TAG,"remote exception in onListItemClick");
   		}
	}

	float trackedPos = 0;
	private SeekBar.OnSeekBarChangeListener sbChangeListener = new SeekBar.OnSeekBarChangeListener() { 
		public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
			trackedPos = (float)((float)progress/100.0);
			if (dontUpdatePlayPos) {
				return;
			}
			if (fromUser) {
				PlayerSeek (trackedPos);
			}
		}
		public void onStartTrackingTouch(SeekBar seekBar) {
			dontUpdatePlayPos = true;
		}
		public void onStopTrackingTouch(SeekBar seekBar) {
			dontUpdatePlayPos = false;
			PlayerSeek (trackedPos);
		}
	};

}

