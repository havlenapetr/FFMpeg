package cz.havlena.ffmpeg.ui;

import java.io.IOException;

import com.media.ffmpeg.FFMpeg;
import com.media.ffmpeg.FFMpegException;
import com.media.ffmpeg.android.FFMpegMovieViewAndroid;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;

public class FFMpegPlayerActivity extends Activity {
	private static final String 	TAG = "FFMpegPlayerActivity";
	//private static final String 	LICENSE = "This software uses libraries from the FFmpeg project under the LGPLv2.1";
	
	private FFMpegMovieViewAndroid 	mMovieView;
	private boolean 				mAutoscale;
	//private WakeLock				mWakeLock;
	
	private static final int MENU_AUTOSCALE = 1;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		Intent i = getIntent();
		String filePath = i.getStringExtra(getResources().getString(R.string.input_file));
		if(filePath == null) {
			Log.d(TAG, "Not specified video file");
			finish();
		} else {
			//PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
		    //mWakeLock = pm.newWakeLock(PowerManager.SCREEN_DIM_WAKE_LOCK, TAG);

			try {
				FFMpeg ffmpeg = new FFMpeg();
				mMovieView = ffmpeg.getMovieView(this);
				try {
					mMovieView.setVideoPath(filePath);
				} catch (IllegalArgumentException e) {
					Log.e(TAG, "Can't set video: " + e.getMessage());
					FFMpegMessageBox.show(this, e);
				} catch (IllegalStateException e) {
					Log.e(TAG, "Can't set video: " + e.getMessage());
					FFMpegMessageBox.show(this, e);
				} catch (IOException e) {
					Log.e(TAG, "Can't set video: " + e.getMessage());
					FFMpegMessageBox.show(this, e);
				}
				setContentView(mMovieView);
			} catch (FFMpegException e) {
				Log.d(TAG, "Error when inicializing ffmpeg: " + e.getMessage());
				FFMpegMessageBox.show(this, e);
				finish();
			}
		}
		
		mAutoscale = true;
	}
	
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		menu.add(0, MENU_AUTOSCALE, 1, "Autoscale");
		return true;
	}
	
	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		switch(item.getItemId())
		{
		case MENU_AUTOSCALE:
			mAutoscale = !mAutoscale;
			mMovieView.setAutoscale(mAutoscale);
			return true;
		}
		return false;
	}
}
