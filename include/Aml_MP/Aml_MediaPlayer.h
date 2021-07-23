/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _AML_MP_MEDIAPLAYER_H_
#define _AML_MP_MEDIAPLAYER_H_

/**
 * all interfaces need to be thread safe and nonblocking.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "Common.h"
///////////////////////////////////////////////////////////////////////////////
//                                  Player                                   //
///////////////////////////////////////////////////////////////////////////////
typedef void* AML_MP_MEDIAPLAYER;

typedef Aml_MP_PlayerEventType Aml_MP_MediaPlayerEventType;
typedef void (*Aml_MP_MediaPlayerEventCallback)(void* userData, Aml_MP_MediaPlayerEventType event, int64_t param);


///////////////////////////////////////////////////////////////////////////////
#define AML_MP_MAX_STREAM_PARAMETER_SIZE 64
#define AML_MP_MAX_STREAMS_COUNT 16
#define AML_MP_MAX_MEDIA_PARAMETER_SIZE 256

typedef struct {
    uint16_t                id;
    Aml_MP_CodecID          videoCodec;
    uint32_t                width;
    uint32_t                height;
    char mine[AML_MP_MAX_STREAM_PARAMETER_SIZE];
} Aml_MP_VideoTrackInfo;

typedef struct {
    uint16_t                id;
    Aml_MP_CodecID          audioCodec;
    uint32_t                nChannels;
    uint32_t                nSampleRate;
    char mine[AML_MP_MAX_STREAM_PARAMETER_SIZE];
    char language[AML_MP_MAX_STREAM_PARAMETER_SIZE];
} Aml_MP_AudioTrackInfo;

typedef struct {
    uint16_t                id;
    Aml_MP_CodecID subtitleCodec;
    char mine[AML_MP_MAX_STREAM_PARAMETER_SIZE];
    char language[AML_MP_MAX_STREAM_PARAMETER_SIZE];
} Aml_MP_SubTrackInfo;

typedef struct {
    Aml_MP_StreamType streamType;
    union {
        Aml_MP_VideoTrackInfo vInfo;
        Aml_MP_AudioTrackInfo aInfo;
        Aml_MP_SubTrackInfo   sInfo;
    };
} Aml_MP_StreamInfo;

typedef enum {
    AML_MP_MEDIAPLAYER_INVOKE_ID_BASE = 0x100,
    AML_MP_MEDIAPLAYER_INVOKE_ID_GET_TRACK_INFO,            //getTrackInfo(..., Aml_MP_TrackInfo)
    AML_MP_MEDIAPLAYER_INVOKE_ID_GET_MEDIA_INFO,            //getMediaInfo(..., Aml_MP_MediaInfo)
    AML_MP_MEDIAPLAYER_INVOKE_ID_SELECT_TRACK,              //setSelectTrack(int32_t, ...)
    AML_MP_MEDIAPLAYER_INVOKE_ID_UNSELECT_TRACK,            //setUnselectTrack(int32_t, ...)
} Aml_MP_MediaPlayerInvokeID;

////////////////////////////////////////
//AML_MP_MEDIAPLAYER_INVOKE_ID_GET_TRACK_INFO
typedef struct {
    int nb_streams;
    Aml_MP_StreamInfo streamInfo[AML_MP_MAX_STREAMS_COUNT];
}Aml_MP_TrackInfo;

////////////////////////////////////////
//AML_MP_MEDIAPLAYER_INVOKE_ID_GET_MEDIA_INFO
typedef struct {
    char    filename[AML_MP_MAX_MEDIA_PARAMETER_SIZE];
    int64_t duration;
    int64_t file_size;
    int64_t bitrate;
    int     nb_streams;
}Aml_MP_MediaInfo;

////////////////////////////////////////
//Aml_MP_MediaPlayerInvokeRequest
typedef struct {
    Aml_MP_MediaPlayerInvokeID requestId;
    union {
    int32_t data32;
    int64_t data64;
    }u;
} Aml_MP_MediaPlayerInvokeRequest;

////////////////////////////////////////
//Aml_MP_MediaPlayerInvokeReply
typedef struct {
    Aml_MP_MediaPlayerInvokeID requestId;
    union {
        int32_t data32;
        int64_t data64;
        Aml_MP_TrackInfo trackInfo;
        Aml_MP_MediaInfo mediaInfo;
    }u;
} Aml_MP_MediaPlayerInvokeReply;

///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
//********************* BASIC INTERFACES BEGIN **************************/
/**
 * \brief Aml_MP_MediaPlayer_Create
 * Create player
 *
 * \param [out] player handle
 *
 * \return 0 if success
 */
int Aml_MP_MediaPlayer_Create(AML_MP_MEDIAPLAYER* handle);

/**
 * \brief Aml_MP_MediaPlayer_Destroy
 * Release and destroy player
 *
 * \param [in]  player handle
 *
 * \return 0 if success
 */
