package cz.havlena.ffmpeg.ui;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;

import com.media.ffmpeg.FFMpeg;
import com.media.ffmpeg.FFMpegAVFormatContext;
import com.media.ffmpeg.FFMpegFile;
import com.media.ffmpeg.FFMpegMediaScannerNotifier;
import com.media.ffmpeg.FFMpegReport;
import com.media.ffmpeg.FFMpeg.IFFMpegListener;
import com.media.ffmpeg.android.FFMpegConfigAndroid;

import cz.havlena.android.ui.MessageBox;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.Intent;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.RadioButton;
import android.widget.TextView;

/**
 * Main android activity which handles all ffmpeg operations
 * 
 * @author petr
 *
 */
public class FFMpegActivity extends Activity {
	
	private static final String TAG = "FFMpegActivity";
	
	private static final int		FILE_SELECT = 0;
	public static final String		FILE_INPUT = "FFMpeg_file";

	private TextView 				mTextViewInputVideo;
	private TextView				mTextViewInputVideoLength;
	private ImageButton				mSelectButton;
	private Button					mButton;
	private RadioButton 			mRadioButtonVideo128;
	private RadioButton 			mRadioButtonVideo512;
	private RadioButton 			mRadioButtonVideo1024;
	
	private RadioButton  			mRadioButtonAudio16;
	private RadioButton  			mRadioButtonAudio32;
	
	private RadioButton  			mRadioButtonAudioCH1;
	private RadioButton  			mRadioButtonAudioCH2;
	
	private EditText 				mEditTextFrames;
	
	private CheckBox    			mCheckBox;
	
