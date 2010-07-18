package com.media.ffmpeg;

public class FFMpegAVFormatContext {
	
	public static final long AV_TIME_BASE = 1000000;
	
	protected int  						pointer;
	private int 						nb_streams;
	private String 						filename;
	private long 						timestamp;
	private String 						title;
	private String 						author;
	private String 						copyright;
	private String 						comment;
	private String 						album;
	private int 						year; 
	private int 						track; 
	private String  					genre; 
	private int 						ctx_flags;
	private long  						start_time; 
	private long 						duration; 
	private long 						file_size; 
	private int 						bit_rate;
	private int 						mux_rate;
	private int 						packet_size;
	private int 						preload;
	private int 						max_delay;
	private int 						loop_output; 
	private int 						flags;
	private int 						loop_input;
	private FFMpegAVInputFormat 		mInFormat;
	private FFMpegAVOutputFormat		mOutFormat;
	
	public int getNbStreams() {
		return nb_streams;
	}

	public String getFilename() {
		return filename;
	}

	public long getTimestamp() {
		return timestamp;
	}

	public String getTitle() {
		return title;
	}

	public String getAuthor() {
		return author;
	}

	public String getCopyright() {
		return copyright;
	}

	public String getComment() {
		return comment;
	}

	public String getAlbum() {
		return album;
	}

	public int getYear() {
		return year;
	}

	public int getTrack() {
		return track;
	}

	public String getGenre() {
		return genre;
	}

	public int getCtxFlags() {
		return ctx_flags;
	}

	public long getStartTime() {
		return start_time;
	}


	public long getFileSize() {
		return file_size;
	}

	public int getBitrate() {
		return bit_rate;
	}

	public int getMuxrate() {
		return mux_rate;
	}

	public int getPacketSize() {
		return packet_size;
	}

	public int getPreload() {
		return preload;
	}

	public int getMaxDelay() {
		return max_delay;
	}

	public int getLoopOutput() {
		return loop_output;
	}

	public int getFlags() {
		return flags;
	}

	public int getLoop_input() {
		return loop_input;
	}
	
	private FFMpegAVFormatContext(){}
	
	public void release() {
		nativeRelease(pointer);
	}
	
	public Duration getDuration() {
		Duration d = new Duration();
		d.secs = (int) (duration / FFMpegAVFormatContext.AV_TIME_BASE);
		d.mins = d.secs / 60;
		d.secs %= 60;
		d.hours = d.mins / 60;
		d.mins %= 60;
		return d;
	}
	
	public int getDurationInSeconds() {
		return (int) (duration / FFMpegAVFormatContext.AV_TIME_BASE);
	}
	
	public int getDurationInMiliseconds() {
		return (int) duration / 1000;
	}
	
	private native void nativeRelease(int pointer);
	
	public class Duration {
		public int hours;
		public int mins;
		public int secs;
		
		private Duration(){}
	}
	
}
