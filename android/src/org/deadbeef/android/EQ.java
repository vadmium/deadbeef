package org.deadbeef.android;

import android.app.Activity;
import android.os.Bundle;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.SeekBar;
import android.widget.TextView;
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
		R.id.band10,
		R.id.band11,
		R.id.band12,
		R.id.band13,
		R.id.band14,
		R.id.band15,
		R.id.band16,
		R.id.band17
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
		R.id.db_band10,
		R.id.db_band11,
		R.id.db_band12,
		R.id.db_band13,
		R.id.db_band14,
		R.id.db_band15,
		R.id.db_band16,
		R.id.db_band17
	};

	public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.eq);
        
        final int plugin = DeadbeefAPI.dsp_find("supereq");
        if (0 != plugin) {
        	boolean enabled = DeadbeefAPI.dsp_is_enabled(plugin);
        	ToggleButton tb = (ToggleButton)findViewById (R.id.eq_onoff); 
        	tb.setChecked(enabled);
        	tb.setOnCheckedChangeListener(new OnCheckedChangeListener () {
        		public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
        			DeadbeefAPI.dsp_enable(plugin, isChecked);
        			DeadbeefAPI.dsp_save_config();
        		}
        	});
        
	        for (int i = 0; i < 19; i++) {
	        	SeekBar sb = (SeekBar)findViewById(sliders[i]);
	        	String val = DeadbeefAPI.dsp_get_param (plugin, i);
	        	float fv = Float.valueOf(val);
	        	sb.setProgress((int)(((fv / 40.0f) + 0.5) * 100));
	        	
	        	sb.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() { 
					public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
						int dB = (int)(((float)progress/100.0f - 0.5f) * 40);
						for (int i = 0; i < 19; i++) {
							if (seekBar.getId () == sliders[i]) {
								String val;
								if (dB >= 0) {
									val = "+" + dB + " dB";
								}
								else {
									val = dB + " dB";
								}
								TextView tv = (TextView)findViewById(values[i]);
								tv.setText (val);
								DeadbeefAPI.dsp_set_param (plugin, i, String.valueOf(val));
								DeadbeefAPI.dsp_save_config();
							}
						}
					}
					public void onStartTrackingTouch(SeekBar seekBar) {
					}
					public void onStopTrackingTouch(SeekBar seekBar) {
					}
				});
	        }
        }
	}

}
