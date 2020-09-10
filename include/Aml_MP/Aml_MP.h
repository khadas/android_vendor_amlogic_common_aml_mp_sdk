/*
 * Copyright (C) 2020 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
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

int Aml_MP_IsCASystemIdSupported(int caSystemId);

///////////////////////////////////////////////////////////////////////////////
//                                  Player                                   //
///////////////////////////////////////////////////////////////////////////////
typedef struct {
    int                     userId;
    Aml_MP_DemuxId          demuxId;
    Aml_MP_InputSourceType  sourceType;
    Aml_MP_InputStreamType  drmMode;
} Aml_MP_PlayerCreateParams;

typedef enum {
    AML_MP_PLAYER_EVENT_PTS  = 0,
    AML_MP_PLAYER_EVENT_DTV_SUBTITLE,
    AML_MP_PLAYER_EVENT_USERDATA_AFD,
    AML_MP_PLAYER_EVENT_USERDATA_CC,
    AML_MP_PLAYER_EVENT_VIDEO_CHANGED,
    AML_MP_PLAYER_EVENT_AUDIO_CHANGED,
    AML_MP_PLAYER_EVENT_DATA_LOSS,
    AML_MP_PLAYER_EVENT_DATA_RESUME,    // demod data resume
    AML_MP_PLAYER_EVENT_SCRAMBLING,     // scrambling status changed
    AML_MP_PLAYER_EVENT_FIRST_FRAME,     // first video frame showed
    AML_MP_PLAYER_EVENT_STREAM_MODE_EOF, //endof stream mode
    AML_MP_PLAYER_EVENT_DECODE_FIRST_FRAME_VIDEO, //The video decoder outputs the first frame.
    AML_MP_PLAYER_EVENT_DECODE_FIRST_FRAME_AUDIO, //The audio decoder outputs the first frame.
    AML_MP_PLAYER_EVENT_AV_SYNC_DONE //av sync done
} Aml_MP_PlayerEventType;

typedef struct {
    uint32_t frame_width;
    uint32_t frame_height;
    uint32_t frame_rate;
    uint32_t frame_aspectratio;
} Aml_MP_PlayerEventVideoFormat;

typedef struct {
    uint32_t sample_rate;
    uint32_t channels;
} Aml_MP_PlayerEventAudioFormat;

typedef struct {
	Aml_MP_StreamType type;
	int64_t pts;
} Aml_MP_PlayerEventPts;

typedef struct {
	uint8_t  *data;
    size_t   len;
} Aml_MP_PlayerEventUserData;

typedef struct {
	Aml_MP_StreamType type;
	char scramling;
} Aml_MP_PlayerEventScrambling;

typedef struct {
	Aml_MP_PlayerEventType type;
    union {
        Aml_MP_PlayerEventVideoFormat videoFormat;
        Aml_MP_PlayerEventAudioFormat audioFormat;
        Aml_MP_PlayerEventPts pts;
        Aml_MP_PlayerEventUserData userData;
        Aml_MP_PlayerEventScrambling scrambling;
    } event;
} Aml_MP_PlayerEvent;

typedef void (*Aml_MP_PlayerEventCallback)(void* userData, Aml_MP_PlayerEvent* event);

///////////////////////////////////////////////////////////////////////////////

//********************* BASIC INTERFACES BEGIN **************************/
/**
 * \brief create player
 *
 * \return 0 if success
 */
int Aml_MP_Player_Create(Aml_MP_PlayerCreateParams* createParams, AML_MP_HANDLE* handle);

/**
 * \brief destroy player
 *
 * \param handle
 *
 * \return 0 if success
 */
int Aml_MP_Player_Destroy(AML_MP_HANDLE handle);

int Aml_MP_Player_RegisterEventCallBack(AML_MP_HANDLE handle, Aml_MP_PlayerEventCallback cb, void* userData);

int Aml_MP_Player_SetVideoParams(AML_MP_HANDLE handle, Aml_MP_VideoParams* params);

int Aml_MP_Player_SetAudioParams(AML_MP_HANDLE handle, Aml_MP_AudioParams* params);

