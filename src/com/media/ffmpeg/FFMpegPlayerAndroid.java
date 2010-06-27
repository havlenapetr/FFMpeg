package com.media.ffmpeg;

public class FFMpegPlayerAndroid {
	
	public void run(String[] args) {
		native_av_runPlayer(args);
	}
	
	public void runAsync(final String[] args) {
		new Thread() {
			public void run() {
				FFMpegPlayerAndroid.this.run(args);
			};
		}.start();
	}
	
	private native void native_av_runPlayer(String[] args);

}
