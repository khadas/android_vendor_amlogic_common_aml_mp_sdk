LOCAL_PATH:=$(shell pwd)
LIB_DIR := $(TARGET_DIR)/usr/lib
AML_MP_ROOT_PATH = $(LOCAL_PATH)
AML_MP_PLAYER_SRC := \
    player/Aml_MP.cpp \
    player/Aml_MP_Player.cpp \
    player/Aml_MP_PlayerImpl.cpp \
    player/AmlPlayerBase.cpp \
    player/AmlTsPlayer.cpp \

AML_MP_CAS_SRC := \
    cas/Aml_MP_CAS.cpp \
    cas/AmlCasBase.cpp \
    cas/AmlDvbCasHal.cpp \
    cas/vmx_iptvcas/AmlVMXIptvCas.cpp \
    cas/vmx_iptvcas/AmlVMXIptvCas_V2.cpp \
    cas/wv_iptvcas/AmlWVIptvCas.cpp \

AML_MP_DVR_SRC := \
    dvr/Aml_MP_DVR.cpp \
    dvr/AmlDVRPlayer.cpp \
    dvr/AmlDVRRecorder.cpp

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

AML_MP_DEMUX_SRC := \
    demux/AmlDemuxBase.cpp \
    demux/AmlHwDemux.cpp \
    demux/AmlSwDemux.cpp \
    demux/AmlTsParser.cpp \

AML_MP_SRCS := \
    $(AML_MP_PLAYER_SRC) \
    $(AML_MP_DVR_SRC) \
    $(AML_MP_UTILS_SRC) \
    $(AML_MP_DEMUX_SRC)

AML_MP_TEST_SUPPORTER_SRCS := \
    tests/amlMpTestSupporter/AmlMpTestSupporter.cpp \
    tests/amlMpTestSupporter/ParserReceiver.cpp \
    tests/amlMpTestSupporter/TestModule.cpp \
    tests/amlMpTestSupporter/DVRPlayback.cpp \
    tests/amlMpTestSupporter/DVRRecord.cpp \
    tests/amlMpTestSupporter/Playback.cpp \
    tests/amlMpTestSupporter/TestUtils.cpp \
    tests/amlMpTestSupporter/source/Source.cpp \
    tests/amlMpTestSupporter/source/DvbSource.cpp \
    tests/amlMpTestSupporter/source/UdpSource.cpp \
    tests/amlMpTestSupporter/source/FileSource.cpp \
    #tests/amlMpTestSupporter/demux/AmlDemuxBase.cpp \
    tests/amlMpTestSupporter/demux/AmlSwDemux.cpp \
    tests/amlMpTestSupporter/demux/AmlHwDemux.cpp \

AML_MP_PLAYER_DEMO_SRCS := tests/amlMpPlayerDemo/AmlMpPlayerDemo.cpp

AML_MP_OBJS := $(patsubst %.cpp,%.o,$(AML_MP_SRCS))
AML_MP_TEST_SUPPORTER_OBJS := $(patsubst %.cpp,%.o,$(AML_MP_TEST_SUPPORTER_SRCS))
AML_MP_PLAYER_DEMO_OBJS := $(patsubst %.cpp,%.o,$(AML_MP_PLAYER_DEMO_SRCS))

AML_MP_INC := $(AML_MP_ROOT_PATH)/include \
    $(AML_MP_ROOT_PATH)/ \
    $(AML_MP_ROOT_PATH)/tests/amlMpTestSupporter/ \
    $(AML_MP_ROOT_PATH)/tests/amlMpTestSupporter/demux/ \
    $(STAGING_DIR)/usr/include/libdvr \
    $(AML_MP_ROOT_PATH)/../../vendor/amlogic/aml_commonlib/aml_mp/include/ \
    $(AML_MP_ROOT_PATH)/../../vendor/amlogic/aml_commonlib/liblog/include/ \
    $(AML_MP_ROOT_PATH)/../../vendor/amlogic/aml_commonlib/libbinder/include/ \

CFLAGS += $(patsubst %,-I%,$(AML_MP_INC))
LDFLAGS += -lmediahal_tsplayer -L. -lamdvr -llog -lcutils -lpthread -std=c++17 -D__unused=
MP_LDFLAGS += -laml_mp_sdk -L .
SUPPORT_LDFLAGS += -lamlMpTestSupporter -L.

all: libaml_mp_sdk.so amlMpPlayerDemo

libaml_mp_sdk.so :$(AML_MP_OBJS)
	$(CXX) -o $@ $^  $(LDFLAGS) -shared -fPIC -lm -lz

libamlMpTestSupporter.a : libaml_mp_sdk.so $(AML_MP_TEST_SUPPORTER_OBJS)
	$(AR) cq $@ $(AML_MP_TEST_SUPPORTER_OBJS)

$(AML_MP_OBJS) : %.o : %.cpp
	$(CXX) -c $< -o $@ $(CFLAGS) $(LDFLAGS) -fPIC

$(AML_MP_TEST_SUPPORTER_OBJS) : %.o : %.cpp
	$(CXX) -c $< -o $@ $(CFLAGS) $(MP_LDFLAGS) $(LDFLAGS) -fPIC

$(AML_MP_PLAYER_DEMO_OBJS) : %.o : %.cpp
	$(CXX) -c $< -o $@ $(CFLAGS) $(SUPPORT_LDFLAGS) $(MP_LDFLAGS) $(LDFLAGS) -fPIC

amlMpPlayerDemo : libaml_mp_sdk.so libamlMpTestSupporter.a $(AML_MP_PLAYER_DEMO_OBJS)
	$(CXX) $(AML_MP_PLAYER_DEMO_OBJS) -o $@ $(CFLAGS) $(SUPPORT_LDFLAGS) $(MP_LDFLAGS) $(LDFLAGS) -lm -lz -g

.PHONY: install
install:
	install -m 755 -D libaml_mp_sdk.so -t $(TARGET_DIR)/usr/lib/

.PHONY: clean
clean:
	rm -rf $(shell find -name "*.o")
	rm -rf $(shell find -name "*.a")
	rm -f libaml_mp_sdk.so
	rm -f amlMpPlayerDemo
#	rm -f $(TARGET_DIR)/usr/lib/libaml_mp_sdk.so
