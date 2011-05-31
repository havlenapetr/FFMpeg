LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-subdir-java-files)

LOCAL_PACKAGE_NAME := FFMpeg

LOCAL_JNI_SHARED_LIBRARIES := libffmpeg_jni

include $(BUILD_PACKAGE)

# ============================================================

# Also build all of the sub-targets under this one: the shared library.
IN_NDK := true
WITH_PLAYER := true
include $(call all-makefiles-under,$(LOCAL_PATH))
