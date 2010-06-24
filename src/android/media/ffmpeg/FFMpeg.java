package android.media.ffmpeg;

import java.io.File;
import java.io.FileNotFoundException;

public class FFMpeg {
	
	public static final String LIB_NAME = "ffmpeg_jni";
	
	private Thread 						mThread;
	private IFFMpegListener 			mListener;
	
	private String 						mResolution;
	private String 						mCodec;
	private String 						mBitrate;
	private String 						mRatio;
	
	private FFMpegFile					mInputFile;
	private FFMpegFile					mOutputFile;
	
	static {
		System.loadLibrary(LIB_NAME);
	}
	
    public FFMpeg() {
    	native_avcodec_register_all();
		native_av_register_all();
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
	
    public void init(String res, String codec, String bitrate, String ratio, String inputFile, String outputFile) throws RuntimeException, FileNotFoundException {
    	mResolution = res;
    	mCodec = codec;
    	mBitrate = bitrate;
    	mRatio = ratio;
		native_av_init();
		
		mInputFile = setInputFile(inputFile);
		mOutputFile = setOutputFile(outputFile);
		
		native_av_parse_options(new String[] {
				"ffmpeg",
				"-s",
				mResolution, 
				"-vcodec", 
				mCodec, 
				"-ac", 
				"1", 
				"-ar", 
				"16000", 
				"-r", 
				"13", 
				"-b", 
				mBitrate,
				"-aspect", 
				mRatio});
	}
    
    private int setBitrate(String opt, String arg) {
    	return native_av_setBitrate(opt, arg);
    }
    
    private FFMpegFile setInputFile(String filePath) throws FileNotFoundException {
    	File f = new File(filePath);
    	if(!f.exists()) {
    		throw new FileNotFoundException("File: " + filePath + " doesn't exist");
    	}
    	FFMpegAVFormatContext c = native_av_setInputFile(filePath);
    	return new FFMpegFile(f, c);
    }
    
    private FFMpegFile setOutputFile(String filePath) throws FileNotFoundException {
    	File f = new File(filePath);
    	if(f.exists()) {
    		f.delete();
    	}
    	FFMpegAVFormatContext c = native_av_setOutputFile(filePath);
    	return new FFMpegFile(f, c);
    }
    
    private void newVideoStream(FFMpegAVFormatContext context) {
    	native_av_newVideoStream(context.pointer);
    }
	
	public void convert() throws RuntimeException {
		
		if(mListener != null) {
			mListener.onConversionStarted();
		}
		
		native_av_convert();
		
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
			throw new RuntimeException("You didn't call convertAsync method first");
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
    
    private native FFMpegAVFormatContext native_av_setInputFile(String filePath);
    
    private native FFMpegAVFormatContext native_av_setOutputFile(String filePath);
    
    private native int native_av_setBitrate(String opt, String arg);
    
    private native void native_av_newVideoStream(int pointer);
	
	private native void native_av_parse_options(String[] args) throws RuntimeException;
	
	private native void native_av_convert() throws RuntimeException;
	
	private native int native_av_release(int code);
	
}
