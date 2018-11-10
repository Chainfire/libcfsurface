LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libutils
LOCAL_SRC_FILES := ../lib-depend/$(TARGET_ARCH_ABI)/libutils.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libbinder
LOCAL_SRC_FILES := ../lib-depend/$(TARGET_ARCH_ABI)/libbinder.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libgui
LOCAL_SRC_FILES := ../lib-depend/$(TARGET_ARCH_ABI)/libgui.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	cfsurface_main.cpp \
	CFSurface.cpp

LOCAL_CFLAGS += -DGL_GLEXT_PROTOTYPES -DEGL_EGLEXT_PROTOTYPES

LOCAL_SHARED_LIBRARIES := \
	libutils \
	libbinder \
	libgui

LOCAL_MODULE:= cfsurface
LOCAL_LDLIBS := -ldl -llog -lEGL -lGLESv2

#include $(BUILD_EXECUTABLE)
include $(BUILD_SHARED_LIBRARY)
