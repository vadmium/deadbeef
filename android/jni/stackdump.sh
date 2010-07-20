#!/bin/sh
#adb logcat >logcat
./parse_stack.py libdeadbeef.S logcat
