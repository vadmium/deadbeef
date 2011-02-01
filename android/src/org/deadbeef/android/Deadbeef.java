package org.deadbeef.android;

import java.util.Timer;
import java.util.TimerTask;

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ListActivity;
import android.content.ComponentName;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.ServiceConnection;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;
import android.view.ContextMenu;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.AdapterView.AdapterContextMenuInfo;
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
    private static final int REQUEST_SELECT_PLAYLIST = 2;
    private static final int REQUEST_ADD_FOLDER_AFTER = 3;
    
    private boolean isVisible = true;
    
    private Timer mTimer;
    private ProgressTask mTimerTask;
    
    private int mSelected; // track selected for ctx menu
    
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
    
    private long last_br_update = 0;
    private String sbtext = "";
    void updateStatusbar () {
    	String sbtext_new;
    	float songpos;

	    int track = DeadbeefAPI.streamer_get_playing_track ();
	    int fmt = DeadbeefAPI.streamer_get_current_fileinfo_format (); // FIXME: might crash streamer
	    
	
	    float duration = track != 0 ? DeadbeefAPI.pl_get_item_duration (track) : -1;
	
        try {
		    if (MusicUtils.sService.isStopped() || 0 == track || 0 == fmt) {
		    	sbtext_new = "Stopped";
		        songpos = 0;
		    }
		    else {
		        float playpos = DeadbeefAPI.streamer_get_playpos ();
		        int minpos = (int)(playpos / 60);
		        int secpos = (int)(playpos - minpos * 60);
		        int mindur = (int)(duration / 60);
		        int secdur = (int)(duration - mindur * 60);
		
		        String mode;
		        int channels = DeadbeefAPI.fmt_get_channels (fmt);
		        if (channels <= 2) {
		            mode = channels == 1 ? "Mono" : "Stereo";
		        }
		        else {
		            mode = String.format ("%dch Multichannel", channels);
		        }
		        int samplerate = DeadbeefAPI.fmt_get_samplerate (fmt);
		        int bitspersample = DeadbeefAPI.fmt_get_bps (fmt);
		        songpos = playpos;
		
		        String t;
		        if (duration >= 0) {
		            t = String.format("%d:%02d", mindur, secdur);
		        }
		        else {
		            t = "-:--";
		        }
	
		        String spaused = MusicUtils.sService.isPaused() ? "Paused | " : "";
		        String ft = DeadbeefAPI.pl_get_track_filetype (track);
		        sbtext_new = String.format ("%s%s | %dHz | %d bit | %s | %d:%02d / %s", spaused, ft != null ? ft : "-", samplerate, bitspersample, mode, minpos, secpos, t);
		    }
		    if (!sbtext.equals(sbtext_new)) {
		    	sbtext = sbtext_new;
		    	TextView st = (TextView)findViewById(R.id.status);
		    	st.setText(sbtext);
		    }
        }
        catch (RemoteException ex) {
        }
	
	    if (0 != track) {
	        DeadbeefAPI.pl_item_unref (track);
	    }
    }
    
    private String getTotalTimeFormatted () {
    	float pl_totaltime = DeadbeefAPI.pl_get_totaltime ();
	    int daystotal = (int)pl_totaltime / (3600*24);
	    int hourtotal = ((int)pl_totaltime / 3600) % 24;
	    int mintotal = ((int)pl_totaltime/60) % 60;
	    int sectotal = ((int)pl_totaltime) % 60;
	    String totaltime_str;
	    if (daystotal == 0) {
	        totaltime_str = String.format ("%d:%02d:%02d", hourtotal, mintotal, sectotal);
	    }
	    else if (daystotal == 1) {
	        totaltime_str = String.format("1 day %d:%02d:%02d", hourtotal, mintotal, sectotal);
	    }
	    else {
	        totaltime_str = String.format ("%d days %d:%02d:%02d", daystotal, hourtotal, mintotal, sectotal);
	    }
	    return totaltime_str;
    }

    private String plstate_prev = "";
    
    final Runnable UpdateInfoRunnable = new Runnable() {
    	public void run() {
    		if (!isVisible) {
    			return;
    		}
    		try {
	    		TextView tv;
	    		int track = MusicUtils.sService.getCurrentIdx ();
	    		
		    	TextView st = (TextView)findViewById(R.id.plstate);
		    	String totaltime_str = getTotalTimeFormatted ();

		    	String plstate = String.format ("%d tracks | %s total playtime", DeadbeefAPI.pl_getcount (0), totaltime_str);
		    	if (!plstate_prev.equals(plstate)) {
		    		plstate_prev = plstate;
		    		st.setText(plstate);
		    	}
	    		
	    		if (MusicUtils.sService == null) {
			    	st = (TextView)findViewById(R.id.status);
			    	st.setText("Stopped");
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
	    	    		tv = (TextView)findViewById(R.id.np_artist_album);
	    	    		tv.setText(MusicUtils.sService.getArtistName () + " - " + MusicUtils.sService.getAlbumName ());
	    	    		tv = (TextView)findViewById(R.id.np_title);
	    	    		tv.setText(MusicUtils.sService.getTrackName ());
	    			}
	    		}
/*		    	// update numbers
	    		String new_pos_text = DeadbeefAPI.play_get_pos_formatted () + "/" + DeadbeefAPI.play_get_duration_formatted ();
	    		if (!new_pos_text.equals (current_pos_text)) {
	    			current_pos_text = new_pos_text;
		    		current_pos_tv.setText(current_pos_text);
	    		}*/
		
		    	updateStatusbar ();
		    	
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
        
        registerForContextMenu(findViewById(android.R.id.list));
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
    	startActivityForResult(i, REQUEST_ADD_FOLDER);
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
    	if (requestCode == REQUEST_ADD_FOLDER_AFTER && resultCode == RESULT_OK) {
	        final FileListAdapter adapter = new FileListAdapter(this, R.layout.plitem, R.id.title); 
	        handler.post(new Runnable() {
	            public void run() {
	                setListAdapter(adapter);
	            }
	        });
    	}
    	else if (requestCode == REQUEST_SELECT_PLAYLIST && resultCode >= 0) {
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
        else if (id == R.id.menu_manage_playlists) {
        	Intent i = new Intent (this, SelectPlaylist.class);
	    	startActivityForResult(i, REQUEST_SELECT_PLAYLIST);
        }
        else if (id == R.id.menu_about) {
        	showDialog (0);
        }
        return true;
    };
    
    @Override
    protected Dialog onCreateDialog(int id) {
    	LayoutInflater factory = LayoutInflater.from(this);
        final View textView = factory.inflate(R.layout.aboutbox, null);
        return new AlertDialog.Builder(Deadbeef.this)
            .setIcon(R.drawable.icon)
            .setTitle("About")
            .setView(textView)
            .setPositiveButton(R.string.alert_dialog_ok, new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int whichButton) {

                    /* User clicked OK so do some stuff */
                }
            })
            .create();
    }
    
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
    
    
    public static final int MENU_ACT_ADD_FILES = 0;
    public static final int MENU_ACT_ADD_FOLDER = 1;
    public static final int MENU_ACT_REMOVE = 2;
    public static final int MENU_ACT_MOVE_TO_PLAYLIST = 3;
    public static final int MENU_ACT_PROPERTIES = 4;
    
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
	public boolean onContextItemSelected (MenuItem item) {
		switch (item.getItemId()) {
		case MENU_ACT_ADD_FILES:
			break;
		case MENU_ACT_ADD_FOLDER:
	        Intent i = new Intent (this, FileBrowser.class);
	        i.setAction("ADD_FOLDER_AFTER");
	        i.setData(Uri.fromParts("track", String.valueOf (DeadbeefAPI.plt_get_curr()), String.valueOf(mSelected)));
	    	startActivityForResult(i, REQUEST_ADD_FOLDER_AFTER);
			break;
		case MENU_ACT_REMOVE:
			break;
		case MENU_ACT_MOVE_TO_PLAYLIST:
			break;
		}
		return false;
	}
    
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

