package com.media.ffmpeg;

public class FFMpegAVRational {
	
	protected int	pointer;
	private int  	mNum; // numerator 
	private int 	mDen; // denominator
	
	private FFMpegAVRational() {}
	
	public int getNumerator() {
		return mNum;
	}
	
	public int getDenominator() {
		return mDen;
	}
	
	protected void release() {
		nativeRelease(pointer);
	}
	
	private native void nativeRelease(int pointer);

}
