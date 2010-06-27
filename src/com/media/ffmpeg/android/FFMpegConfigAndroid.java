package com.media.ffmpeg.android;

import com.media.ffmpeg.config.FFMpegConfig;

import android.content.Context;
import android.view.Display;
import android.view.WindowManager;

public class FFMpegConfigAndroid extends FFMpegConfig {
	
	public FFMpegConfigAndroid(Context context) {
		overrideParametres(context);
	}
	
	private void overrideParametres(Context context) {
		resolution = getScreenResolution(context);
		ratio = RATIO_3_2;
		audioRate = 16000;
		frameRate = 13;
	}
	
	private int[] getScreenResolution(Context context) {
    	Display display = ((WindowManager) context.getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay(); 
    	int[] res = new int[] {display.getHeight(), display.getWidth()};
    	return res;
    }

}
