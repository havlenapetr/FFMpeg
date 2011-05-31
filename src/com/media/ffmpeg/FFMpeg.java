package com.media.ffmpeg;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;

import android.content.Context;
import android.util.Log;

import com.media.ffmpeg.android.FFMpegMovieViewAndroid;
import com.media.ffmpeg.config.FFMpegConfig;


public class FFMpeg {
	
	public static final String[] LIBS = new String[] {
		"jniaudio", 	// used for access to android native AudioTrack class 
		"jnivideo",
		"ffmpeg_jni"	// ffmpeg libs compiled to jni lib
	};
	
	public static final String[] EXTENSIONS = new String[] {
		".mp4", 
		".flv", 
		".avi", 
		".wmv"
	};
	
	private static boolean				sLoaded = false;
	private Thread 						mThread;
	private IFFMpegListener 			mListener;
	
	private FFMpegFile					mInputFile;
	private FFMpegFile					mOutputFile;
	private boolean 					mConverting;
	
    public FFMpeg() throws FFMpegException {
    	if(!loadLibs()) {
    		throw new FFMpegException(FFMpegException.LEVEL_FATAL, "Couldn't load native libs");
    	}
    	native_avcodec_register_all();
		native_av_register_all();
		mConverting = false;
    }
    
    /**
     * loads all native libraries
     * @return true if all libraries was loaded, otherwise return false
     */
    private static boolean loadLibs() {
    	if(sLoaded) {
    		return true;
    	}
    	boolean err = false;
    	for(int i=0;i<LIBS.length;i++) {
    		try {
    			System.loadLibrary(LIBS[i]);
    		} catch(UnsatisfiedLinkError e) {
    			// fatal error, we can't load some our libs
    			Log.d("FFMpeg", "Couldn't load lib: " + LIBS[i] + " - " + e.getMessage());
    			err = true;
    		}
		}
    	if(!err) {
    		sLoaded = true;
    	}
    	return sLoaded;
    }
    
    public FFMpegUtils getUtils() {
    	return new FFMpegUtils();
    }
    
    public boolean isConverting() {
    	return mConverting;
    }
	
	public void setListener(IFFMpegListener listener) {
		mListener = listener;
	}
	
	public FFMpegFile getOutputFile() {
		return mOutputFile;
	}
	
	public FFMpegFile getInputFile() {
		return mInputFile;
	}
	
    public void init(String inputFile, String outputFile) throws RuntimeException, IOException {
		native_av_init();
		
		mInputFile = setInputFile(inputFile);
		mOutputFile = setOutputFile(outputFile);
	}
    
    public void setConfig(FFMpegConfig config) {
    	setFrameSize(config.resolution[0], config.resolution[1]);
		setAudioChannels(config.audioChannels);
		setAudioRate(config.audioRate);
		setFrameRate(config.frameRate);
		setVideoCodec(config.codec);
		setFrameAspectRatio(config.ratio[0], config.ratio[1]);
		setBitrate(config.bitrate);
		
		native_av_parse_options(new String[] {"ffmpeg", mOutputFile.getFile().getAbsolutePath()});
    }
    
    public FFMpegMovieViewAndroid getMovieView(Context context) {
    	return new FFMpegMovieViewAndroid(context);
    }
    
    public void setBitrate(String bitrate) {
    	native_av_setBitrate("b", bitrate);
    }
    
    public void setFrameAspectRatio(int x, int y) {
    	native_av_setFrameAspectRatio(x, y);
    }
    
    public void setVideoCodec(String codec) {
    	native_av_setVideoCodec(codec);
    }
    
    public void setAudioRate(int rate) {
    	native_av_setAudioRate(rate);
    }
    
    public void setAudioChannels(int channels) {
    	native_av_setAudioChannels(channels);
    }
    
    public void setFrameRate(int rate) {
    	native_av_setFrameRate(rate);
    }
    
    public void setFrameSize(int width, int height) {
    	native_av_setFrameSize(width, height);
    }
    
