package org.deadbeef.android;

import android.app.ListActivity;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageButton;
import android.widget.ListView;

class Player {
	public Player() {
		PlayRunnable playRunnable = new PlayRunnable();
		playThread = new Thread(playRunnable);
		playThread.start();
	}
    private int minSize = AudioTrack.getMinBufferSize(44100,
    		AudioFormat.CHANNEL_CONFIGURATION_STEREO,
    		AudioFormat.ENCODING_PCM_16BIT);
    
//    private DeadbeefAPI ddb = new DeadbeefAPI();
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

    	public void run() {
    		audio.play();

    		short buffer[] = new short[minSize];
    		while (playing) {
    			buffer = DeadbeefAPI.getBuffer(minSize, buffer);
 
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

public class Deadbeef extends ListActivity {
	Player ply;
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
		DeadbeefAPI.start();

        setContentView(R.layout.main);
        
        final FileListAdapter adapter = new FileListAdapter(this, R.layout.plitem, R.id.title); 
        setListAdapter(adapter);
        
//        ListView lv = getListView();
//        lv.setOnClickListener(mListViewClickListener);
        
        ImageButton button = (ImageButton)findViewById(R.id.quit);
        button.setOnClickListener(mQuitListener);
        
        button = (ImageButton)findViewById(R.id.prev);
        button.setOnClickListener(mPrevListener);

        button = (ImageButton)findViewById(R.id.next);
        button.setOnClickListener(mNextListener);
        
        ply = new Player();
        
   }
    
    private OnClickListener mQuitListener = new OnClickListener() {
        public void onClick(View v) {
        	ply.stop ();
        	finish ();
        }
    };

    private OnClickListener mPrevListener = new OnClickListener() {
        public void onClick(View v) {
        	DeadbeefAPI.play_prev ();
        }
    };

    private OnClickListener mNextListener = new OnClickListener() {
        public void onClick(View v) {
        	DeadbeefAPI.play_next ();
        }
    };

    @Override
    public void onListItemClick (ListView l, View v, int position, long id) {
   		DeadbeefAPI.play_idx (position);
    };

}

