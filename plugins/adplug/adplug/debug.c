/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2002 Simon Peter <dn.tlp@gmx.net>, et al.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * debug.h - AdPlug Debug Logger
 * Copyright (c) 2002 Riven the Mage <riven@ok.ru>
 * Copyright (c) 2002 Simon Peter <dn.tlp@gmx.net>
 */

#ifdef DEBUG

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#if TARGET_ANDROID
#include <android/log.h>
#endif

#include <stdio.h>
#include <stdarg.h>

static FILE *log = NULL;

void AdPlug_LogFile(const char *filename)
{
  if(log) fclose(log);
  log = fopen(filename,"wt");
}

void AdPlug_LogWrite(const char *fmt, ...)
{

#if TARGET_ANDROID
    char p[300];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(p, sizeof(p), fmt, ap);
    va_end(ap);
    __android_log_write(ANDROID_LOG_INFO,"DDB",p);
#else
  va_list argptr;

  va_start(argptr, fmt);

  if(log) {
    vfprintf(log, fmt, argptr);
    fflush(log);
  } else
    vfprintf(stderr, fmt, argptr);

  va_end(argptr);
#endif
}

#else

void AdPlug_LogFile(char *filename) { }
void AdPlug_LogWrite(char *fmt, ...) { }

#endif
