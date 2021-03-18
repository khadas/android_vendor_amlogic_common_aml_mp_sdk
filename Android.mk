LOCAL_PATH:= $(call my-dir)

define is-module-source-exist
$(shell find $(TOP)/vendor/amlogic/common/prebuilt/libmediadrm/wvcas/ -maxdepth 1 -type d -name $(1) -exec echo OK \; 2>/dev/null)
endef


AML_MP_PLAYER_SRC := \
	player/Aml_MP.cpp \
	player/Aml_MP_Player.cpp \
	player/Aml_MP_PlayerImpl.cpp \
	player/AmlPlayerBase.cpp \
	player/AmlTsPlayer.cpp \

AML_MP_PLAYER_SRC_29 := \
	player/AmlCTCPlayer.cpp \


AML_MP_CAS_SRC := \
	cas/Aml_MP_CAS.cpp \
	cas/AmlCasBase.cpp \
	cas/AmlDvbCasHal.cpp \

ifeq ($(call is-module-source-exist, include),OK)

AML_MP_CAS_SRC_30 += \
        cas/wv_iptvcas/AmlWVIptvCas.cpp \

AML_MP_CFLAGS_30 := -DHAVE_WVIPTV_CAS

AML_MP_VENDOR_SHARED_LIBS_30 += \
        libdec_ca_wvcas \

endif


AML_MP_DVR_SRC := \
	dvr/Aml_MP_DVR.cpp \
	dvr/AmlDVRPlayer.cpp \
	dvr/AmlDVRRecorder.cpp

AML_MP_DEMUX_SRC := \
	demux/AmlDemuxBase.cpp \
	demux/AmlHwDemux.cpp \
	demux/AmlSwDemux.cpp \
	demux/AmlTsParser.cpp

AML_MP_UTILS_SRC := \
	utils/AmlMpAtomizer.cpp \
	utils/AmlMpBitReader.cpp \
	utils/AmlMpBuffer.cpp \
	utils/AmlMpConfig.cpp \
	utils/AmlMpEventHandler.cpp \
	utils/AmlMpEventLooper.cpp \
	utils/AmlMpEventLooperRoster.cpp \
	utils/AmlMpLooper.cpp \
	utils/AmlMpMessage.cpp \
	utils/AmlMpRefBase.cpp \
	utils/AmlMpStrongPointer.cpp \
	utils/AmlMpThread.cpp \
	utils/AmlMpUtils.cpp \
	utils/Amlsysfsutils.cpp \

AML_MP_SRCS := \
	$(AML_MP_PLAYER_SRC) \
	$(AML_MP_PLAYER_SRC_$(PLATFORM_SDK_VERSION)) \
	$(AML_MP_CAS_SRC) \
	$(AML_MP_CAS_SRC_$(PLATFORM_SDK_VERSION)) \
	$(AML_MP_DVR_SRC) \
	$(AML_MP_DEMUX_SRC) \
	$(AML_MP_UTILS_SRC) \

AML_MP_VENDOR_SRCS := \
	$(AML_MP_PLAYER_SRC) \
	$(AML_MP_PLAYER_SRC_$(PLATFORM_SDK_VERSION)) \
	$(AML_MP_CAS_SRC) \
	$(AML_MP_CAS_SRC_$(PLATFORM_SDK_VERSION)) \
	$(AML_MP_DVR_SRC) \
	$(AML_MP_DEMUX_SRC) \
	$(AML_MP_UTILS_SRC) \

AML_MP_INC := $(LOCAL_PATH)/include \
	$(TOP)/vendor/amlogic/common/apps/LibTsPlayer/jni/include \
	$(TOP)/vendor/amlogic/common/libdvr_release/include \
	$(TOP)/vendor/amlogic/common/external/DTVKit/cas_hal/libamcas/include \
	$(TOP)/vendor/amlogic/common/mediahal_sdk/include \
	$(TOP)/hardware/amlogic/gralloc \
	$(TOP)/hardware/amlogic/media/amcodec/include \
	$(TOP)/vendor/amlogic/common/frameworks/services/subtiltleserver/client \
	$(TOP)/vendor/amlogic/common/prebuilt/libmediadrm/

