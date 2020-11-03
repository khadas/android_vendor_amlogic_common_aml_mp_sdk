LOCAL_PATH:= $(call my-dir)

AML_MP_TEST_SUPPORTER_SRCS := \
	AmlMpTestSupporter.cpp \
	Parser.cpp \
	TestModule.cpp \
	DVRPlayback.cpp \
	DVRRecord.cpp \
	Playback.cpp \
	TestUtils.cpp \
	source/Source.cpp \
	source/DvbSource.cpp \
	source/UdpSource.cpp \
	source/FileSource.cpp \
	demux/AmlDemuxBase.cpp \
	demux/AmlSwDemux.cpp \
	demux/AmlHwDemux.cpp \

AML_MP_TEST_SUPPORTER_INC := $(LOCAL_PATH)/../../

AML_MP_TEST_SUPPORTER_CFLAGS := -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)

AML_MP_TEST_SUPPORTER_SHARED_LIBS := \
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
LOCAL_MODULE := libamlMpTestSupporter
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(AML_MP_TEST_SUPPORTER_SRCS)
LOCAL_CFLAGS := $(AML_MP_TEST_SUPPORTER_CFLAGS)
LOCAL_C_INCLUDES := $(AML_MP_TEST_SUPPORTER_INC)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
LOCAL_SHARED_LIBRARIES := $(AML_MP_TEST_SUPPORTER_SHARED_LIBS)
#LOCAL_STATIC_LIBRARIES :=
#LOCAL_WHOLE_STATIC_LIBRARIES :=
#LOCAL_LDFLAGS :=

include $(BUILD_STATIC_LIBRARY)
