// based on source of google's official Music app 
package org.deadbeef.android;

import java.io.File;
import java.io.IOException;
import java.lang.ref.WeakReference;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.Iterator;
import java.util.List;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.appwidget.AppWidgetManager;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.media.AudioManager;
import android.os.Environment;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.telephony.PhoneStateListener;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.widget.RemoteViews;

public class MediaPlaybackService extends Service {
	private static final String LOGTAG = "MediaPlaybackService";
	public static final int PLAYBACKSERVICE_STATUS = 1;

	public static final String SERVICECMD = "org.deadbeef.android.musicservicecommand";
	public static final String CMDNAME = "command";
	public static final String CMDTOGGLEPAUSE = "togglepause";
	public static final String CMDSTOP = "stop";
	public static final String CMDPAUSE = "pause";
	public static final String CMDPREVIOUS = "previous";
	public static final String CMDNEXT = "next";

	public static final String TOGGLEPAUSE_ACTION = "org.deadbeef.android.musicservicecommand.togglepause";
	public static final String PAUSE_ACTION = "org.deadbeef.android.musicservicecommand.pause";
	public static final String PREVIOUS_ACTION = "org.deadbeef.android.musicservicecommand.previous";
	public static final String NEXT_ACTION = "org.deadbeef.android.musicservicecommand.next";
	
	public static final String META_CHANGED = "org.deadbeef.android.metachanged";
    public static final String PLAYSTATE_CHANGED = "org.deadbeef.android.playstatechanged";
    public static final String QUEUE_CHANGED = "org.deadbeef.android.queuechanged";


	private static final int TRACK_ENDED = 1;
	private static final int RELEASE_WAKELOCK = 2;
	private static final int SERVER_DIED = 3;
	private static final int FADEIN = 4;

	public Player mPlayer;
	private BroadcastReceiver mUnmountReceiver = null;
	private WakeLock mWakeLock;
	private int mServiceStartId = -1;
	private boolean mServiceInUse = false;
	// used to track what type of audio focus loss caused the playback to pause
	private boolean mPausedByTransientLossOfFocus = false;

	// interval after which we stop the service when idle
	private static final int IDLE_DELAY = 60000;

	private static final Class[] mStartForegroundSignature = new Class[] {
			int.class, Notification.class };
	private static final Class[] mStopForegroundSignature = new Class[] { boolean.class };

	private NotificationManager mNM;
	private Method mStartForeground;
	private Method mStopForeground;
	private Object[] mStartForegroundArgs = new Object[2];
	private Object[] mStopForegroundArgs = new Object[1];

	private AudioManager mAudioManager;
    private MediaAppWidgetProvider mAppWidgetProvider = MediaAppWidgetProvider.getInstance();

	/**
	 * This is a wrapper around the new startForeground method, using the older
	 * APIs if it is not available.
	 */
	void startForegroundCompat(int id, Notification notification) {
		// If we have the new startForeground API, then use it.
		if (mStartForeground != null) {
			mStartForegroundArgs[0] = Integer.valueOf(id);
			mStartForegroundArgs[1] = notification;
			try {
				mStartForeground.invoke(this, mStartForegroundArgs);
			} catch (InvocationTargetException e) {
				// Should not happen.
				Log.w("DDB", "Unable to invoke startForeground", e);
			} catch (IllegalAccessException e) {
				// Should not happen.
				Log.w("DDB", "Unable to invoke startForeground", e);
			}
			return;
		}

		// Fall back on the old API.
		setForeground(true);
		mNM.notify(id, notification);
	}

	/**
	 * This is a wrapper around the new stopForeground method, using the older
	 * APIs if it is not available.
	 */
	void stopForegroundCompat(int id) {
		// If we have the new stopForeground API, then use it.
		if (mStopForeground != null) {
			mStopForegroundArgs[0] = Boolean.TRUE;
			try {
				mStopForeground.invoke(this, mStopForegroundArgs);
			} catch (InvocationTargetException e) {
				// Should not happen.
				Log.w("DDB", "Unable to invoke stopForeground", e);
			} catch (IllegalAccessException e) {
				// Should not happen.
				Log.w("DDB", "Unable to invoke stopForeground", e);
			}
			return;
		}

		// Fall back on the old API. Note to cancel BEFORE changing the
		// foreground state, since we could be killed at that point.
		mNM.cancel(id);
		setForeground(false);
	}

