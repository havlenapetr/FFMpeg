package com.media.ffmpeg.android;

import java.io.IOException;

import com.media.ffmpeg.FFMpegPlayer;

import android.content.Context;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.MediaController;
import android.widget.MediaController.MediaPlayerControl;

public class FFMpegMovieViewAndroid extends SurfaceView {
	private static final String 	TAG = "FFMpegMovieViewAndroid"; 
	
	private Context					mContext;
	private FFMpegPlayer			mPlayer;
	private SurfaceHolder			mSurfaceHolder;
	private MediaController			mMediaController;
	private Thread					mRenderThread;
	
	public FFMpegMovieViewAndroid(Context context) {
        super(context);
        initVideoView(context);
    }
    
    public FFMpegMovieViewAndroid(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
        initVideoView(context);
    }
    
    public FFMpegMovieViewAndroid(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        initVideoView(context);
    }
    
    private void initVideoView(Context context) {
    	mContext = context;
    	mPlayer = new FFMpegPlayer();
    	getHolder().addCallback(mSHCallback);
    }
    
    private void attachMediaController() {
    	mMediaController = new MediaController(mContext);
        View anchorView = this.getParent() instanceof View ?
                    (View)this.getParent() : this;
        mMediaController.setMediaPlayer(mMediaPlayerControl);
        mMediaController.setAnchorView(anchorView);
        mMediaController.setEnabled(true);
    }
    
    public void setVideoPath(String filePath) throws IllegalArgumentException, IllegalStateException, IOException {
		mPlayer.setDataSource(filePath);
	}
    
    /**
     * initzialize player
     */
    private void openVideo() {
    	try {
    		mPlayer.setDisplay(mSurfaceHolder);
			mPlayer.prepare();
		} catch (IllegalStateException e) {
			Log.e(TAG, "Couldn't prepare player: " + e.getMessage());
		} catch (IOException e) {
			Log.e(TAG, "Couldn't prepare player: " + e.getMessage());
		}
    }
    
    private void startVideo() {
    	attachMediaController();
    	
    	// we hasn't run player thread so we are launching
    	if(mRenderThread == null) {
    		
    		mRenderThread = new Thread() {
    			public void run() {
    				mPlayer.start();
    				Log.d(TAG, "video ended");
    			};
    		};
    		mRenderThread.start();
    	}
    }
    
    private void release() {
    	Log.d(TAG, "releasing player");
    	
    	mPlayer.stop();
    	try {
			mRenderThread.join();
		} catch (InterruptedException e) {}
		
		Log.d(TAG, "released");
    }
    
    SurfaceHolder.Callback mSHCallback = new SurfaceHolder.Callback() {
        public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
            startVideo();
        }

        public void surfaceCreated(SurfaceHolder holder) {
            mSurfaceHolder = holder;
            openVideo();
        }

        public void surfaceDestroyed(SurfaceHolder holder) {
			release();
			if(mMediaController.isShowing()) {
				mMediaController.hide();
			}
			// after we return from this we can't use the surface any more
            mSurfaceHolder = null;
        }
    };
    
    MediaPlayerControl mMediaPlayerControl = new MediaPlayerControl() {
		
		public void start() {
			//mPlayer.resume();
		}
		
		public void seekTo(int pos) {
			//Log.d(TAG, "want seek to");
		}
		
		public void pause() {
			//mPlayer.pause();
		}
		
		public boolean isPlaying() {
			return true;//mPlayer.isPlaying();
		}
		
		public int getDuration() {
			return 0;//mPlayer.getDuration();
		}
		
		public int getCurrentPosition() {
			//Log.d(TAG, "want get current position");
			return 0;
		}
		
		public int getBufferPercentage() {
			//Log.d(TAG, "want buffer percentage");
			return 0;
		}
	};
}
