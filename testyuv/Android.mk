
LOCAL_PATH:=$(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := testsurface.cpp


LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    libbinder \
    libui \
    libgui
 

LOCAL_MODULE := testsurface

LOCAL_MODULE_TAGS := tests

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := testBufferProducer.cpp


LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    libbinder \
    libui \
    libgui
 

LOCAL_MODULE := testBufferProducer

LOCAL_MODULE_TAGS := tests

include $(BUILD_EXECUTABLE)