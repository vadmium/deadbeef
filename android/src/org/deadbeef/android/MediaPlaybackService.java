// based on stock Music app mediaplaybackservice 
package org.deadbeef.android;

import java.lang.ref.WeakReference;

import android.app.Notification;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
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

    private static final int TRACK_ENDED = 1;
    private static final int RELEASE_WAKELOCK = 2;
    private static final int SERVER_DIED = 3;
    private static final int FADEIN = 4;
    
    public Player mPlayer;
    private BroadcastReceiver mUnmountReceiver = null;
    private WakeLock mWakeLock;
    private int mServiceStartId = -1;
    private boolean mServiceInUse = false;
    private boolean mIsSupposedToBePlaying = false;
    // used to track what type of audio focus loss caused the playback to pause
    private boolean mPausedByTransientLossOfFocus = false;

    private SharedPreferences mPreferences;
    
    // interval after which we stop the service when idle
    private static final int IDLE_DELAY = 60000;
    
    private Handler mMediaplayerHandler = new Handler() {
        float mCurrentVolume = 1.0f;
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case FADEIN:
/*                    if (!isPlaying()) {
                        mCurrentVolume = 0f;
                        mPlayer.setVolume(mCurrentVolume);
                        play();
                        mMediaplayerHandler.sendEmptyMessageDelayed(FADEIN, 10);
                    } else {
                        mCurrentVolume += 0.01f;
                        if (mCurrentVolume < 1.0f) {
                            mMediaplayerHandler.sendEmptyMessageDelayed(FADEIN, 10);
                        } else {
                            mCurrentVolume = 1.0f;
                        }
                        mPlayer.setVolume(mCurrentVolume);
                    }*/
                    break;
                case SERVER_DIED:
/*                    if (mIsSupposedToBePlaying) {
                        next(true);
                    } else {
                        // the server died when we were idle, so just
                        // reopen the same song (it will start again
                        // from the beginning though when the user
                        // restarts)
                        openCurrent();
                    }*/
                    break;
                case TRACK_ENDED:
/*                    if (mRepeatMode == REPEAT_CURRENT) {
                        seek(0);
                        play();
                    } else if (!mOneShot) {
                        next(false);
                    } else {
                        notifyChange(PLAYBACK_COMPLETE);
                        mIsSupposedToBePlaying = false;
                    }*/
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
            } else if (CMDPREVIOUS.equals(cmd) || PREVIOUS_ACTION.equals(action)) {
                prev();
            } else if (CMDTOGGLEPAUSE.equals(cmd) || TOGGLEPAUSE_ACTION.equals(action)) {
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
    };

    public MediaPlaybackService() {
    }

    @Override
    public void onCreate() {
        super.onCreate();

        mPreferences = getSharedPreferences("Music", MODE_WORLD_READABLE | MODE_WORLD_WRITEABLE);
        
        registerExternalStorageListener();

		DeadbeefAPI.start();
        mPlayer = new Player();
        mPlayer.setHandler(mMediaplayerHandler);

        IntentFilter commandFilter = new IntentFilter();
        commandFilter.addAction(SERVICECMD);
        commandFilter.addAction(TOGGLEPAUSE_ACTION);
        commandFilter.addAction(PAUSE_ACTION);
        commandFilter.addAction(NEXT_ACTION);
        commandFilter.addAction(PREVIOUS_ACTION);
        registerReceiver(mIntentReceiver, commandFilter);
        
        PowerManager pm = (PowerManager)getSystemService(Context.POWER_SERVICE);
        mWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, this.getClass().getName());
        mWakeLock.setReferenceCounted(false);

        // If the service was idle, but got killed before it stopped itself, the
        // system will relaunch it. Make sure it gets stopped again in that case.
        Message msg = mDelayedStopHandler.obtainMessage();
        mDelayedStopHandler.sendMessageDelayed(msg, IDLE_DELAY);
    }

    @Override
    public void onDestroy() {
        // Check that we're not being destroyed while something is still playing.
        if (isPlaying()) {
            Log.e(LOGTAG, "Service being destroyed while still playing.");
        }
        // release all MediaPlayer resources, including the native player and wakelocks
        mPlayer.stop ();
        mPlayer = null;

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

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        mServiceStartId = startId;
        mDelayedStopHandler.removeCallbacksAndMessages(null);

        if (intent != null) {
            String action = intent.getAction();
            String cmd = intent.getStringExtra("command");
            
            if (CMDNEXT.equals(cmd) || NEXT_ACTION.equals(action)) {
                next();
            } else if (CMDPREVIOUS.equals(cmd) || PREVIOUS_ACTION.equals(action)) {
                if (position() < 2000) {
                    prev();
                } else {
                    seek(0);
                    play();
                }
            } else if (CMDTOGGLEPAUSE.equals(cmd) || TOGGLEPAUSE_ACTION.equals(action)) {
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
        return START_STICKY;
    }
    
    @Override
    public boolean onUnbind(Intent intent) {
        mServiceInUse = false;

        if (isPlaying() || mPausedByTransientLossOfFocus) {
            // something is currently playing, or will be playing once 
            // an in-progress action requesting audio focus ends, so don't stop the service now.
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
    
    /**
     * Called when we receive a ACTION_MEDIA_EJECT notification.
     *
     * @param storagePath path to mount point for the removed media
     */
    public void closeExternalStorageFiles(String storagePath) {
        // stop playback and clean up if the SD card is going to be unmounted.
        mPlayer.stop();
    }

    /**
     * Registers an intent to listen for ACTION_MEDIA_EJECT notifications.
     * The intent will call closeExternalStorageFiles() if the external media
     * is going to be ejected, so applications can clean up any files they have open.
     */
    public void registerExternalStorageListener() {
        if (mUnmountReceiver == null) {
            mUnmountReceiver = new BroadcastReceiver() {
                @Override
                public void onReceive(Context context, Intent intent) {
                    String action = intent.getAction();
                    if (action.equals(Intent.ACTION_MEDIA_EJECT)) {
                        closeExternalStorageFiles(intent.getData().getPath());
                    } else if (action.equals(Intent.ACTION_MEDIA_MOUNTED)) {
                    }
                }
            };
            IntentFilter iFilter = new IntentFilter();
            iFilter.addAction(Intent.ACTION_MEDIA_EJECT);
            iFilter.addAction(Intent.ACTION_MEDIA_MOUNTED);
            iFilter.addDataScheme("file");
            registerReceiver(mUnmountReceiver, iFilter);
        }
    }

   /**
     * Starts playback of a previously opened file.
     */
    public void play() {
    	RemoteViews views = new RemoteViews(getPackageName(), R.layout.statusbar);
    	views.setImageViewResource(R.id.icon, R.drawable.ddb_24);

    	Notification status = new Notification();
    	status.contentView = views;
    	status.flags |= Notification.FLAG_ONGOING_EVENT;
    	status.icon = R.drawable.ddb_24;
    	status.contentIntent = PendingIntent.getActivity(this, 0,
    			new Intent("org.deadbeef.android.PLAYBACK_VIEWER")
    	.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK), 0);
    	startForeground(PLAYBACKSERVICE_STATUS, status);
    	if (!mIsSupposedToBePlaying) {
    		mIsSupposedToBePlaying = true;
            }
    	mPlayer.paused = false;
		int curr = DeadbeefAPI.pl_get_current_idx ();
		if (curr < 0) {
			DeadbeefAPI.play_idx (0);
		}
    }

    public void playpause () {
    	mPlayer.paused = !mPlayer.paused;
    }

    public boolean isPaused() {
    	return mPlayer.paused;
    }

    private void stop(boolean remove_status_icon) {
    	mPlayer.stop();
        if (remove_status_icon) {
            gotoIdleState();
        } else {
            stopForeground(false);
        }
        if (remove_status_icon) {
            mIsSupposedToBePlaying = false;
        }
    }

    /**
     * Stops playback.
     */
    public void stop() {
        stop(true);
    }

    /**
     * Pauses playback (call play() to resume)
     */
    public void pause() {
        synchronized(this) {
            if (isPlaying()) {
                mPlayer.paused = true;
                gotoIdleState();
                mIsSupposedToBePlaying = false;
            }
        }
    }

    /** Returns whether something is currently playing
     *
     * @return true if something is playing (or will be playing shortly, in case
     * we're currently transitioning between tracks), false if not.
     */
    public boolean isPlaying() {
        return mIsSupposedToBePlaying;
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
        stopForeground(true);
    }
    
    /**
     * Returns the duration of the file in seconds
     */
    public float duration() {
    	return DeadbeefAPI.play_get_duration_seconds ();
    }

    /**
     * Returns the current playback position in seconds
     */
    public float position() {
        return DeadbeefAPI.play_get_pos_seconds ();
    }

    public void seek(float pos) {
    	DeadbeefAPI.play_seek (pos);
    }
    
    String getAlbumName () {
    	int track = DeadbeefAPI.pl_get_current_idx ();
    	return DeadbeefAPI.pl_get_metadata (track, "album");
    }

    String getArtistName () {
    	int track = DeadbeefAPI.pl_get_current_idx ();
    	return DeadbeefAPI.pl_get_metadata (track, "artist");
    }
    
    String getTrackName () {
    	int track = DeadbeefAPI.pl_get_current_idx ();
    	return DeadbeefAPI.pl_get_metadata (track, "title");
    }
    
   /*
    * By making this a static class with a WeakReference to the Service, we
    * ensure that the Service can be GCd even when the system process still
    * has a remote reference to the stub.
    */
   static class ServiceStub extends IMediaPlaybackService.Stub {
       WeakReference<MediaPlaybackService> mService;
       
       ServiceStub(MediaPlaybackService service) {
           mService = new WeakReference<MediaPlaybackService>(service);
       }

       public boolean isPlaying() {
           return !mService.get().mPlayer.paused;
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
       public void playpause () {
           mService.get().playpause();
       }
       public boolean isPaused() {
    	   return mService.get().isPaused();
       }

   }


   private final IBinder mBinder = new ServiceStub(this);


}

