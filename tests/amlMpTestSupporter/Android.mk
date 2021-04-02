LOCAL_PATH:= $(call my-dir)

AML_MP_TEST_SUPPORTER_SRCS := \
	AmlMpTestSupporter.cpp \
	ParserReceiver.cpp \
	TestModule.cpp \
	DVRPlayback.cpp \
	DVRRecord.cpp \
	Playback.cpp \
	TestUtils.cpp \
	source/Source.cpp \
	source/DvbSource.cpp \
	source/UdpSource.cpp \
	source/FileSource.cpp \

AML_MP_TEST_SUPPORTER_INC :=

AML_MP_TEST_SUPPORTER_CFLAGS := -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)

AML_MP_TEST_SUPPORTER_SHARED_LIBS := \
	libutils \
	libcutils \
	liblog \
	libnativewindow \
	libui \
	libstagefright_foundation \

AML_MP_TEST_SUPPORTER_SHARED_LIBS_29 := \
	libaml_mp_sdk \
	libamdvr.product \
	libgui \

AML_MP_TEST_SUPPORTER_VENDOR_SHARED_LIBS_29 := \
	libaml_mp_sdk.vendor \
	libamdvr \

AML_MP_TEST_SUPPORTER_SHARED_LIBS_30 := \
	libaml_mp_sdk \
	libamdvr.system \
	libgui \

AML_MP_TEST_SUPPORTER_VENDOR_SHARED_LIBS_30 := \
	libaml_mp_sdk.vendor \
	libamdvr \

###############################################################################
include $(CLEAR_VARS)
LOCAL_MODULE := libamlMpTestSupporter
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0 SPDX-license-identifier-FTL SPDX-license-identifier-GPL SPDX-license-identifier-LGPL-2.1 SPDX-license-identifier-MIT legacy_by_exception_only legacy_notice
LOCAL_LICENSE_CONDITIONS := by_exception_only notice restricted
LOCAL_NOTICE_FILE := $(LOCAL_PATH)/../../LICENSE
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(AML_MP_TEST_SUPPORTER_SRCS)
LOCAL_CFLAGS := $(AML_MP_TEST_SUPPORTER_CFLAGS)
LOCAL_C_INCLUDES := $(AML_MP_TEST_SUPPORTER_INC)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
LOCAL_SHARED_LIBRARIES := $(AML_MP_TEST_SUPPORTER_SHARED_LIBS) $(AML_MP_TEST_SUPPORTER_SHARED_LIBS_$(PLATFORM_SDK_VERSION))
#LOCAL_STATIC_LIBRARIES :=
#LOCAL_WHOLE_STATIC_LIBRARIES :=
#LOCAL_LDFLAGS :=

ifeq (1, $(shell expr $(PLATFORM_SDK_VERSION) \>= 30))
LOCAL_SYSTEM_EXT_MODULE := true
endif
include $(BUILD_STATIC_LIBRARY)

###############################################################################
include $(CLEAR_VARS)
LOCAL_MODULE := libamlMpTestSupporter.vendor
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(AML_MP_TEST_SUPPORTER_SRCS)
LOCAL_CFLAGS := $(AML_MP_TEST_SUPPORTER_CFLAGS)
LOCAL_C_INCLUDES := $(AML_MP_TEST_SUPPORTER_INC)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
LOCAL_SHARED_LIBRARIES := $(AML_MP_TEST_SUPPORTER_SHARED_LIBS) $(AML_MP_TEST_SUPPORTER_VENDOR_SHARED_LIBS_$(PLATFORM_SDK_VERSION))
#LOCAL_STATIC_LIBRARIES :=
#LOCAL_WHOLE_STATIC_LIBRARIES :=
#LOCAL_LDFLAGS :=
LOCAL_VENDOR_MODULE := true
include $(BUILD_STATIC_LIBRARY)
