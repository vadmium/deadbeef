package org.deadbeef_common.android;

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
import android.provider.Settings.Secure;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.CheckBox;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

import com.android.vending.licensing.AESObfuscator;
import com.android.vending.licensing.LicenseChecker;
import com.android.vending.licensing.LicenseCheckerCallback;
import com.android.vending.licensing.ServerManagedPolicy;

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
    private static final String BASE64_PUBLIC_KEY = "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAkImQu4KYIG7zDP/yrHhHr3bEh+WN+H1g6oQtwoq1L7KWmpD7b5x2a2w0Y3Y60vFb+73694ICZahSdADHlV36DmbmGX7CJFQAF/FYm+DP8hMUEPvUEyHCDZ+mwR6wnh7B4L4ywiHIByxmtAUK07gCRgCF4Y528DQhjlyga3O6r9QpCOPr6Y2cEAZkeDjIEowHcxyG/hFKeSn0fwTfPfc7Pj/W3qfZ9/7ksMquFQUB6DoXKMMMKd4BV7bEeB4bvhUJ/8M9NviBM9wLJjMqcEY8LrBZ49RBNXeW+zwSb6f8f7ZbLkRFoZMPdBwtiFStu8TUlPPUbSt0OBo1QxwiFlCLvQIDAQAB";
    
    private LicenseCheckerCallback mLicenseCheckerCallback;
    private LicenseChecker mChecker;
    private static final byte[] SALT = new byte[] {
    	91, 117, 85, -103, 0, -16, 118, -17, -100, 14, -13, 107, 79, -78, -23, -5, -73, -63, -34, -56
    };
    
    public static final boolean freeversion = true;

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
		Log.e("DDB", "Deadbeef.onCreate");
        super.onCreate(savedInstanceState);
        
        mAlbumArtWorker = new Worker("album art worker");
        mAlbumArtHandler = new AlbumArtHandler(mAlbumArtWorker.getLooper());
        
        setContentView(R.layout.main);
        
        // set album art widget to square size
//        DisplayMetrics dm = new DisplayMetrics();
//        getWindowManager().getDefaultDisplay().getMetrics(dm);
        
        mCover = (ImageView) findViewById(R.id.cover);
        mAlbumArtHandler.obtainMessage(GET_ALBUM_ART, new AlbumSongIdWrapper(-1, -1)).sendToTarget();
        mCover.setVisibility(View.VISIBLE);
        mCover.setOnClickListener(mCoverClickListener);
        
//        mCover.setScaleType (ImageView.ScaleType.CENTER_CROP);
        
