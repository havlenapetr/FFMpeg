package cz.havlena.ffmpeg.ui;

import java.io.File;
import java.io.FileNotFoundException;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;

import android.media.MediaScannerConnection;
import android.media.ffmpeg.FFMpeg;
import android.media.ffmpeg.FFMpegAVFormatContext;
import android.media.ffmpeg.FFMpegConfig;
import android.media.ffmpeg.FFMpegMediaScannerNotifier;
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

public class FFMpegActivity extends Activity {
	
	private static final String TAG = "FFMpegActivity";

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
    	mFFMpegController.setListener(new FFMpegHandler(this));
    
        PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
        mWakeLock = pm.newWakeLock(PowerManager.SCREEN_DIM_WAKE_LOCK, TAG);
        mEditText.setText("/sdcard/Videos/walle1.mp4");
    }
    
    /**
     * Initialize all UI elements from resources.
     */
    private void initResourceRefs() {
    	//mOutput = (TextView) findViewById(R.id.textview_output);
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
				try {
					startConversion(mEditText.getText().toString(), bitrate);
				} catch (FileNotFoundException e) {
					showError(e.getMessage());
				} catch (RuntimeException e) {
					showError(e.getMessage());
				}
			}
		});
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
    
    private String getScreenResolution() {
    	Display display = ((WindowManager) getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay(); 
    	String res = display.getHeight() + "x" + display.getWidth();
    	Log.d(TAG, "Screen res: " + res);
    	return res;
    }
    
    private void startConversion(String filePath, String bitrate) throws FileNotFoundException, RuntimeException {
    	String resolution =  getScreenResolution();
    	String codec = FFMpegConfig.CODEC_MPEG4;
    	String ratio = FFMpegConfig.RATIO_3_2;
    	String inputFile = filePath;
    	int index = filePath.lastIndexOf(".");
    	String before = filePath.substring(0, index);
    	String outputFile = before + ".android.mp4";
    	
    	mFFMpegController.init(resolution, codec, bitrate, ratio, inputFile, outputFile);
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