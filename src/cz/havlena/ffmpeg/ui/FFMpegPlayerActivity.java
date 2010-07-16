package cz.havlena.ffmpeg.ui;

import java.io.IOException;

import com.media.ffmpeg.FFMpeg;
import com.media.ffmpeg.FFMpegException;
import com.media.ffmpeg.IFFMpegPlayer;
import com.media.ffmpeg.android.FFMpegPlayerAndroid;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.util.Log;

public class FFMpegPlayerActivity extends Activity {
	private static final String 	TAG = "FFMpegPlayerActivity";
	
	private FFMpegPlayerAndroid 	mPlayer;
	private WakeLock				mWakeLock;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		Intent i = getIntent();
		String filePath = i.getStringExtra(getResources().getString(R.string.input_file));
		if(filePath == null) {
			Log.d(TAG, "Not specified video file");
			finish();
		} else {
			PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
		    mWakeLock = pm.newWakeLock(PowerManager.SCREEN_DIM_WAKE_LOCK, TAG);

			try {
				FFMpeg ffmpeg = new FFMpeg();
				mPlayer = ffmpeg.getPlayer(this);
				try {
					mPlayer.setVideoPath(filePath);
					mPlayer.setListener(new FFMpegPlayerHandler());
				} catch (IOException e) {
					FFMpegMessageBox.show(this, e);
				}
				setContentView(mPlayer);
			} catch (FFMpegException e) {
				Log.d(TAG, "Error when inicializing ffmpeg: " + e.getMessage());
				FFMpegMessageBox.show(this, e);
				finish();
			}
		}
	}
	
	@Override
    protected void onResume() {
        //-- we will disable screen timeout, while scumm is running
        if( mWakeLock != null ) {
        	Log.d(TAG, "Resuming so acquiring wakeLock");
        	mWakeLock.acquire();
        }
        super.onResume();
    }
    
    @Override
    protected void onPause() {
        //-- we will enable screen timeout, while scumm is paused
        if(mWakeLock != null ) {
        	Log.d(TAG, "Pausing so releasing wakeLock");
        	mWakeLock.release();
        }
        super.onPause();
    }
    
    private void startFileExplorer() {
    	Intent i = new Intent(this, FFMpegFileExplorer.class);
    	startActivity(i);
    }

	private class FFMpegPlayerHandler implements IFFMpegPlayer {

		public void onError(String msg, Exception e) {
			Log.e(TAG, "ERROR: " + e.getMessage());
			startFileExplorer();
		}

		public void onPlay() {
			Log.d(TAG, "starts playing");
		}

		public void onRelease() {
			Log.d(TAG, "released");
			//startFileExplorer();
		}

		public void onStop() {
			Log.d(TAG, "stopped");
			FFMpegPlayerActivity.this.finish();
		}
		
	}
}
