package com.media.ffmpeg.android;

import android.content.Context;
import android.graphics.Canvas;
import android.util.AttributeSet;
import android.view.Surface;
import android.view.SurfaceView;

public class FFMpegPlayerAndroid extends SurfaceView {
	
	private int 							mVideoWidth;
	private int 							mVideoHeight;
	private FFMpegConfigAndroid 			mConfig;
	private int[]							mBuffer;
	
	public FFMpegPlayerAndroid(Context context) {
        super(context);
        initVideoView();
    }
    
    public FFMpegPlayerAndroid(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
        initVideoView();
    }
    
    public FFMpegPlayerAndroid(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        initVideoView();
    }
    
    private void initVideoView() {
    	mVideoWidth = 0;
    	mVideoHeight = 0;
    	setBuffer(new int[mVideoHeight * mVideoWidth]);
    }
    
    private void setBuffer(int[] pixels) {
    	synchronized (this) {
			mBuffer = pixels;
		}
    }
	
    private String[] mArgs = new String[] {"ffplay", "/sdcard/Videos/pixar.flv"};
	public void run(String[] args) {
		mArgs = args;
	}
	
	public void runAsync(final String[] args) {
		new Thread() {
			public void run() {
				FFMpegPlayerAndroid.this.run(args);
			};
		}.start();
	}

	@Override
	protected void onDraw(Canvas canvas) {
		int screenWidth = mConfig.resolution[0];
		int screenHeight = mConfig.resolution[1];
		
		synchronized (this) {
			canvas.scale(((float) mVideoWidth) / screenWidth, ((float) mVideoHeight) / screenHeight);
			canvas.drawBitmap(mBuffer, 0, screenWidth, 0, 0, screenWidth, screenHeight, false, null);
		}
	}
	
	private native void native_av_release();
	private native void native_av_setSurface(Surface surface);
	private native void native_av_runPlayer(String[] args);

}
