LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := eng

LOCAL_SRC_FILES := \
       client.cpp

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH) \

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \


LOCAL_CFLAGS := -DRIL_SHLIB

LOCAL_MODULE:= tcpclient

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := eng

LOCAL_SRC_FILES := \
           server.cpp

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH) \

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \


LOCAL_CFLAGS := -DRIL_SHLIB

LOCAL_MODULE := tcpserver

include $(BUILD_EXECUTABLE)
