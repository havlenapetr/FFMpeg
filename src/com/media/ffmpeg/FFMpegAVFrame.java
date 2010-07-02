package com.media.ffmpeg;

public class FFMpegAVFrame {
	
	protected int mPointer;
	/**\
     * pointer to the picture planes.\
     * This might be different from the first allocated byte\
     * - encoding: \
     * - decoding: \
     */
    byte[]		mData;
    int[]		mLinesize;
	
    /**\
     * pointer to the first allocated byte of the picture. Can be used in get_buffer/release_buffer.\
     * This isn't used by libavcodec unless the default get/release_buffer() is used.\
     * - encoding: \
     * - decoding: \
     */
    byte[]		mBase;
	
    /**\
     * 1 -> keyframe, 0-> not\
     * - encoding: Set by libavcodec.\
     * - decoding: Set by libavcodec.\
     */
    int			mKeyFrame;
	
    /**\
     * Picture type of the frame, see ?_TYPE below.\
     * - encoding: Set by libavcodec. for coded_picture (and set by user for input).\
     * - decoding: Set by libavcodec.\
     */
    int			mPictType;
	
    /**\
     * presentation timestamp in time_base units (time when frame should be shown to user)\
     * If AV_NOPTS_VALUE then frame_rate = 1/time_base will be assumed.\
     * - encoding: MUST be set by user.\
     * - decoding: Set by libavcodec.\
     */
    long		mPts;
	
    /**\
     * picture number in bitstream order\
     * - encoding: set by\
     * - decoding: Set by libavcodec.\
     */
    int			mCodedPictureNumber;
	
    /**\
     * picture number in display order\
     * - encoding: set by\
     * - decoding: Set by libavcodec.\
     */
    int			mDisplayPictureNumber;
	
    /**\
     * quality (between 1 (good) and FF_LAMBDA_MAX (bad)) \
     * - encoding: Set by libavcodec. for coded_picture (and set by user for input).\
     * - decoding: Set by libavcodec.\
     */
    int			mQuality;
	
    /**\
     * buffer age (1->was last buffer and dint change, 2->..., ...).\
     * Set to INT_MAX if the buffer has not been used yet.\
     * - encoding: unused\
     * - decoding: MUST be set by get_buffer().\
     */
    int			mAge;
	
    /**\
     * is this picture used as reference\
     * The values for this are the same as the MpegEncContext.picture_structure\
     * variable, that is 1->top field, 2->bottom field, 3->frame/both fields.\
     * Set to 4 for delayed, non-reference frames.\
     * - encoding: unused\
     * - decoding: Set by libavcodec. (before get_buffer() call)).\
     */
    int			mReference;
	
	
    /**\
     * QP table\
     * - encoding: unused\
     * - decoding: Set by libavcodec.\
     */
    byte		mQscaleTable;
	
    /**\
     * QP store stride\
     * - encoding: unused\
     * - decoding: Set by libavcodec.\
     */
    int			mQstride;
	
    /**\
     * mbskip_table[mb]>=1 if MB didn't change\
     * stride= mb_width = (width+15)>>4\
     * - encoding: unused\
     * - decoding: Set by libavcodec.\
     */
    byte		mBskipTable;
	
    /**\
     * motion vector table\
     * @code\
     * example:\
     * int mv_sample_log2= 4 - motion_subsample_log2;\
     * int mb_width= (width+15)>>4;\
     * int mv_stride= (mb_width << mv_sample_log2) + 1;\
     * motion_val[direction][x + y*mv_stride][0->mv_x, 1->mv_y];\
     * @endcode\
     * - encoding: Set by user.\
     * - decoding: Set by libavcodec.\
     */
    //int16_t (*motion_val[2])[2];
	
    /**\
     * macroblock type table\
     * mb_type_base + mb_width + 2\
     * - encoding: Set by user.\
     * - decoding: Set by libavcodec.\
     */
    long		mBtype;
	
    /**\
     * log2 of the size of the block which a single vector in motion_val represents: \
     * (4->16x16, 3->8x8, 2-> 4x4, 1-> 2x2)\
     * - encoding: unused\
     * - decoding: Set by libavcodec.\
     */
    short		mMotionSubsampleLog2;
	
    /**\
     * for some private data of the user\
     * - encoding: unused\
     * - decoding: Set by user.\
     */
    //void *opaque;
	
    /**\
     * error\
     * - encoding: Set by libavcodec. if flags&CODEC_FLAG_PSNR.\
     * - decoding: unused\
     */
    long[]		mError;
	
    /**\
     * type of the buffer (to keep track of who has to deallocate data[*])\
     * - encoding: Set by the one who allocates it.\
     * - decoding: Set by the one who allocates it.\
     * Note: User allocated (direct rendering) & internal buffers cannot coexist currently.\
     */
    int			mType;
    
    /**\
     * When decoding, this signals how much the picture must be delayed.\
     * extra_delay = repeat_pict / (2*fps)\
     * - encoding: unused\
     * - decoding: Set by libavcodec.\
     */
    int			mRepeatPict;
    
    /**\
     * \
     */
    int			mQscaleType;
    
    /**\
     * The content of the picture is interlaced.\
     * - encoding: Set by user.\
     * - decoding: Set by libavcodec. (default 0)\
     */
    int			mInterlacedFrame;
    
    /**\
     * If the content is interlaced, is top field displayed first.\
     * - encoding: Set by user.\
     * - decoding: Set by libavcodec.\
     */
    int			mTopFieldFirst;
    
    /**\
     * Pan scan.\
     * - encoding: Set by user.\
     * - decoding: Set by libavcodec.\
     */
    //AVPanScan *pan_scan;
    
    /**\
     * Tell user application that palette has changed from previous frame.\
     * - encoding: ??? (no palette-enabled encoder yet)\
     * - decoding: Set by libavcodec. (default 0).\
     */
    int			mPaletteHasChanged;
    
    /**\
     * codec suggestion on buffer type if != 0\
     * - encoding: unused\
     * - decoding: Set by libavcodec. (before get_buffer() call)).\
     */
    int			mBufferHints;
	
    /**\
     * DCT coefficients\
     * - encoding: unused\
     * - decoding: Set by libavcodec.\
     */
    //short		*dct_coeff;\
	
    /**\
     * motion reference frame index\
     * the order in which these are stored can depend on the codec.\
     * - encoding: Set by user.\
     * - decoding: Set by libavcodec.\
     */
    byte[]		mRefIndex;
	
    /**\
     * reordered opaque 64bit number (generally a PTS) from AVCodecContext.reordered_opaque\
     * output in AVFrame.reordered_opaque\
     * - encoding: unused\
     * - decoding: Read by user.\
     */
    long		mReorderedOpaque;
	
    /**\
     * hardware accelerator private data (FFmpeg allocated)\
     * - encoding: unused\
     * - decoding: Set by libavcodec\
     */
    //void *hwaccel_picture_private;\
	
	private native void nativeRelease(int FFMpegAVFrame);
	
}
