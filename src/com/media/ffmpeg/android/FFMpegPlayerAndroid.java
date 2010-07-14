package com.media.ffmpeg.android;

import java.io.IOException;

import com.media.ffmpeg.FFMpegAVFormatContext;
import com.media.ffmpeg.IFFMpegPlayer;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.MediaController;
import android.widget.MediaController.MediaPlayerControl;

public class FFMpegPlayerAndroid extends SurfaceView {
	private static final String 			TAG = "FFMpegPlayerAndroid"; 
	private static final boolean 			D = true;
	
	private int        	 					mVideoWidth;
    private int         					mVideoHeight;
	private int 							mSurfaceWidth;
	private int 							mSurfaceHeight;
	private SurfaceHolder					mSurfaceHolder;
	private MediaController					mMediaController;
	private Thread							mRenderThread;
	private IFFMpegPlayer					mListener;
	private boolean							mPlaying;
	private Bitmap							mBitmap;
	private FFMpegAVFormatContext			mVideoPath;
	private boolean 						mFitToScreen;
	
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
    	mFitToScreen = true;
    	mVideoWidth = 0;
        mVideoHeight = 0;
    	getHolder().addCallback(mSHCallback);
    	attachMediaController(context);
    }
    
    private void attachMediaController(Context context) {
    	mMediaController = new MediaController(context);
        View anchorView = this.getParent() instanceof View ?
                    (View)this.getParent() : this;
        mMediaController.setMediaPlayer(mMediaPlayerControl);
        mMediaController.setAnchorView(anchorView);
        mMediaController.setEnabled(false);
    }
    
    public void setListener(IFFMpegPlayer listener) {
    	mListener = listener;
    }
    
    private void openVideo() {
    	try {
			int[] size = nativeInit(mVideoPath);
			mVideoWidth = size[0];
			mVideoHeight = size[1];
			Log.d(TAG, "Video size: " + mVideoWidth + " x " + mVideoHeight);
		} catch (IOException e) {
			if(mListener != null) {
				mListener.onError("Opening video", e);
			}
		}
    }
    
    private void start() {
		mBitmap = Bitmap.createBitmap(mVideoWidth, mVideoHeight, Bitmap.Config.RGB_565);
		play();
		toggleMediaControlsVisiblity();
    }
    
    /**
     * init player
     * @param filePath path to video which we want to play
     * @throws IOException
     */
    public void setVideoPath(String filePath) throws IOException {
    	mVideoPath = nativeSetInputFile(filePath);
	}
    
    private void play() {
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
    private void release() {
    	if(D) {
    		Log.d(TAG, "releasing player");
    	}
    	
    	try {
			stop();
		} catch (InterruptedException e) {
			if(mListener != null) {
				mListener.onError("Couldn't stop player", e);
			}
		}
    	
    	nativeRelease();
    	
    	if(D) {
    		Log.d(TAG, "player released");
    	}
    	
    	if(mListener != null) {
    		mListener.onRelease();
    	}
    }
    
    @Override
    public boolean onTouchEvent(MotionEvent event) {
    	toggleMediaControlsVisiblity();
    	return false;
    }
    
    private void toggleMediaControlsVisiblity() {
        if (mMediaController.isShowing()) {
            mMediaController.hide();
        } else {
            mMediaController.show(3000);
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
            	doDraw(c);
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
	
	private void doDraw(Canvas c) {
		if(mFitToScreen) {
			float scale_x = (float) mSurfaceWidth/ (float) mVideoWidth;
			float scale_y = (float) mSurfaceHeight/ (float) mVideoHeight;
			c.scale(scale_x, scale_y);
		}
		
		c.drawBitmap(mBitmap, 0, 0, null);
		
		if(D) {
			calcFps();
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
	
	SurfaceHolder.Callback mSHCallback = new SurfaceHolder.Callback() {
        public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
            mSurfaceWidth = w;
            mSurfaceHeight = h;
            start();
        }

        public void surfaceCreated(SurfaceHolder holder) {
        	Log.d(TAG, "Surface created");
            mSurfaceHolder = holder;
            openVideo();
        }

        public void surfaceDestroyed(SurfaceHolder holder) {
        	Log.d(TAG, "Surface destroyed");
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
			// TODO Auto-generated method stub
			
		}
		
		public void seekTo(int pos) {
			// TODO Auto-generated method stub
			
		}
		
		public void pause() {
			// TODO Auto-generated method stub
			
		}
		
		public boolean isPlaying() {
			// TODO Auto-generated method stub
			return false;
		}
		
		public int getDuration() {
			// TODO Auto-generated method stub
			return 0;
		}
		
		public int getCurrentPosition() {
			// TODO Auto-generated method stub
			return 0;
		}
		
		public int getBufferPercentage() {
			// TODO Auto-generated method stub
			return 0;
		}
	};
	
	private native FFMpegAVFormatContext nativeSetInputFile(String filePath) throws IOException;
	private native int[] nativeInit(FFMpegAVFormatContext AVFormatContext) throws IOException;
	private native void nativePlay(Bitmap bitmap)throws IOException;
	private native void nativeStop();
	private native void nativeSetSurface(Surface surface);
	private native void nativeRelease();

}
