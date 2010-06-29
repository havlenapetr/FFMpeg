package com.media.ffmpeg;

import java.io.File;

public class FFMpegFile {
	
	protected File 						mFile;
	protected FFMpegAVFormatContext		mContext;
	
	protected FFMpegFile(File file, FFMpegAVFormatContext context) {
		mFile = file;
		mContext = context;
	}
	
	public File getFile() {
		return mFile;
	}
	
	public FFMpegAVFormatContext getContext() {
		return mContext;
	}
	
	public boolean exists() {
		return mFile.exists();
	}
	
	public void delete() {
		mFile.delete();
		mFile = null;
		//mContext.release();
		//mContext = null;
	}
	
}
