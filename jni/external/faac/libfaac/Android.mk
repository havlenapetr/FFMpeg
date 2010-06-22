LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CFLAGS := -DHAVE_CONFIG_H

LOCAL_SRC_FILES := \
 	aacquant.c bitstream.c fft.c frame.c midside.c psychkni.c util.c backpred.c channels.c filtbank.c huffman.c ltp.c tns.c

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../include

LOCAL_MODULE := libfaac

include $(BUILD_STATIC_LIBRARY)

