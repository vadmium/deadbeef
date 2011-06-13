package org.deadbeef.android;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.ToggleButton;

public class EQ extends Activity {
	
	private static final int sliders[] = {
		R.id.preamp,
		R.id.band0,
		R.id.band1,
		R.id.band2,
		R.id.band3,
		R.id.band4,
		R.id.band5,
		R.id.band6,
		R.id.band7,
		R.id.band8,
		R.id.band9,
	};
	
	private static final int values[] = {
		R.id.db_preamp,
		R.id.db_band0,
		R.id.db_band1,
		R.id.db_band2,
		R.id.db_band3,
		R.id.db_band4,
		R.id.db_band5,
		R.id.db_band6,
		R.id.db_band7,
		R.id.db_band8,
		R.id.db_band9,
	};
	
	private void initParam (int i) {
	    SeekBar sb = (SeekBar)findViewById(sliders[i]);
		float fv = DeadbeefAPI.eq_get_param (i);
		sb.setProgress((int)fv);
		TextView tv = (TextView)findViewById(values[i]);
		int dB = (int)fv * 24 / 100 - 12;
		String val;
		if (dB >= 0) {
			val = "+" + dB + " dB";
		}
		else {
			val = dB + " dB";
		}
		tv.setText (val);
	}
	
	private void initGui () {
        for (int i = 0; i <= 10; i++) {
        	initParam (i);
        }
	}
	

	public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.eq);
        
    	boolean enabled = DeadbeefAPI.eq_is_enabled();
    	ToggleButton tb = (ToggleButton)findViewById (R.id.eq_onoff); 
    	tb.setChecked(enabled);
    	tb.setOnCheckedChangeListener(new OnCheckedChangeListener () {
    		public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
    			DeadbeefAPI.eq_enable(isChecked);
    			DeadbeefAPI.eq_save_config();
    		}
    	});
    
        for (int i = 0; i <= 10; i++) {
        	initParam (i);
        	SeekBar sb = (SeekBar)findViewById(sliders[i]);
        	sb.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() { 
				public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
					for (int i = 0; i <= 10; i++) {
						if (seekBar.getId () == sliders[i]) {
							String val;
							float dB = progress * 24 / 100 - 12;
							if (dB >= 0) {
								val = "+" + dB + " dB";
							}
							else {
								val = dB + " dB";
							}
							TextView tv = (TextView)findViewById(values[i]);
							tv.setText (val);
							DeadbeefAPI.eq_set_param (i, (float)progress);
							DeadbeefAPI.eq_save_config();
						}
					}
				}
				public void onStartTrackingTouch(SeekBar seekBar) {
				}
				public void onStopTrackingTouch(SeekBar seekBar) {
				}
			});
        }
        
        ((Button)findViewById(R.id.preset)).setOnClickListener(new OnClickListener() {
	        public void onClick(View v) {
/*		        if (Deadbeef.freeversion) {
					Toast.makeText(Settings.this, "This feature is not available in free version", Toast.LENGTH_SHORT).show();

		        }
		        else */{
		        	Intent i = new Intent (EQ.this, SelectEqPreset.class);
		        	startActivityForResult(i, 0);
		        }
	        }
        });
	}
	
	@Override
    protected void onActivityResult (int requestCode, int resultCode, Intent data) {
    	if (requestCode == 0) {
   			DeadbeefAPI.eq_load_preset (resultCode);
   			initGui ();
    	}
    }    
}
