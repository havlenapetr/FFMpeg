package com.media.ffmpeg.android;

import java.io.IOException;
import java.util.ArrayList;

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

public class FFMpegPlayerAndroid extends SurfaceView {
	private static final String 			TAG = "FFMpegPlayerAndroid"; 
	private static final boolean 			D = true;
	
	private static final int				EVENTS_PLAY = 1;
	private static final int				EVENTS_STOP = 2;
	private static final int				EVENTS_PAUSE = 3;
	
	private boolean 						mInitzialized;
	private boolean 						mRelease;
	private int        	 					mVideoWidth;
    private int         					mVideoHeight;
	private int 							mSurfaceWidth;
	private int 							mSurfaceHeight;
	private Thread							mRenderThread;
	private Context							mContext;
	private SurfaceHolder					mSurfaceHolder;
	private AudioTrack						mAudioTrack;
	private MediaController					mMediaController;
	private IFFMpegPlayer					mListener;
	private boolean							mPlaying;
	private Bitmap							mBitmap;
	private FFMpegAVFormatContext			mInputVideo;
	private boolean 						mFitToScreen;
	private ArrayList<Integer> 				mEvents;
	
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
    	mInitzialized = false;
    	mEvents = new ArrayList<Integer>();
    	mFitToScreen = true;
    	mVideoWidth = 0;
        mVideoHeight = 0;
        mAudioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, 
        							 44100, 
        							 AudioFormat.CHANNEL_CONFIGURATION_STEREO, 
        							 AudioFormat.ENCODING_PCM_16BIT, 
        							 FFMpegAVCodecTag.AVCODEC_MAX_AUDIO_FRAME_SIZE, 
        							 AudioTrack.MODE_STREAM);
    	getHolder().addCallback(mSHCallback);
    	//mEventThread.start();
    }
    
    private void attachMediaController() {
    	mMediaController = new MediaController(mContext);
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
			FFMpegAVCodecContext context = nativeInit(mInputVideo);
			mVideoWidth = context.getWidth();
			mVideoHeight = context.getHeight();
			Log.d(TAG, "Video size: " + mVideoWidth + " x " + mVideoHeight);
		} catch (IOException e) {
			if(mListener != null) {
				mListener.onError("Opening video", e);
			}
		}
    }
    
    private void startVideo() {
		mBitmap = Bitmap.createBitmap(mVideoWidth, mVideoHeight, Bitmap.Config.RGB_565);
		attachMediaController();
		
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
		
		toggleMediaControlsVisiblity();
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
	
	private void onAudioBuffer(short[] buffer) {
		mAudioTrack.write(buffer, 0, buffer.length);
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
            mInitzialized = true;
        }

        public void surfaceCreated(SurfaceHolder holder) {
        	Log.d(TAG, "Surface created");
            mSurfaceHolder = holder;
            mInitzialized = false;
            openVideo();
        }

        public void surfaceDestroyed(SurfaceHolder holder) {
        	Log.d(TAG, "Surface destroyed");
			release();
			if(mMediaController.isShowing()) {
				mMediaController.hide();
			}
			mInitzialized = false;
			// after we return from this we can't use the surface any more
            mSurfaceHolder = null;
        }
    };
    
    MediaPlayerControl mMediaPlayerControl = new MediaPlayerControl() {
		
		public void start() {
			Log.d(TAG, "want start");
		}
		
		public void seekTo(int pos) {
			Log.d(TAG, "want seek to");
		}
		
		public void pause() {
			Log.d(TAG, "want pause");
		}
		
		public boolean isPlaying() {
			return mPlaying;
		}
		
		public int getDuration() {
			if(mInputVideo != null) {
				return mInputVideo.getDurationInSeconds();
			}
			return 0;
		}
		
		public int getCurrentPosition() {
			Log.d(TAG, "want get current position");
			return 0;
		}
		
		public int getBufferPercentage() {
			Log.d(TAG, "want buffer percentage");
			return 0;
		}
	};
	
	private native FFMpegAVFormatContext nativeSetInputFile(String filePath) throws IOException;
	private native FFMpegAVCodecContext nativeInit(FFMpegAVFormatContext AVFormatContext) throws IOException;
	private native void nativePlay(Bitmap bitmap)throws IOException;
	private native void nativeStop();
	private native void nativeSetSurface(Surface surface);
	private native void nativeRelease();

}
