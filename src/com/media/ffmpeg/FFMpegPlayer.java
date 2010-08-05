package com.media.ffmpeg;

import android.os.PowerManager;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.media.AudioManager;

import java.io.IOException;
import java.lang.ref.WeakReference;

public class FFMpegPlayer
{
    private static final int MEDIA_NOP = 0; // interface test message
    private static final int MEDIA_PREPARED = 1;
    private static final int MEDIA_PLAYBACK_COMPLETE = 2;
    private static final int MEDIA_BUFFERING_UPDATE = 3;
    private static final int MEDIA_SEEK_COMPLETE = 4;
    private static final int MEDIA_SET_VIDEO_SIZE = 5;
    private static final int MEDIA_ERROR = 100;
    private static final int MEDIA_INFO = 200;

    /** Unspecified media player error.
     * @see android.media.MediaPlayer.OnErrorListener
     */
    public static final int MEDIA_ERROR_UNKNOWN = 1;

    /** Media server died. In this case, the application must release the
     * MediaPlayer object and instantiate a new one.
     * @see android.media.MediaPlayer.OnErrorListener
     */
    public static final int MEDIA_ERROR_SERVER_DIED = 100;

    /** The video is streamed and its container is not valid for progressive
     * playback i.e the video's index (e.g moov atom) is not at the start of the
     * file.
     * @see android.media.MediaPlayer.OnErrorListener
     */
    public static final int MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK = 200;


    /** Unspecified media player info.
     * @see android.media.MediaPlayer.OnInfoListener
     */
    public static final int MEDIA_INFO_UNKNOWN = 1;

    /** The video is too complex for the decoder: it can't decode frames fast
     *  enough. Possibly only the audio plays fine at this stage.
     * @see android.media.MediaPlayer.OnInfoListener
     */
    public static final int MEDIA_INFO_VIDEO_TRACK_LAGGING = 700;

    /** Bad interleaving means that a media has been improperly interleaved or
     * not interleaved at all, e.g has all the video samples first then all the
     * audio ones. Video is playing but a lot of disk seeks may be happening.
     * @see android.media.MediaPlayer.OnInfoListener
     */
    public static final int MEDIA_INFO_BAD_INTERLEAVING = 800;

    /** The media cannot be seeked (e.g live stream)
     * @see android.media.MediaPlayer.OnInfoListener
     */
    public static final int MEDIA_INFO_NOT_SEEKABLE = 801;

    /** A new set of metadata is available.
     * @see android.media.MediaPlayer.OnInfoListener
     */
    public static final int MEDIA_INFO_METADATA_UPDATE = 802;
    
    public static final int MEDIA_INFO_FRAMERATE_VIDEO = 900;
    public static final int MEDIA_INFO_FRAMERATE_AUDIO = 901;
    
    private final static String 		TAG = "MediaPlayer";

    private int 						mNativeContext; // accessed by native methods
    //private int 						mListenerContext; // accessed by native methods
    private Surface 					mSurface; // accessed by native methods
    private SurfaceHolder  				mSurfaceHolder;
    private PowerManager.WakeLock 		mWakeLock = null;
    private boolean 					mScreenOnWhilePlaying;
    private boolean 					mStayAwake;
    
    static {
        native_init();
    }

    public FFMpegPlayer() {
        /* Native setup requires a weak reference to our object.
         * It's easier to create it here than in C++.
         */
        native_setup(new WeakReference<FFMpegPlayer>(this));
    }

    /**
     * Called from native code when an interesting event happens.  This method
     * just uses the EventHandler system to post the event back to the main app thread.
     * We use a weak reference to the original MediaPlayer object so that the native
     * code is safe from the object disappearing from underneath it.  (This is
     * the cookie passed to native_setup().)
     */
    private static void postEventFromNative(Object mediaplayer_ref,
                                            int what, int arg1, int arg2, Object obj)
    {
    	switch(what) {
    	case MEDIA_INFO_FRAMERATE_VIDEO:
    		Log.d(TAG, "Video fps:" + arg1);
    		break;
    	case MEDIA_INFO_FRAMERATE_AUDIO:
    		Log.d(TAG, "Audio fps:" + arg1);
    		break;
    	}
    }
    
