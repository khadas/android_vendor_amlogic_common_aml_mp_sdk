cmake_minimum_required (VERSION 2.8)

project(aml_mp_mediaplayer)

SET(AML_MP_MEDIAPLAYER_SRC
    Aml_MP_MediaPlayer.cpp
    Aml_MP_MediaPlayerImpl.cpp
    AmlMediaPlayerBase.cpp
)

SET(AML_MP_SRCS
    ${AML_MP_MEDIAPLAYER_SRC}
)

SET(AML_MP_MEDIAPLAYER_PRIVATE_HEADERS
    AmlMediaPlayerBase.h
)

FIND_PATH(libdvr_INCLUDE_DIRS dvr_wrapper.h PATH_SUFFIXES libdvr REQUIRED)

#find_library(libdrmp_LIBRARY drmp)
#FIND_PATH( libdrmp_INCLUDE_DIR libavformat/mpegts.h
#    PATH_SUFFIXES ffmpeg-vendor
#)

SET(TARGET aml_mp_mediaplayer)

ADD_LIBRARY(${TARGET} SHARED ${AML_MP_SRCS})

TARGET_INCLUDE_DIRECTORIES(${TARGET} PUBLIC
    ${PROJECT_SOURCE_DIR}/../include
    ${PROJECT_SOURCE_DIR}/../
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
    aml_mp_sdk
)

INSTALL(
    TARGETS ${TARGET}
)

INSTALL(
    FILES ${AML_MP_MEDIAPLAYER_PRIVATE_HEADERS} DESTINATION include/Aml_MP/private
)


