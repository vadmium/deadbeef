package org.deadbeef.android;

import android.content.Intent;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Process;
import android.os.RemoteException;
import android.util.Log;

class Player {
	public Player() {
		PlayRunnable playRunnable = new PlayRunnable();
		playThread = new Thread(playRunnable);
		playThread.start();
	}
    private AudioTrack audio = null;
    static public int current_samplerate = 44100;
    static public int current_channels = 2;
    private int minSize = AudioTrack.getMinBufferSize(current_samplerate,
    		AudioFormat.CHANNEL_CONFIGURATION_STEREO,
    		AudioFormat.ENCODING_PCM_16BIT);
    
    
    public static boolean needReinit = false;
    public boolean playing = true;
    private Thread playThread;
    
    public void initAudio (int samplerate, int channels) {
		audio = null;
		current_samplerate = samplerate;
		current_channels = channels;
		
		minSize = AudioTrack.getMinBufferSize(current_samplerate,
	    		channels == 1 ? AudioFormat.CHANNEL_CONFIGURATION_MONO : AudioFormat.CHANNEL_CONFIGURATION_STEREO,
	    		AudioFormat.ENCODING_PCM_16BIT);
		
		int bufsize = DeadbeefAPI.conf_get_int ("javaplayer.buffersize", 32000);
		if (bufsize == -1) {
			bufsize = minSize;
		}
		if (bufsize < minSize) {
			bufsize = minSize;
		}
		
   		Log.i("DDB","bufSize="+bufsize);

   		while (true) {
   			try {
				audio = new AudioTrack(
						AudioManager.STREAM_MUSIC, samplerate,
						channels == 1 ? AudioFormat.CHANNEL_CONFIGURATION_MONO : AudioFormat.CHANNEL_CONFIGURATION_STEREO,
						AudioFormat.ENCODING_PCM_16BIT,
						bufsize,
						AudioTrack.MODE_STREAM
				);
   			}
   			catch (IllegalArgumentException	ex) {
   				audio = null;
				if (bufsize != minSize) {
					bufsize = minSize;
					continue;
				}
   			}
			
			if (audio == null) {
				if (bufsize != minSize) {
					bufsize = minSize;
					continue;
				}
			}
			break;
   		}
    }

    private class PlayRunnable implements Runnable {
	    private int curr_track = 0; // pointer to streamer_playing_track
	    private boolean curr_state = false; // false=stopped/paused
	    private boolean playback_state = false;

    	public void run() {
    		Process.setThreadPriority(Process.THREAD_PRIORITY_URGENT_AUDIO);
    		int prevsize = minSize;
    	    short buffer[] = new short[minSize];
    		Log.i("DDB","PlayerRunnable.run started");

    		while (playing) {
    			// handle 
    			if (DeadbeefAPI.event_is_pending ()) {
    				try {
    					if (null != MusicUtils.sService) {
    						MusicUtils.sService.handle_ddb_events ();
    					}
    				}
    				catch (RemoteException ex) {
    				}
    			}
    			
    			int samplerate = DeadbeefAPI.getSamplerate ();
    			int channels = DeadbeefAPI.getChannels ();
    			if (0 == samplerate) {
    				try {
    					Thread.sleep(200);
    				} catch (InterruptedException e) {
    				}
    				continue;
    			}
    			
        	    if (needReinit || audio == null || samplerate != current_samplerate || channels != current_channels) {
        	    	initAudio (samplerate, channels);
		    		try {
		    			audio.play ();
		    		}
		    		catch (IllegalStateException ex) {
		    			needReinit = true;
		    			continue;
		    		}
        	    	needReinit = false;
        	    }

        	    if (prevsize != minSize) {
        	    	buffer = null;
        	    	buffer = new short[minSize];
        	    	prevsize = minSize;
        	    }
        	    if (audio.getPlayState () == AudioTrack.PLAYSTATE_PLAYING) {
        	    	DeadbeefAPI.getBuffer(minSize, buffer);
        	    	int err = audio.write(buffer, 0, minSize);
        	    	if (err == AudioTrack.ERROR_INVALID_OPERATION || err == AudioTrack.ERROR_BAD_VALUE) {
        	    		needReinit = true;
        	    		continue;
        	    	}
        	    }

    			if (null != MusicUtils.sService) {
	    			try {
		    			boolean state = MusicUtils.sService.isPlaying ();
		    			int track = DeadbeefAPI.streamer_get_playing_track();
			    		if (track != curr_track || (playback_state != state && state)) {
			    			if (0 != track) {
			    				DeadbeefAPI.pl_item_unref(track);
			    			}
			    			curr_track = track;
			    			playback_state = state;
			    			
			    			if (curr_track != 0) {
								MusicUtils.sService.refreshStatus();
			    			}
			    		}
					}
					catch (RemoteException ex) {
					}
    			}
		    	if (!DeadbeefAPI.play_is_playing () && audio.getPlayState () != AudioTrack.PLAYSTATE_PAUSED) {
		    		try {
		    			audio.stop ();
		    		}
		    		catch (IllegalStateException ex) {
		    		}
		    	}
    			if (!DeadbeefAPI.play_is_playing ()) {
    				try {
    					Thread.sleep(200);
    				} catch (InterruptedException e) {
    				}
    				continue;
    			}
		    	if (DeadbeefAPI.play_is_playing () && audio.getPlayState () != AudioTrack.PLAYSTATE_PLAYING) {
		    		try {
		    			audio.play ();
		    		}
		    		catch (IllegalStateException ex) {
		    			needReinit = true;
		    			continue;
		    		}
		    	}    			
    		}

    		Log.i("DDB","PlayerRunnable.run exit loop");
    		if (audio != null) {
	    		try {
	    			audio.stop ();
	    		}
	    		catch (IllegalStateException ex) {
	    		}
    			audio = null;
    		}
	   		if (0 != curr_track) {
	   			DeadbeefAPI.pl_item_unref(curr_track);
	   			curr_track = 0;
	   		}

    		Log.i("DDB","PlayerRunnable.run audio stop");
    		DeadbeefAPI.stop();
    		Log.i("DDB","PlayerRunnable.run stoped");
    	}
    }
    
    public void stop () {
   		Log.i("DDB","Player.stop");
    	playing = false;
    	try {
    		playThread.join();
    	}
    	catch (InterruptedException e) { }
    }
   
}
