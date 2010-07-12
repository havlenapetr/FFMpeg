LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CFLAGS := -D__STDC_CONSTANT_MACROS

#ifeq ($(WITH_PLAYER),true)
LOCAL_CFLAGS += -DBUILD_WITH_PLAYER
#endif

LOCAL_SRC_FILES := \
		android/onLoad.cpp \
		android/com_media_ffmpeg_FFMpegAVFrame.cpp \
		android/com_media_ffmpeg_FFMpegAVInputFormat.c \
		android/com_media_ffmpeg_FFMpegAVRational.c \
		android/com_media_ffmpeg_FFMpegAVFormatContext.c \
		android/com_media_ffmpeg_FFMpeg.c \
		android/com_media_ffmpeg_FFMpegUtils.cpp \
		cmdutils.c
		
#ifeq ($(WITH_PLAYER),true)
LOCAL_SRC_FILES += \
	android/com_media_ffmpeg_android_FFMpegPlayerAndroid.cpp
#endif

ifeq ($(IN_NDK),true)	
LOCAL_LDLIBS := -llog
else
LOCAL_PRELINK_MODULE := false
LOCAL_SHARED_LIBRARIES := liblog
endif

LOCAL_STATIC_LIBRARIES := libavcodec libavformat libavutil libpostproc libswscale

LOCAL_MODULE := libffmpeg_jni

include $(BUILD_SHARED_LIBRARY)

# build libs
include $(CLEAR_VARS)
include $(call all-makefiles-under,$(LOCAL_PATH))
