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
    libaml_mp_sdk \
	libgui \
	libui \
	libstagefright_foundation \
	libjsoncpp

LOCAL_STATIC_LIBRARIES := libamlMpTestSupporter

include $(BUILD_NATIVE_TEST)
