/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _AML_MP_H_
#define _AML_MP_H_

/**
 * all interfaces need to be thread safe and nonblocking.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "Common.h"

///////////////////////////////////////////////////////////////////////////////
//                                  Aml_MP                                   //
///////////////////////////////////////////////////////////////////////////////
/**
 * \brief Aml_MP_Initialize
 * Initialize Aml MP Player
 *
 * \return 0 if success
 */
int Aml_MP_Initialize();

/**
 * \brief Aml_MP_GetVersion
 * Get Aml MP SDK version
 *
 * \param [out] Aml MP SDK version string
 *
 * \return Aml MP SDK version int
 */
int Aml_MP_GetVersion(const char** versionString);

/**
 * \brief Aml_MP_SetDemuxSource
 * Set demux source for Aml MP
 *
 * \param [in]  demux id
 * \param [in]  demux source
 *
 * \return 0 if success
 */
int Aml_MP_SetDemuxSource(Aml_MP_DemuxId demuxId, Aml_MP_DemuxSource source);

/**
 * \brief Aml_MP_GetDemuxSource
 * Get demux source form Aml MP
 *
 * \param [in]  demux id
 * \param [out] demux source
 *
 * \return 0 if success
 */
int Aml_MP_GetDemuxSource(Aml_MP_DemuxId demuxId, Aml_MP_DemuxSource *source);

///////////////////////////////////////////////////////////////////////////////
//                                  Player                                   //
///////////////////////////////////////////////////////////////////////////////
typedef struct {
    Aml_MP_ChannelId        channelId;
    Aml_MP_DemuxId          demuxId;
    Aml_MP_InputSourceType  sourceType;
    Aml_MP_InputStreamType  drmMode;
} Aml_MP_PlayerCreateParams;

///////////////////////////////////////////////////////////////////////////////
//********************* BASIC INTERFACES BEGIN **************************/
/**
 * \brief Aml_MP_Player_Create
 * Create player
 *
 * \param [in]  player create param
 * \param [out] player handle
 *
 * \return 0 if success
 */
int Aml_MP_Player_Create(Aml_MP_PlayerCreateParams* createParams, AML_MP_PLAYER* handle);

/**
 * \brief Aml_MP_Player_Destroy
 * Destroy player
 *
 * \param [in]  player handle
 *
 * \return 0 if success
 */
int Aml_MP_Player_Destroy(AML_MP_PLAYER handle);

/**
 * \brief Aml_MP_Player_RegisterEventCallBack
 * Register event callback function
 *
 * \param [in]  player handle
 * \param [in]  callback function
 * \param [in]  user data
 *
 * \return 0 if success
 */
int Aml_MP_Player_RegisterEventCallBack(AML_MP_PLAYER handle, Aml_MP_PlayerEventCallback cb, void* userData);

/**
 * \brief Aml_MP_Player_SetVideoParams
 * Set video params
 *
 * \param [in]  player handle
 * \param [in]  video params
 *
 * \return 0 if success
 */
int Aml_MP_Player_SetVideoParams(AML_MP_PLAYER handle, Aml_MP_VideoParams* params);

/**
 * \brief Aml_MP_Player_SetAudioParams
 * Set audio params
 *
 * \param [in]  player handle
 * \param [in]  audio params
 *
 * \return 0 if success
 */
int Aml_MP_Player_SetAudioParams(AML_MP_PLAYER handle, Aml_MP_AudioParams* params);

/**
 * \brief Aml_MP_Player_SetSubtitleParams
 * Set subtitle params
 *
 * \param [in]  player handle
 * \param [in]  subtitle params
 *
 * \return 0 if success
 */
int Aml_MP_Player_SetSubtitleParams(AML_MP_PLAYER handle, Aml_MP_SubtitleParams* params);

/**
 * \brief Aml_MP_Player_SetCASParams
 * Set CAS params
 * Mainly used for iptv
 *
 * \param [in]  player handle
 * \param [in]  CAS params
 *
 * \return 0 if success
 */
int Aml_MP_Player_SetCASParams(AML_MP_PLAYER handle, Aml_MP_CASParams* params);

/**
 * \brief Aml_MP_Player_Start
 * start player
 * Please make sure have set need video, audio and subtitle params before call this function.
 *
 * \param [in]  player handle
 *
 * \return 0 if success
 */
int Aml_MP_Player_Start(AML_MP_PLAYER handle);

/**
 * \brief Aml_MP_Player_Stop
 * Stop player
 *
 * \param [in]  player handle
 *
 * \return 0 if success
 */
int Aml_MP_Player_Stop(AML_MP_PLAYER handle);

/**
 * \brief Aml_MP_Player_Pause
 * Pause player
 *
 * \param [in]  player handle
 *
 * \return 0 if success
 */
int Aml_MP_Player_Pause(AML_MP_PLAYER handle);

/**
 * \brief Aml_MP_Player_Resume
 * Resume player
 *
 * \param [in]  player handle
 *
 * \return 0 if success
 */