    /**
     * Sets the SurfaceHolder to use for displaying the video portion of the media.
     * This call is optional. Not calling it when playing back a video will
     * result in only the audio track being played.
     *
     * @param sh the SurfaceHolder to use for video display
     * @throws IOException 
     */
    public void setDisplay(SurfaceHolder sh) throws IOException {
        mSurfaceHolder = sh;
        if (sh != null) {
            mSurface = sh.getSurface();
        } else {
            mSurface = null;
        }
        _setVideoSurface(mSurface);
        //updateSurfaceScreenOn();
    }


    /**
     * Starts or resumes playback. If playback had previously been paused,
     * playback will continue from where it was paused. If playback had
     * been stopped, or never started before, playback will start at the
     * beginning.
     *
     * @throws IllegalStateException if it is called in an invalid state
     */
    public  void start() throws IllegalStateException {
        //stayAwake(true);
        _start();
    }

    /**
     * Stops playback after playback has been stopped or paused.
     *
     * @throws IllegalStateException if the internal player engine has not been
     * initialized.
     */
    public void stop() throws IllegalStateException {
        //stayAwake(false);
        _stop();
    }

    /**
     * Pauses playback. Call start() to resume.
     *
     * @throws IllegalStateException if the internal player engine has not been
     * initialized.
     */
    public void pause() throws IllegalStateException {
        //stayAwake(false);
        _pause();
    }

    /**
     * Set the low-level power management behavior for this MediaPlayer.  This
     * can be used when the MediaPlayer is not playing through a SurfaceHolder
     * set with {@link #setDisplay(SurfaceHolder)} and thus can use the
     * high-level {@link #setScreenOnWhilePlaying(boolean)} feature.
     *
     * <p>This function has the MediaPlayer access the low-level power manager
     * service to control the device's power usage while playing is occurring.
     * The parameter is a combination of {@link android.os.PowerManager} wake flags.
     * Use of this method requires {@link android.Manifest.permission#WAKE_LOCK}
     * permission.
     * By default, no attempt is made to keep the device awake during playback.
     *
     * @param context the Context to use
     * @param mode    the power/wake mode to set
     * @see android.os.PowerManager
     */
    /*
    public void setWakeMode(Context context, int mode) {
        boolean washeld = false;
        if (mWakeLock != null) {
            if (mWakeLock.isHeld()) {
                washeld = true;
                mWakeLock.release();
            }
            mWakeLock = null;
        }

        PowerManager pm = (PowerManager)context.getSystemService(Context.POWER_SERVICE);
        mWakeLock = pm.newWakeLock(mode|PowerManager.ON_AFTER_RELEASE, FFMpegPlayer.class.getName());
        mWakeLock.setReferenceCounted(false);
        if (washeld) {
            mWakeLock.acquire();
        }
    }
    */

    /**
     * Control whether we should use the attached SurfaceHolder to keep the
     * screen on while video playback is occurring.  This is the preferred
     * method over {@link #setWakeMode} where possible, since it doesn't
     * require that the application have permission for low-level wake lock
     * access.
     *
     * @param screenOn Supply true to keep the screen on, false to allow it
     * to turn off.
     */
    public void setScreenOnWhilePlaying(boolean screenOn) {
        if (mScreenOnWhilePlaying != screenOn) {
            mScreenOnWhilePlaying = screenOn;
           // updateSurfaceScreenOn();
        }
    }

    /*
    private void stayAwake(boolean awake) {
        if (mWakeLock != null) {
            if (awake && !mWakeLock.isHeld()) {
                mWakeLock.acquire();
            } else if (!awake && mWakeLock.isHeld()) {
                mWakeLock.release();
            }
        }
        mStayAwake = awake;
        updateSurfaceScreenOn();
    }
    */

    /*
    private void updateSurfaceScreenOn() {
        if (mSurfaceHolder != null) {
            mSurfaceHolder.setKeepScreenOn(mScreenOnWhilePlaying && mStayAwake);
        }
    }
    */

    /**
     * Update the MediaPlayer ISurface. Call after updating mSurface.
     */
    private native void _setVideoSurface(Surface surface) throws IOException;
    
    /**
     * Sets the data source (file-path or http/rtsp URL) to use.
     *
     * @param path the path of the file you want to play
     * @throws IllegalStateException if it is called in an invalid state
     */
    public native void setDataSource(String path) throws IOException, IllegalArgumentException, IllegalStateException;

