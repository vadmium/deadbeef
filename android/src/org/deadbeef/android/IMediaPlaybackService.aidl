/* //device/samples/SampleCode/src/com/android/samples/app/RemoteServiceInterface.java
**
** Copyright 2007, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License"); 
** you may not use this file except in compliance with the License. 
** You may obtain a copy of the License at 
**
**     http://www.apache.org/licenses/LICENSE-2.0 
**
** Unless required by applicable law or agreed to in writing, software 
** distributed under the License is distributed on an "AS IS" BASIS, 
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
** See the License for the specific language governing permissions and 
** limitations under the License.
*/

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

