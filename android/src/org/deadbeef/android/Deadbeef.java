package org.deadbeef.android;

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
    int curr_state = -1; // -1=unknown, 0 = paused/stopped, 1 = playing
    
    private IMediaPlaybackService mPlaybackService;
    private boolean mIsBound;
    
    private ServiceConnection mConnection = new ServiceConnection() {
        public void onServiceConnected(ComponentName className, IBinder obj) {
        	mPlaybackService = IMediaPlaybackService.Stub.asInterface(obj);
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
    		try {
	    		TextView tv;
	    		int track = DeadbeefAPI.pl_get_current_idx ();
	    		
	    		int new_state = mPlaybackService.isPaused() ? 0 : 1;
	    		if (new_state != curr_state) {
	    			curr_state = new_state;
	        		ImageButton button = (ImageButton)findViewById(R.id.play);
	        		if (curr_state == 0) {
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
	    			}
	    		}
		    	// update numbers
		    	tv = (TextView)findViewById(R.id.current_pos_text);
		    	tv.setText(DeadbeefAPI.play_get_pos_formatted ());
		    	tv = (TextView)findViewById(R.id.duration_text);
		    	tv.setText(DeadbeefAPI.play_get_duration_formatted ());
		
		    	// update seekbar
		    	if (dontUpdatePlayPos) {
		    		return;
		    	}
		        SeekBar sb = (SeekBar)findViewById(R.id.seekbar);
		        float normpos = DeadbeefAPI.play_get_pos_normalized ();
		        sb.setProgress ((int)(normpos * 1024));
    		}
    		catch (RemoteException e) {
    			Log.e(TAG, "playback service error");
    		}
    	}
    };
    
    private class ProgressThread extends Thread {
    	@Override
        public void run() {
    		while (!terminate) {
    			try {
                    sleep(250);
                } catch (InterruptedException e) {
                    Log.e(TAG, e.getMessage());
                }
    			handler.post(UpdateInfoRunnable);
    		}
	    	Log.i(TAG, "progress terminated");
    	}
    };
    private ProgressThread progressThread;

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
		DeadbeefAPI.start();

        setContentView(R.layout.main);
        
        final FileListAdapter adapter = new FileListAdapter(this, R.layout.plitem, R.id.title); 
        setListAdapter(adapter);
        
        ImageButton button;
        
/*        ImageButton button = (ImageButton)findViewById(R.id.quit);
        button.setOnClickListener(mQuitListener);*/
        
        button = (ImageButton)findViewById(R.id.prev);
        button.setOnClickListener(mPrevListener);

        button = (ImageButton)findViewById(R.id.next);
        button.setOnClickListener(mNextListener);
        
/*        button = (ImageButton)findViewById(R.id.add);
        button.setOnClickListener(mAddListener);*/

        button = (ImageButton)findViewById(R.id.play);
        button.setOnClickListener(mPlayPauseListener);
        
        SeekBar sb = (SeekBar)findViewById(R.id.seekbar);
        sb.setMax(1024);
        sb.setOnSeekBarChangeListener(sbChangeListener);
        
        doBindService();

        progressThread = new ProgressThread();
        progressThread.start();
   }
    
    @Override
    public void onDestroy() {
    	terminate = true;
    	try {
    		progressThread.join ();
	    } catch (InterruptedException e) {
	    	Log.e(TAG, e.getMessage());
	    }
        super.onDestroy();
    }

    
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
    	MenuInflater inflater = getMenuInflater();
    	inflater.inflate(R.menu.optionsmenu, menu);
    	return true;
    }
             
/*    private OnClickListener mQuitListener = new OnClickListener() {
        public void onClick(View v) {
        	ply.stop ();
        	finish ();
        }
    };*/

    private OnClickListener mPrevListener = new OnClickListener() {
        public void onClick(View v) {
        	DeadbeefAPI.play_prev ();
        }
    };
    
    private OnClickListener mNextListener = new OnClickListener() {
        public void onClick(View v) {
        	DeadbeefAPI.play_next ();
        }
    };
    
    private void AddFolder () {
        Log.i(TAG,"AddFolder ()");
        Intent i = new Intent (this, FileBrowser.class);
    	startActivityForResult(
        		i,
                10);
    }

    @Override
    protected void onActivityResult (int requestCode, int resultCode, Intent data) {
        // add folder to playlist
    	if (requestCode == 10) {
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
        	terminate = true;
        	try {
        		progressThread.join ();
	        } catch (InterruptedException e) {
	            Log.e(TAG, e.getMessage());
	        }
	        try {
	        	mPlaybackService.stop();
	        }
	        catch (RemoteException e) {
	        	Log.e(TAG, "remote exception on quit");
	        }
            finish ();
        }
        return true;
    };
    
    private void PlayPause () {
    	try {
    		if (mPlaybackService.isPaused()) {
    			mPlaybackService.play ();
	    		ImageButton button = (ImageButton)findViewById(R.id.play);
	            button.setImageResource (R.drawable.ic_media_pause);    		
    		}
    		else {
    			mPlaybackService.pause ();
	    		ImageButton button = (ImageButton)findViewById(R.id.play);
	            button.setImageResource (R.drawable.ic_media_play);    		
    			
    		}
    		/*
    		mPlaybackService.playpause ();
	    	if (!mPlaybackService.isPaused()) {
	    		int curr = DeadbeefAPI.pl_get_current_idx ();
	    		if (curr < 0) {
	    			DeadbeefAPI.play_idx (0);
	    		}
	    		ImageButton button = (ImageButton)findViewById(R.id.play);
	            button.setImageResource (R.drawable.ic_media_pause);    		
	    	}
	    	else {
	    		ImageButton button = (ImageButton)findViewById(R.id.play);
	            button.setImageResource (R.drawable.ic_media_play);    		
	    	}*/
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
   		DeadbeefAPI.play_idx (position);
   		try {
	   		if (mPlaybackService.isPaused ()) {
	   			mPlaybackService.play ();
	   		}
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
			trackedPos = (float)((float)progress/1024.0);
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

