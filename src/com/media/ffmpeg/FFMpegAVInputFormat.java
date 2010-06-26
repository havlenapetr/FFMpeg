package com.media.ffmpeg;

public class FFMpegAVInputFormat {
	
	protected long					pointer;
	private String 					mName;
	private String 					mLongName; // Descriptive name for the format, meant to be more human-readable than name. 
	private int 					mPrivDataSize; // Size of private data so that it can be allocated in the wrapper.
	private int 					mFlags; // Can use flags: AVFMT_NOFILE, AVFMT_NEEDNUMBER. 
	private String 					mExtensions; // If extensions are defined, then no probe is done. 
	private int 					mValue; // General purpose read-only value that the format can use.
	private FFMpegAVCodecTag		mCodecTag;
	private FFMpegAVInputFormat 	mNext;
	
	public String getName() {
		return mName;
	}
	public String getLongName() {
		return mLongName;
	}
	public int getPrivDataSize() {
		return mPrivDataSize;
	}
	public int getFlags() {
		return mFlags;
	}
	public String getExtensions() {
		return mExtensions;
	}
	public int getValue() {
		return mValue;
	}
	public FFMpegAVCodecTag getCodecTag() {
		return mCodecTag;
	}
	public FFMpegAVInputFormat getNext() {
		return mNext;
	}
	
	private native void nativeRelease(int pointer);
}
