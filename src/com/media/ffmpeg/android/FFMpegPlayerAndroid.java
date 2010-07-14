package com.media.ffmpeg.android;

import java.io.IOException;

import com.media.ffmpeg.FFMpegAVFormatContext;
import com.media.ffmpeg.IFFMpegPlayer;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class FFMpegPlayerAndroid extends SurfaceView {
	private static final String 			TAG = "FFMpegPlayerAndroid"; 
	private static final boolean 			D = true;
	
	public static class Drawing {
		public static final int DRAWING_JAVA 	= 1;	// drawed here in java
		public static final int DRAWING_NATIVE 	= 2; 	// drawed in native code, should be faster
	}
	
	private int 							mVideoWidth;
	private int 							mVideoHeight;
	private SurfaceHolder					mSurfaceHolder;
	private Thread							mRenderThread;
	private int								mDrawingType;
	private IFFMpegPlayer					mListener;
	private boolean							mPlaying;
	private Bitmap							mBitmap;
	private FFMpegConfigAndroid 			mConfig;
	
	public FFMpegPlayerAndroid(Context context) {
        super(context);
        initVideoView(context);
    }
    
    public FFMpegPlayerAndroid(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
        initVideoView(context);
    }
    
    public FFMpegPlayerAndroid(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        initVideoView(context);
    }
    
    private void initVideoView(Context context) {
    	mVideoWidth = 480;
    	mVideoHeight = 320;
    	mDrawingType = Drawing.DRAWING_JAVA;
    	mConfig = new FFMpegConfigAndroid(context);
    	mSurfaceHolder = getHolder();
		nativeSurfaceChanged(mVideoWidth, mVideoHeight);
		mBitmap = Bitmap.createBitmap(mVideoWidth, mVideoHeight, Bitmap.Config.RGB_565);
		play();
    }
    
    public void setListener(IFFMpegPlayer listener) {
    	mListener = listener;
    }
    
    /**
     * init player
     * @param filePath path to video which we want to play
     * @throws IOException
     */
    public void init(String filePath) throws IOException {
    	FFMpegAVFormatContext input = nativeSetInputFile(filePath);
    	nativeInit(input);
	}
    
    public void play() {
    	mRenderThread = new Thread() {
			public void run() {
				mPlaying = true;
				
				if(mListener != null) {
					mListener.onPlay();
				}
				
				try {
					nativePlay(mBitmap);
				} catch (IOException e) {
					Log.e(TAG, "Error while playing: " + e.getMessage());
					mPlaying = false;
					if(mListener != null) {
						mListener.onError("Error while playing", e);
					}
				}
				
				if(mListener != null) {
		    		mListener.onStop();
		    	}
			}
		};
		mRenderThread.start();
    }
    
    /**
     * stops player
     * @throws InterruptedException
     */
    public void stop() throws InterruptedException {
    	if(!mPlaying) {
    		return;
    	}
    	
    	if(D) {
    		Log.d(TAG, "stopping player");
    	}
    	
    	mPlaying = false;
    	
    	nativeStop();
    	if(mRenderThread != null) {
    		mRenderThread.join();
    	}
    	
    	if(D) {
    		Log.d(TAG, "player stopped");
    	}
    	
    	if(mListener != null) {
    		mListener.onStop();
    	}
    }
    
    /**
     * release all allocated objects by player
     */
    public void release() {
    	if(D) {
    		Log.d(TAG, "releasing player");
    	}
    	
    	nativeRelease();
    	
    	if(D) {
    		Log.d(TAG, "player released");
    	}
    	
    	if(mListener != null) {
    		mListener.onRelease();
    	}
    }
	
	/**
	 * native callback which receive pixels from ffmpeg
	 * @param pixels
	 */
	private void onVideoFrame() {
		Canvas c = null;
        try {
            c = mSurfaceHolder.lockCanvas(null);
            synchronized (mSurfaceHolder) {
            	Log.d(TAG, "drawing");
            	
            	c.drawBitmap(mBitmap, 0, 0, null);
            	
            	Log.d(TAG, "drawed");
        		
        		if(D) {
        			calcFps();
        		}
			}
        } finally {
            // do this in a finally so that if an exception is thrown
            // during the above, we don't leave the Surface in an
            // inconsistent state
            if (c != null) {
                mSurfaceHolder.unlockCanvasAndPost(c);
            }
        }
	}
	
	private long mStart;
	private byte mScreenCounter;
	private void calcFps() {
		long now = System.currentTimeMillis();
		if(mStart == 0) {
			mStart = System.currentTimeMillis();
		} else if(now > mStart + 1000) {
			Log.d(TAG, "Fps: " + mScreenCounter);
			mScreenCounter = 0;
			mStart = 0;
		} else {
			mScreenCounter++;
		}
	}
	
	@Override
	protected void onDetachedFromWindow() {
		Log.d(TAG, "Surface destroyed");
		try {
			stop();
		} catch (InterruptedException e) {
			if(mListener != null) {
				mListener.onError("Couldn't stop player", e);
			}
		}
		release();
	}
	
	/*
	private class FFMpegSurfaceHandler implements SurfaceHolder.Callback {

		public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
			//mVideoHeight = height;
			//mVideoWidth = width;
			Log.d(TAG, "Surface width: " + width + ", height: " + height);
			nativeSurfaceChanged(width, height);
			mBitmap = Bitmap.createBitmap(width, height, Bitmap.Config.RGB_565);
		}

		public void surfaceCreated(SurfaceHolder holder) {
			Log.d(TAG, "Surface created");
		}

		public void surfaceDestroyed(SurfaceHolder holder) {
			Log.d(TAG, "Surface destroyed");
			try {
				stop();
			} catch (InterruptedException e) {
				if(mListener != null) {
					mListener.onError("Couldn't stop player", e);
				}
			}
			release();
		}
		
	}
	*/
	
	private native void nativeSurfaceChanged(int width, int height);
	private native FFMpegAVFormatContext nativeSetInputFile(String filePath) throws IOException;
	private native void nativeInit(FFMpegAVFormatContext AVFormatContext) throws IOException;
	private native void nativePlay(Bitmap bitmap)throws IOException;
	private native void nativeStop();
	private native void nativeSetSurface(Surface surface);
	private native void nativeRelease();

}
