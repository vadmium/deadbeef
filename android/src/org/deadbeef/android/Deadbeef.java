package org.deadbeef.android;

import java.util.Timer;
import java.util.TimerTask;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.RemoteException;
import android.provider.MediaStore;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup.LayoutParams;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.SeekBar;
import android.widget.TextView;

public class Deadbeef extends Activity {
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

    private boolean isVisible = true;
    
    private Timer mTimer;
    private ProgressTask mTimerTask;

    public static final int REQUEST_ADD_FOLDER = 1;
    public static final int REQUEST_SELECT_PLAYLIST = 2;
    public static final int REQUEST_ADD_FOLDER_AFTER = 3;
    
    private Worker mAlbumArtWorker;
    private AlbumArtHandler mAlbumArtHandler;
    private ImageView mCover;
    private final static int IDCOLIDX = 0;

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
    private ProgressDialog progressDialog;
    
    private static final int REFRESH = 1;
    private static final int QUIT = 2;
    private static final int GET_ALBUM_ART = 3;
    private static final int ALBUM_ART_DECODED = 4;

    private void queueNextRefresh(long delay) {
    	try {
	        if (!MusicUtils.sService.isPaused()) {
	            Message msg = mHandler.obtainMessage(REFRESH);
	            mHandler.removeMessages(REFRESH);
	            mHandler.sendMessageDelayed(msg, delay);
	        }
    	}
        catch (RemoteException ex) {
        }
    }

    private long refreshNow() {
        return 500;
    }

    private final Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case ALBUM_ART_DECODED:
                    mCover.setImageBitmap((Bitmap)msg.obj);
                    mCover.getDrawable().setDither(true);
                    break;

                case REFRESH:
                    long next = refreshNow();
                    queueNextRefresh(next);
                    break;
                    
                case QUIT:
                    // This can be moved back to onCreate once the bug that prevents
                    // Dialogs from being started from onCreate/onResume is fixed.
/*                    new AlertDialog.Builder(MediaPlaybackActivity.this)
                            .setTitle(R.string.service_start_error_title)
                            .setMessage(R.string.service_start_error_msg)
                            .setPositiveButton(R.string.service_start_error_button,
                                    new DialogInterface.OnClickListener() {
                                        public void onClick(DialogInterface dialog, int whichButton) {
                                            finish();
                                        }
                                    })
                            .setCancelable(false)
                            .show();*/
                    break;

