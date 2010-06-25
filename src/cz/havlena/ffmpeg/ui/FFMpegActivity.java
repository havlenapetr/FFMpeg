package cz.havlena.ffmpeg.ui;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;

import com.media.ffmpeg.FFMpeg;
import com.media.ffmpeg.FFMpegAVFormatContext;
import com.media.ffmpeg.FFMpegMediaScannerNotifier;
import com.media.ffmpeg.FFMpegReport;
import com.media.ffmpeg.FFMpeg.IFFMpegListener;
import com.media.ffmpeg.config.FFMpegConfig;
import com.media.ffmpeg.config.FFMpegConfigAndroid;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.RadioButton;

public class FFMpegActivity extends Activity {
	
	private static final String TAG = "FFMpegActivity";

	private EditText 	mEditText;
	private Button		mButton;
	private RadioButton mRadioButton1;
	private RadioButton mRadioButton2;
	private RadioButton mRadioButton3;
	
	private RadioButton  mRadioButtonAudio16;
	private RadioButton  mRadioButtonAudio32;
	
	private RadioButton  mRadioButtonAudioCH1;
	private RadioButton  mRadioButtonAudioCH2;
	
	private EditText 	mEditTextFrames;
	
	private CheckBox    mCheckBox;
	
	private FFMpeg 		mFFMpegController;
	private PowerManager.WakeLock mWakeLock = null;
	
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
    
        initResourceRefs();
        setListeners();
        
        mFFMpegController = new FFMpeg();
    	mFFMpegController.setListener(new FFMpegHandler(this));
    
        PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
        mWakeLock = pm.newWakeLock(PowerManager.SCREEN_DIM_WAKE_LOCK, TAG);
        mEditText.setText("/sdcard/Videos/pixar.flv");
    }
    
    /**
     * Initialize all UI elements from resources.
     */
    private void initResourceRefs() {
    	//mOutput = (TextView) findViewById(R.id.textview_output);
    	mEditText = (EditText) findViewById(R.id.edittext_inputfile);
    	mEditTextFrames = (EditText) findViewById(R.id.edittext_frames);
    	mButton = (Button) findViewById(R.id.button_convert);

    	mRadioButton1 = (RadioButton) findViewById(R.id.radiobutton_option1);
    	mRadioButton2 = (RadioButton) findViewById(R.id.radiobutton_option2);
    	mRadioButton3 = (RadioButton) findViewById(R.id.radiobutton_option3);
    	
    	mRadioButtonAudio16 = (RadioButton) findViewById(R.id.radiobutton_audio_option1);
    	mRadioButtonAudio32 = (RadioButton) findViewById(R.id.radiobutton_audio_option2);
    	
    	mRadioButtonAudioCH1 = (RadioButton) findViewById(R.id.radiobutton_audio_channel1);
    	mRadioButtonAudioCH2 = (RadioButton) findViewById(R.id.radiobutton_audio_channel2);
    	
    	mCheckBox = (CheckBox) findViewById(R.id.checkbox_delete_source);
    }
    
    private void setListeners() {
    	mButton.setOnClickListener(new OnClickListener() {
			
			public void onClick(View v) {
				FFMpegConfigAndroid config = parseConfig();
				try {
					startConversion(mEditText.getText().toString(), config);
				} catch (FileNotFoundException e) {
					showError(e.getMessage());
				} catch (RuntimeException e) {
					showError(e.getMessage());
				} catch (IOException e) {
					showError(e.getMessage());
				}
			}
		});
    }
    
    private FFMpegConfigAndroid parseConfig() {
    	FFMpegConfigAndroid config = new FFMpegConfigAndroid(FFMpegActivity.this);
		if(mRadioButton2.isChecked()) {
			config.bitrate = FFMpegConfigAndroid.BITRATE_MEDIUM;
		}
		else if(mRadioButton3.isChecked()) {
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
			showError(e.getMessage());
		}
		
		Log.d(TAG, "Audio ch: " + config.audioChannels);
		Log.d(TAG, "Audio rate: " + config.audioRate);
		Log.d(TAG, "Bit rate: " + config.bitrate);
		Log.d(TAG, "Frame rate: " + config.frameRate);
		
		return config;
    }
    
    private void showError(String msg) {
    	new AlertDialog.Builder(this)  
        .setMessage(msg)  
        .setTitle("Error")  
        .setCancelable(true)  
        .setNeutralButton(android.R.string.cancel,  
           new DialogInterface.OnClickListener() {  
           public void onClick(DialogInterface dialog, int whichButton){}  
           })  
        .show();
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
        	mFFMpegController.release();
        }
        super.onPause();
    }
    
    private void startConversion(String filePath, FFMpegConfig config) throws RuntimeException, IOException {
    	String inputFile = filePath;
    	int index = filePath.lastIndexOf(".");
    	String before = filePath.substring(0, index);
    	String outputFile = before + ".android.mp4";
    	
    	mFFMpegController.init(inputFile, outputFile);
    	mFFMpegController.setConfig(config);
    	mFFMpegController.convertAsync();
    }
    
    private class FFMpegHandler implements IFFMpegListener {
    	
    	private static final int CONVERSION_ERROR = -1;
    	private static final int CONVERSION_PROGRESS = 1;
    	private static final int CONVERSION_STARTED = 3;
    	private static final int CONVERSION_ENDED = 4;
    	
    	private ProgressDialog 	mDialog;
    	private Context 		mContext;
    	
    	public FFMpegHandler(Context context) {
    		mContext = context;
    		mDialog = new ProgressDialog(context);
    		mDialog.setMax(100);
    		mDialog.setMessage("Converting video to mp4 ...");
    		mDialog.setProgress(0);
    		mDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
    		mDialog.setCancelable(false);
		}
    	
    	private Handler mHanlder = new Handler() {
    		private int mDuration;
    		
    		@Override
    		public void handleMessage(Message msg) {
    			switch(msg.what) {
    			
    			case CONVERSION_STARTED:
    				mDialog.show();
    				FFMpegAVFormatContext context = mFFMpegController.getInputFile().getContext();
    				mDuration = context.getDurationInSeconds();
    				break;
    				
    			case CONVERSION_ENDED:
    				mDialog.dismiss();
				    File outFile = mFFMpegController.getOutputFile().getFile();
    				FFMpegMediaScannerNotifier.scan(mContext, outFile.getAbsolutePath());
    				mDuration = 0;
    				if(mCheckBox.isChecked()) {
    					mFFMpegController.getInputFile().delete();
    				}
    				break;
    			
    			case CONVERSION_PROGRESS:
    				FFMpegReport report = (FFMpegReport) msg.obj;
    				int res = (int) ((report.time * 100) / mDuration);
    				mDialog.setProgress(res);
    				break;
    				
    			case CONVERSION_ERROR:
    				Exception e = (Exception) msg.obj;
    				mDialog.dismiss();
    				showError(e.getMessage());
    				mDuration = 0;
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