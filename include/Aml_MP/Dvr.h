/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
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
    int                         fend_dev_id;
    int                         userId;
    Aml_MP_DemuxId              demuxId;
    char                        location[AML_MP_MAX_PATH_SIZE];
    bool                        isTimeShift;
    int64_t                     segmentSize;
    Aml_MP_DVRRecorderFlag      flags;
    int                         bufferSize;         //flush size
    int                         ringbufSize;        //dvbcore ringbuf size,this buf is uesd to cache ts data,
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
    AML_MP_DVRRECORDER_EVENT_WRITE_ERROR        = 0x9001,         /**< Signal data write error*/
} AML_MP_DVRRecorderEventType;

typedef void (*Aml_MP_DVRRecorderEventCallback) (void* userdata, AML_MP_DVRRecorderEventType event, int64_t params);

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
/**
 * \brief Aml_MP_DVRRecorder_Create
 * Create DVR recorder
 *
 * \param [in]  create params
 * \param [out] DVR recorder handle
 *
 * \return 0 if success
 */
int Aml_MP_DVRRecorder_Create(Aml_MP_DVRRecorderCreateParams* createParams, AML_MP_DVRRECORDER* recorder);

/**
 * \brief Aml_MP_DVRRecorder_Destroy
 * Destory DVR recorder
 *
 * \param [in]  DVR recorder handle
 *
 * \return 0 if success
 */
int Aml_MP_DVRRecorder_Destroy(AML_MP_DVRRECORDER recorder);

/**
 * \brief Aml_MP_DVRRecorder_RegisterEventCallback
 * Register DVR recorder callback function
 *
 * \param [in]  DVR recorder handle
 * \param [in]  callback function
 * \param [in]  user data
 *
 * \return 0 if success
 */
int Aml_MP_DVRRecorder_RegisterEventCallback(AML_MP_DVRRECORDER recorder, Aml_MP_DVRRecorderEventCallback cb, void* userData);

/**
 * \brief Aml_MP_DVRRecorder_SetStreams
 * Set DVR recorder streams
 * After start recorder, can use this function to update stream info.
 *
 * \param [in]  DVR recorder handle
 * \param [in]  streams to be recored
 *
 * \return 0 if success
 */
int Aml_MP_DVRRecorder_SetStreams(AML_MP_DVRRECORDER recorder, Aml_MP_DVRStreamArray* streams);

/**
 * \brief Aml_MP_DVRRecorder_Start
 * Start DVR recorder
 *
 * \param [in]  DVR recorder handle
 *
 * \return 0 if success
 */
int Aml_MP_DVRRecorder_Start(AML_MP_DVRRECORDER recorder);

/**
 * \brief Aml_MP_DVRRecorder_Stop
 * Stop DVR recorder
 *
 * \param [in]  DVR recorder handle
 *
 * \return 0 if success
 */
int Aml_MP_DVRRecorder_Stop(AML_MP_DVRRECORDER recorder);

/**
 * \brief Aml_MP_DVRRecorder_Pause
 * Pause DVR recorder
 *
 * \param [in]  DVR recorder handle
 *
 * \return 0 if success
 */
int Aml_MP_DVRRecorder_Pause(AML_MP_DVRPLAYER player);

/**
 * \brief Aml_MP_DVRRecorder_Resume
 * Resume DVR recorder
 *
 * \param [in]  DVR recorder handle
 *
 * \return 0 if success
 */
int Aml_MP_DVRRecorder_Resume(AML_MP_DVRPLAYER player);


/**
 * \brief Aml_MP_DVRRecorder_GetStatus
 * Get DVR recorder status
 *
 * \param [in]  DVR recorder handle
 * \param [out] DVR recorder status
 *
 * \return 0 if success
 */
int Aml_MP_DVRRecorder_GetStatus(AML_MP_DVRRECORDER recorder, Aml_MP_DVRRecorderStatus* status);

/**
 * \brief Aml_MP_DVRRecorder_GetSegmentList
 * Get DVR recorder segment list
 *
 * \param [in]  record file location
 * \param [out] record file list length
 * \param [out] record file list
 *
 * \return 0 if success
 */
