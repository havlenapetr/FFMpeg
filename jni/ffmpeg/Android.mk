LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CFLAGS := -D__STDC_CONSTANT_MACROS

WITH_CONVERTOR := true

#ifeq ($(WITH_PLAYER),true)
LOCAL_CFLAGS += -DBUILD_WITH_PLAYER
#endif

ifeq ($(WITH_CONVERTOR),true)
LOCAL_CFLAGS += -DBUILD_WITH_CONVERTOR
endif

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../include

LOCAL_SRC_FILES := \
		android/onLoad.cpp \
		android/com_media_ffmpeg_FFMpegAVFrame.cpp \
		android/com_media_ffmpeg_FFMpegAVInputFormat.c \
		android/com_media_ffmpeg_FFMpegAVRational.c \
		android/com_media_ffmpeg_FFMpegAVFormatContext.c \
		android/com_media_ffmpeg_FFMpegAVCodecContext.cpp \
		android/com_media_ffmpeg_FFMpegUtils.cpp


ifeq ($(WITH_CONVERTOR),true)
LOCAL_SRC_FILES += \
	android/com_media_ffmpeg_FFMpeg.c \
	cmdutils.c
endif
		
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

LOCAL_SHARED_LIBRARIES := libjniaudio libjnivideo

LOCAL_STATIC_LIBRARIES := libavcodec libavformat libavutil libpostproc libswscale

LOCAL_MODULE := libffmpeg_jni

include $(BUILD_SHARED_LIBRARY)

# build libs
include $(CLEAR_VARS)
include $(call all-makefiles-under,$(LOCAL_PATH))