int Aml_MP_MediaPlayer_Destroy(AML_MP_MEDIAPLAYER handle);

/**
 * \brief Aml_MP_MediaPlayer_SetVideoWindow
 * Set video window.
 * if set nativeWindow, video window set will not effect.
 *
 * \param [in]  player handle
 * \param [in]  video location (x)
 * \param [in]  video location (y)
 * \param [in]  video width
 * \param [in]  video height
 *
 * \return 0 if success
 */
int Aml_MP_MediaPlayer_SetVideoWindow(AML_MP_MEDIAPLAYER handle, int32_t x, int32_t y, int32_t width, int32_t height);

/**
 * \brief Aml_MP_MediaPlayer_SetDataSource
 * Set data source.
 *
 * \param [in]  player handle
 * \param [in]  playback url
 *
 * \return 0 if success
 */
int Aml_MP_MediaPlayer_SetDataSource(AML_MP_MEDIAPLAYER handle, const char *uri);

/**
 * \brief Aml_MP_MediaPlayer_Prepare
 * Prepare player in sync mode
 *
 * \param [in]  player handle
 *
 * \return 0 if success
 */
int Aml_MP_MediaPlayer_Prepare(AML_MP_MEDIAPLAYER handle);

/**
 * \brief Aml_MP_MediaPlayer_PrepareAsync
 * Prepare player in async mode
 *
 * \param [in]  player handle
 *
 * \return 0 if success
 */
int Aml_MP_MediaPlayer_PrepareAsync(AML_MP_MEDIAPLAYER handle);

/**
 * \brief Aml_MP_MediaPlayer_Start
 * Start player
 * Please make sure have set playback url call this function.
 *
 * \param [in]  player handle
 *
 * \return 0 if success
 */
int Aml_MP_MediaPlayer_Start(AML_MP_MEDIAPLAYER handle);

/**
 * \brief Aml_MP_MediaPlayer_SeekTo
 * Seek player
 *
 * \param [in]  player handle
 * \param [in]  target time
 *
 * \return 0 if success
 */
int Aml_MP_MediaPlayer_SeekTo(AML_MP_MEDIAPLAYER handle, int msec);

/**
 * \brief Aml_MP_MediaPlayer_Pause
 * Pause player
 *
 * \param [in]  player handle
 *
 * \return 0 if success
 */
int Aml_MP_MediaPlayer_Pause(AML_MP_MEDIAPLAYER handle);

/**
 * \brief Aml_MP_MediaPlayer_Resume
 * Resume player
 *
 * \param [in]  player handle
 *
 * \return 0 if success
 */
int Aml_MP_MediaPlayer_Resume(AML_MP_MEDIAPLAYER handle);

/**
 * \brief Aml_MP_MediaPlayer_Stop
 * Stop player
 *
 * \param [in]  player handle
 *
 * \return 0 if success
 */
int Aml_MP_MediaPlayer_Stop(AML_MP_MEDIAPLAYER handle);

/**
 * \brief Aml_MP_MediaPlayer_Reset
 * Reset player
 *
 * \param [in]  player handle
 *
 * \return 0 if success
 */
int Aml_MP_MediaPlayer_Reset(AML_MP_MEDIAPLAYER handle);

/**
 * \brief Aml_MP_MediaPlayer_GetCurrentPosition
 * Get current position
 *
 * \param [in]   player handle
 * \param [out]  current position
 *
 * \return 0 if success
 */
int Aml_MP_MediaPlayer_GetCurrentPosition(AML_MP_MEDIAPLAYER handle, int* msec);

/**
 * \brief Aml_MP_MediaPlayer_GetDuration
 * Get duration
 *
 * \param [in]   player handle
 * \param [out]  duration
 *
 * \return 0 if success
 */
int Aml_MP_MediaPlayer_GetDuration(AML_MP_MEDIAPLAYER handle, int* msec);

/**
 * \brief Aml_MP_MediaPlayer_Invoke
 * Invoke a generic method on the player by using opaque parcels
 *
 * \param [in]   player handle
 * \param [in]   request value
 * \param [out]  reply value
 *
 * \return 0 if success
 */
int Aml_MP_MediaPlayer_Invoke(AML_MP_MEDIAPLAYER handle, Aml_MP_MediaPlayerInvokeRequest* request, Aml_MP_MediaPlayerInvokeReply* reply);

/**
 * \brief Aml_MP_MediaPlayer_RegisterEventCallBack
 * Register event callback function
 *
 * \param [in]  player handle
 * \param [in]  callback function
 * \param [in]  user data
 *
 * \return 0 if success
 */
int Aml_MP_MediaPlayer_RegisterEventCallBack(AML_MP_MEDIAPLAYER handle, Aml_MP_MediaPlayerEventCallback cb, void* userData);

#ifdef __cplusplus
}
#endif

#endif