	private void handleExternalStorageState (boolean avail, boolean writable) {
		Log.i("DDB", "handleExternalStorageState " + avail + " " + writable);
		if (!avail) {
			DeadbeefAPI.play_stop ();
		}
	}
	
    BroadcastReceiver mExternalStorageReceiver;
	boolean mExternalStorageAvailable = false;
	boolean mExternalStorageWriteable = false;
	
	void updateExternalStorageState() {
	    String state = Environment.getExternalStorageState();
	    if (Environment.MEDIA_MOUNTED.equals(state)) {
	        mExternalStorageAvailable = mExternalStorageWriteable = true;
	    } else if (Environment.MEDIA_MOUNTED_READ_ONLY.equals(state)) {
	        mExternalStorageAvailable = true;
	        mExternalStorageWriteable = false;
	    } else {
	        mExternalStorageAvailable = mExternalStorageWriteable = false;
	    }
	    handleExternalStorageState(mExternalStorageAvailable,
	            mExternalStorageWriteable);
	}
	
	void startWatchingExternalStorage() {
	    mExternalStorageReceiver = new BroadcastReceiver() {
	        @Override
	        public void onReceive(Context context, Intent intent) {
	            Log.i("test", "Storage: " + intent.getData());
	            updateExternalStorageState();
	        }
	    };
	    IntentFilter filter = new IntentFilter();
	    filter.addAction(Intent.ACTION_MEDIA_MOUNTED);
	    filter.addAction(Intent.ACTION_MEDIA_REMOVED);
	    registerReceiver(mExternalStorageReceiver, filter);
	    updateExternalStorageState();
	}
	
	void stopWatchingExternalStorage() {
	    unregisterReceiver(mExternalStorageReceiver);
	}

	
	private PhoneStateListener mPhoneStateListener = new PhoneStateListener() {
		@Override
		public void onCallStateChanged(int state, String incomingNumber) {
			if (state == TelephonyManager.CALL_STATE_RINGING) {
				/*
				 * AudioManager audioManager = (AudioManager)
				 * getSystemService(Context.AUDIO_SERVICE); int ringvolume =
				 * audioManager.getStreamVolume(AudioManager.STREAM_RING); if
				 * (ringvolume > 0) { pause(); }
				 */
			} else if (state == TelephonyManager.CALL_STATE_OFFHOOK) {
				// pause the music while a conversation is in progress
				pause();
			} else if (state == TelephonyManager.CALL_STATE_IDLE) {
				// call finished, do nothing
			}
		}
	};

    /**
     * Notify the change-receivers that something has changed.
     * The intent that is sent contains the following data
     * for the currently playing track:
     * "id" - Integer: the database row ID
     * "artist" - String: the name of the artist
     * "album" - String: the name of the album
     * "track" - String: the name of the track
     * The intent has an action that is one of
     * "com.android.music.metachanged"
     * "com.android.music.queuechanged",
     * "com.android.music.playbackcomplete"
     * "com.android.music.playstatechanged"
     * respectively indicating that a new track has
     * started playing, that the playback queue has
     * changed, that playback has stopped because
     * the last file in the list has been played,
     * or that the play-state changed (paused/resumed).
     */
    private void notifyChange(String what) {
        
        Intent i = new Intent(what);
        i.putExtra("artist", getArtistName());
        i.putExtra("album",getAlbumName());
        i.putExtra("track", getTrackName());
        i.putExtra("playing", DeadbeefAPI.is_streamer_active());//isPlaying());
        sendStickyBroadcast(i);
        
/*        if (what.equals(QUEUE_CHANGED)) {
            saveQueue(true);
        } else {
            saveQueue(false);
        }*/
        
        // Share this notification directly with our widgets
        mAppWidgetProvider.notifyChange(this, what);
    }

	
	private Handler mMediaplayerHandler = new Handler() {
		@Override
		public void handleMessage(Message msg) {
			switch (msg.what) {
			case FADEIN:
				/*
				 * if (!isPlaying()) { mCurrentVolume = 0f;
				 * mPlayer.setVolume(mCurrentVolume); play();
				 * mMediaplayerHandler.sendEmptyMessageDelayed(FADEIN, 10); }
				 * else { mCurrentVolume += 0.01f; if (mCurrentVolume < 1.0f) {
				 * mMediaplayerHandler.sendEmptyMessageDelayed(FADEIN, 10); }
				 * else { mCurrentVolume = 1.0f; }
				 * mPlayer.setVolume(mCurrentVolume); }
				 */
				break;
			case SERVER_DIED:
				/*
				 * if (mIsSupposedToBePlaying) { next(true); } else { // the
				 * server died when we were idle, so just // reopen the same
				 * song (it will start again // from the beginning though when
				 * the user // restarts) openCurrent(); }
				 */
				break;
			case TRACK_ENDED:
				/*
				 * if (mRepeatMode == REPEAT_CURRENT) { seek(0); play(); } else
				 * if (!mOneShot) { next(false); } else {
				 * notifyChange(PLAYBACK_COMPLETE); mIsSupposedToBePlaying =
				 * false; }
				 */
				break;
			case RELEASE_WAKELOCK:
				mWakeLock.release();
				break;
			default:
				break;
			}
		}
	};

