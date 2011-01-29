package org.deadbeef.android;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Process;
import android.util.Log;

class Player {
	public Player() {
		PlayRunnable playRunnable = new PlayRunnable();
		playThread = new Thread(playRunnable);
		playThread.start();
	}
    private AudioTrack audio = null;
    int current_samplerate = 44100;
    private int minSize = AudioTrack.getMinBufferSize(current_samplerate,
    		AudioFormat.CHANNEL_CONFIGURATION_STEREO,
    		AudioFormat.ENCODING_PCM_16BIT);
    
    
    public boolean playing = true;
    private Thread playThread;
    
    public void initAudio (int samplerate) {
		audio = null;
		current_samplerate = samplerate;
		
		minSize = AudioTrack.getMinBufferSize(current_samplerate,
	    		AudioFormat.CHANNEL_CONFIGURATION_STEREO,
	    		AudioFormat.ENCODING_PCM_16BIT);
		
		audio = new AudioTrack(
		AudioManager.STREAM_MUSIC, samplerate,
		AudioFormat.CHANNEL_CONFIGURATION_STEREO,
		AudioFormat.ENCODING_PCM_16BIT,
		minSize,
		AudioTrack.MODE_STREAM);
    }

    private class PlayRunnable implements Runnable {

    	public void run() {
    		Process.setThreadPriority(Process.THREAD_PRIORITY_AUDIO);
    		int prevsize = minSize;
    	    short buffer[] = new short[minSize];
    		Log.e("DDB","PlayerRunnable.run started");

    		while (playing) {
    			int samplerate = DeadbeefAPI.getSamplerate ();
    			if (0 == samplerate) {
    				try {
    					Thread.sleep(200);
    				} catch (InterruptedException e) {
    					break;
    				}
    				continue;
    			}

    			
        	    if (audio == null || samplerate != current_samplerate) {
        	    	initAudio (samplerate);
        	    	audio.play();
        	    }

        	    if (prevsize != minSize) {
        	    	buffer = null;
        	    	buffer = new short[minSize];
        	    	prevsize = minSize;
        	    }
        	    DeadbeefAPI.getBuffer(minSize, buffer);
    			audio.write(buffer, 0, minSize);
    			
		    	if (0==DeadbeefAPI.play_is_playing () && audio.getPlayState () != AudioTrack.PLAYSTATE_PAUSED) {
		    		audio.pause ();
		    	}
    			while (0 == DeadbeefAPI.play_is_playing ()) {
    				try {
    					Thread.sleep(200);
    				} catch (InterruptedException e) {
    					break;
    				}
    			}
    			
		    	if (1==DeadbeefAPI.play_is_playing () && audio.getPlayState () != AudioTrack.PLAYSTATE_PLAYING) {
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