AML_MP_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include \
	$(LOCAL_PATH) \
	$(TOP)/hardware/amlogic/media/amcodec/include \
	$(TOP)/vendor/amlogic/common/libdvr_release/include \
	$(TOP)/vendor/amlogic/common/mediahal_sdk/include \

AML_MP_CFLAGS := -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)

AML_MP_CFLAGS_29 := -DHAVE_SUBTITLE \
	-DHAVE_CTC \


AML_MP_SHARED_LIBS := \
	libutils \
	libcutils \
	liblog \
	libnativewindow \
	libui \
	libstagefright_foundation \

AML_MP_SYSTEM_SHARED_LIBS := \
	libamdvr.product \
	libmediahal_tsplayer.system \
	libamgralloc_ext@2 \
	libSubtitleClient \
	libCTC_MediaProcessor \
	libgui \

AML_MP_SYSTEM_STATIC_LIBS_29 := \
	libam_cas

AML_MP_VENDOR_SHARED_LIBS := \
	libamdvr \
	libmediahal_tsplayer \

AML_MP_VENDOR_SHARED_LIBS_29 += \
	libamgralloc_ext_vendor@2 \
	libCTC_MediaProcessor.vendor \

AML_MP_VENDOR_SHARED_LIBS_30 += \
	libamgralloc_ext \

ifeq ($(shell expr $(PLATFORM_SDK_VERSION) \>= 30), 1)
AML_MP_VENDOR_STATIC_LIBS_$(PLATFORM_SDK_VERSION) := \
	libam_cas
endif

###############################################################################
ifeq (1, $(shell expr $(PLATFORM_SDK_VERSION) \<= 29))
include $(CLEAR_VARS)
LOCAL_MODULE := libaml_mp_sdk
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(AML_MP_SRCS)
LOCAL_CFLAGS := $(AML_MP_CFLAGS) $(AML_MP_CFLAGS_$(PLATFORM_SDK_VERSION))
LOCAL_C_INCLUDES := $(AML_MP_INC)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(AML_MP_EXPORT_C_INCLUDE_DIRS)
LOCAL_SHARED_LIBRARIES := $(AML_MP_SHARED_LIBS) $(AML_MP_SYSTEM_SHARED_LIBS)
LOCAL_STATIC_LIBRARIES := $(AML_MP_SYSTEM_STATIC_LIBS_$(PLATFORM_SDK_VERSION))
#LOCAL_WHOLE_STATIC_LIBRARIES :=
#LOCAL_LDFLAGS :=
include $(BUILD_SHARED_LIBRARY)
endif

###############################################################################
ifeq (1, $(shell expr $(PLATFORM_SDK_VERSION) \>= 29))
include $(CLEAR_VARS)
LOCAL_MODULE := libaml_mp_sdk.vendor
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(AML_MP_VENDOR_SRCS)
LOCAL_CFLAGS := $(AML_MP_CFLAGS) $(AML_MP_CFLAGS_$(PLATFORM_SDK_VERSION))
LOCAL_C_INCLUDES := $(AML_MP_INC)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(AML_MP_EXPORT_C_INCLUDE_DIRS)
LOCAL_SHARED_LIBRARIES := $(AML_MP_SHARED_LIBS) $(AML_MP_VENDOR_SHARED_LIBS) $(AML_MP_VENDOR_SHARED_LIBS_$(PLATFORM_SDK_VERSION))
LOCAL_STATIC_LIBRARIES := $(AML_MP_VENDOR_STATIC_LIBS_$(PLATFORM_SDK_VERSION))
#LOCAL_WHOLE_STATIC_LIBRARIES :=
#LOCAL_LDFLAGS :=
include $(BUILD_SHARED_LIBRARY)
endif

###############################################################################
include $(call all-makefiles-under,$(LOCAL_PATH))

