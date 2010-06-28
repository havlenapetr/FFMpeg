LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := test/ffplayer.c

LOCAL_STATIC_LIBRARIES := libavcodec libavformat libavutil libpostproc libswscale

LOCAL_MODULE := ffplayer

include $(BUILD_EXECUTABLE)

# build android native lib
include $(CLEAR_VARS)

WITH_PLAYER := false

ifeq ($(WITH_PLAYER),true)
LOCAL_CFLAGS := -DBUILD_WITH_PLAYER
endif

LOCAL_SRC_FILES := \
		android/onLoad.c \
		android/com_media_ffmpeg_FFMpegAVInputFormat.c \
		android/com_media_ffmpeg_FFMpegAVRational.c \
		android/com_media_ffmpeg_FFMpegAVFormatContext.c \
		android/com_media_ffmpeg_FFMpeg.c \
		cmdutils.c
		
ifeq ($(WITH_PLAYER),true)
LOCAL_SRC_FILES += \
	android/com_media_ffmpeg_android_FFMpegPlayerAndroid.c
endif
		
LOCAL_LDLIBS := -llog
LOCAL_STATIC_LIBRARIES := libavcodec libavformat libavutil libpostproc libswscale

LOCAL_MODULE := libffmpeg_jni

include $(BUILD_SHARED_LIBRARY)

# build libs
include $(CLEAR_VARS)
include $(call all-makefiles-under,$(LOCAL_PATH))