	private BroadcastReceiver mIntentReceiver = new BroadcastReceiver() {
		@Override
		public void onReceive(Context context, Intent intent) {
			String action = intent.getAction();
			String cmd = intent.getStringExtra("command");
			if (CMDNEXT.equals(cmd) || NEXT_ACTION.equals(action)) {
				next();
			} else if (CMDPREVIOUS.equals(cmd)
					|| PREVIOUS_ACTION.equals(action)) {
				prev();
			} else if (CMDTOGGLEPAUSE.equals(cmd)
					|| TOGGLEPAUSE_ACTION.equals(action)) {
				if (isPlaying()) {
					pause();
					mPausedByTransientLossOfFocus = false;
				} else {
					play();
				}
			} else if (CMDPAUSE.equals(cmd) || PAUSE_ACTION.equals(action)) {
				pause();
				mPausedByTransientLossOfFocus = false;
			} else if (CMDSTOP.equals(cmd)) {
				pause();
				mPausedByTransientLossOfFocus = false;
				seek(0);
            } else if (MediaAppWidgetProvider.CMDAPPWIDGETUPDATE.equals(cmd)) {
                // Someone asked us to refresh a set of specific widgets, probably
                // because they were just added.
                int[] appWidgetIds = intent.getIntArrayExtra(AppWidgetManager.EXTRA_APPWIDGET_IDS);
                mAppWidgetProvider.performUpdate(MediaPlaybackService.this, appWidgetIds);
            }
		}
	};

	public MediaPlaybackService() {
	}
	
	static {
       initCompatibility();
    };
    
    private static Method mRegisterMediaButtonEventReceiver;
    private static void initCompatibility() {
       try {
           mRegisterMediaButtonEventReceiver = AudioManager.class.getMethod(
                   "registerMediaButtonEventReceiver", new Class[] { ComponentName.class } );
           /* success, this is a newer device */
       } catch (NoSuchMethodException nsme) {
           /* failure, must be older device */
       }
   }
    
    private static void registerMediaButtonEventReceiver(AudioManager audioManager, ComponentName componentName) throws IOException {
       try {
           mRegisterMediaButtonEventReceiver.invoke(audioManager, componentName);
       } catch (InvocationTargetException ite) {
           /* unpack original exception when possible */
           Throwable cause = ite.getCause();
           if (cause instanceof IOException) {
               throw (IOException) cause;
           } else if (cause instanceof RuntimeException) {
               throw (RuntimeException) cause;
           } else if (cause instanceof Error) {
               throw (Error) cause;
           } else {
               /* unexpected checked exception; wrap and re-throw */
               throw new RuntimeException(ite);
           }
       } catch (IllegalAccessException ie) {
           System.err.println("unexpected " + ie);
       }
   }
    

