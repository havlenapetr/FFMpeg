LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libjnivideo
LOCAL_SRC_FILES := libjnivideo.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libjniaudio
LOCAL_SRC_FILES := libjniaudio.so
include $(PREBUILT_SHARED_LIBRARY)