	private FFMpeg					mFFMpegController;
	private PowerManager.WakeLock 	mWakeLock = null;
	
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.ffmpeg_main);
    
	    initResourceRefs();
	    setListeners();
	    
	    PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
	    mWakeLock = pm.newWakeLock(PowerManager.SCREEN_DIM_WAKE_LOCK, TAG);
	
	    Intent i = getIntent();
	    if(!i.getAction().equals(Intent.ACTION_INPUT_METHOD_CHANGED)) {
	    	startFileExplorer();
	    } else {
	    	String filePath = i.getStringExtra(FILE_INPUT);
	    	mTextViewInputVideo.setText(filePath);
	    	try {
	    		initFFMpeg(filePath);
	    		FFMpegFile input = mFFMpegController.getInputFile();
	    		FFMpegAVFormatContext.Duration duration = input.getContext().getDuration();
	    		mTextViewInputVideoLength.setText(getString(R.string.input_file_info) + " " + 
	    				duration.hours + "h " + duration.mins + "min " + duration.secs + "sec");
	    	}
	    	catch (Exception e) {
	    		showError(this, e);
			}
	    }
	}
    
    private void initFFMpeg(String filePath) throws RuntimeException, IOException {
    	String inputFile = filePath;
    	int index = filePath.lastIndexOf(".");
    	String before = filePath.substring(0, index);
    	String outputFile = before + ".android.mp4";
    	
    	mFFMpegController = new FFMpeg();
  		mFFMpegController.setListener(new FFMpegHandler(this));
    	mFFMpegController.init(inputFile, outputFile);
    }
    
    private void startFileExplorer() {
    	Intent i = new Intent(FFMpegActivity.this, FFMpegFileExplorer.class);
    	startActivityForResult(i, FILE_SELECT);
    }
        
    /**
     * Initialize all UI elements from resources.
     */
    private void initResourceRefs() {
    	mTextViewInputVideo = (TextView) findViewById(R.id.textview_inputfile);
    	mTextViewInputVideoLength = (TextView) findViewById(R.id.textview_inputfile_length);
    	mSelectButton = (ImageButton) findViewById(R.id.button_selectfile);
    	
    	mEditTextFrames = (EditText) findViewById(R.id.edittext_frames);
    	mButton = (Button) findViewById(R.id.button_convert);

    	mRadioButtonVideo128 = (RadioButton) findViewById(R.id.radiobutton_video_option1);
    	mRadioButtonVideo512 = (RadioButton) findViewById(R.id.radiobutton_video_option2);
    	mRadioButtonVideo1024 = (RadioButton) findViewById(R.id.radiobutton_video_option3);
    	
    	mRadioButtonAudio16 = (RadioButton) findViewById(R.id.radiobutton_audio_option1);
    	mRadioButtonAudio32 = (RadioButton) findViewById(R.id.radiobutton_audio_option2);
    	
    	mRadioButtonAudioCH1 = (RadioButton) findViewById(R.id.radiobutton_audio_channel1);
    	mRadioButtonAudioCH2 = (RadioButton) findViewById(R.id.radiobutton_audio_channel2);
    	
    	mCheckBox = (CheckBox) findViewById(R.id.checkbox_delete_source);
    }
    
    private void setListeners() {
    	mSelectButton.setOnClickListener(new OnClickListener() {
			
			public void onClick(View v) {
				startFileExplorer();
			}
		});
    	
    	mButton.setOnClickListener(new OnClickListener() {
			
			public void onClick(View v) {
				FFMpegConfigAndroid config = parseConfig();
				if(config == null) return;
				try {
					startConversion(config);
				} catch (FileNotFoundException e) {
					showError(FFMpegActivity.this, e);
				} catch (RuntimeException e) {
					showError(FFMpegActivity.this, e);
				} catch (IOException e) {
					showError(FFMpegActivity.this, e);
				}
			}
		});
    }
    
    private FFMpegConfigAndroid parseConfig() {
    	FFMpegConfigAndroid config = new FFMpegConfigAndroid(FFMpegActivity.this);
		if(mRadioButtonVideo512.isChecked()) {
			config.bitrate = FFMpegConfigAndroid.BITRATE_MEDIUM;
		}
		else if(mRadioButtonVideo1024.isChecked()) {
			config.bitrate = FFMpegConfigAndroid.BITRATE_HIGH;
		}
		
		if(mRadioButtonAudio16.isChecked()) {
			config.audioRate = 16000;
		}
		else if(mRadioButtonAudio32.isChecked()) {
			config.audioRate = 32000;
		}
		
		if(mRadioButtonAudioCH1.isChecked()) {
			config.audioChannels = 1;
		}
		else if(mRadioButtonAudioCH2.isChecked()) {
			config.audioChannels = 2;
		}
		
		try
		{
			int frames = Integer.valueOf(mEditTextFrames.getText().toString());
			if(frames > 0 && frames < 30)
				config.frameRate = frames;
		}
		catch (NumberFormatException e) {
			showError(this, e);
			return null;
		}
		
		Log.d(TAG, "Audio ch: " + config.audioChannels);
		Log.d(TAG, "Audio rate: " + config.audioRate);
		Log.d(TAG, "Bit rate: " + config.bitrate);
		Log.d(TAG, "Frame rate: " + config.frameRate);
		
		return config;
    }
    
    protected void showError(Context context, Exception ex) {
    	MessageBox.show(context, ex);
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
        if(mFFMpegController != null) {
        	Log.d(TAG, "Releasing ffmpeg");
			if(mFFMpegController.isConverting()) {
				Log.d(TAG, "Deleting outputfile because conversion wasn't completed");
				mFFMpegController.getOutputFile().delete();
			}
        	mFFMpegController.release();
        }
        finish();
        super.onPause();
    }
    
    private void startConversion(FFMpegConfigAndroid config) throws RuntimeException, IOException {
    	mFFMpegController.setConfig(config);
    	mFFMpegController.convertAsync();
    }
    
    /**
     * Listener for handling ffmpeg events a process them on android gui
     * 
     * @author petr
     *
     */
    private class FFMpegHandler implements IFFMpegListener {
    	
    	private static final int CONVERSION_ERROR = -1;
    	private static final int CONVERSION_PROGRESS = 1;
    	private static final int CONVERSION_STARTED = 3;
    	private static final int CONVERSION_ENDED = 4;
    	private static final int CHANGE_BUTTON_IMAGE = 5;
    	
    	private ProgressDialog 	mDialog;
    	private Context 		mContext;
    	private Thread			mDrawerThread;
    	private boolean 		mAnimating;
    	
    	public FFMpegHandler(Context context) {
    		mContext = context;
    		mAnimating = false;
    		mDialog = new ProgressDialog(context);
    		mDialog.setMax(100);
    		mDialog.setMessage("Converting video to mp4 ...");
    		mDialog.setProgress(0);
    		mDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
    		mDialog.setCancelable(false);
		}
    	
    	private void runAnimation() {
    		mAnimating = true;
    		Log.d(TAG, "Starting animation");
    		mDrawerThread = new Thread() {
    			@Override
    			public void run() {
    	    		int imageIndex = 0;
    				while(mAnimating) {
    					int id = 0;
    					switch(imageIndex) {
    					case 0:
    						id = R.drawable.ic_popup_sync_1;
    						break;
    					case 1:
    						id = R.drawable.ic_popup_sync_2;
    						break;
    					case 2:
    						id = R.drawable.ic_popup_sync_3;
    						break;
    					case 3:
    						id = R.drawable.ic_popup_sync_4;
    						break;
    					case 4:
    						id = R.drawable.ic_popup_sync_5;
    						break;
    					case 5:
    						id = R.drawable.ic_popup_sync_6;
    						break;
    					}
    					Message msg = mHanlder.obtainMessage(CHANGE_BUTTON_IMAGE, id, 0);
    					mHanlder.sendMessage(msg);
    					
    					imageIndex++;
						if(imageIndex > 5) {
							imageIndex = 0;
						}
						
    					try {
							sleep(200);
						} catch (InterruptedException e) {}
    				}
    				
    				Message msg = mHanlder.obtainMessage(CHANGE_BUTTON_IMAGE, R.drawable.ic_menu_movie, 0);
    				mHanlder.sendMessage(msg);
    			}
    		};
    		mDrawerThread.start();
    	}
    	
    	private void stopAnimation() {
    		mAnimating = false;
    		Log.d(TAG, "Stopping animation");
    		try {
				mDrawerThread.join();
				mDrawerThread = null;
			} catch (InterruptedException e) {
				Log.d(TAG, "Can't wait no more for mDrawerThread");
			}
    	}
    	
    	private Handler mHanlder = new Handler() {
    		private int mDuration;
    		
    		@Override
    		public void handleMessage(Message msg) {
    			switch(msg.what) {
    			
    			case CONVERSION_STARTED:
    				Log.d(TAG, "Conversion started");
    				mDialog.show();
    				FFMpegAVFormatContext context = mFFMpegController.getInputFile().getContext();
    				mDuration = context.getDurationInSeconds();
    				setTitle(getString(R.string.processing));
        			setProgressBarIndeterminateVisibility(true);
        			runAnimation();
    				break;
    				
    			case CONVERSION_ENDED:
    				Log.d(TAG, "Conversion ended");
    				mDialog.dismiss();
				    File outFile = mFFMpegController.getOutputFile().getFile();
    				FFMpegMediaScannerNotifier.scan(mContext, outFile.getAbsolutePath());
    				mDuration = 0;
    				if(mCheckBox.isChecked()) {
    					mFFMpegController.getInputFile().delete();
    				}
    				setTitle(getString(R.string.app_name));
        			setProgressBarIndeterminateVisibility(false);
        			stopAnimation();
    				break;
    			
    			case CONVERSION_PROGRESS:
    				FFMpegReport report = (FFMpegReport) msg.obj;
    				int res = (int) ((report.time * 100) / mDuration);
    				mDialog.setProgress(res);
    				break;
    				
    			case CONVERSION_ERROR:
    				Exception e = (Exception) msg.obj;
    				mDialog.dismiss();
    				setTitle(getString(R.string.app_name));
    				setProgressBarIndeterminateVisibility(false);
    				showError(FFMpegActivity.this, e);
    				mDuration = 0;
    				stopAnimation();
    				break;
    				
    			case CHANGE_BUTTON_IMAGE:
    				mSelectButton.setImageResource(msg.arg1);
    				break;
    			}
    		}
    	};

		public void onConversionCompleted() {
			Message m = mHanlder.obtainMessage(CONVERSION_ENDED);
			mHanlder.sendMessage(m);
		}

		public void onConversionStarted() {
			Message m = mHanlder.obtainMessage(CONVERSION_STARTED);
			mHanlder.sendMessage(m);
		}

		public void onConversionProcessing(FFMpegReport report) {
			Message m = mHanlder.obtainMessage(CONVERSION_PROGRESS);
			m.obj = report;
			mHanlder.sendMessage(m);
		}

		public void onError(Exception e) {
			Message m = mHanlder.obtainMessage(CONVERSION_ERROR);
			m.obj = e.getMessage();
			mHanlder.sendMessage(m);
		}
    	
    }
}