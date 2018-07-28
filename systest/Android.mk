LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

FILE_LIST := $(wildcard $(LOCAL_PATH)/*.cpp)

#FILE_LIST := $(wildcard $(LOCAL_PATH)/../../vr/*.cpp)
#FILE_LIST += $(wildcard $(LOCAL_PATH)/../../vr/HWC/*.cpp)
#FILE_LIST += $(LOCAL_PATH)/../../vr/HWC/DisplayHardware/HWComposer.cpp
#FILE_LIST += $(wildcard $(LOCAL_PATH)/../../vr/Layer/*.cpp)
#FILE_LIST += $(wildcard $(LOCAL_PATH)/../../vr/Service/*.cpp)
#FILE_LIST += $(wildcard $(LOCAL_PATH)/../../vr/VSync/*.cpp)
#FILE_LIST += $(wildcard $(LOCAL_PATH)/../../vr/Utility/*.cpp)

LOCAL_SRC_FILES += $(FILE_LIST:$(LOCAL_PATH)/%=%)

LOCAL_SHARED_LIBRARIES := \
    libirisvr  \
    libcutils \
    liblog \
    libutils \
    libbinder \
    libui \
    libEGL \
    libGLESv1_CM \
    libgui \
    libhardware \
	libmedia \


LOCAL_STATIC_LIBRARIES := \
	libbatteryservice 

include $(IRIS_ANDROID_MODULES_ROOT)/targets.mk

LOCAL_C_INCLUDES += $(IRIS_OS_ROOT)/systems/include

LOCAL_C_INCLUDES += $(PREBUILT_C_INCLUDES) $(IRIS-WIN_C_INCLUDES) \
					$(IRIS_ANDROID_MODULES_ROOT)/vr \
					$(IRIS_ANDROID_MODULES_ROOT)/vr/HWC \
					$(IRIS_ANDROID_MODULES_ROOT)/includes \

LOCAL_C_INCLUDES += \
	$(ANDROID_BUILD_TOP)/external/openssl/include \
	$(ANDROID_BUILD_TOP)/frameworks/base/core/jni/android/graphics \
	$(ANDROID_BUILD_TOP)/frameworks/base/include \
	$(ANDROID_BUILD_TOP)/frameworks/av/include \
	$(ANDROID_BUILD_TOP)/frameworks/base/libs/hwui \
	$(ANDROID_BUILD_TOP)/frameworks/native/services/inputflinger \
	$(ANDROID_BUILD_TOP)/system/core/include \

LOCAL_CPPFLAGS += -isystem $(ANDROID_BUILD_TOP)/external/skia/include/core \
	-isystem $(ANDROID_BUILD_TOP)/external/skia/include/config \
	-isystem $(ANDROID_BUILD_TOP)/external/skia/include/images \
	-isystem $(ANDROID_BUILD_TOP)/external/skia/include/effects \
	-isystem $(ANDROID_BUILD_TOP)/external/skia/include \

#ifdef TARGET_BOARD_MTK
#LOCAL_C_INCLUDES += $(MTK_PATH_SOURCE)/hardware/perfservice/perfservicenative/
#endif


LOCAL_CFLAGS += -DLOG_TAG=\"systest\" -DTARGET_PRODUCT_N6P -g

LOCAL_MODULE := systest

include $(BUILD_EXECUTABLE)
