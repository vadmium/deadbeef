package org.deadbeef.android;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Handler;
import android.os.Process;

class Player {
	public Player() {
		PlayRunnable playRunnable = new PlayRunnable();
		playThread = new Thread(playRunnable);
		playThread.start();
	}
    private int minSize = AudioTrack.getMinBufferSize(44100,
    		AudioFormat.CHANNEL_CONFIGURATION_STEREO,
    		AudioFormat.ENCODING_PCM_16BIT);
    
    private AudioTrack audio = new AudioTrack(
    		AudioManager.STREAM_MUSIC, 44100,
    		AudioFormat.CHANNEL_CONFIGURATION_STEREO,
    		AudioFormat.ENCODING_PCM_16BIT,
    		minSize < 4096 ? 4096 : minSize,
    		AudioTrack.MODE_STREAM);
    public boolean paused = true;
    public boolean playing = true;
    private Thread playThread;

    private class PlayRunnable implements Runnable {

    	public void run() {
    		audio.play();
    		 Process.setThreadPriority(Process.THREAD_PRIORITY_AUDIO);

    		short buffer[] = new short[minSize];
    		while (playing) {
    			DeadbeefAPI.getBuffer(minSize, buffer);
 
    			audio.write(buffer, 0, minSize);
    			
		    	if (paused && audio.getPlayState () != AudioTrack.PLAYSTATE_PAUSED) {
		    		audio.pause ();
		    	}
    			while (paused) {
    				try {
    					Thread.sleep(200);
    				} catch (InterruptedException e) {
    					break;
    				}
    			}
    			
		    	if (!paused && audio.getPlayState () != AudioTrack.PLAYSTATE_PLAYING) {
		    		audio.play ();
		    	}    			
    		}

    		audio.stop();
    		DeadbeefAPI.stop();
    	}
    }
    
    public void stop () {
    	playing = false;
    	try {
    		playThread.join();
    	}
    	catch (InterruptedException e) { }
    }
   
}
