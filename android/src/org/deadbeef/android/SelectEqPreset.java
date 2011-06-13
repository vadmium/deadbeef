package org.deadbeef.android;

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ListActivity;
import android.content.DialogInterface;
import android.os.Bundle;
import android.util.Log;
import android.view.ContextMenu;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.AdapterView.AdapterContextMenuInfo;

public class SelectEqPreset extends ListActivity {
	
	private static final int ACT_CREATE = 0;
	private static final int ACT_DELETE = 1;
	private static final int ACT_RENAME = 2;
	
	private int mSelected;
	
	private void fillList () {
        final ArrayAdapter<String> adapter = new ArrayAdapter<String>(this,R.layout.trackproperty,R.id.property);
        
       	adapter.add("Save as new preset");
       	
        int n = DeadbeefAPI.eq_num_presets();
        for (int i = 0; i < n ; i++) {
        	String nm = DeadbeefAPI.eq_preset_name(i);
        	adapter.add(nm);
        }
        
        setListAdapter(adapter);
	}
	
	@Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.playlists);
        fillList ();
        registerForContextMenu(findViewById(android.R.id.list));
	}
	
	@Override
    protected Dialog onCreateDialog(int id) {
		switch (id) {
		case ACT_CREATE: {
            LayoutInflater factory = LayoutInflater.from(this);
            final View textEntryView = factory.inflate(R.layout.editplaylist, null);
            return new AlertDialog.Builder(SelectEqPreset.this)
                .setIcon(R.drawable.icon)
                .setTitle(R.string.new_eq_preset)
                .setView(textEntryView)
                .setPositiveButton(R.string.alert_dialog_ok, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                    	// ...save here...
                    	String title = ((TextView)textEntryView.findViewById(R.id.title)).getText().toString();
                    	DeadbeefAPI.eq_save_preset(title);
                    	fillList ();
                    }
                })
                .setNegativeButton(R.string.alert_dialog_cancel, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {

                        /* User clicked cancel so do some stuff */
                    }
                })
                .create();
			}
		case ACT_RENAME: {
            LayoutInflater factory = LayoutInflater.from(this);
            final View textEntryView = factory.inflate(R.layout.editplaylist, null);
            return new AlertDialog.Builder(SelectEqPreset.this)
                .setIcon(R.drawable.icon)
                .setTitle(R.string.rename_playlist)
                .setView(textEntryView)
                .setPositiveButton(R.string.alert_dialog_ok, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                    	String title = ((TextView)textEntryView.findViewById(R.id.title)).getText().toString();
                    	DeadbeefAPI.eq_rename_preset (mSelected, title);
                    	fillList ();
                    }
                })
                .setNegativeButton(R.string.alert_dialog_cancel, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        /* User clicked cancel so do some stuff */
                    }
                })
                .create();
			}
		case ACT_DELETE:
            return new AlertDialog.Builder(SelectEqPreset.this)
                .setIcon(R.drawable.icon)
                .setTitle(R.string.delete_playlist_confirm)
                .setPositiveButton(R.string.alert_dialog_ok, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                    	DeadbeefAPI.eq_delete_preset (mSelected);
                    	fillList ();
                    }
                })
                .setNegativeButton(R.string.alert_dialog_cancel, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        /* User clicked Cancel so do some stuff */
                    }
                })
                .create();
		}
		return null;
	}

	@Override
    public void onListItemClick (ListView l, View v, int position, long id) {
		if (position == 0) {
			// showDialog
			showDialog(ACT_CREATE);
		}
		else {
			setResult(position-1);
	        finish ();
		}
    }
    
	@Override
	public void onCreateContextMenu (ContextMenu menu, View v, ContextMenu.ContextMenuInfo menuInfo) {
		Log.e("DDB","onCreateContextMenu");
		mSelected = ((AdapterContextMenuInfo)menuInfo).position-1;
		menu.add(0, ACT_RENAME, 0, R.string.rename_eq_preset);
		menu.add(0, ACT_DELETE, 1, R.string.delete_eq_preset);
	}
    	
	@Override
	public boolean onContextItemSelected (MenuItem item) {
		switch (item.getItemId()) {
		case ACT_RENAME:
			showDialog (ACT_RENAME);
			break;
		case ACT_DELETE:
			showDialog (ACT_DELETE);
			break;
		}
		return false;
	}
    
}