int Aml_MP_DVRRecorder_GetSegmentList(const char* location, uint32_t* segmentNums, uint64_t** segmentIds);

/**
 * \brief Aml_MP_DVRRecorder_GetSegmentInfo
 * Get specific record segment file info
 *
 * \param [in]  record file location
 * \param [in]  record file id
 * \param [out] record file info
 *
 * \return 0 if success
 */
int Aml_MP_DVRRecorder_GetSegmentInfo(const char* location, uint64_t segmentId, Aml_MP_DVRSegmentInfo* segmentInfo);

/**
 * \brief Aml_MP_DVRRecorder_DeleteSegment
 * Delete specific record segment file
 *
 * \param [in]  record file location
 * \param [in]  record file id
 *
 * \return 0 if success
 */
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
    Aml_MP_DVRPlayerState state;        /**< DVR playback state*/
    Aml_MP_DVRSourceInfo infoCur;       /**< DVR playback current information*/
    Aml_MP_DVRSourceInfo infoFull;      /**< DVR playback total(non-obsolete) infomation*/
    Aml_MP_DVRStreamArray pids;         /**< DVR playback pids information*/
    float speed;                        /**< DVR playback current speed*/
    Aml_MP_DVRPlayerSegmentFlag flags;  /**< DVR playback flags*/
    Aml_MP_DVRSourceInfo infoObsolete;  /**< DVR playback obsolete information, take into account for timeshift*/
} Aml_MP_DVRPlayerStatus;

