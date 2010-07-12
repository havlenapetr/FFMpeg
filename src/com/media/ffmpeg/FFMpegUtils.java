package com.media.ffmpeg;

import java.io.File;
import java.io.IOException;

import android.util.Log;

import com.media.ffmpeg.FFMpegAVFormatContext;

public class FFMpegUtils {
	
	protected FFMpegUtils() {};
	
	public void setOutput(String path) throws IOException {
		File f = new File(path);
		if(!f.exists()) {
			if(!f.mkdir()) {
				throw new IOException("Couldn't create directory: " + path);
			}
		}
		native_av_setOutput(path);
	}
	
	public FFMpegAVFormatContext setInputFile(String filePath) throws IOException {
		return native_av_setInputFile(filePath);
	}
	
	public void printToSdcard(FFMpegAVFormatContext context) throws IOException {
		native_av_print(context.pointer);
	}
	
	public void onVideoFrame(int[] pixels) {
		Log.d("FFMpegUtils", "pixels length: " + pixels.length);
	}
	
	private native FFMpegAVFormatContext native_av_setInputFile(String filePath) throws IOException;
	private native void native_av_setOutput(String path);
	private native void native_av_print(int pAVFormatContext) throws IOException;
	private native void native_av_release();

}
