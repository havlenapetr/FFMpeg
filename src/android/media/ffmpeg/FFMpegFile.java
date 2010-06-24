package android.media.ffmpeg;

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
	
}
