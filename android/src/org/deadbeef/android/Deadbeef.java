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
	private String current_pos_text = "-:--";
    private SeekBar seekbar;
    private int seekbar_pos = -1;
    private int play_mode = -1;
    private int play_order = -1;
    private boolean playback_state = false;

    private static final int REQUEST_ADD_FOLDER = 1;
    
    private boolean isVisible = true;
    
    private Timer mTimer;
    private ProgressTask mTimerTask;
    
    private class ProgressTask extends TimerTask {
    	public void run () {
    		handler.post(UpdateInfoRunnable);
    	}
    }
    
    public void onWindowFocusChanged (boolean hasFocus) {
		Log.e(TAG, "hasFocus="+hasFocus);
    	isVisible = hasFocus;
    	
    	if (isVisible && null == mTimer) {
	        mTimer = new Timer();
	        mTimerTask = new ProgressTask();
	        mTimer.schedule (mTimerTask, 0, 250);
    	}
    	else if (null != mTimer) {
    		if (null != mTimerTask) {
    			mTimerTask.cancel ();
    			mTimerTask = null;
    		}
    		if (null != mTimer) {
    			mTimer.cancel ();
    			mTimer = null;
    		}
    	}
    }
    
    final Runnable UpdateInfoRunnable = new Runnable() {
    	public void run() {
    		if (!isVisible) {
    			return;
    		}
    		try {
	    		TextView tv;
	    		int track = MusicUtils.sService.getCurrentIdx ();
	    		
	    		if (MusicUtils.sService == null) {
	    			return;
	    		}
	    		
	    		// playpause button
	    		boolean new_state = MusicUtils.sService.isPlaying ();
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
	    		
	    		// shuffle button
	    		int new_order = MusicUtils.sService.getPlayOrder();
	    		if (new_order != play_order) {
	    			play_order = new_order;
	        		ImageButton button = (ImageButton)findViewById(R.id.ShuffleMode);
	    			if (play_order == 0) {
	        			button.setImageResource (R.drawable.ic_media_shuffle_off);
	    			}
	    			else if (play_order == 1) {
	        			button.setImageResource (R.drawable.ic_media_shuffle_tracks);
	    			}
	    			else if (play_order == 3) {
	        			button.setImageResource (R.drawable.ic_media_shuffle_albums);
	    			}
	    		}
	    		
	    		// repeat button
	    		int new_mode = MusicUtils.sService.getPlayMode();
	    		if (new_mode != play_mode) {
	    			play_mode = new_mode;
	        		ImageButton button = (ImageButton)findViewById(R.id.RepeatMode);
	    			if (play_mode == 0) {
	        			button.setImageResource (R.drawable.ic_media_repeat_on);
	    			}
	    			else if (play_mode == 1) {
	        			button.setImageResource (R.drawable.ic_media_repeat_off);
	    			}
	    			else if (play_mode == 2) {
	        			button.setImageResource (R.drawable.ic_media_repeat_track);
	    			}
	    		}
	    		
	    		boolean state = MusicUtils.sService.isPlaying ();
	    		if (track != curr_track || (playback_state != state && state)) {
	    			curr_track = track;
	    			playback_state = state;
	    			
	    			if (curr_track >= 0) {
	    	    		// update album/artist/title
	    	    		tv = (TextView)findViewById(R.id.np_album);
	    	    		tv.setText(MusicUtils.sService.getAlbumName ());
	    	    		tv = (TextView)findViewById(R.id.np_artist);
	    	    		tv.setText(MusicUtils.sService.getArtistName ());
	    	    		tv = (TextView)findViewById(R.id.np_title);
	    	    		tv.setText(MusicUtils.sService.getTrackName ());
	    			}
	    		}
		    	// update numbers
	    		String new_pos_text = DeadbeefAPI.play_get_pos_formatted () + "/" + DeadbeefAPI.play_get_duration_formatted ();
	    		if (!new_pos_text.equals (current_pos_text)) {
	    			current_pos_text = new_pos_text;
		    		current_pos_tv.setText(current_pos_text);
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
    
    @Override
    public void onResume() {
    	super.onResume();
    	Intent i = getIntent();
    	if (i.getAction().equals("android.intent.action.VIEW")) {
    		try {
    			MusicUtils.sService.startFile (i.getData().toString());
    		} catch (RemoteException ex) {
    		}
    	}
    }

    @Override
    public void onNewIntent(Intent intent) {
        setIntent(intent);
    }

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
		Log.e("DDB", "Deadbeef.onCreate");
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        
        ImageButton button;
        
        button = (ImageButton)findViewById(R.id.prev);
        button.setOnClickListener(mPrevListener);

        button = (ImageButton)findViewById(R.id.next);
        button.setOnClickListener(mNextListener);
        
        button = (ImageButton)findViewById(R.id.play);
        button.setOnClickListener(mPlayPauseListener);
        
        button = (ImageButton)findViewById(R.id.ShuffleMode);
        button.setOnClickListener(mShuffleModeListener);
        
        button = (ImageButton)findViewById(R.id.RepeatMode);
        button.setOnClickListener(mRepeatModeListener);

        SeekBar sb = (SeekBar)findViewById(R.id.seekbar);
        sb.setMax(100);
        sb.setOnSeekBarChangeListener(sbChangeListener);

		current_pos_tv = (TextView)findViewById(R.id.pos_duration_text);
        seekbar = (SeekBar)findViewById(R.id.seekbar);

        MusicUtils.bindToService(this, new ServiceConnection() {
	        public void onServiceConnected(ComponentName className, IBinder obj) {
	        	MusicUtils.sService = IMediaPlaybackService.Stub.asInterface(obj);
		        final FileListAdapter adapter = new FileListAdapter(Deadbeef.this, R.layout.plitem, R.id.title); 
		        handler.post(new Runnable() {
		            public void run() {
		                setListAdapter(adapter);
		            }
		        });
	        }
	
	        public void onServiceDisconnected(ComponentName className) {
	        	MusicUtils.sService = null;
	        }
	    });

        final FileListAdapter adapter = new FileListAdapter(this, R.layout.plitem, R.id.title); 
        setListAdapter(adapter);   
    }
    
    @Override
    public void onDestroy() {
    	if (null != mTimer) {
	    	mTimerTask.cancel ();
	    	mTimer.cancel ();
	    	mTimerTask = null;
	    	mTimer = null;
    	}
    	MusicUtils.unbindFromService (this);
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
        		MusicUtils.sService.prev();
        	}
        	catch (RemoteException e) {
        	}
        }
    };
    
    private OnClickListener mNextListener = new OnClickListener() {
        public void onClick(View v) {
        	try {
        		MusicUtils.sService.next();
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
   	    	if (null != mTimer) {
	        	mTimerTask.cancel ();
	        	mTimer.cancel ();
	        	mTimerTask = null;
	        	mTimer = null;
   	    	}
            finish ();
        }
        return true;
    };
    
    private void PlayPause () {
    	try {
    		if (!MusicUtils.sService.isPlaying()) {
    			MusicUtils.sService.play ();
    		}
    		else {
    			MusicUtils.sService.pause ();
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

    private OnClickListener mRepeatModeListener = new OnClickListener() {
        public void onClick(View v) {
        	try {
        		MusicUtils.sService.cycleRepeatMode ();
        	}
        	catch (RemoteException e) {
        	}
        }
    };
    
    private OnClickListener mShuffleModeListener = new OnClickListener() {
        public void onClick(View v) {
        	try {
        		MusicUtils.sService.cycleShuffleMode ();
        	}
        	catch (RemoteException e) {
        	}
        }
    };

    @Override
    public void onListItemClick (ListView l, View v, int position, long id) {
   		try {
   			MusicUtils.sService.playIdx (position);
   		}
   		catch (RemoteException e) {
   			Log.e(TAG,"remote exception in onListItemClick");
   		}
    };
    
	void PlayerSeek (float value) {
		try {
	   		MusicUtils.sService.seek(value);
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

