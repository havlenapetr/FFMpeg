package cz.havlena.ffmpeg.ui;

import com.media.ffmpeg.FFMpeg;
import com.media.ffmpeg.android.FFMpegPlayerAndroid;

import android.app.Activity;
import android.os.Bundle;

public class FFMpegPlayerActivity extends Activity {
	
	private FFMpegPlayerAndroid mVideoContainer;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		
		/*FFMpeg ffmpeg = FFMpeg.getInstance();
		if(ffmpeg != null) {
			mVideoContainer = ffmpeg.getPlayer(this);
			setContentView(mVideoContainer);
		
			runVideo();
		}*/
	}
	
	private void runVideo() {
		mVideoContainer.runAsync(new String[] {"ffplay", "/sdcard/Videos/pixar.flv"});
	}

}
