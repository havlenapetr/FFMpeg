package com.media.ffmpeg;

public class FFMpegAVOption {

	protected long 	pointer;
	private String 	mName;
	private String  mHelp; //short English help text
	private int 	mOffset; //The offset relative to the context structure where the option value is stored.
	private int 	mType; // AVOptionType
	private double 	mDefaultVal; // the default value for scalar options
	private double 	mMin; // minimum valid value for the option
	private double 	mMax; // maximum valid value for the option 
	private int 	mFlags;
	private String  mUnit; // The logical unit to which the option belongs.
	
	public String getName() {
		return mName;
	}
	public String getHelp() {
		return mHelp;
	}
	public int getOffset() {
		return mOffset;
	}
	public int getType() {
		return mType;
	}
	public double getDefaultVal() {
		return mDefaultVal;
	}
	public double getMin() {
		return mMin;
	}
	public double getMax() {
		return mMax;
	}
	public int getFlags() {
		return mFlags;
	}
	public String getUnit() {
		return mUnit;
	}
	
}
