LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    displaytest_main.cpp \
    DisplayTest.cpp \

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    liblog \
    libutils \
    libbinder \
    libui \
    libEGL \
    libGLESv1_CM \
    libgui \
    libhardware \

LOCAL_C_INCLUDES += \
	$(ANDROID_BUILD_TOP)/external/openssl/include \
	$(ANDROID_BUILD_TOP)/frameworks/base/core/jni/android/graphics \
	$(ANDROID_BUILD_TOP)/frameworks/base/include \
	$(ANDROID_BUILD_TOP)/frameworks/base/libs/hwui \
	$(ANDROID_BUILD_TOP)/frameworks/native/services/inputflinger \
	$(ANDROID_BUILD_TOP)/system/core/include \

LOCAL_CPPFLAGS += -isystem $(ANDROID_BUILD_TOP)/external/skia/include/core \
	-isystem $(ANDROID_BUILD_TOP)/external/skia/include/config \
	-isystem $(ANDROID_BUILD_TOP)/external/skia/include/images \
	-isystem $(ANDROID_BUILD_TOP)/external/skia/include/effects \
	-isystem $(ANDROID_BUILD_TOP)/external/skia/include \
#ifdef TARGET_BOARD_MTK
LOCAL_C_INCLUDES += $(MTK_PATH_SOURCE)/hardware/perfservice/perfservicenative/
#endif


LOCAL_CFLAGS += -DLOG_TAG=\"sftest\"

LOCAL_MODULE := sftest

include $(BUILD_EXECUTABLE)
