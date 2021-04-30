LOCAL_PATH:= $(call my-dir)

AML_MP_PLAYER_DEMO_SRCS := \
	AmlMpPlayerDemo.cpp \

AML_MP_PLAYER_DEMO_INC :=

AML_MP_PLAYER_DEMO_CFLAGS := -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)

AML_MP_PLAYER_DEMO_SHARED_LIBS := \
	libutils \
	libcutils \
	liblog \
	libnativewindow \
	libui \
	libstagefright_foundation \

AML_MP_PLAYER_DEMO_SHARED_LIBS_29 := \
	libamdvr.product \
	libaml_mp_sdk \
	libgui \

AML_MP_PLAYER_DEMO_SHARED_LIBS_30 := \
    libaml_mp_sdk.vendor \
	libamdvr

AML_MP_PLAYER_DEMO_STATIC_LIBS := \
	libamlMpTestSupporter

###############################################################################
include $(CLEAR_VARS)
LOCAL_MODULE := amlMpPlayerDemo
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(AML_MP_PLAYER_DEMO_SRCS)
LOCAL_CFLAGS := $(AML_MP_PLAYER_DEMO_CFLAGS)
LOCAL_C_INCLUDES := $(AML_MP_PLAYER_DEMO_INC)
LOCAL_SHARED_LIBRARIES := $(AML_MP_PLAYER_DEMO_SHARED_LIBS) $(AML_MP_PLAYER_DEMO_SHARED_LIBS_$(PLATFORM_SDK_VERSION))
LOCAL_STATIC_LIBRARIES := $(AML_MP_PLAYER_DEMO_STATIC_LIBS)
#LOCAL_WHOLE_STATIC_LIBRARIES :=
#LOCAL_LDFLAGS :=

ifeq (1, $(shell expr $(PLATFORM_SDK_VERSION) \>= 30))
LOCAL_VENDOR_MODULE := true
endif
include $(BUILD_EXECUTABLE)
