package cz.havlena.ffmpeg.ui;

import android.app.Activity;
import android.content.Context;

import android.media.ffmpeg.FFMpeg;
import android.media.ffmpeg.FFMpegConfig;
import android.media.ffmpeg.FFMpegReport;
import android.media.ffmpeg.FFMpeg.IFFMpegListener;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.util.Log;
import android.view.Display;
import android.view.View;
import android.view.WindowManager;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TextView;

public class FFMpegActivity extends Activity {
	
	private static final String TAG = "FFMpegActivity";
	
	private TextView 	mOutput;
	private EditText 	mEditText;
	private Button		mButton;
	private RadioButton mRadioButton1;
	private RadioButton mRadioButton2;
	private RadioButton mRadioButton3;
	private RadioGroup  mRadioButtons;
	
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
    	mFFMpegController.setListener(new FFMpegHandler());
    
        PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
        mWakeLock = pm.newWakeLock(PowerManager.SCREEN_DIM_WAKE_LOCK, TAG);
        mEditText.setText("/sdcard/Videos/walle1.mp4");
    }
    
    /**
     * Initialize all UI elements from resources.
     */
    private void initResourceRefs() {
    	mOutput = (TextView) findViewById(R.id.textview_output);
    	mEditText = (EditText) findViewById(R.id.edittext_inputfile);
    	mButton = (Button) findViewById(R.id.button_convert);
    	
    	mRadioButtons = (RadioGroup) findViewById(R.id.radiogroup_radiobuttons);
    	mRadioButton1 = (RadioButton) findViewById(R.id.radiobutton_option1);
    	mRadioButton2 = (RadioButton) findViewById(R.id.radiobutton_option2);
    	mRadioButton3 = (RadioButton) findViewById(R.id.radiobutton_option3); 
    }
    
    private void setListeners() {
    	mButton.setOnClickListener(new OnClickListener() {
			
			public void onClick(View v) {
				String bitrate = FFMpegConfig.BITRATE_LOW;
				if(mRadioButton2.isChecked()) {
					bitrate = FFMpegConfig.BITRATE_MEDIUM;
				}
				else if(mRadioButton3.isChecked()) {
					bitrate = FFMpegConfig.BITRATE_HIGH;
				}
				startConversion(mEditText.getText().toString(), bitrate);
			}
		});
    }
    
    private void hideControls() {
    	mRadioButtons.setVisibility(View.GONE);
    	mButton.setVisibility(View.GONE);
    	mEditText.setVisibility(View.GONE);
    }
    
    private void showControls() {
    	mRadioButtons.setVisibility(View.VISIBLE);
    	mButton.setVisibility(View.VISIBLE);
    	mEditText.setVisibility(View.VISIBLE);
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
    
    private String getScreenResolution() {
    	Display display = ((WindowManager) getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay(); 
    	String res = display.getHeight() + "x" + display.getWidth();
    	Log.d(TAG, "Screen res: " + res);
    	return res;
    }
    
    private void startConversion(String filePath, String bitrate) {
    	String resolution =  getScreenResolution();
    	String codec = FFMpegConfig.CODEC_MPEG4;
    	String ratio = FFMpegConfig.RATIO_3_2;
    	
    	mFFMpegController.init(resolution, codec, bitrate, ratio);

    	String inputFile = filePath;
    	int index = filePath.lastIndexOf(".");
    	String before = filePath.substring(0, index);
    	String outputFile = before + ".android.mp4";
    	
    	mFFMpegController.convertAsync(inputFile, outputFile);
    }
    
    private class FFMpegHandler implements IFFMpegListener {
    	
    	private static final int MSG_REPORT = 1;
    	private static final int MSG_STRING = 2;
    	private static final int MSG_SHOW_CONTROLS = 3;
    	private static final int MSG_HIDE_CONTROLS = 4;
    	
    	private Handler mHanlder = new Handler() {
    		@Override
    		public void handleMessage(Message msg) {
    			switch(msg.what) {
    			
    			case MSG_STRING:
    				if(msg.arg1 == MSG_SHOW_CONTROLS) {
    					showControls();
    				}
    				if(msg.arg1 == MSG_HIDE_CONTROLS) {
    					hideControls();
    				}
	    			String m = (String) msg.obj;
	    			mOutput.setText(mOutput.getText() + m + "\n");
	    			break;
    			
    			case MSG_REPORT:
    				FFMpegReport report = (FFMpegReport) msg.obj;
    				mOutput.setText(mOutput.getText() + 
    						"bitrate: " + Math.round(report.bitrate) + 
    						", time: " + Math.round(report.time) + 
    						", total size: " + Math.round(report.total_size) + 
    						"\n");
    				break;
    			}
    		}
    	};

		public void onConversionCompleted() {
			Message m = mHanlder.obtainMessage(MSG_STRING);
			m.arg1 = MSG_SHOW_CONTROLS;
			m.obj = "Conversion completed";
			mHanlder.sendMessage(m);
		}

		public void onConversionStarted() {
			Message m = mHanlder.obtainMessage(MSG_STRING);
			m.arg1 = MSG_HIDE_CONTROLS;
			m.obj = "Conversion started";
			mHanlder.sendMessage(m);
		}

		public void onConversionProcessing(FFMpegReport report) {
			Message m = mHanlder.obtainMessage(MSG_REPORT);
			m.obj = report;
			mHanlder.sendMessage(m);
		}

		public void onError(Exception e) {
			Message m = mHanlder.obtainMessage(MSG_STRING);
			m.obj = e.getMessage();
			m.arg1 = MSG_SHOW_CONTROLS;
			mHanlder.sendMessage(m);
		}
    	
    }
}