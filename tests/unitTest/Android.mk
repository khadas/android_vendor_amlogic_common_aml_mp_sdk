LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := amlMpUnitTest
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0 SPDX-license-identifier-FTL SPDX-license-identifier-GPL SPDX-license-identifier-LGPL-2.1 SPDX-license-identifier-MIT legacy_by_exception_only legacy_notice
LOCAL_LICENSE_CONDITIONS := by_exception_only notice restricted
LOCAL_NOTICE_FILE := $(LOCAL_PATH)/../../LICENSE
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

LOCAL_STATIC_LIBRARIES := libamlMpTestSupporter

ifeq (1, $(shell expr $(PLATFORM_SDK_VERSION) \>= 30))
LOCAL_VENDOR_MODULE := true
LOCAL_SHARED_LIBRARIES += \
    libaml_mp_sdk.vendor
else
LOCAL_SHARED_LIBRARIES += \
    libaml_mp_sdk \
    libgui
endif
include $(BUILD_NATIVE_TEST)

