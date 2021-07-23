cmake_minimum_required (VERSION 2.8)

project(aml_mp_sdk)

OPTION(HAVE_CTC "CTC player" OFF)
OPTION(HAVE_SUBTITLE "subtitle" OFF)
OPTION(HAVE_VMXIPTV "verimatrix iptv" ON)
OPTION(HAVE_VMXWEB "verimatrix web" ON)
OPTION(HAVE_WVCAS "widevine cas" OFF)


SET(AML_MP_PLAYER_SRC
    player/Aml_MP.cpp
    player/Aml_MP_Player.cpp
    player/Aml_MP_PlayerImpl.cpp
    player/AmlPlayerBase.cpp
    player/AmlTsPlayer.cpp
)

SET(AML_MP_CAS_SRC
    cas/Aml_MP_CAS.cpp
    cas/AmlCasBase.cpp
    cas/AmlDvbCasHal.cpp
    cas/AmCasLibWrapper.cpp
)

SET(AML_MP_DVR_SRC
    dvr/Aml_MP_DVR.cpp
    dvr/AmlDVRPlayer.cpp
    dvr/AmlDVRRecorder.cpp
)

SET(AML_MP_UTILS_SRC
    utils/AmlMpAtomizer.cpp
    utils/AmlMpBitReader.cpp
    utils/AmlMpBuffer.cpp
    utils/AmlMpConfig.cpp
    utils/AmlMpEventHandler.cpp
    utils/AmlMpEventLooper.cpp
    utils/AmlMpEventLooperRoster.cpp
    utils/AmlMpLooper.cpp
    utils/AmlMpMessage.cpp
    utils/AmlMpRefBase.cpp
    utils/AmlMpStrongPointer.cpp
    utils/AmlMpThread.cpp
    utils/AmlMpUtils.cpp
    utils/AmlMpChunkFifo.cpp
    utils/Amlsysfsutils.cpp
    utils/AmlMpPlayerRoster.cpp
)

SET(AML_MP_DEMUX_SRC
    demux/AmlDemuxBase.cpp
    demux/AmlHwDemux.cpp
    demux/AmlSwDemux.cpp
    demux/AmlTsParser.cpp
)

#OPTION SETTING START
IF(HAVE_VMXIPTV)
    LIST(APPEND AML_MP_CAS_SRC cas/vmx_iptvcas/AmlVMXIptvCas_V2.cpp)
    LIST(APPEND AML_MP_DEFINITIONS HAVE_VMXIPTV_CAS_V2)
ENDIF()

IF(HAVE_VMXWEB)
    LIST(APPEND AML_MP_CAS_SRC cas/vmx_webcas/AmlVMXWebCas.cpp)
    LIST(APPEND AML_MP_DEFINITIONS HAVE_VMXWEB_CAS)
ENDIF()

IF(HAVE_WVCAS)
    LIST(APPEND AML_MP_CAS_SRC cas/wv_iptvcas/AmlWVIptvCas_V2.cpp)
    LIST(APPEND AML_MP_DEFINITIONS HAVE_WVIPTV_CAS_V2)
ENDIF()

IF(HAVE_SUBTITLE)
    LIST(APPEND AML_MP_DEFINITIONS HAVE_SUBTITLE)
ENDIF()

IF(HAVE_CTC)
    LIST(APPEND AML_MP_DEFINITIONS HAVE_CTC)
ENDIF()
#OPTION SETTING DONE

SET(AML_MP_SRCS
    ${AML_MP_PLAYER_SRC}
    ${AML_MP_DVR_SRC}
    ${AML_MP_UTILS_SRC}
    ${AML_MP_DEMUX_SRC}
    ${AML_MP_CAS_SRC}
)

SET(AML_MP_PUBLIC_HEADERS
    include/Aml_MP/Aml_MP.h
    include/Aml_MP/Cas.h
    include/Aml_MP/Common.h
    include/Aml_MP/Dvr.h
    include/Aml_MP/Aml_MediaPlayer.h
)

FIND_PATH(libdvr_INCLUDE_DIRS dvr_wrapper.h PATH_SUFFIXES libdvr REQUIRED)

SET(TARGET aml_mp_sdk)

ADD_LIBRARY(${TARGET} SHARED ${AML_MP_SRCS})

TARGET_INCLUDE_DIRECTORIES(${TARGET} PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}
    ${libdvr_INCLUDE_DIRS}
)

TARGET_COMPILE_OPTIONS(${TARGET} PUBLIC -std=c++17)

TARGET_COMPILE_DEFINITIONS(${TARGET} PUBLIC __unused= )
TARGET_COMPILE_DEFINITIONS(${TARGET} PUBLIC ${AML_MP_DEFINITIONS})

TARGET_LINK_LIBRARIES(${TARGET} PUBLIC
    mediahal_tsplayer
    amdvr
    log
    cutils
    pthread
    dl
)

INSTALL(
    TARGETS ${TARGET}
)

INSTALL(
    FILES ${AML_MP_PUBLIC_HEADERS} DESTINATION include/Aml_MP
)

ADD_SUBDIRECTORY(tests/amlMpTestSupporter)
ADD_SUBDIRECTORY(tests/amlMpPlayerDemo)
ADD_SUBDIRECTORY(tests/amlMpMediaPlayerDemo)
ADD_SUBDIRECTORY(tests/unitTest)
ADD_SUBDIRECTORY(mediaplayer)









