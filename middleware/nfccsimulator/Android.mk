LOCAL_PATH := $(call my-dir)
APP_STL:= gnustl_static
APP_STL:= stlport_static

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := nfccsimulator 
LOCAL_SRC_FILES := nfccsimulator.cpp

include $(BUILD_EXECUTABLE)