    public FFMpegFile setInputFile(String filePath) throws IOException {
    	File f = new File(filePath);
    	if(!f.exists()) {
    		throw new FileNotFoundException("File: " + filePath + " doesn't exist");
    	}
    	FFMpegAVFormatContext c = native_av_setInputFile(filePath);
    	return new FFMpegFile(f, c);
    }
    
    public FFMpegFile setOutputFile(String filePath) throws FileNotFoundException {
    	File f = new File(filePath);
    	if(f.exists()) {
    		f.delete();
    	}
    	//FFMpegAVFormatContext c = native_av_setOutputFile(filePath);
    	return new FFMpegFile(f, null);
    }
    
    public void newVideoStream(FFMpegAVFormatContext context) {
    	native_av_newVideoStream(context.pointer);
    }
	
	public void convert() throws RuntimeException {
		mConverting = true;
		if(mListener != null) {
			mListener.onConversionStarted();
		}
		
		native_av_convert();
		
		mConverting = false;
		if(mListener != null) {
			mListener.onConversionCompleted();
		}
	}
	
	public void convertAsync() throws RuntimeException {
		mThread = new Thread() {
			@Override
			public void run() {
				try {
					convert();
				} catch (RuntimeException e) {
					if(mListener != null) {
						mListener.onError(e);
					}
				}
			}
		};
		mThread.start();
	}
	
	public void waitOnEnd() throws InterruptedException {
		if(mThread == null) {
			return;
		}
		mThread.join();
	}
	
	public void release() {
		native_av_release(1);
	}
	
	/**
	 * callback called by native code to inform java about conversion
	 * @param total_size
	 * @param time
	 * @param bitrate
	 */
    private void onReport(double total_size, double time, double bitrate) {
		if(mListener != null) {
			FFMpegReport report = new FFMpegReport();
			report.total_size = total_size;
			report.time = time;
			report.bitrate = bitrate;
			mListener.onConversionProcessing(report);
		}
	}
    
    @Override
    protected void finalize() throws Throwable {
    	Log.d("FFMpeg", "finalizing ffmpeg main class");
    	sLoaded = false;
    }
	
    public interface IFFMpegListener {
    	public void onConversionProcessing(FFMpegReport report);
		public void onConversionStarted();
		public void onConversionCompleted();
		public void onError(Exception e);
	}
	
	private native void native_avcodec_register_all();
	
	//{ "native_avdevice_register_all", "()V", (void*) avdevice_register_all },
	
	//{ "native_avfilter_register_all", "()V", (void*) avfilter_register_all },
	
	private native void native_av_register_all();
	
    private native void native_av_init() throws RuntimeException;
    
    private native FFMpegAVFormatContext native_av_setInputFile(String filePath) throws IOException;
    
    private native FFMpegAVFormatContext native_av_setOutputFile(String filePath) throws IOException;
    
    private native int native_av_setBitrate(String opt, String arg);
    
    private native void native_av_newVideoStream(int pointer);
	
    /**
     * ar
     * @param rate
     */
	private native void native_av_setAudioRate(int rate);
	
	private native void native_av_setAudioChannels(int channels);
	
	private native void native_av_setVideoChannel(int channel);
	
	/**
	 * r
	 * @param rate
	 * @throws RuntimeException
	 */
	private native FFMpegAVRational native_av_setFrameRate(int rate) throws RuntimeException;
	
	/**
	 * ration
	 * @param x
	 * @param y
	 */
	private native void native_av_setFrameAspectRatio(int x, int y);
	
	/**
	 * codec
	 * @param codec
	 */
	private native void native_av_setVideoCodec(String codec);
	
	/**
	 * resolution
	 * @param width
	 * @param height
	 */
	private native void native_av_setFrameSize(int width, int height);
	
	private native void native_av_parse_options(String[] args) throws RuntimeException;
	
	private native void native_av_convert() throws RuntimeException;
	
	private native int native_av_release(int code);
	
}