	@Override
	public void onCreate() {
		super.onCreate();
		mAudioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
		if (mRegisterMediaButtonEventReceiver != null) {
			try {
				registerMediaButtonEventReceiver(mAudioManager, new ComponentName(
						getPackageName(), MediaButtonIntentReceiver.class.getName()));
			}
			catch (IOException ex) {
			}
		}

		mNM = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
		try {
			mStartForeground = getClass().getMethod("startForeground",
					mStartForegroundSignature);
			mStopForeground = getClass().getMethod("stopForeground",
					mStopForegroundSignature);
		} catch (NoSuchMethodException e) {
			// Running on an older platform.
			mStartForeground = mStopForeground = null;
		}

		startWatchingExternalStorage ();

		// get all packages matching org.android.deadbeef.*
		PackageManager pkm = getPackageManager();
		List<PackageInfo> list = pkm.getInstalledPackages(0);
		Iterator<PackageInfo> it=list.iterator();
		
		String pluginPath = "";

        while(it.hasNext())
        {
        	String nm=(String)it.next().packageName;
        	
        	if (nm.startsWith ("org.deadbeef.android.")) {
        		pluginPath += ":" + nm;
        	}
        }
		
        File dir = Environment.getExternalStorageDirectory();
		String filesDir = dir.getAbsolutePath();
		filesDir += "/.deadbeef";
		
		DeadbeefAPI.start(filesDir, pluginPath);
		mPlayer = new Player();

		IntentFilter commandFilter = new IntentFilter();
		commandFilter.addAction(SERVICECMD);
		commandFilter.addAction(TOGGLEPAUSE_ACTION);
		commandFilter.addAction(PAUSE_ACTION);
		commandFilter.addAction(NEXT_ACTION);
		commandFilter.addAction(PREVIOUS_ACTION);
		registerReceiver(mIntentReceiver, commandFilter);

		TelephonyManager tmgr = (TelephonyManager) getSystemService(Context.TELEPHONY_SERVICE);
		tmgr.listen(mPhoneStateListener, PhoneStateListener.LISTEN_CALL_STATE);

		PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
		mWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, this
				.getClass().getName());
		mWakeLock.setReferenceCounted(false);

