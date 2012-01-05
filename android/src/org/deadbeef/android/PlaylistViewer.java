package org.deadbeef.android;
import org.deadbeefpro.android.R;

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ListActivity;
import android.app.ProgressDialog;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;
import android.view.ContextMenu;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.AdapterView.AdapterContextMenuInfo;
import android.widget.Button;
import android.widget.ListView;

public class PlaylistViewer extends ListActivity {
    private ProgressDialog progressDialog;
    private BroadcastReceiver mMediaServiceReceiver;
    Handler handler = new Handler();
    private int mSelected = -1; // track selected for ctx menu

    public static final int MENU_ACT_ADD_FILES = 0;
    public static final int MENU_ACT_ADD_FOLDER = 1;
    public static final int MENU_ACT_REMOVE = 2;
    public static final int MENU_ACT_MOVE_TO_PLAYLIST = 3;
    public static final int MENU_ACT_PROPERTIES = 4;
    public int progressDepth = 0;

    public static final int DLG_CONFIRM_REMOVE = 0;

    private void showProgress (boolean show) {
     synchronized (this) {
      if (show) {
       progressDepth++;
      }
      else {
         progressDepth--;
      }
     }
        handler.post(new Runnable() {
            public void run() {
             if (progressDepth > 0 && null == progressDialog) {
     Log.w("DDB", "show progress");
     progressDialog = ProgressDialog.show(PlaylistViewer.this,
      "Please wait",
      "Adding files to playlist...", true);
             }
             else if (progressDepth == 0 && null != progressDialog) {
     Log.w("DDB", "hide progress");
           progressDialog.dismiss();
           progressDialog = null;
             }
            }
        });
    }

    void startMediaServiceListener() {
     mMediaServiceReceiver = new BroadcastReceiver() {
         @Override
         public void onReceive(Context context, Intent intent) {
          if (intent.getAction().toString().equals ("org.deadbeef.android.ADD_FILES_START")) {
     Log.w("DDB", "playlist received ADD_FILES_START");
     setListAdapter(null);
     showProgress (true);
          }
          else if (intent.getAction().toString().equals ("org.deadbeef.android.ADD_FILES_FINISH")) {
     Log.w("DDB", "playlist received ADD_FILES_END");
        DeadbeefAPI.plt_save_current ();
           final FileListAdapter adapter = new FileListAdapter(PlaylistViewer.this, R.layout.plitem, R.id.title);
                 setListAdapter(adapter);
     showProgress (false);
       }
         }
     };
     IntentFilter filter = new IntentFilter();
     filter.addAction("org.deadbeef.android.ADD_FILES_START");
     filter.addAction("org.deadbeef.android.ADD_FILES_FINISH");
     registerReceiver(mMediaServiceReceiver, filter);
    }

    void stopMediaServiceListener () {
     unregisterReceiver(mMediaServiceReceiver);
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
  Log.e("DDB", "Deadbeef.onCreate");
        super.onCreate(savedInstanceState);
        setContentView(R.layout.playlist);
        MusicUtils.bindToService(this, new ServiceConnection() {
         public void onServiceConnected(ComponentName className, IBinder obj) {
          Log.e("DDB", "Deadbeef.onCreate connected");
          MusicUtils.sService = IMediaPlaybackService.Stub.asInterface(obj);
          final FileListAdapter adapter = new FileListAdapter(PlaylistViewer.this, R.layout.plitem, R.id.title);
          handler.post(new Runnable() {
              public void run() {
                  setListAdapter(adapter);
              }
          });
          startMediaServiceListener ();
         }

         public void onServiceDisconnected(ComponentName className) {
          stopMediaServiceListener ();
          MusicUtils.sService = null;
         }
     });

        registerForContextMenu(findViewById(android.R.id.list));

        ((Button)findViewById(R.id.add)).setOnClickListener(mAddListener);
        ((Button)findViewById(R.id.clear)).setOnClickListener(mClearListener);
        setTitle(DeadbeefAPI.plt_get_title(DeadbeefAPI.plt_get_curr()));
    }
    @Override
    public void onDestroy () {
     super.onDestroy();
    }

    @Override
    public void onListItemClick (ListView l, View v, int position, long id) {
     try {
      MusicUtils.sService.playIdx (position);
     }
     catch (RemoteException e) {
     }
    };


