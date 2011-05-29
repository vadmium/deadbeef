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
	public static native int pl_getcount (int iter);
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
	public static native float pl_get_totaltime ();
	public static native float pl_get_item_duration (int trk);
	public static native String pl_get_track_filetype (int trk);
	public static native int pl_insert_dir (int plt, int after, String path);
	public static native String pl_get_track_path (int trk);
	public static native int plt_get_item_bitrate (int trk);
	
	// playlist manager
	public static native int plt_get_count ();
    public static native int plt_get_sel_count (int plt);
    public static native int plt_add (int before, String title);
    public static native void plt_remove (int plt);
    public static native void plt_set_curr (int plt);
    public static native int plt_get_curr ();
    public static native String plt_get_title (int plt);
    public static native int plt_set_title (int plt, String title);
    public static native void plt_move (int from, int before);	
	
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
	public static native boolean play_is_paused ();
	public static native boolean play_is_playing ();
	public static native boolean play_is_stopped ();
	public static native boolean is_streamer_active ();

	// 0=loop_all,1=noloop,2=loopsingle
	public static native void set_play_mode (int mode);
	public static native int get_play_mode ();
	// 0=linear,1=shuffletracks,2=random,3=shufflealbums
	public static native void set_play_order (int order);
	public static native int get_play_order ();
	
	// streamer
	public static native int streamer_get_playing_track ();
	public static native int streamer_get_streaming_track ();
	public static native int streamer_get_current_fileinfo ();
	public static native int streamer_get_current_fileinfo_format ();
	public static native float streamer_get_playpos ();
	public static native int streamer_get_apx_bitrate ();
	
	// format
	public static native int fmt_get_channels (int fmt);
	public static native int fmt_get_bps (int fmt);
	public static native int fmt_get_samplerate (int fmt);
	
	// streamer events
	public static native boolean event_is_pending ();
	public static native void event_dispatch ();
	public static native String event_get_type ();
	public static native String event_get_string (int idx);
	public static native int event_get_int (int idx);
	
	// dsp
	public static native int dsp_find (String name);
	public static native void dsp_enable (int dsp, boolean enable);
	public static native boolean dsp_is_enabled (int dsp);
	public static native String dsp_get_param (int dsp, int p);
	public static native void dsp_set_param (int dsp, int p, String val);
	public static native void dsp_save_config ();
	public static native int dsp_num_presets (int dsp);
	public static native String dsp_preset_name (int dsp, int idx);
	public static native int dsp_delete_preset (int dsp, int idx);
	public static native int dsp_load_preset (int dsp, int idx);
	public static native int dsp_save_preset (int dsp, String name);
	public static native int dsp_rename_preset (int dsp, int idx, String newname);
	
	// config
	public static native int conf_save ();

	static {
		System.loadLibrary("deadbeef");
	}
}
