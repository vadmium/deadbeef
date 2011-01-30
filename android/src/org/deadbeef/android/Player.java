package org.deadbeef.android;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Process;
import android.os.RemoteException;
import android.util.Log;
import android.widget.TextView;

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
    
    
    public boolean playing = true;
    private Thread playThread;
    
    public void initAudio (int samplerate, int channels) {
		audio = null;
		current_samplerate = samplerate;
		current_channels = channels;
		
		minSize = AudioTrack.getMinBufferSize(current_samplerate,
	    		channels == 1 ? AudioFormat.CHANNEL_CONFIGURATION_MONO : AudioFormat.CHANNEL_CONFIGURATION_STEREO,
	    		AudioFormat.ENCODING_PCM_16BIT);
		
		audio = new AudioTrack(
		AudioManager.STREAM_MUSIC, samplerate,
		channels == 1 ? AudioFormat.CHANNEL_CONFIGURATION_MONO : AudioFormat.CHANNEL_CONFIGURATION_STEREO,
		AudioFormat.ENCODING_PCM_16BIT,
		minSize,
		AudioTrack.MODE_STREAM);
    }

    private class PlayRunnable implements Runnable {
	    private int curr_track = -1;
	    private boolean curr_state = false; // false=stopped/paused
	    private boolean playback_state = false;

    	public void run() {
    		Process.setThreadPriority(Process.THREAD_PRIORITY_AUDIO);
    		int prevsize = minSize;
    	    short buffer[] = new short[minSize];
    		Log.e("DDB","PlayerRunnable.run started");

    		while (playing) {
    			int samplerate = DeadbeefAPI.getSamplerate ();
    			int channels = DeadbeefAPI.getChannels ();
    			if (0 == samplerate) {
    				try {
    					Thread.sleep(200);
    				} catch (InterruptedException e) {
    					break;
    				}
    				continue;
    			}
    			
        	    if (audio == null || samplerate != current_samplerate || channels != current_channels) {
        	    	initAudio (samplerate, channels);
        	    	audio.play();
        	    }

        	    if (prevsize != minSize) {
        	    	buffer = null;
        	    	buffer = new short[minSize];
        	    	prevsize = minSize;
        	    }
        	    DeadbeefAPI.getBuffer(minSize, buffer);
        	    audio.write(buffer, 0, minSize);

    			if (null != MusicUtils.sService) {
	    			try {
		    			boolean state = MusicUtils.sService.isPlaying ();
		    			int track = MusicUtils.sService.getCurrentIdx ();
			    		if (track != curr_track || (playback_state != state && state)) {
			    			curr_track = track;
			    			playback_state = state;
			    			
			    			if (curr_track >= 0) {
								MusicUtils.sService.refreshStatus();
			    			}
			    		}    			
					}
					catch (RemoteException ex) {
					}
    			}
    			
		    	if (!DeadbeefAPI.play_is_playing () && audio.getPlayState () != AudioTrack.PLAYSTATE_PAUSED) {
		    		audio.stop ();
		    	}
    			while (!DeadbeefAPI.play_is_playing ()) {
    				try {
    					Thread.sleep(200);
    				} catch (InterruptedException e) {
    					break;
    				}
    			}
    			
		    	if (DeadbeefAPI.play_is_playing () && audio.getPlayState () != AudioTrack.PLAYSTATE_PLAYING) {
		    		audio.play ();
		    	}    			
    		}

    		Log.e("DDB","PlayerRunnable.run exit loop");
    		if (audio != null) {
    			audio.stop();
    			audio = null;
    		}
    		Log.e("DDB","PlayerRunnable.run audio stop");
    		DeadbeefAPI.stop();
    		Log.e("DDB","PlayerRunnable.run stoped");
    	}
    }
    
    public void stop () {
   		Log.e("DDB","Player.stop");

    	playing = false;
    	try {
    		playThread.join();
    	}
    	catch (InterruptedException e) { }
    }
   
}
