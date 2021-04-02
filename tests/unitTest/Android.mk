LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := amlMpUnitTest
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := \
    TestUrlList.cpp \
    AmlMpPlayerTest.cpp

LOCAL_CFLAGS := -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)
LOCAL_C_INCLUDES :=
LOCAL_SHARED_LIBRARIES := libutils \
    libcutils \
    liblog \
	libui \
	libstagefright_foundation \
	libjsoncpp

LOCAL_STATIC_LIBRARIES := libamlMpTestSupporter libgtest

LOCAL_SHARED_LIBRARIES += \
    libaml_mp_sdk \
    libgui

ifeq (1, $(shell expr $(PLATFORM_SDK_VERSION) \>= 30))
LOCAL_SYSTEM_EXT_MODULE := true
endif
include $(BUILD_EXECUTABLE)

