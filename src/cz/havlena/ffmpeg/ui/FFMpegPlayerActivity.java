package cz.havlena.ffmpeg.ui;

import java.io.IOException;

import com.media.ffmpeg.FFMpeg;
import com.media.ffmpeg.IFFMpegPlayer;
import com.media.ffmpeg.android.FFMpegPlayerAndroid;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;

public class FFMpegPlayerActivity extends Activity {
	private static final String TAG = "FFMpegPlayerActivity";
	
	private FFMpegPlayerAndroid mPlayer;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		
		FFMpeg ffmpeg = new FFMpeg();
		mPlayer = ffmpeg.getPlayer(this);
		try {
			mPlayer.setVideoPath("/sdcard/Videos/pixar.flv");
			mPlayer.setListener(new FFMpegPlayerHandler());
		} catch (IOException e) {
			FFMpegMessageBox.show(this, e);
		}
		setContentView(mPlayer);
	}

	private class FFMpegPlayerHandler implements IFFMpegPlayer {

		public void onError(String msg, Exception e) {
			Log.e(TAG, "ERROR: " + e.getMessage());
		}

		public void onPlay() {
			Log.d(TAG, "starts playing");
		}

		public void onRelease() {
			Log.d(TAG, "released");
		}

		public void onStop() {
			FFMpegPlayerActivity.this.finish();
		}
		
	}
}