int Aml_MP_Player_Resume(AML_MP_PLAYER handle);

/**
 * \brief Aml_MP_Player_Flush
 * Flush player
 * This function is not implement, not recommand to use it
 *
 * \param [in]  player handle
 *
 * \return 0 if success
 */
int Aml_MP_Player_Flush(AML_MP_PLAYER handle);

/**
 * \brief Aml_MP_Player_SetPlaybackRate
 * Set playback rate
 *
 * \param [in]  player handle
 * \param [in]  play rate
 *
 * \return 0 if success
 */
int Aml_MP_Player_SetPlaybackRate(AML_MP_PLAYER handle, float rate);

/**
 * \brief Aml_MP_Player_SwitchAudioTrack
 * Switch audio track
 *
 * \param [in]  player handle
 * \param [in]  params for new audio track
 *
 * \return 0 if success
 */
int Aml_MP_Player_SwitchAudioTrack(AML_MP_PLAYER handle, Aml_MP_AudioParams* params);

/**
 * \brief Aml_MP_Player_SwitchSubtitleTrack
 * Switch aubtitle track
 *
 * \param [in]  player handle
 * \param [in]  params for new subtitle track
 *
 * \return 0 if success
 */
int Aml_MP_Player_SwitchSubtitleTrack(AML_MP_PLAYER handle, Aml_MP_SubtitleParams* params);

/**
 * \brief Aml_MP_Player_WriteData
 * Write TS data to player
 *
 * \param [in]  player handle
 * \param [in]  TS data
 * \param [in]  TS data size
 *
 * \return num of byte be writed if success
 * \return negative number if fail
 */
int Aml_MP_Player_WriteData(AML_MP_PLAYER handle, const uint8_t* buffer, size_t size);

/**
 * \brief Aml_MP_Player_WriteEsData
 * Write ES data to player
 *
 * \param [in]  player handle
 * \param [in]  ES data type
 * \param [in]  ES data
 * \param [in]  ES data size
 * \param [in]  ES data timestamp (optional)
 *
 * \return num of byte be writed if success
 * \return negative number if fail
 */
int Aml_MP_Player_WriteEsData(AML_MP_PLAYER handle, Aml_MP_StreamType streamType, const uint8_t* buffer, size_t size, int64_t pts);

/**
 * \brief Aml_MP_Player_GetCurrentPts
 * Get current pts
 *
 * \param [in]  player handle
 * \param [in]  stream type
 * \param [out] timestamp
 *
 * \return 0 if success
 */
int Aml_MP_Player_GetCurrentPts(AML_MP_PLAYER handle, Aml_MP_StreamType streamType, int64_t* pts);

/**
 * \brief Aml_MP_Player_GetBufferStat
 * Get buffer status
 *
 * \param [in]  player handle
 * \param [out] buffer states
 *
 * \return 0 if success
 */
int Aml_MP_Player_GetBufferStat(AML_MP_PLAYER handle, Aml_MP_BufferStat* bufferStat);

/**
 * \brief Aml_MP_Player_SetANativeWindow
 * Set nativeWindow
 * For Android platform, nativeWindoe is pointer to ANativeWindow
 *
 * \param [in]  player handle
 * \param [in]  nativeWindow handle
 *
 * \return 0 if success
 */
int Aml_MP_Player_SetANativeWindow(AML_MP_PLAYER handle, void* nativeWindow);

/**
 * \brief Aml_MP_Player_SetVideoWindow
 * Set video window.
 * Conflict with Aml_MP_Player_GetBufferStat, if set nativeWindow, video window set will not effect.
 *
 * \param [in]  player handle
 * \param [in]  video location (x)
 * \param [in]  video location (y)
 * \param [in]  video width
 * \param [in]  video height
 *
 * \return 0 if success
 */
int Aml_MP_Player_SetVideoWindow(AML_MP_PLAYER handle, int32_t x, int32_t y, int32_t width, int32_t height);

/**
 * \brief Aml_MP_Player_SetVolume
 * Set volume
 *
 * \param [in]  player handle
 * \param [in]  volume [0.0 ~ 100.0]
 *
 * \return 0 if success
 */
int Aml_MP_Player_SetVolume(AML_MP_PLAYER handle, float volume);

/**
 * \brief Aml_MP_Player_GetVolume
 * Get volume
 *
 * \param [in]  player handle
 * \param [out] volume [0.0 ~ 100.0]
 *
 * \return 0 if success
 */
int Aml_MP_Player_GetVolume(AML_MP_PLAYER handle, float* volume);

/**
 * \brief Aml_MP_Player_ShowVideo
 * Show video
 *
 * \param [in]  player handle
 *
 * \return 0 if success
 */
int Aml_MP_Player_ShowVideo(AML_MP_PLAYER handle);

/**
 * \brief Aml_MP_Player_HideVideo
 * Hide video
 *
 * \param [in]  player handle
 *
 * \return 0 if success
 */
