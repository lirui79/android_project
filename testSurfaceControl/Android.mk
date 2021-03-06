
LOCAL_PATH:=$(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := testsurface.cpp


ifeq ($(PLATFORM_VERSION), 5.1)
	LOCAL_CPPFLAGS += -DPLATFORM_VERSION_5
endif

ifeq ($(PLATFORM_VERSION), 5.1.1)
	LOCAL_CPPFLAGS += -DPLATFORM_VERSION_5
endif

ifeq ($(PLATFORM_VERSION), 6.0.1)
	LOCAL_CPPFLAGS += -DPLATFORM_VERSION_6
endif

ifeq ($(PLATFORM_VERSION), 6.0)
	LOCAL_CPPFLAGS += -DPLATFORM_VERSION_6
endif

ifeq ($(PLATFORM_VERSION), 7.1.1)
	LOCAL_CPPFLAGS += -DPLATFORM_VERSION_7
endif

ifeq ($(PLATFORM_VERSION), 7.1.2)
	LOCAL_CPPFLAGS += -DPLATFORM_VERSION_7
endif

ifeq ($(PLATFORM_VERSION), 8.1.0)
	LOCAL_CPPFLAGS += -DPLATFORM_VERSION_8
endif

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    libbinder \
    libui \
    libgui
 

LOCAL_MODULE := testsurface

LOCAL_MODULE_TAGS := tests

include $(BUILD_EXECUTABLE)