                default:
                    break;
            }
        }
    };
    
    BroadcastReceiver mMediaServiceReceiver;
    void startMediaServiceListener() {
	    mMediaServiceReceiver = new BroadcastReceiver() {
	        @Override
	        public void onReceive(Context context, Intent intent) {
	        	if (null == progressDialog && intent.getAction().toString().equals ("org.deadbeef.android.ADD_FILES_START")) {
					Log.w("DDB", "received ADD_FILES_START");
    				progressDialog = ProgressDialog.show(Deadbeef.this,      
    					"Please wait",
    					"Adding files to playlist...", true);
    			}
	        	else if (null != progressDialog && intent.getAction().toString().equals ("org.deadbeef.android.ADD_FILES_FINISH")) {
					Log.w("DDB", "received ADD_FILES_END");
    				progressDialog.dismiss();
    				progressDialog = null;
			        /*final FileListAdapter adapter = new FileListAdapter(Deadbeef.this, R.layout.plitem, R.id.title); 
			        handler.post(new Runnable() {
			            public void run() {
			                setListAdapter(adapter);
			            }
			        });*/
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

    private static class AlbumSongIdWrapper {
        public long albumid;
        public long songid;
        AlbumSongIdWrapper(long aid, long sid) {
            albumid = aid;
            songid = sid;
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
	    		
		    	TextView st;
		    	String totaltime_str = getTotalTimeFormatted ();

		    	/*st = (TextView)findViewById(R.id.plstate);
		    	String plstate = String.format ("%d tracks | %s total playtime", DeadbeefAPI.pl_getcount (0), totaltime_str);
		    	if (!plstate_prev.equals(plstate)) {
		    		plstate_prev = plstate;
		    		st.setText(plstate);
		    	}*/
	    		
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
	        			button.setImageResource (R.drawable.playbar_play);
	        		}
	        		else {
	        			button.setImageResource (R.drawable.playbar_pause);
	        		}
	    		}
	    		
	    		// shuffle button
	    		int new_order = MusicUtils.sService.getPlayOrder();
	    		if (new_order != play_order) {
	    			play_order = new_order;
	        		ImageButton button = (ImageButton)findViewById(R.id.ShuffleMode);
	    			if (play_order == 0) {
	        			button.setImageResource (-1);
	    			}
	    			else if (play_order == 1) {
	        			button.setImageResource (R.drawable.shuffle);
	    			}
	    			else if (play_order == 3) {
	        			button.setImageResource (R.drawable.shuffle_albums);
	    			}
	    		}
	    		
	    		// repeat button
	    		int new_mode = MusicUtils.sService.getPlayMode();
	    		if (new_mode != play_mode) {
	    			play_mode = new_mode;
	    			ImageButton button = (ImageButton)findViewById(R.id.RepeatMode);
	    			if (play_mode == 0) {
	        			button.setImageResource (-1);
	    			}
	    			else if (play_mode == 1) {
	        			button.setImageResource (R.drawable.repeat);
	    			}
	    			else if (play_mode == 2) {
	        			button.setImageResource (R.drawable.repeat_single);
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
	    	    		
	    	    		int trk = DeadbeefAPI.streamer_get_playing_track ();
	    	    		if (0 != trk) {
				            String path = DeadbeefAPI.pl_get_track_path (trk);
				            
			    			if (path.toLowerCase().endsWith(".sid")) {
						        BitmapFactory.Options opts = new BitmapFactory.Options();
						        opts.inPreferredConfig = Bitmap.Config.ARGB_8888;
						        Context context = Deadbeef.this;
						        Bitmap bmp = BitmapFactory.decodeStream(
						        		context.getResources().openRawResource(R.drawable.albumart_sid), null, opts);
			    				mCover.setImageBitmap(bmp);
		                    	mCover.getDrawable().setDither(true);
			    			}
			    			else if (path.toLowerCase().endsWith(".nsf")) {
						        BitmapFactory.Options opts = new BitmapFactory.Options();
						        opts.inPreferredConfig = Bitmap.Config.ARGB_8888;
						        Context context = Deadbeef.this;
						        Bitmap bmp = BitmapFactory.decodeStream(
						        		context.getResources().openRawResource(R.drawable.albumart_nes), null, opts);
			    				mCover.setImageBitmap(bmp);
		                    	mCover.getDrawable().setDither(true);
			    			}
			    			else {
					            long songid = -1;
				                long albumid = -1;
					            if (path != null) {
	
								    Cursor mCursor = null;
								    String[] mCursorCols = new String[] {
								            "audio._id AS _id",             // index must match IDCOLIDX below
								            MediaStore.Audio.Media.ALBUM_ID,
								    };
						            
					                ContentResolver resolver = getContentResolver();
					                Uri uri;
					                String where;
					                String selectionArgs[];
					                if (path.startsWith("content://media/")) {
					                    uri = Uri.parse(path);
					                    where = null;
					                    selectionArgs = null;
					                } else {
					                   uri = MediaStore.Audio.Media.getContentUriForPath(path);
					                   where = MediaStore.Audio.Media.DATA + "=?";
					                   selectionArgs = new String[] { path };
					                }
					                
					                try {
					                    mCursor = resolver.query(uri, mCursorCols, where, selectionArgs, null);
					                    if  (mCursor != null) {
					                        if (mCursor.getCount() == 0) {
					                            mCursor.close();
					                            mCursor = null;
					                        } else {
					                            mCursor.moveToNext();
					                            songid = mCursor.getLong(IDCOLIDX);
					                        }
					                    }
					                } catch (UnsupportedOperationException ex) {
					                }   	    			
	
						            if (songid >= 0) {
						            	if (mCursor != null) {
			            	                albumid = mCursor.getLong(mCursor.getColumnIndexOrThrow(MediaStore.Audio.Media.ALBUM_ID));
						            	}
						            }
					            }
				                mAlbumArtHandler.removeMessages(GET_ALBUM_ART);
				                mAlbumArtHandler.obtainMessage(GET_ALBUM_ART, new AlbumSongIdWrapper(albumid, songid)).sendToTarget();
			    			}
			                mCover.setVisibility(View.VISIBLE);
	    	    			DeadbeefAPI.pl_item_unref (trk);
	    	    			
	    	    		}
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
    	if (null != i.getAction()) {
	    	if (i.getAction().equals("android.intent.action.VIEW")) {
	    		try {
	    			MusicUtils.sService.startFile (i.getData().toString());
	    		} catch (RemoteException ex) {
	    		}
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

        mAlbumArtWorker = new Worker("album art worker");
        mAlbumArtHandler = new AlbumArtHandler(mAlbumArtWorker.getLooper());
        
        setContentView(R.layout.main);
        
        // set album art widget to square size
        DisplayMetrics dm = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(dm);
        
        mCover = (ImageView) findViewById(R.id.cover);
        mCover.setScaleType (ImageView.ScaleType.CENTER_CROP);
        mAlbumArtHandler.obtainMessage(GET_ALBUM_ART, new AlbumSongIdWrapper(-1, -1)).sendToTarget();
        mCover.setVisibility(View.VISIBLE);
        
        LayoutParams p = mCover.getLayoutParams();
        p.height = p.width = MusicUtils.mArtworkWidth = MusicUtils.mArtworkHeight = dm.widthPixels;
        mCover.setLayoutParams(p);
        
        ((ImageButton)findViewById(R.id.playlist)).setOnClickListener(mPlaylistListener);
        ((ImageButton)findViewById(R.id.prev)).setOnClickListener(mPrevListener);
        ((ImageButton)findViewById(R.id.play)).setOnClickListener(mPlayPauseListener);
        ((ImageButton)findViewById(R.id.next)).setOnClickListener(mNextListener);
        ((ImageButton)findViewById(R.id.add)).setOnClickListener(mAddListener);
        ((ImageButton)findViewById(R.id.ShuffleMode)).setOnClickListener(mShuffleModeListener);
        ((ImageButton)findViewById(R.id.RepeatMode)).setOnClickListener(mRepeatModeListener);

        SeekBar sb = (SeekBar)findViewById(R.id.seekbar);
        sb.setMax(100);
        sb.setOnSeekBarChangeListener(sbChangeListener);

        seekbar = (SeekBar)findViewById(R.id.seekbar);

		Log.e("DDB", "Deadbeef.onCreate bind");
        MusicUtils.bindToService(this, new ServiceConnection() {
	        public void onServiceConnected(ComponentName className, IBinder obj) {
	        	Log.e("DDB", "Deadbeef.onCreate connected");
	        	MusicUtils.sService = IMediaPlaybackService.Stub.asInterface(obj);
		        /*final FileListAdapter adapter = new FileListAdapter(Deadbeef.this, R.layout.plitem, R.id.title); 
		        handler.post(new Runnable() {
		            public void run() {
		                setListAdapter(adapter);
		            }
		        });*/
		        startMediaServiceListener ();
	        }
	
	        public void onServiceDisconnected(ComponentName className) {
		        stopMediaServiceListener ();
	        	MusicUtils.sService = null;
	        }
	    });

//        final FileListAdapter adapter = new FileListAdapter(this, R.layout.plitem, R.id.title); 
//        setListAdapter(adapter);   
        
    }
    
    @Override
    public void onDestroy() {
        mAlbumArtWorker.quit();
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
/*	        final FileListAdapter adapter = new FileListAdapter(this, R.layout.plitem, R.id.title); 
	        handler.post(new Runnable() {
	            public void run() {
	                setListAdapter(adapter);
	            }
	        });*/
    	}
    	if (requestCode == REQUEST_ADD_FOLDER_AFTER && resultCode == RESULT_OK) {
/*	        final FileListAdapter adapter = new FileListAdapter(this, R.layout.plitem, R.id.title); 
	        handler.post(new Runnable() {
	            public void run() {
	                setListAdapter(adapter);
	            }
	        });*/
    	}
    	else if (requestCode == REQUEST_SELECT_PLAYLIST && resultCode >= 0) {
/*    		DeadbeefAPI.plt_set_curr (resultCode);
	        final FileListAdapter adapter = new FileListAdapter(this, R.layout.plitem, R.id.title); 
	        handler.post(new Runnable() {
	            public void run() {
	                setListAdapter(adapter);
	            }
	        });*/
    	}
    }

    @Override
    public boolean onOptionsItemSelected (MenuItem item) {
        int id = item.getItemId ();
        if (id == R.id.menu_clear_playlist) {
        	DeadbeefAPI.pl_clear (); // FIXME
/*	        final FileListAdapter adapter = new FileListAdapter(this, R.layout.plitem, R.id.title); 
	        handler.post(new Runnable() {
	            public void run() {
	                setListAdapter(adapter);
	            }
	        });*/
        }
        else if (id == R.id.menu_manage_playlists) {
        	Intent i = new Intent (this, SelectPlaylist.class);
	    	startActivityForResult(i, REQUEST_SELECT_PLAYLIST);
        }
        else if (id == R.id.menu_about) {
        	showDialog (0);
        }
        else if (id == R.id.menu_equalizer) {
        	Intent i = new Intent (this, EQ.class);
	    	startActivity(i);
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

    private OnClickListener mPlaylistListener = new OnClickListener() {
        public void onClick(View v) {
	        Intent i = new Intent (Deadbeef.this, PlaylistViewer.class);
	    	startActivity(i);
        }
    };

    private OnClickListener mAddListener = new OnClickListener() {
        public void onClick(View v) {
        	AddFolder ();
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
    
    public class AlbumArtHandler extends Handler {
        private long mAlbumId = -1;
        
        public AlbumArtHandler(Looper looper) {
            super(looper);
        }
        @Override
        public void handleMessage(Message msg)
        {
            long albumid = ((AlbumSongIdWrapper) msg.obj).albumid;
            long songid = ((AlbumSongIdWrapper) msg.obj).songid;
            if (msg.what == GET_ALBUM_ART && (mAlbumId != albumid || albumid < 0)) {
                // while decoding the new image, show the default album art
                Message numsg = mHandler.obtainMessage(ALBUM_ART_DECODED, null);
                mHandler.removeMessages(ALBUM_ART_DECODED);
                mHandler.sendMessageDelayed(numsg, 300);
                Bitmap bm = MusicUtils.getArtwork(Deadbeef.this, songid, albumid);
                if (bm == null) {
                    bm = MusicUtils.getArtwork(Deadbeef.this, songid, -1);
                    albumid = -1;
                }
                if (bm != null) {
                    numsg = mHandler.obtainMessage(ALBUM_ART_DECODED, bm);
                    mHandler.removeMessages(ALBUM_ART_DECODED);
                    mHandler.sendMessage(numsg);
                }
                mAlbumId = albumid;
            }
        }
    }
	
    private static class Worker implements Runnable {
        private final Object mLock = new Object();
        private Looper mLooper;
        
        /**
         * Creates a worker thread with the given name. The thread
         * then runs a {@link android.os.Looper}.
         * @param name A name for the new thread
         */
        Worker(String name) {
            Thread t = new Thread(null, this, name);
            t.setPriority(Thread.MIN_PRIORITY);
            t.start();
            synchronized (mLock) {
                while (mLooper == null) {
                    try {
                        mLock.wait();
                    } catch (InterruptedException ex) {
                    }
                }
            }
        }
        
        public Looper getLooper() {
            return mLooper;
        }
        
        public void run() {
            synchronized (mLock) {
                Looper.prepare();
                mLooper = Looper.myLooper();
                mLock.notifyAll();
            }
            Looper.loop();
        }
        
        public void quit() {
            mLooper.quit();
        }
    }

}
