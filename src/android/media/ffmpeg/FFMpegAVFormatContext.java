package android.media.ffmpeg;

public class FFMpegAVFormatContext {
	
	public static final long AV_TIME_BASE = 1000000;
	
	protected long  pointer;
	public int 		nb_streams;
	public String 	filename;
	public long 	timestamp;
	public String 	title;
	public String 	author;
	public String 	copyright;
	public String 	comment;
	public String 	album;
	public int 		year; 
	public int 		track; 
	public String   genre; 
	public int 		ctx_flags;
	public long  	start_time; 
	public long 	duration; 
	public long 	file_size; 
	public int 		bit_rate;
	public int 		mux_rate;
	public int 		packet_size;
	public int 		preload;
	public int 		max_delay;
	public int 		loop_output; 
	public int 		flags;
	public int 		loop_input;
	
	private FFMpegAVFormatContext(){}
	
	
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
	
	public class Duration {
		public int hours;
		public int mins;
		public int secs;
		
		private Duration(){}
	}
	
}
