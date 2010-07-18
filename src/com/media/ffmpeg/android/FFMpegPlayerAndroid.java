package com.media.ffmpeg.android;

import java.io.IOException;

import com.media.ffmpeg.FFMpegAVCodecContext;
import com.media.ffmpeg.FFMpegAVCodecTag;
import com.media.ffmpeg.FFMpegAVFormatContext;
import com.media.ffmpeg.IFFMpegPlayer;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.MediaController;
import android.widget.MediaController.MediaPlayerControl;

/**
 * 
 * @author petr
 *
 */
public class FFMpegPlayerAndroid extends SurfaceView {
	private static final String 			TAG = "FFMpegPlayerAndroid"; 
	private static final boolean 			D = true;
	
	/*
	private static final int				EVENTS_PLAY = 1;
	private static final int				EVENTS_STOP = 2;
	private static final int				EVENTS_PAUSE = 3;
	*/
	
	private FFMpegAVCodecContext			mVideoCodecCtx;
	private FFMpegAVCodecContext			mAudioCodecCtx;
	private int 							mSurfaceWidth;
	private int 							mSurfaceHeight;
	private Thread							mRenderThread;
	private Context							mContext;
	private SurfaceHolder					mSurfaceHolder;
	private MediaController					mMediaController;
	private IFFMpegPlayer					mListener;
	private boolean							mPlaying;
	private FFMpegAVFormatContext			mInputVideo;
	
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
    	mContext = context;
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
    
    public void setListener(IFFMpegPlayer listener) {
    	mListener = listener;
    }
    
    private void printContextInfo(FFMpegAVCodecContext context, int type) {
    	if(context == null) {
    		return;
    	}
    	String label = "Unknown";
    	if(type == 1) {
    		label = "Video";
    	}else if(type == 2) {
    		label = "Audio";
    	}
    	Log.d(TAG, "****************** " + label + " info **********************");
    	Log.d(TAG, "bitrate: " + context.getBitRate());
    	Log.d(TAG, "bitrate tolerance: " + context.getBitRateTolerance());
    	Log.d(TAG, "channels: " + context.getChannels());
    	Log.d(TAG, "frame number: " + context.getFrameNumber());
    	Log.d(TAG, "frame size: " + context.getFrameSize());
    	Log.d(TAG, "sample rate: " + context.getSampleRate());
    	Log.d(TAG, "width: " + context.getWidth());
    	Log.d(TAG, "height: " + context.getHeight());
    	Log.d(TAG, "****************************************************");
    }
    
    /**
     * initzialize player
     */
    private void openVideo() {
    	try {
    		nativeEnableErrorCallback();
			mVideoCodecCtx = nativeInitVideo(mInputVideo);
			mAudioCodecCtx = nativeInitAudio(mInputVideo);
			if(D) {
				printContextInfo(mVideoCodecCtx, 1);
				printContextInfo(mAudioCodecCtx, 2);
			}
			nativeSetSurface(mSurfaceHolder.getSurface());
		} catch (IOException e) {
			if(mListener != null) {
				mListener.onError("Opening video", e);
			}
		}
    }
    
    /**
     * starts playing of video
     */
    private void startVideo() {
		mPlaying = true;
		
		// we hasn't run player thread so we are launching
		if(mRenderThread == null) {
			attachMediaController();
			
			mRenderThread = new Thread() {
				
				public void run() {
					if(mListener != null) {
						mListener.onPlay();
					}
					
					try {
						nativePlay();
					} catch (IOException e) {
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
			toggleMediaControlsVisiblity();
		}
		// player thread is running so we are unpausing
		else {
			Log.d(TAG, "repausing player " + nativePause(false));
		}
    }
    
    /**
     * init player
     * @param filePath path to video which we want to play
     * @throws IOException
     */
    public void setVideoPath(String filePath) throws IOException {
    	mInputVideo = nativeSetInputFile(filePath);
	}
    
    /*
    public void play() {
    	synchronized (mEvents) {
        	mEvents.add(EVENTS_PLAY);
		}
    }
    */
    
    public boolean pause() {
    	mPlaying = false;
    	return nativePause(true);
    }
    
    public boolean resume() {
    	mPlaying = true;
    	return nativePause(false);
    }
    
    public void decodeAudio(boolean decode) {
    	nativeDecodeAudio(decode);
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
    	mRenderThread = null;
    	
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
		/*
		mRelease = true;
		try {
			mEventThread.join();
		} catch (InterruptedException e) {
			if(mListener != null) {
				mListener.onError("Couldn't stop event thread", e);
			}
		}
		mEventThread = null;
		*/
    	
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
	
	/*
	private Thread	mEventThread = new Thread() {
		
		private void processEvent(int event) {
			Log.d(TAG, "Processing event: " + event);
			
			switch(event) {
			case EVENTS_PLAY:
				FFMpegPlayerAndroid.this.openVideo();
				FFMpegPlayerAndroid.this.startVideo();
				break;
			}
		}
		
		public void run() {
			while(!mRelease) {
				if(mInitzialized) {
					synchronized (mEvents) {
						for(int i=0;i<mEvents.size();i++) {
							processEvent(mEvents.get(i));
						}
					}
				}
				
				try {
					sleep(100);
				} catch (InterruptedException e) {}
			}
		};
	};
	*/
	
	SurfaceHolder.Callback mSHCallback = new SurfaceHolder.Callback() {
        public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
        	Log.d(TAG, "Surface changed");
            mSurfaceWidth = w;
            mSurfaceHeight = h;
            startVideo();
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
			FFMpegPlayerAndroid.this.startVideo();
		}
		
		public void seekTo(int pos) {
			//Log.d(TAG, "want seek to");
		}
		
		public void pause() {
			FFMpegPlayerAndroid.this.pause();
		}
		
		public boolean isPlaying() {
			return mPlaying;
		}
		
		public int getDuration() {
			return mInputVideo.getDurationInMiliseconds();
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
	
	/**
	 * set input file for playing
	 * @param filePath path to video file
	 * @return FFMpeg AVFormatContext struct which should be passed to init player
	 * @throws IOException if file doesn't exist or couldn't be opened
	 */
	private native FFMpegAVFormatContext 	nativeSetInputFile(String filePath) throws IOException;
	

	private native FFMpegAVCodecContext 	nativeInitAudio(FFMpegAVFormatContext AVFormatContext) throws IOException;
	
	private native FFMpegAVCodecContext 	nativeInitVideo(FFMpegAVFormatContext AVFormatContext) throws IOException;
	
	private native void nativeEnableErrorCallback();
	
	/**
	 * pause video playing
	 * @param pause if true play -> pause, if false pause -> play
	 * @return true if operation was successful
	 */
	public native boolean					nativePause(boolean pause);
	
	/**
	 * starts playing movie 
	 * @param bitmap to which player will draw pixels
	 * @throws IOException
	 */
	private native void 					nativePlay() throws IOException;
	
	private native void 					nativeDecodeAudio(boolean decode);
	
	public native boolean 					isDecodingAudio();
	
	/**
	 * stops playing movie
	 */
	private native void 					nativeStop();
	
	/**
	 * sets native pointer of player surface view
	 * @param surface
	 */
	private native void 					nativeSetSurface(Surface surface);
	
	/**
	 * release all native resources allocated by player 
	 */
	private native void 					nativeRelease();

}
