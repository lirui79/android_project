# Build the unit tests.
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk
 
LOCAL_MODULE := opengl_c
 
LOCAL_MODULE_TAGS := opengl_c
 
LOCAL_SRC_FILES := \
	GLUtil.cpp \
	show_yuv.cpp \
	main.cpp 
 
LOCAL_SHARED_LIBRARIES := \
	libEGL \
	libGLESv2 \
	libbinder \
	libcutils \
	libgui \
	libmedia \
	libstagefright \
	libstagefright_foundation \
	libstagefright_omx \
	libsync \
	libui \
	libutils \
	liblog
 
LOCAL_C_INCLUDES := \
	frameworks/av/media/libstagefright \
	frameworks/av/media/libstagefright/include \
	$(TOP)/frameworks/native/include/media/openmax \
 
#LOCAL_CFLAGS += -Werror -Wall
LOCAL_CLANG := true
 
LOCAL_32_BIT_ONLY := true
 
include $(BUILD_EXECUTABLE)  
