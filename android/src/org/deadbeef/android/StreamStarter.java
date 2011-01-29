package org.deadbeef.android;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;

public class StreamStarter extends Activity
{
	private void startFile (IMediaPlaybackService playbackService) {
        ProgressDialog progressDialog;
        progressDialog = ProgressDialog.show(this,      
                "Please wait",
                "Adding files to playlist...", true);
        try {
        	playbackService.startFile (getIntent().getData().toString());
        }
        catch (RemoteException e) {
        	Log.e("ddb StreamStarter onResume", "remote exception");
        }
		progressDialog.dismiss();
		setResult(RESULT_OK);
    	Intent intent = new Intent("org.deadbeef.android.PLAYBACK_VIEWER");
    	intent.putExtra("oneshot", true);
        startActivity(intent);
        finish ();
	}
	
    @Override
    public void onCreate(Bundle icicle) {
		Log.e("DDB", "StreamStarter.onCreate intent action: " + getIntent().getAction().toString());
        super.onCreate(icicle);
    }
    
    @Override
    public void onResume() {
		Log.e("DDB", "StreamStarter.onResume");
        super.onResume();
        if (getIntent().getData() != null) {        	
        	MusicUtils.bindToService(this, new ServiceConnection() {
			        public void onServiceConnected(ComponentName className, IBinder obj) {
			        	IMediaPlaybackService playbackService = IMediaPlaybackService.Stub.asInterface(obj);
			        	startFile (playbackService);
			        }
			
			        public void onServiceDisconnected(ComponentName className) {
			        }
			    }
        	);
        }
    }
    
    @Override
    public void onPause() {
		Log.e("DDB", "StreamStarter.onPause");
    	MusicUtils.unbindFromService(this);
        super.onPause();
    }
    
}
