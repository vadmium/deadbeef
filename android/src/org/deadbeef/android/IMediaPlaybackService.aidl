package org.deadbeef.android;

interface IMediaPlaybackService
{
    void stop();
    void pause();
    void play();
    boolean isPaused();
    void prev();
    void next();
    float duration();
    float position();
    void seek(float pos);
    String getTrackName();
    String getAlbumName();
    String getArtistName();
    void refreshStatus();
}

