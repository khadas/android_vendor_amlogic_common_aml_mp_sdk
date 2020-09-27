LOCAL_PATH:= $(call my-dir)

AML_MP_PLAYER_DEMO_SRCS := \
	amlMpPlayerDemo.cpp \
	Source.cpp \
	DvbSource.cpp \
	UdpSource.cpp \
	FileSource.cpp \
	Parser.cpp \
	Playback.cpp

AML_MP_PLAYER_DEMO_INC := $(LOCAL_PATH)/../../

AML_MP_PLAYER_DEMO_CFLAGS := -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)

AML_MP_PLAYER_DEMO_SHARED_LIBS := \
	libutils \
	libcutils \
	liblog \
	libaml_mp_sdk \
	libnativewindow \
	libgui \
	libui \
	libstagefright_foundation \
	libamdvr.product

###############################################################################
include $(CLEAR_VARS)
LOCAL_MODULE := amlMpPlayerDemo
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(AML_MP_PLAYER_DEMO_SRCS)
LOCAL_CFLAGS := $(AML_MP_PLAYER_DEMO_CFLAGS)
LOCAL_C_INCLUDES := $(AML_MP_PLAYER_DEMO_INC)
#LOCAL_EXPORT_C_INCLUDE_DIRS :=
LOCAL_SHARED_LIBRARIES := $(AML_MP_PLAYER_DEMO_SHARED_LIBS)
#LOCAL_STATIC_LIBRARIES :=
#LOCAL_WHOLE_STATIC_LIBRARIES :=
#LOCAL_LDFLAGS :=

include $(BUILD_EXECUTABLE)

