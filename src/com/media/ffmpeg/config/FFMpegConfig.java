package com.media.ffmpeg.config;

public class FFMpegConfig {
	
	public static final String 	CODEC_MPEG4 = "mpeg4";
	
	public static final String 	BITRATE_HIGH = "1024000";
	public static final String 	BITRATE_MEDIUM = "512000";
	public static final String 	BITRATE_LOW = "128000";
	
	public static final int[] 	RATIO_3_2 = new int[]{3,2};
	public static final int[] 	RATIO_4_3 = new int[]{4,3};
	
	public int[] 				resolution = new int[]{800, 600};
	public String 				codec = CODEC_MPEG4;
	public String 				bitrate = BITRATE_LOW;
	public int[]				ratio = RATIO_4_3;
	public int 					audioRate = 44000;
	public int 					frameRate = 24;
	public int 					audioChannels = 2;
	
}
