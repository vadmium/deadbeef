package org.deadbeef.android;

public class DeadbeefAPI
{
	public static native int start(String confdir, String pluginpath);
	public static native int stop();
	// returns number of bytes read
	public static native int getBuffer(int size, short buffer[]);
	public static native int getSamplerate();
	public static native int getChannels();
	
	// playlist access
	public static native int pl_get_count ();
	public static native String pl_get_item_text (int idx);
	public static native int pl_add_folder (String path);
	public static native int pl_add_file (String path);
	public static native void pl_clear ();
	public static native int pl_get_current_idx ();
	public static native String pl_get_metadata (int idx, String key);
	public static native String pl_get_duration_formatted (int idx);
	public static native int pl_get_for_idx (int idx);
	public static native int pl_get_meta (int trk);
	public static native String meta_get_key (int meta);
	public static native String meta_get_value (int meta);
	public static native int meta_get_next (int meta);
	public static native void pl_item_unref (int trk);
	
	// playback ctrl
	public static native void play_prev ();
	public static native void play_next ();
	public static native void play_idx (int idx);
	public static native float play_get_pos_normalized ();
	public static native float play_get_pos_seconds ();
	public static native String play_get_pos_formatted ();
	public static native float play_get_duration_seconds ();
	public static native String play_get_duration_formatted ();
	public static native void play_seek (float pos);
	public static native void play_toggle_pause ();
	public static native void play_play ();
	public static native void play_pause ();
	public static native void play_stop ();
	public static native int play_is_paused ();
	public static native int play_is_playing ();
	public static native boolean is_streamer_active ();

	// 0=loop_all,1=noloop,2=loopsingle
	public static native void set_play_mode (int mode);
	public static native int get_play_mode ();
	// 0=linear,1=shuffletracks,2=random,3=shufflealbums
	public static native void set_play_order (int order);
	public static native int get_play_order ();

	static {
		System.loadLibrary("deadbeef");
	}
}
