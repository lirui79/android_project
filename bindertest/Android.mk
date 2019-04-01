LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := eng

LOCAL_SRC_FILES := \
       sampleClient.cpp

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH) \

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libbinder \
    libutils \
    libhardware

LOCAL_CFLAGS := -DRIL_SHLIB

LOCAL_MODULE:= sampleClient

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := eng

LOCAL_SRC_FILES := \
           sampleService.cpp

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH) \

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libbinder \
    libutils \
    libhardware

LOCAL_CFLAGS := -DRIL_SHLIB

LOCAL_MODULE := sampleService

include $(BUILD_EXECUTABLE)

