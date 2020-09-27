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

#ifndef _AML_MP_DVR_H_
#define _AML_MP_DVR_H_

#include "Common.h"
#include "Cas.h"
#include <stdbool.h>

typedef int (*Aml_MP_CAS_CryptoFunction) (Aml_MP_CASCryptoParams *params, void *userdata);

///////////////////////////////////////////////////////////////////////////////
typedef enum {
    AML_MP_DVRRECORDER_SCRAMBLED = (1 << 0),
    AML_MP_DVRRECORDER_ACCURATE  = (1 << 1),
} Aml_MP_DVRRecorderFlag;

typedef struct {
    int                         userId;
    Aml_MP_DemuxId              demuxId;
    char                        location[AML_MP_MAX_PATH_SIZE];
    bool                        isTimeShift;
    int64_t                     segmentSize;
    Aml_MP_DVRRecorderFlag      flags;
    int                       bufferSize;         //flush size
} Aml_MP_DVRRecorderBasicParams;

typedef struct {
    int64_t                     maxSize;
    int64_t                     maxTime;
} Aml_MP_DVRRecorderTimeShiftParams;

typedef struct {
    int64_t                     intervalBytes;      //crypto period
    bool                        notifyClearPeriods; //crypto period
    Aml_MP_CAS_CryptoFunction   cryptoFn;
    void*                       cryptoData;
    uint8_t*                    secureBuffer;
    size_t                      secureBufferSize;
} Aml_MP_DVRRecorderEncryptParams;

typedef struct {
    Aml_MP_DVRRecorderBasicParams       basicParams;
    Aml_MP_DVRRecorderTimeShiftParams   timeshiftParams;
    Aml_MP_DVRRecorderEncryptParams     encryptParams;
} Aml_MP_DVRRecorderCreateParams;

typedef enum {
  AML_MP_DVRRECORDER_EVENT_ERROR              = 0x1000,         /**< Signal a critical DVR error*/
  AML_MP_DVRRECORDER_EVENT_STATUS             = 0x1001,         /**< Signal the current record status which reach a certain size*/
  AML_MP_DVRRECORDER_EVENT_SYNC_END           = 0x1002,         /**< Signal that data sync has ended*/
  AML_MP_DVRRECORDER_EVENT_CRYPTO_STATUS      = 0x2001,         /**< Signal the current crypto status*/
  AML_MP_DVRRECORDER_EVENT_WRITE_ERROR       = 0x9001,         /**< Signal the current crypto status*/
} AML_MP_DVRRecorderEvent;

typedef int (*Aml_MP_DVRRecorderEventCallback) (AML_MP_DVRRecorderEvent event, void *params, void *userdata);

typedef enum {
    AML_MP_DVRRECORDER_STREAM_CREATE,          /**< Create a new pid used to record*/
    AML_MP_DVRRECORDER_STREAM_KEEP,            /**< Indicate this pid keep last state*/
    AML_MP_DVRRECORDER_STREAM_CLOSE            /**< Close this pid record*/
} Aml_MP_DVRRecorderStreamAction;

typedef struct {
    Aml_MP_DVRStreamArray streamArray;
    Aml_MP_DVRRecorderStreamAction streamActions[AML_MP_DVR_STREAMS_COUNT];
} Aml_MP_DVRRecorderStreams;

typedef enum {
  AML_MP_DVRRECORDER_STATE_OPENED,        /**< Record state is opened*/
  AML_MP_DVRRECORDER_STATE_STARTED,       /**< Record state is started*/
  AML_MP_DVRRECORDER_STATE_STOPPED,       /**< Record state is stopped*/
  AML_MP_DVRRECORDER_STATE_CLOSED,        /**< Record state is closed*/
} Aml_MP_DVRRecorderState;

typedef struct {
    Aml_MP_DVRRecorderState state;
    Aml_MP_DVRSourceInfo info;
    Aml_MP_DVRStreamArray streams;
    Aml_MP_DVRSourceInfo infoObsolete;
} Aml_MP_DVRRecorderStatus;

typedef struct {
    uint64_t id;
    Aml_MP_DVRStreamArray streams;
    time_t duration;
    size_t size;
    uint32_t nbPackets;
} Aml_MP_DVRSegmentInfo;

#ifdef __cplusplus
extern "C" {
#endif
int Aml_MP_DVRRecorder_Create(Aml_MP_DVRRecorderCreateParams* createParams, AML_MP_HANDLE* recorder);

int Aml_MP_DVRRecorder_Destroy(AML_MP_HANDLE recorder);

int Aml_MP_DVRRecorder_RegisterEventCallback(AML_MP_HANDLE recorder, Aml_MP_DVRRecorderEventCallback cb, void* userData);

int Aml_MP_DVRRecorder_SetStreams(AML_MP_HANDLE recorder, Aml_MP_DVRRecorderStreams* streams); //update record pids

int Aml_MP_DVRRecorder_Start(AML_MP_HANDLE recorder);

int Aml_MP_DVRRecorder_Stop(AML_MP_HANDLE recorder);

int Aml_MP_DVRRecorder_GetStatus(AML_MP_HANDLE recorder, Aml_MP_DVRRecorderStatus* status);

int Aml_MP_DVRRecorder_GetSegmentList(const char* location, uint32_t* segmentNums, uint64_t** segmentIds);

int Aml_MP_DVRRecorder_GetSegmentInfo(const char* location, uint64_t segmentId, Aml_MP_DVRSegmentInfo* segmentInfo);

int Aml_MP_DVRRecorder_DeleteSegment(const char* location, uint64_t segmentId);
#ifdef __cplusplus
}
#endif