    /**
     * Prepares the player for playback, synchronously.
     *
     * After setting the datasource and the display surface, you need to either
     * call prepare(). For files, it is OK to call prepare(),
     * which blocks until MediaPlayer is ready for playback.
     *
     * @throws IllegalStateException if it is called in an invalid state
     */
    public native void prepare() throws IOException, IllegalStateException;

    private native void _start() throws IllegalStateException;
    
    private native void _stop() throws IllegalStateException;
    
    private native void _pause() throws IllegalStateException;
    
    /**
     * Returns the width of the video.
     *
     * @return the width of the video, or 0 if there is no video,
     * no display surface was set, or the width has not been determined
     * yet. The OnVideoSizeChangedListener can be registered via
     * {@link #setOnVideoSizeChangedListener(OnVideoSizeChangedListener)}
     * to provide a notification when the width is available.
     */
    public native int getVideoWidth();

    /**
     * Returns the height of the video.
     *
     * @return the height of the video, or 0 if there is no video,
     * no display surface was set, or the height has not been determined
     * yet. The OnVideoSizeChangedListener can be registered via
     * {@link #setOnVideoSizeChangedListener(OnVideoSizeChangedListener)}
     * to provide a notification when the height is available.
     */
    public native int getVideoHeight();

    /**
     * Checks whether the MediaPlayer is playing.
     *
     * @return true if currently playing, false otherwise
     */
    public native boolean isPlaying();

    /**
     * Seeks to specified time position.
     *
     * @param msec the offset in milliseconds from the start to seek to
     * @throws IllegalStateException if the internal player engine has not been
     * initialized
     */
    public native void seekTo(int msec) throws IllegalStateException;

    /**
     * Gets the current playback position.
     *
     * @return the current position in milliseconds
     */
    public native int getCurrentPosition();

    /**
     * Gets the duration of the file.
     *
     * @return the duration in milliseconds
     */
    public native int getDuration();
    
    private native void _release();
    
    private native void _reset();
    
    /**
     * @hide
     */
    private native int native_suspend_resume(boolean isSuspend);

    /**
     * Sets the audio stream type for this MediaPlayer. See {@link AudioManager}
     * for a list of stream types. Must call this method before prepare() or
     * prepareAsync() in order for the target stream type to become effective
     * thereafter.
     *
     * @param streamtype the audio stream type
     * @see android.media.AudioManager
     */
    public native void setAudioStreamType(int streamtype);

    

    private static native final void native_init() throws RuntimeException;
    private native final void native_setup(Object mediaplayer_this);
    private native final void native_finalize();

    /**
     * Releases resources associated with this MediaPlayer object.
     * It is considered good practice to call this method when you're
     * done using the MediaPlayer.
     */
    public void release() {
        //stayAwake(false);
        //updateSurfaceScreenOn();
        _release();
    }

    /**
     * Resets the MediaPlayer to its uninitialized state. After calling
     * this method, you will have to initialize it again by setting the
     * data source and calling prepare().
     */
    public void reset() {
        //stayAwake(false);
        _reset();
    }

    /**
     * Suspends the MediaPlayer. The only methods that may be called while
     * suspended are {@link #reset()}, {@link #release()} and {@link #resume()}.
     * MediaPlayer will release its hardware resources as far as
     * possible and reasonable. A successfully suspended MediaPlayer will
     * cease sending events.
     * If suspension is successful, this method returns true, otherwise
     * false is returned and the player's state is not affected.
     * @hide
     */
    public boolean suspend() {
        if (native_suspend_resume(true) < 0) {
            return false;
        }

        //stayAwake(false);

        // make sure none of the listeners get called anymore
        //mEventHandler.removeCallbacksAndMessages(null);

        return true;
    }

    /**
     * Resumes the MediaPlayer. Only to be called after a previous (successful)
     * call to {@link #suspend()}.
     * MediaPlayer will return to a state close to what it was in before
     * suspension.
     * @hide
     */
    public boolean resume() {
        if (native_suspend_resume(false) < 0) {
            return false;
        }

        /*
        if (isPlaying()) {
            stayAwake(true);
        }
		*/
        
        return true;
    }

    @Override
    protected void finalize() { native_finalize(); }
    
    public interface IFFMpegPlayer {
    	public void onPlay();
    	public void onStop();
    	public void onRelease();
    	public void onError(String msg, Exception e);
    }
}
