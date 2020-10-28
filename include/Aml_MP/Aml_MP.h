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
int Aml_MP_Initialize();

int Aml_MP_GetVersion(const char** versionString);

int Aml_MP_SetDemuxSource(Aml_MP_DemuxId demuxId, Aml_MP_DemuxSource source);

int Aml_MP_GetDemuxSource(Aml_MP_DemuxId demuxId, Aml_MP_DemuxSource *source);

///////////////////////////////////////////////////////////////////////////////
//                                  Player                                   //
///////////////////////////////////////////////////////////////////////////////
typedef struct {
    int                     userId;
    Aml_MP_DemuxId          demuxId;
    Aml_MP_InputSourceType  sourceType;
    Aml_MP_InputStreamType  drmMode;
} Aml_MP_PlayerCreateParams;

///////////////////////////////////////////////////////////////////////////////
//********************* BASIC INTERFACES BEGIN **************************/
/**
 * \brief create player
 *
 * \return 0 if success
 */
int Aml_MP_Player_Create(Aml_MP_PlayerCreateParams* createParams, AML_MP_PLAYER* handle);

/**
 * \brief destroy player
 *
 * \param handle
 *
 * \return 0 if success
 */
int Aml_MP_Player_Destroy(AML_MP_PLAYER handle);

int Aml_MP_Player_RegisterEventCallBack(AML_MP_PLAYER handle, Aml_MP_PlayerEventCallback cb, void* userData);

int Aml_MP_Player_SetVideoParams(AML_MP_PLAYER handle, Aml_MP_VideoParams* params);

int Aml_MP_Player_SetAudioParams(AML_MP_PLAYER handle, Aml_MP_AudioParams* params);

int Aml_MP_Player_SetSubtitleParams(AML_MP_PLAYER handle, Aml_MP_SubtitleParams* params);

int Aml_MP_Player_SetCASParams(AML_MP_PLAYER handle, Aml_MP_CASParams* params);

int Aml_MP_Player_Start(AML_MP_PLAYER handle);

int Aml_MP_Player_Stop(AML_MP_PLAYER handle);

int Aml_MP_Player_Pause(AML_MP_PLAYER handle);

int Aml_MP_Player_Resume(AML_MP_PLAYER handle);

int Aml_MP_Player_Flush(AML_MP_PLAYER handle);

int Aml_MP_Player_SetPlaybackRate(AML_MP_PLAYER handle, float rate);

int Aml_MP_Player_SwitchAudioTrack(AML_MP_PLAYER handle, Aml_MP_AudioParams* params);

int Aml_MP_Player_SwitchSubtitleTrack(AML_MP_PLAYER handle, Aml_MP_SubtitleParams* params);

int Aml_MP_Player_WriteData(AML_MP_PLAYER handle, const uint8_t* buffer, size_t size);

int Aml_MP_Player_WriteEsData(AML_MP_PLAYER handle, Aml_MP_StreamType streamType, const uint8_t* buffer, size_t size, int64_t pts);

int Aml_MP_Player_GetCurrentPts(AML_MP_PLAYER handle, Aml_MP_StreamType streamType, int64_t* pts);

int Aml_MP_Player_GetBufferStat(AML_MP_PLAYER handle, Aml_MP_BufferStat* bufferStat);

int Aml_MP_Player_SetANativeWindow(AML_MP_PLAYER handle, void* nativeWindow);

int Aml_MP_Player_SetVideoWindow(AML_MP_PLAYER handle, int32_t x, int32_t y, int32_t width, int32_t height);

int Aml_MP_Player_SetVolume(AML_MP_PLAYER handle, float volume);

int Aml_MP_Player_GetVolume(AML_MP_PLAYER handle, float* volume);

int Aml_MP_Player_ShowVideo(AML_MP_PLAYER handle);

int Aml_MP_Player_HideVideo(AML_MP_PLAYER handle);

int Aml_MP_Player_ShowSubtitle(AML_MP_PLAYER handle);

int Aml_MP_Player_HideSubtitle(AML_MP_PLAYER handle);

int Aml_MP_Player_SetParameter(AML_MP_PLAYER handle, Aml_MP_PlayerParameterKey key, void* parameter);

int Aml_MP_Player_GetParameter(AML_MP_PLAYER handle, Aml_MP_PlayerParameterKey key, void* parameter);

int Aml_MP_Player_SetSubtitleWindow(AML_MP_PLAYER handle, int x, int y, int width, int height);

//********************* BASIC INTERFACES END **************************/

//sync
int Aml_MP_Player_SetAVSyncSource(AML_MP_PLAYER handle, Aml_MP_AVSyncSource syncSource);

int Aml_MP_Player_SetPcrPid(AML_MP_PLAYER handle, int pid);

//video
int Aml_MP_Player_StartVideoDecoding(AML_MP_PLAYER handle);

int Aml_MP_Player_StopVideoDecoding(AML_MP_PLAYER handle);

//audio
int Aml_MP_Player_StartAudioDecoding(AML_MP_PLAYER handle);

int Aml_MP_Player_StopAudioDecoding(AML_MP_PLAYER handle);

//subtitle
int Aml_MP_Player_StartSubtitleDecoding(AML_MP_PLAYER handle);

int Aml_MP_Player_StopSubtitleDecoding(AML_MP_PLAYER handle);

//CAS
//int Aml_MP_Player_StartDescrambling(AML_MP_PLAYER handle);

//int Aml_MP_Player_StopDescrambling(AML_MP_PLAYER handle);

//AD
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
