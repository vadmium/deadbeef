package org.deadbeef.android;

public class DeadbeefAPI
{
	public native int start();
	public native int stop();
	public native short[] getBuffer(int size, short buffer[]);
	
	static {
		System.loadLibrary("deadbeef");
	}
}