int Aml_MP_Player_HideVideo(AML_MP_PLAYER handle);

/**
 * \brief Aml_MP_Player_ShowSubtitle
 * Show subtitle
 *
 * \param [in]  player handle
 *
 * \return 0 if success
 */
int Aml_MP_Player_ShowSubtitle(AML_MP_PLAYER handle);

/**
 * \brief Aml_MP_Player_HideSubtitle
 * Hide subtitle
 *
 * \param [in]  player handle
 *
 * \return 0 if success
 */
int Aml_MP_Player_HideSubtitle(AML_MP_PLAYER handle);

/**
 * \brief Aml_MP_Player_SetParameter
 * Set expand player parameter
 *
 * \param [in]  player handle
 * \param [in]  player expand parameter key
 * \param [in]  player expand parameter value
 *
 * \return 0 if success
 */
int Aml_MP_Player_SetParameter(AML_MP_PLAYER handle, Aml_MP_PlayerParameterKey key, void* parameter);

/**
 * \brief Aml_MP_Player_GetParameter
 * Get expand player parameter
 *
 * \param [in]  player handle
 * \param [in]  player expand parameter key
 * \param [out] player expand parameter value
 *
 * \return 0 if success
 */
int Aml_MP_Player_GetParameter(AML_MP_PLAYER handle, Aml_MP_PlayerParameterKey key, void* parameter);

/**
 * \brief Aml_MP_Player_SetSubtitleWindow
 * Set subtitle window.
 *
 * \param [in]  player handle
 * \param [in]  subtitle location (x)
 * \param [in]  subtitle location (y)
 * \param [in]  subtitle width
 * \param [in]  subtitle height
 *
 * \return 0 if success
 */
int Aml_MP_Player_SetSubtitleWindow(AML_MP_PLAYER handle, int x, int y, int width, int height);

//********************* BASIC INTERFACES END **************************/

//sync
/**
 * \brief Aml_MP_Player_SetAVSyncSource
 * Set AV sync clock source
 *
 * \param [in]  player handle
 * \param [in]  sync clock source
 *
 * \return 0 if success
 */
int Aml_MP_Player_SetAVSyncSource(AML_MP_PLAYER handle, Aml_MP_AVSyncSource syncSource);

/**
 * \brief Aml_MP_Player_SetPcrPid
 * Set pcr PID
 *
 * \param [in]  player handle
 * \param [in]  pcr PID
 *
 * \return 0 if success
 */
int Aml_MP_Player_SetPcrPid(AML_MP_PLAYER handle, int pid);

//video
/**
 * \brief Aml_MP_Player_StartVideoDecoding
 * Start video decoding
 *
 * \param [in]  player handle
 *
 * \return 0 if success
 */
int Aml_MP_Player_StartVideoDecoding(AML_MP_PLAYER handle);

/**
 * \brief Aml_MP_Player_StopVideoDecoding
 * Stop video decoding
 *
 * \param [in]  player handle
 *
 * \return 0 if success
 */
int Aml_MP_Player_StopVideoDecoding(AML_MP_PLAYER handle);

//audio
/**
 * \brief Aml_MP_Player_StartAudioDecoding
 * Start audio decoding
 *
 * \param [in]  player handle
 *
 * \return 0 if success
 */
int Aml_MP_Player_StartAudioDecoding(AML_MP_PLAYER handle);

/**
 * \brief Aml_MP_Player_StopAudioDecoding
 * Stop audio decoding
 *
 * \param [in]  player handle
 *
 * \return 0 if success
 */
int Aml_MP_Player_StopAudioDecoding(AML_MP_PLAYER handle);

//subtitle
/**
 * \brief Aml_MP_Player_StartSubtitleDecoding
 * Start subtitle decoding
 *
 * \param [in]  player handle
 *
 * \return 0 if success
 */
int Aml_MP_Player_StartSubtitleDecoding(AML_MP_PLAYER handle);

/**
 * \brief Aml_MP_Player_StopSubtitleDecoding
 * Stop subtitle decoding
 *
 * \param [in]  player handle
 *
 * \return 0 if success
 */
int Aml_MP_Player_StopSubtitleDecoding(AML_MP_PLAYER handle);

//AD
/**
 * \brief Aml_MP_Player_SetADParams
 * Set AD params
 * In some case, player will not support AD
 *
 * \param [in]  player handle
 * \param [in]  AD params
 *
 * \return 0 if success
 */
int Aml_MP_Player_SetADParams(AML_MP_PLAYER handle, Aml_MP_AudioParams* params);

///////////////////////////////////////////////////////////////////////////////
//                                  CAS                                      //
///////////////////////////////////////////////////////////////////////////////
#include "Cas.h"

///////////////////////////////////////////////////////////////////////////////
//                                  DVR                                      //
///////////////////////////////////////////////////////////////////////////////
#include "Dvr.h"

#ifdef __cplusplus
}
#endif

#endif
