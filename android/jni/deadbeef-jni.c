/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include <stdlib.h>
#include <jni.h>
#include "org_deadbeef_android_DeadbeefAPI.h"

JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_start
  (JNIEnv *env, jobject obj) {
    return 0;
}

JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_stop
  (JNIEnv *env, jobject obj) {
    return 0;
}

JNIEXPORT jshortArray JNICALL Java_org_deadbeef_android_DeadbeefAPI_getBuffer
  (JNIEnv *env, jobject obj, jint size, jshortArray buffer) {
    short b[size];
    int i;
    for (i = 0; i < size; i++) {
        b[i] = (rand () % 65535) - 32768;
    }
    // TODO: int length = deadbeef->streamer_read (b, size*2);
    (*env)->SetShortArrayRegion(env, buffer, 0, size, b);

    return buffer;
}
