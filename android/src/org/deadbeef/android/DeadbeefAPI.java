package org.deadbeef.android;

public class DeadbeefAPI
{
	public static native int start();
	public static native int stop();
	public static native short[] getBuffer(int size, short buffer[]);
	
	// playlist access
	public static native int pl_get_count ();
	public static native String pl_get_item_text (int idx);
	
	// playback ctrl
	public static native void play_prev ();
	public static native void play_next ();
	public static native void play_idx (int idx);
	
	static {
		System.loadLibrary("deadbeef");
	}
}
