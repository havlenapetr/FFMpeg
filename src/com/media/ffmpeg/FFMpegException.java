package com.media.ffmpeg;

public class FFMpegException extends Exception {
	
	public static final int LEVEL_FATAL = -1;
	public static final int LEVEL_ERROR = -2;
	public static final int LEVEL_WARNING = -3;
	
	private int mLevel;
	
	public FFMpegException(int level, String msg) {
		super(msg);
		mLevel = level;
	}
	
	public int getLevel() {
		return mLevel;
	}

}
