#!/bin/sh
$NDK/build/prebuilt/linux-x86/arm-eabi-4.2.1/bin/arm-eabi-objdump -S ../libs/armeabi/libdeadbeef.so >libdeadbeef.S
