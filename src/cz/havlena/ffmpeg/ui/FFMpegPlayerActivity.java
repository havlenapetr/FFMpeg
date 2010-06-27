package cz.havlena.ffmpeg.ui;

import com.media.ffmpeg.FFMpegPlayerAndroid;

import android.app.Activity;
import android.os.Bundle;

public class FFMpegPlayerActivity extends Activity {
	
	private FFMpegPlayerAndroid mVideoContainer;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);
		
		mVideoContainer = new FFMpegPlayerAndroid();
		runVideo();
	}
	
	private void runVideo() {
		mVideoContainer.run(new String[] {"ffplay", "/sdcard/Videos/pixar.flv"});
	}

}
