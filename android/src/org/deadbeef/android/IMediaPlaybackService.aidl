package org.deadbeef.android;

interface IMediaPlaybackService
{
    void stop();
    void pause();
    void play();
    void playIdx(int idx);
    boolean isPaused();
    boolean isPlaying();
    boolean isStopped();
    void prev();
    void next();
    float duration();
    float position();
    void seek(float pos);
    String getTrackName();
    String getAlbumName();
    String getArtistName();
    void refreshStatus();
    void cycleRepeatMode();
    void cycleShuffleMode();
    void startFile(String fname);
    int getCurrentIdx();
    int getPlayOrder();
    int getPlayMode();
    
    void pl_add_dir (String dir);
    void pl_insert_dir (String dir, int plt, int after);
    boolean busyAddingFiles ();
    
    void handle_ddb_events ();
}

