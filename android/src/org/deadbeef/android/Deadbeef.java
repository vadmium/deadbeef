package org.deadbeef.android;

import android.app.Activity;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Bundle;
import android.widget.TextView;

class Player {
	public Player() {
		PlayRunnable playRunnable = new PlayRunnable();
		playThread = new Thread(playRunnable);
		playThread.start();
	}
    private int minSize = AudioTrack.getMinBufferSize(44100,
    		AudioFormat.CHANNEL_CONFIGURATION_STEREO,
    		AudioFormat.ENCODING_PCM_16BIT);
    
    private DeadbeefAPI ddb = new DeadbeefAPI();
    private AudioTrack audio = new AudioTrack(
    		AudioManager.STREAM_MUSIC, 44100,
    		AudioFormat.CHANNEL_CONFIGURATION_STEREO,
    		AudioFormat.ENCODING_PCM_16BIT,
    		minSize < 4096 ? 4096 : minSize,
    		AudioTrack.MODE_STREAM);
    private boolean paused = false;
    private boolean playing = true;
    private Thread playThread;

    private class PlayRunnable implements Runnable {
//    	private static final String TAG = "DDB_Player_Thread";

    	public void run() {
    		ddb.start();
    		audio.play();

    		short buffer[] = new short[minSize];
    		while (playing) {
    			//buffer = ddb.getBuffer(minSize, buffer);
                //Log.v(TAG, "playing... 1st word: " + buffer[0]);
    			java.util.Random r = new java.util.Random (); 
    			for (int i = 0; i < minSize; i++) {
    				buffer[i] = (short)r.nextInt ();
    			}
 
    			audio.write(buffer, 0, minSize / 2);
    			while (paused) {
    				try {
    					Thread.sleep(500);
    				} catch (InterruptedException e) {
    					break;
    				}
    			}
    		}

    		audio.stop();
    		ddb.stop();
    	}
    }
    
}

public class Deadbeef extends Activity {
	Player ply;
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        TextView tv = new TextView(this);
        tv.setText("Deadbeef starting...");
        setContentView(tv);
//        setContentView(R.layout.main);
        
        ply = new Player();
        
/*		try {
			Thread.sleep(10000);
		} catch (InterruptedException e) {
		}
        playing = false;*/
    }
    
}

