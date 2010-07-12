package com.media.ffmpeg;

public interface IFFMpegPlayer {

	public void onPlay();
	
	public void onStop();
	
	public void onRelease();
	
	public void onError(String msg, Exception e);

}
