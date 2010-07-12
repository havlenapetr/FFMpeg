package cz.havlena.ffmpeg.ui;

import java.io.IOException;

import com.media.ffmpeg.FFMpeg;
import com.media.ffmpeg.android.FFMpegPlayerAndroid;

import android.app.Activity;
import android.os.Bundle;

public class FFMpegPlayerActivity extends Activity {
	
	private FFMpegPlayerAndroid mPlayer;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		
		FFMpeg ffmpeg = new FFMpeg();
		mPlayer = ffmpeg.getPlayer(this);
		try {
			mPlayer.init("/sdcard/Videos/pixar.flv");
		} catch (IOException e) {
			FFMpegMessageBox.show(this, e);
		}
		setContentView(mPlayer);
	}
}