/*        View coverwrap = (View)findViewById(R.id.coverwrap);
        coverwrap.setVisibility(View.VISIBLE);
        
        LinearLayout ll = (LinearLayout)findViewById (R.id.mainview);
        
        LayoutParams p = coverwrap.getLayoutParams();
        int min = p.height;
        if (p.width < min) {
        	min = p.width;
        }
        p.height = p.width = MusicUtils.mArtworkWidth = MusicUtils.mArtworkHeight = min;
        coverwrap.setLayoutParams(p);*/
        
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
		        startMediaServiceListener ();
    	        handler.post(new Runnable() {
				    public void run() {
				        if (0 == DeadbeefAPI.conf_get_int("android.freeplugins_dont_ask", 0)) {
					        if (!DeadbeefAPI.plugin_exists("stdmpg")) {
					        	showDialog (1);
					        }
				        }
				    }
				});

	        }
	
	        public void onServiceDisconnected(ComponentName className) {
	        	Log.e("DDB", "service disconnected");
		        stopMediaServiceListener ();
	        	MusicUtils.sService = null;
	        }
	    });
     
		// Construct the LicenseCheckerCallback. The library calls this when done.
        mLicenseCheckerCallback = new MyLicenseCheckerCallback();

        // Construct the LicenseChecker with a Policy.
        String deviceId = Secure.getString(getContentResolver(), Secure.ANDROID_ID);
        mChecker = new LicenseChecker(
            this, new ServerManagedPolicy(this,
                new AESObfuscator(SALT, getPackageName(), deviceId)),
                BASE64_PUBLIC_KEY
            );      
        
        mChecker.checkAccess(mLicenseCheckerCallback);
}
    
    @Override
    public void onDestroy() {
    	mChecker.onDestroy();
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

    
    private class MyLicenseCheckerCallback implements LicenseCheckerCallback {
        public void allow() {
            if (isFinishing()) {
                // Don't update UI if Activity is finishing.
                return;
            }
            // Should allow user access.
            Toast toast = Toast.makeText(Deadbeef.this, "Access Allowed", Toast.LENGTH_SHORT);
            toast.show();
        }

        public void dontAllow() {
            if (isFinishing()) {
                // Don't update UI if Activity is finishing.
                return;
            }
            Toast toast = Toast.makeText(Deadbeef.this, "Access Denied", Toast.LENGTH_SHORT);
            toast.show();

        }

		@Override
		public void applicationError(ApplicationErrorCode errorCode) {
			// TODO Auto-generated method stub
			
		}
    }

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
	
//		        String spaused = MusicUtils.sService.isPaused() ? "Paused | " : "";
		        String ft = DeadbeefAPI.pl_get_track_filetype (track);
		        int br = DeadbeefAPI.plt_get_item_bitrate (track);
		        sbtext_new = String.format ("%s | %s %dHz | %d bit | %s | %d:%02d / %s", ft != null ? ft : "-", br == -1 ? "" : br + " Kbps |", samplerate, bitspersample, mode, minpos, secpos, t);
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
    private String pltitle_prev = "";
    private ProgressDialog progressDialog;
    public int progressDepth = 0; 
    
    private void showProgress (boolean show) {
    	synchronized (this) {
	    	if (show) {
	    		progressDepth++;
	    	}
	    	else {
	    	  	progressDepth--;
	    	}
    	}
        handler.post(new Runnable() {
            public void run() {
            	if (progressDepth > 0 && null == progressDialog) {
					Log.w("DDB", "show progress");
					progressDialog = ProgressDialog.show(Deadbeef.this,      
						"Please wait",
						"Adding files to playlist...", true);
            	}
            	else if (progressDepth == 0 && null != progressDialog) {
					Log.w("DDB", "hide progress");
		        	progressDialog.dismiss();
		        	progressDialog = null;
            	}
            }
        });
    }
    
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
	        	if (intent.getAction().toString().equals ("org.deadbeef.android.ADD_FILES_START")) {
					Log.w("DDB", "main received ADD_FILES_START");
					showProgress (true);
    			}
	        	else if (intent.getAction().toString().equals ("org.deadbeef.android.ADD_FILES_FINISH")) {
					Log.w("DDB", "main received ADD_FILES_END");
					showProgress (false);
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
		    	
		    	String pltitle = DeadbeefAPI.plt_get_title (DeadbeefAPI.plt_get_curr ());
		    	if (pltitle != null && !pltitle.equals (pltitle_prev)) {
		    		st = (TextView)findViewById(R.id.playlist_title);
		    		st.setText(pltitle);
		    		pltitle_prev = pltitle;
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
    		DeadbeefAPI.plt_save_current ();
    	}
    	if (requestCode == REQUEST_ADD_FOLDER_AFTER && resultCode == RESULT_OK) {
    		DeadbeefAPI.plt_save_current ();
    	}
    	else if (requestCode == REQUEST_SELECT_PLAYLIST && resultCode >= 0) {
    		DeadbeefAPI.plt_set_curr_idx (resultCode);
    		DeadbeefAPI.conf_save ();
    	}
    }

    @Override
    public boolean onOptionsItemSelected (MenuItem item) {
        int id = item.getItemId ();
        if (id == R.id.menu_clear_playlist) {
        	DeadbeefAPI.pl_clear (); // FIXME: should be called through mediaservice, only when connected
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
        else if (id == R.id.menu_add_location) {
        	showDialog (2);
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
    	if (id == 0) {
	        final View textView = factory.inflate(R.layout.aboutbox, null);
	        return new AlertDialog.Builder(Deadbeef.this)
	            .setIcon(R.drawable.icon)
	            .setTitle("About")
	            .setView(textView)
	            .setNeutralButton(R.string.osslicenses_button, new DialogInterface.OnClickListener() {
	                public void onClick(DialogInterface dialog, int whichButton) {
	                	showDialog (3);
	                }
	            })
	            .create();
    	}
    	else if (id == 1) {
	        final View textView = factory.inflate(R.layout.install_freeplugins, null);
	        return new AlertDialog.Builder(Deadbeef.this)
	            .setIcon(R.drawable.icon)
	            .setTitle("Install Free Plugins")
	            .setView(textView)
	            .setPositiveButton("Yes", new DialogInterface.OnClickListener() {
	                public void onClick(DialogInterface dialog, int whichButton) {
	                	CheckBox b = (CheckBox)textView.findViewById(R.id.nomore);
	                	if (b.isChecked ()) {
	                		DeadbeefAPI.conf_set_int ("android.freeplugins_dont_ask", 1);
	                	}
	                	else {
	                		DeadbeefAPI.conf_set_int ("android.freeplugins_dont_ask", 0);
	                	}
	                	DeadbeefAPI.conf_save ();
						Intent i = new Intent(Intent.ACTION_VIEW,
								Uri.parse("market://search?q=pname:org.deadbeef.android.freeplugins"));
						startActivity(i);
	                }
	            })
	            .setNegativeButton("No", new DialogInterface.OnClickListener() {
	                public void onClick(DialogInterface dialog, int whichButton) {
	                	CheckBox b = (CheckBox)textView.findViewById(R.id.nomore);
	                	if (b.isChecked ()) {
	                		DeadbeefAPI.conf_set_int ("android.freeplugins_dont_ask", 1);
	                	}
	                	else {
	                		DeadbeefAPI.conf_set_int ("android.freeplugins_dont_ask", 0);
	                	}
	                	DeadbeefAPI.conf_save ();
	                }
	            })
	            .create();
    	}
    	else if (id == 2) {
    		return AddLocation ();
    	}
    	else if (id == 3) {
	        final View textView = factory.inflate(R.layout.osslicenses, null);
	        return new AlertDialog.Builder(Deadbeef.this)
	            .setIcon(R.drawable.icon)
	            .setTitle("Open source licenses")
	            .setView(textView)
	            .create();
    	}
    	return null;
    }
    
    private Dialog AddLocation () {
        LayoutInflater factory = LayoutInflater.from(this);
        final View textEntryView = factory.inflate(R.layout.addlocation, null);
        return new AlertDialog.Builder(Deadbeef.this)
            .setIcon(R.drawable.icon)
            .setTitle(R.string.add_location)
            .setView(textEntryView)
            .setPositiveButton(R.string.alert_dialog_ok, new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int whichButton) {
                	DeadbeefAPI.pl_add_file(((TextView)textEntryView.findViewById(R.id.title)).getText().toString());
                	DeadbeefAPI.plt_save_current ();
                }
            })
            .setNegativeButton(R.string.alert_dialog_cancel, new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int whichButton) {
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

    private OnClickListener mCoverClickListener = new OnClickListener() {
        public void onClick(View v) {
        	int trk = DeadbeefAPI.pl_get_current_idx ();
        	if (trk != -1) {
				Intent i = new Intent (Deadbeef.this, TrackPropertiesViewer.class);
				i.setData(Uri.fromParts(
						"track", String.valueOf (DeadbeefAPI.plt_get_curr()), String.valueOf(trk)
						));
		    	startActivity(i);
        	}
        }
    };

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
        		MusicUtils.sService.conf_save ();
        	}
        	catch (RemoteException e) {
        	}
        }
    };
    
    private OnClickListener mShuffleModeListener = new OnClickListener() {
        public void onClick(View v) {
        	try {
        		MusicUtils.sService.cycleShuffleMode ();
        		MusicUtils.sService.conf_save ();
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

