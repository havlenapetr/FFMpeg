package com.media.ffmpeg;

public class FFMpegAVCodecContext {
	
	protected FFMpegAVCodecContext 		mPointer;
	private FFMpegAVClass 				mAVClass;
	private int 						mBitRate;
	private int 						mBitRateTolerance; 
	private int 						mFlags;
 	
	private int 						mSubId; 
	private int 						mMeMethod; 				// Motion estimation algorithm used for video coding. 
	private short 						mExtraData; 			// some codecs need / can use extradata like Huffman tables. 
	private int 						mExtradataSize;

}
