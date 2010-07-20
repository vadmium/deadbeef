package org.deadbeef.android;

import java.util.Timer;
import java.util.TimerTask;

import android.app.ListActivity;
import android.content.Intent;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageButton;
import android.widget.ListView;
import android.widget.SeekBar;
import android.widget.BaseAdapter;
import android.widget.TextView;
import android.util.Log;
import android.os.Handler;

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
    public boolean paused = false;
    public boolean playing = true;
    private Thread playThread;

    private class PlayRunnable implements Runnable {

    	public void run() {
    		audio.play();

    		short buffer[] = new short[minSize];
    		while (playing) {
    			buffer = DeadbeefAPI.getBuffer(minSize, buffer);
 
    			audio.write(buffer, 0, minSize);
    			
		    	if (paused && audio.getPlayState () != AudioTrack.PLAYSTATE_PAUSED) {
		    		audio.pause ();
		    	}
    			while (paused) {
    				try {
    					Thread.sleep(500);
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
    
    public void playpause () {
    	paused = !paused;
    }
}

public class Deadbeef extends ListActivity {
	Player ply;
	Timer sbTimer;
	String TAG = "DDB";
	Handler handler = new Handler();
	boolean terminate = false;
	
    boolean dontUpdatePlayPos = false;
    
    final Runnable UpdateInfoRunnable = new Runnable() {
    	public void run () {
	    	// update numbers
	    	TextView tv = (TextView)findViewById(R.id.current_pos_text);
	    	String pos = DeadbeefAPI.play_get_pos_formatted ();
	    	tv.setText(pos);
	    	tv = (TextView)findViewById(R.id.duration_text);
	    	String dur = DeadbeefAPI.play_get_duration_formatted ();
	    	tv.setText(dur);
	
	    	// update seekbar
	    	if (dontUpdatePlayPos) {
	    		return;
	    	}
	        SeekBar sb = (SeekBar)findViewById(R.id.seekbar);
	        float normpos = DeadbeefAPI.play_get_pos_normalized ();
	        sb.setProgress ((int)(normpos * 1024));
    	}
    };
    
    private class ProgressThread extends Thread {
    	@Override
        public void run() {
    		do {
    			try {
                    sleep(250);
                } catch (InterruptedException e) {
                    Log.e(TAG, e.getMessage());
                }
    			handler.post(UpdateInfoRunnable);
    		} while (!terminate);
    	}
    };
    private ProgressThread progressThread;

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
		DeadbeefAPI.start();

        setContentView(R.layout.main);
        
        final FileListAdapter adapter = new FileListAdapter(this, R.layout.plitem, R.id.title); 
        setListAdapter(adapter);
        
        ImageButton button;
        
/*        ImageButton button = (ImageButton)findViewById(R.id.quit);
        button.setOnClickListener(mQuitListener);*/
        
        button = (ImageButton)findViewById(R.id.prev);
        button.setOnClickListener(mPrevListener);

        button = (ImageButton)findViewById(R.id.next);
        button.setOnClickListener(mNextListener);
        
/*        button = (ImageButton)findViewById(R.id.add);
        button.setOnClickListener(mAddListener);*/

        button = (ImageButton)findViewById(R.id.play);
        button.setOnClickListener(mPlayPauseListener);
        
        SeekBar sb = (SeekBar)findViewById(R.id.seekbar);
        sb.setMax(1024);
        sb.setOnSeekBarChangeListener(sbChangeListener);
        
        ply = new Player();     
        progressThread = new ProgressThread();
        progressThread.start();
   }
    
    @Override
    public void onDestroy() {
    	ply.stop();
    	ply = null;
    	terminate = true;
        super.onDestroy();
    }

    
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
    	MenuInflater inflater = getMenuInflater();
    	inflater.inflate(R.menu.optionsmenu, menu);
    	return true;
    }
             
/*    private OnClickListener mQuitListener = new OnClickListener() {
        public void onClick(View v) {
        	ply.stop ();
        	finish ();
        }
    };*/

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
    
    private void AddFolder () {
        Log.i(TAG,"AddFolder ()");
        Intent i = new Intent (this, FileBrowser.class);
    	startActivityForResult(
        		i,
                10);
    }

    @Override
    protected void onActivityResult (int requestCode, int resultCode, Intent data) {
        // add folder to playlist
    	if (requestCode == 10) {
	        final FileListAdapter adapter = new FileListAdapter(this, R.layout.plitem, R.id.title); 
	        handler.post(new Runnable() {
	            public void run() {
	                setListAdapter(adapter);
	            }
	        });
    	}
    }

    @Override
    public boolean onOptionsItemSelected (MenuItem item) {
        int id = item.getItemId ();
        if (id == R.id.menu_add_folder) {
            AddFolder ();
        }
        else if (id == R.id.menu_clear_playlist) {
        	DeadbeefAPI.pl_clear ();
	        final FileListAdapter adapter = new FileListAdapter(this, R.layout.plitem, R.id.title); 
	        handler.post(new Runnable() {
	            public void run() {
	                setListAdapter(adapter);
	            }
	        });
        }
        else if (id == R.id.menu_quit) {
            ply.stop ();
            finish ();
        }
        return true;
    };
    
    private void PlayPause () {
    	ply.playpause ();
    	if (!ply.paused) {
    		ImageButton button = (ImageButton)findViewById(R.id.play);
            button.setImageResource (R.drawable.ic_media_pause);    		
    	}
    	else {
    		ImageButton button = (ImageButton)findViewById(R.id.play);
            button.setImageResource (R.drawable.ic_media_play);    		
    	}
    }

    private OnClickListener mPlayPauseListener = new OnClickListener() {
        public void onClick(View v) {
        	PlayPause();
        }
    };

    @Override
    public void onListItemClick (ListView l, View v, int position, long id) {
   		DeadbeefAPI.play_idx (position);
    };
    
	void PlayerSeek (float value) {
		DeadbeefAPI.play_seek (value);
	}

	float trackedPos = 0;
	private SeekBar.OnSeekBarChangeListener sbChangeListener = new SeekBar.OnSeekBarChangeListener() { 
		public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
			trackedPos = (float)((float)progress/1024.0);
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

}

