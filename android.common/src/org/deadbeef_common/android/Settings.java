package org.deadbeef_common.android;

import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.PreferenceActivity;
import android.widget.Toast;
 
public class Settings extends PreferenceActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.preferences);
        CheckBoxPreference pref;
        
        // lastfm
        findPreference("enable_lastfm").setOnPreferenceClickListener(new OnPreferenceClickListener() {
            public boolean onPreferenceClick(Preference preference) {
            	/*if (Deadbeef.freeversion) {
            		((CheckBoxPreference)preference).setChecked(false);
                    Toast.makeText(Settings.this, "This feature is not available in free version", Toast.LENGTH_SHORT).show();
            	}
            	else*/ {
            		DeadbeefAPI.conf_set_int("android.enable_lastfm", ((CheckBoxPreference)preference).isChecked() ? 1 : 0);
            	}
            	return true;                    	
            }
        });
        
        // replaygain mode
        findPreference("rg_mode").setOnPreferenceChangeListener(new OnPreferenceChangeListener() {
			@Override
			public boolean onPreferenceChange(Preference preference,
					Object newValue) {
            	String val = ((ListPreference)preference).getValue();
            	if (val.equals("Track")) {
            		DeadbeefAPI.conf_set_int("replaygain_mode", 1);
            	}
            	else if (val.equals("Album")) {
            		DeadbeefAPI.conf_set_int("replaygain_mode", 2);
            	}
            	else {
            		DeadbeefAPI.conf_set_int("replaygain_mode", 0);
            	}
            	return false;                    	
            }
        });

        // replaygain peak scale
        findPreference("rg_peakscale").setOnPreferenceClickListener(new OnPreferenceClickListener() {
            public boolean onPreferenceClick(Preference preference) {
           		DeadbeefAPI.conf_set_int("replaygain_scale", ((CheckBoxPreference)preference).isChecked() ? 1 : 0);
            	return true;                    	
            }
        });
        
        // archive support
        findPreference("disable_archives").setOnPreferenceClickListener(new OnPreferenceClickListener() {
            public boolean onPreferenceClick(Preference preference) {
           		DeadbeefAPI.conf_set_int("ignore_archives", ((CheckBoxPreference)preference).isChecked() ? 1 : 0);
            	return true;                    	
            }
        });
        
        // default playlist
        findPreference("default_playlist").setOnPreferenceChangeListener(new OnPreferenceChangeListener() {
			@Override
			public boolean onPreferenceChange(Preference preference,
					Object newValue) {
				DeadbeefAPI.conf_set_str("cli_add_playlist_name", newValue.toString());
				return false;
			}
        });
    }
}