int Aml_MP_Player_SetSubtitleParams(AML_MP_HANDLE handle, Aml_MP_SubtitleParams* params);

int Aml_MP_Player_SetCASParams(AML_MP_HANDLE handle, Aml_MP_CASParams* params);

int Aml_MP_Player_Start(AML_MP_HANDLE handle);

int Aml_MP_Player_Stop(AML_MP_HANDLE handle);

int Aml_MP_Player_Pause(AML_MP_HANDLE handle);

int Aml_MP_Player_Resume(AML_MP_HANDLE handle);

int Aml_MP_Player_Flush(AML_MP_HANDLE handle);

int Aml_MP_Player_SetPlaybackRate(AML_MP_HANDLE handle, float rate);

int Aml_MP_Player_SwitchAudioTrack(AML_MP_HANDLE handle, Aml_MP_AudioParams* params);

int Aml_MP_Player_SwitchSubtitleTrack(AML_MP_HANDLE handle, Aml_MP_SubtitleParams* params);

int Aml_MP_Player_WriteData(AML_MP_HANDLE handle, const uint8_t* buffer, size_t size);

int Aml_MP_Player_WriteEsData(AML_MP_HANDLE handle, Aml_MP_StreamType streamType, const uint8_t* buffer, size_t size, int64_t pts);

int Aml_MP_Player_GetCurrentPts(AML_MP_HANDLE handle, Aml_MP_StreamType streamType, int64_t* pts);

int Aml_MP_Player_GetBufferStat(AML_MP_HANDLE handle, Aml_MP_BufferStat* bufferStat);

int Aml_MP_Player_SetANativeWindow(AML_MP_HANDLE handle, void* nativeWindow);

int Aml_MP_Player_SetVideoWindow(AML_MP_HANDLE handle, int32_t x, int32_t y, int32_t width, int32_t height);

int Aml_MP_Player_SetVolume(AML_MP_HANDLE handle, float volume);

int Aml_MP_Player_GetVolume(AML_MP_HANDLE handle, float* volume);

int Aml_MP_Player_ShowVideo(AML_MP_HANDLE handle);

int Aml_MP_Player_HideVideo(AML_MP_HANDLE handle);

int Aml_MP_Player_ShowSubtitle(AML_MP_HANDLE handle);

int Aml_MP_Player_HideSubtitle(AML_MP_HANDLE handle);

int Aml_MP_Player_SetParameter(AML_MP_HANDLE handle, Aml_MP_PlayerParameterKey key, void* parameter);

int Aml_MP_Player_GetParameter(AML_MP_HANDLE handle, Aml_MP_PlayerParameterKey key, void* parameter);

int Aml_MP_Player_SetSubtitleWindow(AML_MP_HANDLE handle, int x, int y, int width, int height);

//********************* BASIC INTERFACES END **************************/

//sync
int Aml_MP_Player_SetAVSyncSource(AML_MP_HANDLE handle, Aml_MP_AVSyncSource syncSource);

int Aml_MP_Player_SetPcrPid(AML_MP_HANDLE handle, int pid);

//video
int Aml_MP_Player_StartVideoDecoding(AML_MP_HANDLE handle);

int Aml_MP_Player_StopVideoDecoding(AML_MP_HANDLE handle);

//audio
int Aml_MP_Player_StartAudioDecoding(AML_MP_HANDLE handle);

int Aml_MP_Player_StopAudioDecoding(AML_MP_HANDLE handle);

//subtitle
int Aml_MP_Player_StartSubtitleDecoding(AML_MP_HANDLE handle);

int Aml_MP_Player_StopSubtitleDecoding(AML_MP_HANDLE handle);

//CAS
int Aml_MP_Player_StartDescrambling(AML_MP_HANDLE handle);

int Aml_MP_Player_StopDescrambling(AML_MP_HANDLE handle);

//AD
int Aml_MP_Player_SetADParams(AML_MP_HANDLE handle, Aml_MP_AudioParams* params);

////////////////////////////////////////////////////////////////////////////////


#ifdef __cplusplus
}
#endif


#endif