		// If the service was idle, but got killed before it stopped itself, the
		// system will relaunch it. Make sure it gets stopped again in that
		// case.
		Message msg = mDelayedStopHandler.obtainMessage();
		mDelayedStopHandler.sendMessageDelayed(msg, IDLE_DELAY);
	}

	@Override
	public void onDestroy() {
		// Check that we're not being destroyed while something is still
		// playing.
		if (isPlaying()) {
			Log.e(LOGTAG, "Service being destroyed while still playing.");
		}
		// release all MediaPlayer resources, including the native player and
		// wakelocks
   		Log.e("DDB","mediaPlaybackService onDestroy");
		mPlayer.stop();
		mPlayer = null;
		DeadbeefAPI.stop();
		stopWatchingExternalStorage ();

		// make sure there aren't any other messages coming
		mDelayedStopHandler.removeCallbacksAndMessages(null);
		mMediaplayerHandler.removeCallbacksAndMessages(null);

		unregisterReceiver(mIntentReceiver);
		if (mUnmountReceiver != null) {
			unregisterReceiver(mUnmountReceiver);
			mUnmountReceiver = null;
		}
		mWakeLock.release();
		super.onDestroy();
	}

	@Override
	public IBinder onBind(Intent intent) {
		mDelayedStopHandler.removeCallbacksAndMessages(null);
		mServiceInUse = true;
		return mBinder;
	}

	@Override
	public void onRebind(Intent intent) {
		mDelayedStopHandler.removeCallbacksAndMessages(null);
		mServiceInUse = true;
	}

	private void handleCommand(Intent intent, int startId) {
		mServiceStartId = startId;
		mDelayedStopHandler.removeCallbacksAndMessages(null);

		if (intent != null) {
			String action = intent.getAction();
			String cmd = intent.getStringExtra("command");

			if (CMDNEXT.equals(cmd) || NEXT_ACTION.equals(action)) {
				next();
			} else if (CMDPREVIOUS.equals(cmd)
					|| PREVIOUS_ACTION.equals(action)) {
				if (position() < 2000) {
					prev();
				} else {
					seek(0);
					play();
				}
			} else if (CMDTOGGLEPAUSE.equals(cmd)
					|| TOGGLEPAUSE_ACTION.equals(action)) {
				if (isPlaying()) {
					pause();
					mPausedByTransientLossOfFocus = false;
				} else {
					play();
				}
			} else if (CMDPAUSE.equals(cmd) || PAUSE_ACTION.equals(action)) {
				pause();
				mPausedByTransientLossOfFocus = false;
			} else if (CMDSTOP.equals(cmd)) {
				pause();
				mPausedByTransientLossOfFocus = false;
				seek(0);
			}
		}

		// make sure the service will shut down on its own if it was
		// just started but not bound to and nothing is playing
		mDelayedStopHandler.removeCallbacksAndMessages(null);
		Message msg = mDelayedStopHandler.obtainMessage();
		mDelayedStopHandler.sendMessageDelayed(msg, IDLE_DELAY);
	}

	// This is the old onStart method that will be called on the pre-2.0
	// platform. On 2.0 or later we override onStartCommand() so this
	// method will not be called.
	@Override
	public void onStart(Intent intent, int startId) {
		handleCommand(intent, startId);
	}

	/*
	 * @Override public int onStartCommand(Intent intent, int flags, int
	 * startId) { handleCommand(intent, startId); return START_STICKY; }
	 */

	@Override
	public boolean onUnbind(Intent intent) {
		mServiceInUse = false;

		if (DeadbeefAPI.is_streamer_active() || mPausedByTransientLossOfFocus) {
			return true;
		}

		// No active playlist, OK to stop the service right now
		stopSelf(mServiceStartId);
		return true;
	}

	private Handler mDelayedStopHandler = new Handler() {
		@Override
		public void handleMessage(Message msg) {
			// Check again to make sure nothing is playing right now
			if (isPlaying() || mPausedByTransientLossOfFocus || mServiceInUse
					|| mMediaplayerHandler.hasMessages(TRACK_ENDED)) {
				return;
			}
			stopSelf(mServiceStartId);
		}
	};

	// play current (or 1st)
	public void play() {
		synchronized (this) {
			boolean  paused = DeadbeefAPI.play_is_paused();
			if (paused) {
				DeadbeefAPI.play_play();
			} else {
				DeadbeefAPI.play_idx(0);
			}
			refreshStatus();
		}
	}

	// play specific
	public void playIdx(int idx) {
		synchronized (this) {
			DeadbeefAPI.play_idx(idx);
			refreshStatus();
		}
	}

	public void refreshStatus() {
		if (!isPlaying()) {
			return;
		}
        notifyChange(META_CHANGED);
		// update statusbar
		RemoteViews views = new RemoteViews(getPackageName(),
				R.layout.statusbar);
		views.setTextViewText(R.id.trackname, getTrackName());
		views.setTextViewText(R.id.artistalbum, getArtistName() + " - "
				+ getAlbumName());
		views.setImageViewResource(R.id.icon,
				R.drawable.stat_notify_musicplayer);
		Notification status = new Notification();
		status.contentView = views;
		status.flags |= Notification.FLAG_ONGOING_EVENT;
		status.icon = R.drawable.stat_notify_musicplayer;
		status.contentIntent = PendingIntent.getActivity(this, 0, new Intent(
				"org.deadbeef.android.PLAYBACK_VIEWER")
				.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK), 0);
		startForegroundCompat(PLAYBACKSERVICE_STATUS, status);
	}

	public boolean isPaused() {
		return DeadbeefAPI.play_is_paused();
	}

	private void stop() {
		synchronized (this) {
			Log.e("DDB","MediaPlaybackService.stop");
			mPlayer.stop();
			gotoIdleState();
			stopForegroundCompat(PLAYBACKSERVICE_STATUS);
		}
	}

	/**
	 * Pauses playback (call play() to resume)
	 */
	public void pause() {
		synchronized (this) {
			if (isPlaying()) {
				DeadbeefAPI.play_pause();
				gotoIdleState();
				notifyChange(PLAYSTATE_CHANGED);
			}
		}
	}

	public boolean isPlaying() {
		return DeadbeefAPI.play_is_playing();
	}

	public boolean isStopped() {
		return DeadbeefAPI.play_is_stopped();
	}

	public void prev() {
		DeadbeefAPI.play_prev();
	}

	public void next() {
		DeadbeefAPI.play_next();
	}

	private void gotoIdleState() {
		mDelayedStopHandler.removeCallbacksAndMessages(null);
		Message msg = mDelayedStopHandler.obtainMessage();
		mDelayedStopHandler.sendMessageDelayed(msg, IDLE_DELAY);
		stopForegroundCompat(PLAYBACKSERVICE_STATUS);
	}

	/**
	 * Returns the duration of the file in seconds
	 */
	public float duration() {
		synchronized (this) {
			return DeadbeefAPI.play_get_duration_seconds();
		}
	}

	/**
	 * Returns the current playback position in seconds
	 */
	public float position() {
		synchronized (this) {
			return DeadbeefAPI.play_get_pos_seconds();
		}
	}

	public void seek(float pos) {
		synchronized (this) {
			DeadbeefAPI.play_seek(pos);
		}
	}

	String getAlbumName() {
		synchronized (this) {
			int track = DeadbeefAPI.pl_get_current_idx();
			String album = DeadbeefAPI.pl_get_metadata(track, "album");
			if (album == "" || album == null) {
				album = "Unknown Album";
			}
			return album;
		}
	}

	String getArtistName() {
		synchronized (this) {
			int track = DeadbeefAPI.pl_get_current_idx();
			String artist = DeadbeefAPI.pl_get_metadata(track, "artist");
			if (artist == "" || artist == null) {
				artist = "Unknown Artist";
			}
			return artist;
		}
	}

	String getTrackName() {
		synchronized (this) {
			int track = DeadbeefAPI.pl_get_current_idx();
			return DeadbeefAPI.pl_get_metadata(track, "title");
		}
	}
	
	
	boolean m_bAddingFiles = false;
	public boolean busyAddingFiles () {
		return m_bAddingFiles;
	}

    public void pl_add_dir (final String dir) {
		synchronized (this) {
			if (m_bAddingFiles) {
				return;
			}
			m_bAddingFiles = true;
			Log.w("DDB", "ADD_DIR_START");
			sendBroadcast (new Intent ().setAction ("org.deadbeef.android.ADD_FILES_START"));
			new Thread () {
				public void run () {
					DeadbeefAPI.pl_add_folder (dir);
					m_bAddingFiles = false;
					Log.w("DDB", "ADD_DIR_END");
					sendBroadcast (new Intent ().setAction ("org.deadbeef.android.ADD_FILES_FINISH"));
				}
			}.start ();
		}
    }
    
    public void pl_insert_dir (final String dir, final int plt, final int after) {		
		synchronized (this) {
			if (m_bAddingFiles) {
				return;
			}
			final String d = dir;
			m_bAddingFiles = true;
			Log.w("DDB", "INSERT_DIR_START");
			sendBroadcast (new Intent ().setAction ("org.deadbeef.android.ADD_FILES_START"));
			new Thread () {
				public void run () {
					DeadbeefAPI.pl_insert_dir (plt, after, dir);
					m_bAddingFiles = false;
					Log.w("DDB", "INSERT_DIR_END");
					sendBroadcast (new Intent ().setAction ("org.deadbeef.android.ADD_FILES_FINISH"));
				}
			}.start ();
		}
    }
    
    public void handle_ddb_events () {
    	while (DeadbeefAPI.event_is_pending ()) {
			String id = DeadbeefAPI.event_get_type ();
			
			if (id.equals("songstarted")) {
				String artist = DeadbeefAPI.event_get_string(0);
				String album = DeadbeefAPI.event_get_string(1);
				String title = DeadbeefAPI.event_get_string(2);
				int dur = DeadbeefAPI.event_get_int(0);
				if (title != null && artist != null && dur > 0) {
					Intent i = new Intent("fm.last.android.metachanged");
					i.putExtra("artist", artist);
					if (album != null) {
						i.putExtra("album", album);
					}
					i.putExtra("track", title);
					i.putExtra("duration", dur);
					sendBroadcast(i);
				}
			}
			else if (id.equals ("paused")) {
				Intent i = new Intent("fm.last.android.playbackpaused");
				sendBroadcast(i);
			}
			else if (id.equals ("resumed")) {
				String artist = DeadbeefAPI.event_get_string(0);
				String album = DeadbeefAPI.event_get_string(1);
				String title = DeadbeefAPI.event_get_string(2);
				int dur = DeadbeefAPI.event_get_int(0);
				int pos = DeadbeefAPI.event_get_int(1);
				if (title != null && artist != null && dur > 0 && pos >= 0) {
					Intent i = new Intent("fm.last.android.metachanged");
					i.putExtra("artist", artist);
					if (album != null) {
						i.putExtra("album", album);
					}
					i.putExtra("track", title);
					i.putExtra("duration", dur);
					i.putExtra("position", pos);
					sendBroadcast(i);
				}
			}    				
			else if (id.equals("songfinished")) {
			}
			
			DeadbeefAPI.event_dispatch ();
		}
    }
	
	
	/*
	 * By making this a static class with a WeakReference to the Service, we
	 * ensure that the Service can be GCd even when the system process still has
	 * a remote reference to the stub.
	 */
	static class ServiceStub extends IMediaPlaybackService.Stub {
		WeakReference<MediaPlaybackService> mService;

		ServiceStub(MediaPlaybackService service) {
			mService = new WeakReference<MediaPlaybackService>(service);
		}

		public boolean isPlaying() {
			return mService.get().isPlaying();
		}
		public boolean isStopped() {
			return mService.get().isStopped();
		}

		public void stop() {
			mService.get().stop();
		}

		public void pause() {
			mService.get().pause();
		}

		public void play() {
			mService.get().play();
		}

		public void prev() {
			mService.get().prev();
		}

		public void next() {
			mService.get().next();
		}

		public String getTrackName() {
			return mService.get().getTrackName();
		}

		public String getAlbumName() {
			return mService.get().getAlbumName();
		}

		public String getArtistName() {
			return mService.get().getArtistName();
		}

		public float position() {
			return mService.get().position();
		}

		public float duration() {
			return mService.get().duration();
		}

		public void seek(float pos) {
			mService.get().seek(pos);
		}

		public boolean isPaused() {
			return mService.get().isPaused();
		}

		public void refreshStatus() {
			mService.get().refreshStatus();
		}

		public void playIdx(int idx) {
			mService.get().playIdx(idx);
		}

		public void cycleRepeatMode() {
			int mode = DeadbeefAPI.get_play_mode();
			mode++;
			if (mode > 2) {
				mode = 0;
			}
			DeadbeefAPI.set_play_mode(mode);
		}

		public void cycleShuffleMode() {
			int mode = DeadbeefAPI.get_play_order();
			mode++;
			if (mode == 2) { // skip random mode -- shuffle is enough
				mode = 3;
			}
			if (mode > 3) {
				mode = 0;
			}
			DeadbeefAPI.set_play_order(mode);
		}

		public void startFile (String fname) {
			int idx = DeadbeefAPI.pl_add_file (fname);
			if (idx != -1) {
				DeadbeefAPI.play_idx(idx);
			}
		}
		public int getCurrentIdx() {
			return DeadbeefAPI.pl_get_current_idx ();
		}
		public int getPlayOrder() {
			return DeadbeefAPI.get_play_order ();
		}
		public int getPlayMode() {
			return DeadbeefAPI.get_play_mode ();
		}
		
	    public void pl_add_dir (String dir) {
	    	mService.get().pl_add_dir (dir);
	    }
	    public void pl_insert_dir (String dir, int plt, int after) {		
	    	mService.get().pl_insert_dir (dir, plt, after);
	    }
	    public boolean busyAddingFiles () {
	    	return mService.get().busyAddingFiles ();
	    }
	    public void handle_ddb_events () {
	    	mService.get().handle_ddb_events ();
	    }
	    public int conf_save () {
	    	return DeadbeefAPI.conf_save ();
	    }
	}

	private final IBinder mBinder = new ServiceStub(this);

}
