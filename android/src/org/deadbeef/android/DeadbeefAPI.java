package org.deadbeef.android;

public class DeadbeefAPI
{
	public static native int start();
	public static native int stop();
	// returns number of bytes read
	public static native int getBuffer(int size, short buffer[]);
	public static native int getSamplerate();
	
	// playlist access
	public static native int pl_get_count ();
	public static native String pl_get_item_text (int idx);
	public static native int pl_add_folder (String path);
	public static native void pl_clear ();
	public static native int pl_get_current_idx ();
	public static native String pl_get_metadata (int idx, String key);
	public static native String pl_get_duration_formatted (int idx);
	
	
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
	

	static {
		System.loadLibrary("deadbeef");
	}
}
