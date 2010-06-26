LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ffmpeg.c cmdutils.c

LOCAL_STATIC_LIBRARIES := libavcodec libavformat libavutil libpostproc libswscale

LOCAL_MODULE := ffmpeg

include $(BUILD_EXECUTABLE)

# build android native lib
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
		android/onLoad.c \
		android/com_media_FFMpegAVInputFormat.c \
		android/com_media_FFMpegAVRational.c \
		android/com_media_FFMpegAVFormatContext.c \
		android/com_media_FFMpeg.c \
		cmdutils.c

LOCAL_LDLIBS := -llog
LOCAL_STATIC_LIBRARIES := libavcodec libavformat libavutil libpostproc libswscale

LOCAL_MODULE := libffmpeg_jni

include $(BUILD_SHARED_LIBRARY)

# build libs
include $(CLEAR_VARS)
include $(call all-makefiles-under,$(LOCAL_PATH))