 @Override
 public void onCreateContextMenu (ContextMenu menu, View v, ContextMenu.ContextMenuInfo menuInfo) {
  mSelected = ((AdapterContextMenuInfo)menuInfo).position;
  Log.e("DDB","onCreateContextMenu");
//		menu.add(0, MENU_ACT_ADD_FILES, 0, R.string.ctx_menu_add_files);
  menu.add(0, MENU_ACT_ADD_FOLDER, 1, R.string.ctx_menu_add_folder);
  menu.add(0, MENU_ACT_REMOVE, 2, R.string.ctx_menu_remove);
//		menu.add(0, MENU_ACT_MOVE_TO_PLAYLIST, 3, R.string.ctx_menu_move_to_playlist);

  Intent i = new Intent (this, TrackPropertiesViewer.class);
  i.setData(Uri.fromParts("track", String.valueOf (DeadbeefAPI.plt_get_curr()), String.valueOf(((AdapterContextMenuInfo)menuInfo).position)));
  menu.add(0, MENU_ACT_PROPERTIES, 4, R.string.ctx_menu_properties).setIntent (i);
 }

    @Override
    protected void onActivityResult (int requestCode, int resultCode, Intent data) {
        // add folder to playlist
     if (requestCode == Deadbeef.REQUEST_ADD_FOLDER && resultCode == RESULT_OK) {
     }
     if (requestCode == Deadbeef.REQUEST_ADD_FOLDER_AFTER && resultCode == RESULT_OK) {
     }
     else if (requestCode == Deadbeef.REQUEST_SELECT_PLAYLIST && resultCode >= 0) {
      DeadbeefAPI.plt_set_curr_idx (resultCode);
      DeadbeefAPI.conf_save ();
         final FileListAdapter adapter = new FileListAdapter(this, R.layout.plitem, R.id.title);
         handler.post(new Runnable() {
             public void run() {
                 setListAdapter(adapter);
             }
         });
     }
    }

 @Override
 public boolean onContextItemSelected (MenuItem item) {
  switch (item.getItemId()) {
  case MENU_ACT_ADD_FILES:
   break;
  case MENU_ACT_ADD_FOLDER:
         Intent i = new Intent (this, FileBrowser.class);
         i.setAction("ADD_FOLDER_AFTER");
         i.setData(Uri.fromParts("track", String.valueOf (DeadbeefAPI.plt_get_curr()), String.valueOf(mSelected)));
      startActivityForResult(i, Deadbeef.REQUEST_ADD_FOLDER_AFTER);
   break;
  case MENU_ACT_REMOVE:
   showDialog (DLG_CONFIRM_REMOVE);
   break;
  case MENU_ACT_MOVE_TO_PLAYLIST:
   break;
  }
  return false;
 }

 private void AddFolder () {
        Intent i = new Intent (this, FileBrowser.class);
     startActivityForResult(i, Deadbeef.REQUEST_ADD_FOLDER);
    }

 private OnClickListener mAddListener = new OnClickListener() {
        public void onClick(View v) {
         // this works in background thread, need to disable android listadapter nonsense
         AddFolder ();
        }
    };

    private OnClickListener mClearListener = new OnClickListener() {
        public void onClick(View v) {
         DeadbeefAPI.pl_clear (); // FIXME: should be called through mediaservice, only when connected
      DeadbeefAPI.plt_save_current ();
         final FileListAdapter adapter = new FileListAdapter(PlaylistViewer.this, R.layout.plitem, R.id.title);
            setListAdapter(adapter);
        }
    };

    @Override
    protected Dialog onCreateDialog(int id) {
     LayoutInflater factory = LayoutInflater.from(this);
     if (id == DLG_CONFIRM_REMOVE) {
            return new AlertDialog.Builder(PlaylistViewer.this)
                .setIcon(R.drawable.icon)
                .setTitle(R.string.remove_track_confirm)
                .setPositiveButton(R.string.alert_dialog_ok, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                     DeadbeefAPI.plt_remove_item(DeadbeefAPI.plt_get_curr(), mSelected);
            final FileListAdapter adapter = new FileListAdapter(PlaylistViewer.this, R.layout.plitem, R.id.title);
               setListAdapter(adapter);
                    }
                })
                .setNegativeButton(R.string.alert_dialog_cancel, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                    }
                })
                .create();
     }
     return null;
    }
}
