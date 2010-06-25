package com.media.ffmpeg;

import android.content.Context;
import android.media.MediaScannerConnection;
import android.media.MediaScannerConnection.MediaScannerConnectionClient;
import android.net.Uri;

public class FFMpegMediaScannerNotifier implements MediaScannerConnectionClient {
    private MediaScannerConnection mConnection;
    private String mPath;

    private FFMpegMediaScannerNotifier(Context context, String path) {
        mPath = path;
        mConnection = new MediaScannerConnection(context, this);
        mConnection.connect();
    }
    
    public static void scan(Context context, String path) {
    	new FFMpegMediaScannerNotifier(context, path);
    }

    public void onMediaScannerConnected() {
        mConnection.scanFile(mPath, null);
    }

    public void onScanCompleted(String path, Uri uri) {
        mConnection.disconnect();
    }
}