///////////////////////////////////////////////////////////////////////////////
typedef struct {
    int                         userId;
    Aml_MP_DemuxId              demuxId;
    char                        location[AML_MP_MAX_PATH_SIZE];
    int64_t                     blockSize;          //inject block size
    bool                        isTimeShift;
    Aml_MP_InputStreamType      drmMode;
} Aml_MP_DVRPlayerBasicParams;

typedef struct {
    Aml_MP_CAS_CryptoFunction   cryptoFn;
    void*                       cryptoData;
    uint8_t*                    secureBuffer;
    size_t                      secureBufferSize;
} Aml_MP_DVRPlayerDecryptParams;

typedef struct {
    Aml_MP_DVRPlayerBasicParams         basicParams;
    Aml_MP_DVRPlayerDecryptParams       decryptParams;
} Aml_MP_DVRPlayerCreateParams;

typedef enum {
  AML_MP_DVRPLAYER_EVENT_ERROR              = 0x1000,   /**< Signal a critical playback error*/
  AML_MP_DVRPLAYER_EVENT_TRANSITION_OK    ,             /**< transition ok*/
  AML_MP_DVRPLAYER_EVENT_TRANSITION_FAILED,             /**< transition failed*/
  AML_MP_DVRPLAYER_EVENT_KEY_FAILURE,                   /**< key failure*/
  AML_MP_DVRPLAYER_EVENT_NO_KEY,                        /**< no key*/
  AML_MP_DVRPLAYER_EVENT_REACHED_BEGIN     ,            /**< reached begin*/
  AML_MP_DVRPLAYER_EVENT_REACHED_END,                    /**< reached end*/
  AML_MP_DVRPLAYER_EVENT_NOTIFY_PLAYTIME,               /**< notify play cur segmeng time ms*/
  AML_MP_DVRPLAYER_EVENT_FIRST_FRAME                    /**< first frame*/
} Aml_MP_DVRPlayerEvent;

typedef int (*Aml_MP_DVRPlayerEventCallback)(Aml_MP_DVRPlayerEvent event, void *params, void *userdata);

/**\brief playback play state*/
typedef enum
{
  AML_MP_DVRPLAYER_STATE_START,       /**< start play */
  AML_MP_DVRPLAYER_STATE_STOP,        /**< stop */
  AML_MP_DVRPLAYER_STATE_PAUSE,       /**< pause */
  AML_MP_DVRPLAYER_STATE_FF,          /**< fast forward */
  AML_MP_DVRPLAYER_STATE_FB           /**< fast backword */
} Aml_MP_DVRPlayerState;

/**\brief dvr play segment flag */
typedef enum
{
  AML_MP_DVRPLAYER_SEGMENT_ENCRYPTED   = (1 << 0), /**< encrypted stream */
  AML_MP_DVRPLAYER_SEGMENT_DISPLAYABLE = (1 << 1), /**< displayable stream */
  AML_MP_DVRPLAYER_SEGMENT_CONTINUOUS  = (1 << 2)  /**< continuous stream with pre one */
} Aml_MP_DVRPlayerSegmentFlag;

typedef struct {
  Aml_MP_DVRPlayerState state;      /**< DVR playback state*/
  Aml_MP_DVRSourceInfo infoCur;         /**< DVR playback current information*/
  Aml_MP_DVRSourceInfo infoFull;        /**< DVR playback total(non-obsolete) infomation*/
  Aml_MP_DVRStreamArray pids;            /**< DVR playback pids information*/
  float speed;                        /**< DVR playback current speed*/
  Aml_MP_DVRPlayerSegmentFlag flags;    /**< DVR playback flags*/
  Aml_MP_DVRSourceInfo infoObsolete;    /**< DVR playback obsolete information, take into account for timeshift*/
} Aml_MP_DVRPlayerStatus;

#ifdef __cplusplus
extern "C" {
#endif
int Aml_MP_DVRPlayer_Create(Aml_MP_DVRPlayerCreateParams* createParams, AML_MP_HANDLE* player);

int Aml_MP_DVRPlayer_Destroy(AML_MP_HANDLE player);

int Aml_MP_DVRPlayer_RegisterEventCallback(AML_MP_HANDLE player, Aml_MP_DVRPlayerEventCallback cb, void* userData);

int Aml_MP_DVRPlayer_SetStreams(AML_MP_HANDLE player, Aml_MP_DVRStreamArray* streams);

int Aml_MP_DVRPlayer_Start(AML_MP_HANDLE player, bool initialPaused);

int Aml_MP_DVRPlayer_Stop(AML_MP_HANDLE player);

int Aml_MP_DVRPlayer_Pause(AML_MP_HANDLE player);

int Aml_MP_DVRPlayer_Resume(AML_MP_HANDLE player);

int Aml_MP_DVRPlayer_Seek(AML_MP_HANDLE player, int timeOffset);

int Aml_MP_DVRPlayer_SetPlaybackRate(AML_MP_HANDLE player, float rate);

int Aml_MP_DVRPlayer_GetStatus(AML_MP_HANDLE player, Aml_MP_DVRPlayerStatus* status);

int Aml_MP_DVRPlayer_ShowVideo(AML_MP_HANDLE handle);

int Aml_MP_DVRPlayer_HideVideo(AML_MP_HANDLE handle);

int Aml_MP_DVRPlayer_SetVolume(AML_MP_HANDLE handle, float volume);

int Aml_MP_DVRPlayer_GetVolume(AML_MP_HANDLE handle, float* volume);

int Aml_MP_DVRPlayer_SetParameter(AML_MP_HANDLE handle, Aml_MP_PlayerParameterKey key, void* parameter);

int Aml_MP_DVRPlayer_GetParameter(AML_MP_HANDLE handle, Aml_MP_PlayerParameterKey key, void* parameter);
#ifdef __cplusplus
}
#endif

#endif
