package org.deadbeef.android;

import java.util.ArrayList;
import java.util.List;

import android.app.Activity;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageButton;

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
    	float osc = 0;
		float incr = 440.0f / 44100.0f;
    	public void run() {
    		ddb.start();
    		audio.play();

    		short buffer[] = new short[minSize];
    		while (playing) {
    			buffer = ddb.getBuffer(minSize, buffer);
 
    			audio.write(buffer, 0, minSize);
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
    
    public void stop () {
    	playing = false;
    	try {
    		playThread.join();
    	}
    	catch (InterruptedException e) { }
    }
}

public class Deadbeef extends Activity {
	Player ply;
	private List<String> fileList = new ArrayList<String>();
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        setContentView(R.layout.main);
        
//        final FileListAdapter adapter = new FileListAdapter(Deadbeef.this, R.layout.plitem, R.id.title, fileList);
//        setListAdapter(adapter);
        
        ImageButton button = (ImageButton)findViewById(R.id.quit);
        button.setOnClickListener(mQuitListener);
        
//        ListView playlist = (ListView)findViewById(R.id.playlist);
//        ListAdapter	 adapter = playlist.getAdapter();
//        playlist.add ("test.nsf");
        
        ply = new Player();
        
   }
    
    private OnClickListener mQuitListener = new OnClickListener() {
        public void onClick(View v) {
        	ply.stop ();
        	finish ();
        }
    };
    
}

