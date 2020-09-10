LOCAL_PATH:= $(call my-dir)

AML_MP_PLAYER_PLAYER_SRC := \
	Aml_MP.cpp \
	Aml_MP_Player.cpp \
	Aml_MP_PlayerImpl.cpp \
	AmlPlayerBase.cpp \
	AmlTsPlayer.cpp \
	AmlCTCPlayer.cpp \

AML_MP_PLAYER_DEMUX_SRC := \
	demux/AmlDemuxBase.cpp \
	demux/AmlSwDemux.cpp \
	demux/AmlHwDemux.cpp \

AML_MP_PLAYER_CAS_SRC := \
	cas/AmlCasBase.cpp \
	cas/AmlDvbCasHal.cpp \
	cas/AmlIptvCas.cpp \

AML_MP_PLAYER_UTILS_SRC := \
	utils/AmlMpUtils.cpp \
	utils/AmlMpConfig.cpp \

AML_MP_PLAYER_SRCS := \
	$(AML_MP_PLAYER_PLAYER_SRC) \
	$(AML_MP_PLAYER_DEMUX_SRC) \
	$(AML_MP_PLAYER_CAS_SRC) \
	$(AML_MP_PLAYER_UTILS_SRC) \

AML_MP_PLAYER_INC := $(LOCAL_PATH)/../include \
	$(TOP)/vendor/amlogic/common/apps/LibTsPlayer/jni/include \
	$(TOP)/vendor/amlogic/common/libdvr_release/include \
	$(TOP)/vendor/amlogic/common/external/DTVKit/cas_hal/libamcas/include \
	$(TOP)/vendor/amlogic/common/mediahal_sdk/include \
	$(TOP)/hardware/amlogic/gralloc \
	$(TOP)/hardware/amlogic/media/amcodec/include \
	$(TOP)/vendor/amlogic/common/frameworks/services/subtiltleserver/client

AML_MP_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/../include

AML_MP_PLAYER_CFLAGS := -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)

AML_MP_PLAYER_SHARED_LIBS := \
	libutils \
	libcutils \
	liblog \
	libnativewindow \
	libui \
	libstagefright_foundation \

AML_MP_PLAYER_SYSTEM_SHARED_LIBS := \
	libliveplayer \
	libamdvr.product \
	libmediahal_tsplayer.system \
	libamgralloc_ext@2 \
	libSubtitleClient \

AML_MP_PLAYER_SYSTEM_STATIC_LIBS_29 := \
	libam_cas

AML_MP_PLAYER_VENDOR_SHARED_LIBS := \
	libliveplayer.vendor \
	libamdvr \
	libmediahal_tsplayer \
	libamgralloc_ext_vendor@2 \

###############################################################################
include $(CLEAR_VARS)
LOCAL_MODULE := libaml_mp_sdk
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(AML_MP_PLAYER_SRCS)
LOCAL_CFLAGS := $(AML_MP_PLAYER_CFLAGS)
LOCAL_C_INCLUDES := $(AML_MP_PLAYER_INC)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(AML_MP_EXPORT_C_INCLUDE_DIRS)
LOCAL_SHARED_LIBRARIES := $(AML_MP_PLAYER_SHARED_LIBS) $(AML_MP_PLAYER_SYSTEM_SHARED_LIBS)
LOCAL_STATIC_LIBRARIES := $(AML_MP_PLAYER_SYSTEM_STATIC_LIBS_$(PLATFORM_SDK_VERSION))
#LOCAL_WHOLE_STATIC_LIBRARIES :=
#LOCAL_LDFLAGS :=
include $(BUILD_SHARED_LIBRARY)

###############################################################################
include $(CLEAR_VARS)
LOCAL_MODULE := libaml_mp_sdk.vendor
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(AML_MP_PLAYER_SRCS)
LOCAL_CFLAGS := $(AML_MP_PLAYER_CFLAGS)
LOCAL_C_INCLUDES := $(AML_MP_PLAYER_INC)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(AML_MP_EXPORT_C_INCLUDE_DIRS)
LOCAL_SHARED_LIBRARIES := $(AML_MP_PLAYER_SHARED_LIBS) $(AML_MP_PLAYER_VENDOR_SHARED_LIBS)
#LOCAL_STATIC_LIBRARIES :=
#LOCAL_WHOLE_STATIC_LIBRARIES :=
#LOCAL_LDFLAGS :=
include $(BUILD_SHARED_LIBRARY)
