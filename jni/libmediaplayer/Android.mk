LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CFLAGS := -D__STDC_CONSTANT_MACROS

WITH_ANDROID_VECTOR := true

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../libffmpeg \
	$(LOCAL_PATH)/../include \
	$(LOCAL_PATH)/../include/android

LOCAL_SRC_FILES := \
	packetqueue.cpp \
	output.cpp \
	mediaplayer.cpp \
	decoder.cpp \
	decoder_audio.cpp \
	decoder_video.cpp \
	thread.cpp

ifeq ($(WITH_ANDROID_VECTOR),true)
LOCAL_SRC_FILES += \
	android/atomic.c \
	android/atomic-android-arm.S \
	android/SharedBuffer.cpp \
	android/VectorImpl.cpp
endif

LOCAL_LDLIBS := -llog

LOCAL_SHARED_LIBRARIES := libjniaudio libjnivideo

LOCAL_STATIC_LIBRARIES := libavcodec libavformat libavutil libpostproc libswscale

LOCAL_MODULE := libmediaplayer

include $(BUILD_STATIC_LIBRARY)
