LOCAL_PATH := $(call my-dir)
APP_STL:= gnustl_static
APP_STL:= stlport_static

include $(CLEAR_VARS)

LOCAL_CFLAGS += -ggdb
LOCAL_STRIP_MODULE = flase

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := NfcMiddlewareSimulator 
LOCAL_SRC_FILES := NciLogFileProcessor.cpp NfcMiddlewareSimulator.cpp

include $(BUILD_EXECUTABLE)
