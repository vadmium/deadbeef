package org.deadbeef.android;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.net.Uri;
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
		finish ();
	}
	
	private ServiceConnection mConnection = new ServiceConnection() {
        public void onServiceConnected(ComponentName className, IBinder obj) {
        	IMediaPlaybackService playbackService = IMediaPlaybackService.Stub.asInterface(obj);
        	startFile (playbackService);
        	unbindService(this);
        }

        public void onServiceDisconnected(ComponentName className) {
        }
    };
    
    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);
    }
    
    @Override
    public void onResume() {
        super.onResume();
        if (getIntent().getData() != null) {
        	bindService(new Intent(StreamStarter.this, 
        		MediaPlaybackService.class), mConnection, Context.BIND_AUTO_CREATE);
        }
    }
    
}
