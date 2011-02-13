package org.deadbeef.android;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
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
	
	private void initParam (int dsp, int i) {
	    SeekBar sb = (SeekBar)findViewById(sliders[i]);
		String val = DeadbeefAPI.dsp_get_param (dsp, i);
		float fv = Float.valueOf(val);
		sb.setProgress((int)(((fv / 40.0f) + 0.5) * 100));
		TextView tv = (TextView)findViewById(values[i]);
		int dB = (int)fv;
		if (dB >= 0) {
			val = "+" + dB + " dB";
		}
		else {
			val = dB + " dB";
		}
		tv.setText (val);
	}
	
	private void initGui (int dsp) {
        for (int i = 0; i < 19; i++) {
        	initParam (dsp, i);
        }
	}
	

	public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.eq);
        
        final int dsp = DeadbeefAPI.dsp_find("supereq");
        if (0 != dsp) {
        	boolean enabled = DeadbeefAPI.dsp_is_enabled(dsp);
        	ToggleButton tb = (ToggleButton)findViewById (R.id.eq_onoff); 
        	tb.setChecked(enabled);
        	tb.setOnCheckedChangeListener(new OnCheckedChangeListener () {
        		public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
        			DeadbeefAPI.dsp_enable(dsp, isChecked);
        			DeadbeefAPI.dsp_save_config();
        		}
        	});
        
	        for (int i = 0; i < 19; i++) {
	        	initParam (dsp, i);
	        	SeekBar sb = (SeekBar)findViewById(sliders[i]);
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
								DeadbeefAPI.dsp_set_param (dsp, i, String.valueOf(val));
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
            ((Button)findViewById(R.id.preset)).setOnClickListener(new OnClickListener() {
		        public void onClick(View v) {
			        Intent i = new Intent (EQ.this, SelectEqPreset.class);
			    	startActivityForResult(i, 0);
		        }
            });

        }
	}
	
	@Override
    protected void onActivityResult (int requestCode, int resultCode, Intent data) {
    	if (requestCode == 0) {
    		final int dsp = DeadbeefAPI.dsp_find("supereq");
    		if (0 != dsp) {
    			DeadbeefAPI.dsp_load_preset (dsp, resultCode);
    			initGui (dsp);
    		}    		
    	}
    }
	
}