#ifdef __cplusplus
extern "C" {
#endif
/**
 * \brief Aml_MP_DVRPlayer_Create
 * Create DVR player
 *
 * \param [in]  DVR player create params
 * \param [out] DVR player handle
 *
 * \return 0 if success
 */
int Aml_MP_DVRPlayer_Create(Aml_MP_DVRPlayerCreateParams* createParams, AML_MP_DVRPLAYER* player);

/**
 * \brief Aml_MP_DVRPlayer_Destroy
 * Destory DVR player
 *
 * \param [in]  DVR player handle
 *
 * \return 0 if success
 */
int Aml_MP_DVRPlayer_Destroy(AML_MP_DVRPLAYER player);

/**
 * \brief Aml_MP_DVRPlayer_RegisterEventCallback
 * Regist DVR player callback function
 *
 * \param [in]  DVR player handle
 * \param [in]  callback function
 * \param [in]  user data
 *
 * \return 0 if success
 */
int Aml_MP_DVRPlayer_RegisterEventCallback(AML_MP_DVRPLAYER player, Aml_MP_PlayerEventCallback cb, void* userData);

/**
 * \brief Aml_MP_DVRPlayer_SetStreams
 * Set DVR player streams
 *
 * \param [in]  DVR player handle
 * \param [in]  DVR play streams
 *
 * \return 0 if success
 */
int Aml_MP_DVRPlayer_SetStreams(AML_MP_DVRPLAYER player, Aml_MP_DVRStreamArray* streams);

/**
 * \brief Aml_MP_DVRPlayer_Start
 * Strat DVR player
 *
 * \param [in]  DVR player handle
 * \param [in]  need pause after show first frame
 *
 * \return 0 if success
 */
int Aml_MP_DVRPlayer_Start(AML_MP_DVRPLAYER player, bool initialPaused);

/**
 * \brief Aml_MP_DVRPlayer_Stop
 * Stop DVR player
 *
 * \param [in]  DVR player handle
 *
 * \return 0 if success
 */
int Aml_MP_DVRPlayer_Stop(AML_MP_DVRPLAYER player);

/**
 * \brief Aml_MP_DVRPlayer_Pause
 * Pause DVR player
 *
 * \param [in]  DVR player handle
 *
 * \return 0 if success
 */
int Aml_MP_DVRPlayer_Pause(AML_MP_DVRPLAYER player);

/**
 * \brief Aml_MP_DVRPlayer_Resume
 * Resume DVR player
 *
 * \param [in]  DVR player handle
 *
 * \return 0 if success
 */
int Aml_MP_DVRPlayer_Resume(AML_MP_DVRPLAYER player);

/**
 * \brief Aml_MP_DVRPlayer_SetLimit
 * Set limit DVR player
 *
 * \param [in]  DVR player handle
 * \param [in]  record start time (ms)
 * \param [in]  record limit time (ms)
 * \return 0 if success
 */

int Aml_MP_DVRPlayer_SetLimit(AML_MP_DVRPLAYER player, int time, int limit);

/**
 * \brief Aml_MP_DVRPlayer_Seek
 * Seek to specific time
 *
 * \param [in]  DVR player handle
 * \param [in]  seek to time (ms)
 *
 * \return 0 if success
 */
int Aml_MP_DVRPlayer_Seek(AML_MP_DVRPLAYER player, int timeOffset);

/**
 * \brief Aml_MP_DVRPlayer_SetPlaybackRate
 * Set DVR player playback rate
 *
 * \param [in]  DVR player handle
 * \param [in]  playback rate
 *
 * \return 0 if success
 */
int Aml_MP_DVRPlayer_SetPlaybackRate(AML_MP_DVRPLAYER player, float rate);

/**
 * \brief Aml_MP_DVRPlayer_GetStatus
 * Get DVR player status
 *
 * \param [in]  DVR player handle
 * \param [in]  DVR player status
 *
 * \return 0 if success
 */
int Aml_MP_DVRPlayer_GetStatus(AML_MP_DVRPLAYER player, Aml_MP_DVRPlayerStatus* status);

/**
 * \brief Aml_MP_DVRPlayer_ShowVideo
 * Show video in DVR player
 *
 * \param [in]  DVR player handle
 *
 * \return 0 if success
 */
int Aml_MP_DVRPlayer_ShowVideo(AML_MP_DVRPLAYER handle);

/**
 * \brief Aml_MP_DVRPlayer_HideVideo
 * Hide video in DVR player
 *
 * \param [in]  DVR player handle
 *
 * \return 0 if success
 */
int Aml_MP_DVRPlayer_HideVideo(AML_MP_DVRPLAYER handle);

/**
 * \brief Aml_MP_DVRPlayer_SetVolume
 * Set DVR player volume
 *
 * \param [in]  DVR player handle
 * \param [in]  volume [0.0 ~ 100.0]
 *
 * \return 0 if success
 */
int Aml_MP_DVRPlayer_SetVolume(AML_MP_DVRPLAYER handle, float volume);

/**
 * \brief Aml_MP_DVRPlayer_GetVolume
 * Get DVR player volume
 *
 * \param [in]  DVR player handle
 * \param [out] volume [0.0 ~ 100.0]
 *
 * \return 0 if success
 */
int Aml_MP_DVRPlayer_GetVolume(AML_MP_DVRPLAYER handle, float* volume);

/**
 * \brief Aml_MP_DVRPlayer_SetParameter
 * Set DVR player parameter
 *
 * \param [in]  DVR player handle
 * \param [in]  DVR player parameter key
 * \param [in]  DVR player parameter value
 *
 * \return 0 if success
 */
int Aml_MP_DVRPlayer_SetParameter(AML_MP_DVRPLAYER handle, Aml_MP_PlayerParameterKey key, void* parameter);

/**
 * \brief Aml_MP_DVRPlayer_GetParameter
 * Get DVR player parameter
 *
 * \param [in]  DVR player handle
 * \param [in]  DVR player parameter key
 * \param [out] DVR player parameter value
 *
 * \return 0 if success
 */
int Aml_MP_DVRPlayer_GetParameter(AML_MP_DVRPLAYER handle, Aml_MP_PlayerParameterKey key, void* parameter);

/**
 * \brief Aml_MP_DVRPlayer_SetANativeWindow
 * Set DVR player native window
 *
 * \param [in]  DVR player handle
 * \param [in]  native window
 *
 * \return 0 if success
 */
int Aml_MP_DVRPlayer_SetANativeWindow(AML_MP_DVRPLAYER handle, ANativeWindow* nativeWindow);


/**
 * \brief Set video window
 *
 * \param handle
 * \param x
 * \param y
 * \param width
 * \param height
 *
 * \return 0 if success
 */
int Aml_MP_DVRPlayer_SetVideoWindow(AML_MP_DVRPLAYER handle, int32_t x, int32_t y, int32_t width, int32_t height);



#ifdef __cplusplus
}
#endif

#endif
