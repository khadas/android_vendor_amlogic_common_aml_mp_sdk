/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_TAG "AmlDVRPlayer"
#include "AmlDVRPlayer.h"
#include <Aml_MP/Dvr.h>
#include <utils/AmlMpHandle.h>
#include <utils/AmlMpUtils.h>
#include <utils/AmlMpLog.h>
#ifdef ANDROID
#ifndef __ANDROID_VNDK__
#include <gui/Surface.h>
#endif
#endif

#ifdef ANDROID
#include <amlogic/am_gralloc_ext.h>
#endif

namespace aml_mp {

static Aml_MP_DVRPlayerState convertToMpDVRPlayerState(DVR_PlaybackPlayState_t state);
static Aml_MP_DVRPlayerSegmentFlag convertToMpDVRSegmentFlag(DVR_PlaybackSegmentFlag_t flag);
static void convertToMpDVRPlayerStatus(Aml_MP_DVRPlayerStatus* mpStatus, DVR_WrapperPlaybackStatus_t* dvrStatus);

///////////////////////////////////////////////////////////////////////////////
AmlDVRPlayer::AmlDVRPlayer(Aml_MP_DVRPlayerBasicParams* basicParams, Aml_MP_DVRPlayerDecryptParams* decryptParams)
{
    snprintf(mName, sizeof(mName), "%s", LOG_TAG);
    MLOG();

    AmlMpConfig::instance().init();

    memset(&mPlaybackOpenParams, 0, sizeof(DVR_WrapperPlaybackOpenParams_t));
    setBasicParams(basicParams);
    mRecStartTime = 0;
    mLimit = 0;
    mIsEncryptStream = basicParams->drmMode != AML_MP_INPUT_STREAM_NORMAL;

    if (decryptParams != nullptr) {
        setDecryptParams(decryptParams);
    }

    createTsPlayerIfNeeded();
}

AmlDVRPlayer::~AmlDVRPlayer()
{
    MLOG();
}

int AmlDVRPlayer::registerEventCallback(Aml_MP_PlayerEventCallback cb, void* userData)
{
    AML_MP_TRACE(10);

    mEventCb = cb;
    mEventUserData = userData;

    return 0;
}

int AmlDVRPlayer::setStreams(Aml_MP_DVRStreamArray* streams)
{
    int ret = 0;
    Aml_MP_DVRStream* videoStream   = &streams->streams[AML_MP_DVR_VIDEO_INDEX];
    Aml_MP_DVRStream* audioStream   = &streams->streams[AML_MP_DVR_AUDIO_INDEX];
    Aml_MP_DVRStream* adStream      = &streams->streams[AML_MP_DVR_AD_INDEX];

    MLOG("video:pid %#x, codecId: %s, audio:pid %#x, codecId: %s, ad:pid: %#x, codecId: %s", videoStream->pid, mpCodecId2Str(videoStream->codecId),
            audioStream->pid, mpCodecId2Str(audioStream->codecId), adStream->pid, mpCodecId2Str(adStream->codecId));

    memset(&mPlayPids, 0, sizeof(mPlayPids));
    mPlayPids.video.type = DVR_STREAM_TYPE_VIDEO;
    mPlayPids.video.pid = videoStream->pid;
    mPlayPids.video.format = convertToDVRVideoFormat(videoStream->codecId);

    mPlayPids.audio.type = DVR_STREAM_TYPE_AUDIO;
    mPlayPids.audio.pid = audioStream->pid;
    mPlayPids.audio.format = convertToDVRAudioFormat(audioStream->codecId);

    mPlayPids.ad.type = DVR_STREAM_TYPE_AD;
    mPlayPids.ad.pid = adStream->pid;
    mPlayPids.ad.format = convertToDVRAudioFormat(adStream->codecId);

    //If audio and AD pid is same, set invalid pid for AD
    if (mPlayPids.ad.pid == mPlayPids.audio.pid) {
        mPlayPids.ad.pid = AML_MP_INVALID_PID;
        mPlayPids.ad.format = (DVR_AudioFormat_t)(-1);
    }

    if (mDVRPlayerHandle) {
        ret = dvr_wrapper_update_playback(mDVRPlayerHandle, &mPlayPids);
        if (ret < 0) {
            MLOGE("update playback failed!");
        }
    }

    return ret;
}

int AmlDVRPlayer::start(bool initialPaused)
{
    MLOG();

    int ret = createTsPlayerIfNeeded();
    if (ret != 0) {
        return -1;
    }

    ret = AmTsPlayer_registerCb(mTsPlayerHandle,  [](void *user_data, am_tsplayer_event *event) {
        static_cast<AmlDVRPlayer*>(user_data)->eventHandlerPlayer(event);
    }, this);
    MLOGI("TsPlayer set Callback function %s, result(%d)", (ret)? "FAIL" : "OK", ret);
    ret = AmTsPlayer_setWorkMode(mTsPlayerHandle, TS_PLAYER_MODE_NORMAL);
    MLOGI("TsPlayer set Workmode NORMAL %s, result(%d)", (ret)? "FAIL" : "OK", ret);
    ret = AmTsPlayer_setSyncMode(mTsPlayerHandle, TS_SYNC_PCRMASTER );
    MLOGI(" TsPlayer set Syncmode PCRMASTER %s, result(%d)", (ret)? "FAIL" : "OK", ret);
#if ANDROID_PLATFORM_SDK_VERSION >= 30
    if (mUseTif != -1) {
        am_tsplayer_audio_patch_manage_mode audioPatchManageMode = mUseTif ? AUDIO_PATCH_MANAGE_FORCE_DISABLE : AUDIO_PATCH_MANAGE_FORCE_ENABLE;
        ret = AmTsPlayer_setParams(mTsPlayerHandle, AM_TSPLAYER_KEY_SET_AUDIO_PATCH_MANAGE_MODE, (void*)&audioPatchManageMode);
        MLOGI(" TsPlayer set AudioPatchManageMode: %d, return %s, result(%d)", audioPatchManageMode, (ret)? "FAIL" : "OK", ret);
    }
#endif

    if (AmlMpConfig::instance().mTsPlayerNonTunnel) {
        if (AmlMpConfig::instance().mUseVideoTunnel == 0) {
#ifdef ANDROID
#ifndef __ANDROID_VNDK__
        android::Surface* surface = nullptr;
        if (mNativeWindow != nullptr) {
            surface = (android::Surface*)mNativeWindow.get();
        }
        MLOGI("setANativeWindow nativeWindow: %p, surface: %p", mNativeWindow.get(), surface);
        ret = AmTsPlayer_setSurface(mTsPlayerHandle, surface);
#endif
#endif
        } else {
#ifdef ANDROID
            ret = mNativeWindowHelper.setSidebandNonTunnelMode(mNativeWindow.get(), mVideoTunnelId);
            if (ret == 0) {
                AmTsPlayer_setSurface(mTsPlayerHandle, (void*)&mVideoTunnelId);
            }
#endif
        }

    } else {
    #ifdef ANDROID
        ret = mNativeWindowHelper.setSiebandTunnelMode(mNativeWindow.get());
    #endif
    }

    mPlaybackOpenParams.playback_handle = (Playback_DeviceHandle_t)mTsPlayerHandle;
    mPlaybackOpenParams.event_fn = [](DVR_PlaybackEvent_t event, void* params, void* userData) {
        AmlDVRPlayer* player = static_cast<AmlDVRPlayer*>(userData);
        return player->eventHandlerLibDVR(event, params);
    };
    mPlaybackOpenParams.event_userdata = this;

    //apply parameters
    #ifdef ANDROID
    mPlaybackOpenParams.vendor = (DVR_PlaybackVendor_t)mVendorID;
    #endif
    int error = dvr_wrapper_open_playback(&mDVRPlayerHandle, &mPlaybackOpenParams);
    if (error < 0) {
        MLOGE("open playback failed!");
        return error;
    }

    if (mIsEncryptStream) {
        dvr_wrapper_set_playback_secure_buffer(mDVRPlayerHandle, mSecureBuffer, mSecureBufferSize);
    }
    if (mRecStartTime > 0) {
        dvr_wrapper_setlimit_playback(mDVRPlayerHandle, mRecStartTime, mLimit);
    }
    DVR_PlaybackFlag_t play_flag = initialPaused ? DVR_PLAYBACK_STARTED_PAUSEDLIVE : (DVR_PlaybackFlag_t)0;
    error = dvr_wrapper_start_playback(mDVRPlayerHandle, play_flag, &mPlayPids);
    return error;
}

int AmlDVRPlayer::stop()
{
    MLOG();

    int error = dvr_wrapper_stop_playback(mDVRPlayerHandle);
    //Add support cas
    if (mIsEncryptStream) {
    }

    error = dvr_wrapper_close_playback(mDVRPlayerHandle);
    AmTsPlayer_release(mTsPlayerHandle);
    return 0;
}

int AmlDVRPlayer::pause()
{
    MLOG();

    int ret = 0;

    ret = dvr_wrapper_pause_playback(mDVRPlayerHandle);
    if (ret < 0) {
        MLOGE("pause playback failed!");
    }

    return ret;
}

int AmlDVRPlayer::resume()
{
    MLOG();

    int ret = 0;

    ret = dvr_wrapper_resume_playback(mDVRPlayerHandle);
    if (ret < 0) {
        MLOGE("resume playback failed!");
    }

    return ret;
}

int AmlDVRPlayer::setLimit(int time, int limit)
{
    MLOG("rec start time:%d limit:%d", time, limit);
    mRecStartTime = time;
    mLimit = limit;
    if (mRecStartTime > 0) {
        dvr_wrapper_setlimit_playback(mDVRPlayerHandle, mRecStartTime, mLimit);
    }
    return 0;
}

int AmlDVRPlayer::seek(int timeOffset)
{
    MLOG("timeOffset:%d", timeOffset);

    int ret = dvr_wrapper_seek_playback(mDVRPlayerHandle, timeOffset);
    if (ret < 0) {
        MLOGE("seek playback %d failed!", timeOffset);
    }

    return ret;
}

int AmlDVRPlayer::setPlaybackRate(float rate)
{
    MLOG("rate:%f", rate);

    int ret = 0;

    ret = dvr_wrapper_set_playback_speed(mDVRPlayerHandle, rate * 100);
    if (ret < 0) {
        MLOGE("set playback speed failed!");
    }

    return 0;
}

int AmlDVRPlayer::getStatus(Aml_MP_DVRPlayerStatus* status)
{
    MLOG();

    DVR_WrapperPlaybackStatus_t dvrStatus;
    int ret = dvr_wrapper_get_playback_status(mDVRPlayerHandle, &dvrStatus);
    if (ret < 0) {
        MLOGE("get playback status failed!");
        return ret;
    }

    convertToMpDVRPlayerStatus(status, &dvrStatus);

    return 0;
}

int AmlDVRPlayer::showVideo()
{
    MLOG();

    am_tsplayer_result ret;
    ret = AmTsPlayer_showVideo(mTsPlayerHandle);

    return ret;
}

int AmlDVRPlayer::hideVideo()
{
    MLOG();

    am_tsplayer_result ret;
    ret = AmTsPlayer_hideVideo(mTsPlayerHandle);

    return ret;
}

int AmlDVRPlayer::setVolume(float volume)
{
    MLOG("volume:%f", volume);

    am_tsplayer_result ret;
    ret = AmTsPlayer_setAudioVolume(mTsPlayerHandle, volume);

    return ret;
}

int AmlDVRPlayer::getVolume(float* volume)
{
    MLOG();

    am_tsplayer_result ret;
    int32_t tsVolume;
    ret = AmTsPlayer_getAudioVolume(mTsPlayerHandle, &tsVolume);
    *volume = tsVolume;

    return ret;
}

int AmlDVRPlayer::setParameter(Aml_MP_PlayerParameterKey key, void* parameter)
{
    am_tsplayer_result ret = AM_TSPLAYER_ERROR_INVALID_PARAMS;
    am_tsplayer_handle mPlayer = mTsPlayerHandle;

    MLOGI("Call setParameter, key is %s, mPlayer:%#x", mpPlayerParameterKey2Str(key), mPlayer);
    switch (key) {
        case AML_MP_PLAYER_PARAMETER_VIDEO_DISPLAY_MODE:
            ret = AmTsPlayer_setVideoMatchMode(mPlayer, convertToTsPlayerVideoMatchMode(*(Aml_MP_VideoDisplayMode*)parameter));
            break;

        case AML_MP_PLAYER_PARAMETER_BLACK_OUT:
            ret = AmTsPlayer_setVideoBlackOut(mPlayer, *(bool_t*)parameter);
            break;

        case AML_MP_PLAYER_PARAMETER_VIDEO_DECODE_MODE:
            ret = AmTsPlayer_setTrickMode(mPlayer, convertToTsplayerVideoTrickMode(*(Aml_MP_VideoDecodeMode*)parameter));
            break;

        case AML_MP_PLAYER_PARAMETER_VIDEO_PTS_OFFSET:
            break;

        case AML_MP_PLAYER_PARAMETER_AUDIO_OUTPUT_MODE:
            ret = AmTsPlayer_setAudioOutMode(mPlayer, convertToTsPlayerAudioOutMode(*(Aml_MP_AudioOutputMode*)parameter));
            break;

        case AML_MP_PLAYER_PARAMETER_AUDIO_OUTPUT_DEVICE:
            break;

        case AML_MP_PLAYER_PARAMETER_AUDIO_PTS_OFFSET:
            break;

        case AML_MP_PLAYER_PARAMETER_AUDIO_BALANCE:
            ret = AmTsPlayer_setAudioStereoMode(mPlayer, convertToTsPlayerAudioStereoMode(*(Aml_MP_AudioBalance*)parameter));
            break;

        case AML_MP_PLAYER_PARAMETER_AUDIO_MUTE:
        {
            bool mute = *(bool*)parameter;
            ret =AmTsPlayer_setAudioMute(mPlayer, mute, mute);
            break;
        }

        case AML_MP_PLAYER_PARAMETER_NETWORK_JITTER:
            break;

        case AML_MP_PLAYER_PARAMETER_AD_STATE:
        {
            int isEnable = *(int*)parameter;
            if (isEnable)
                ret = AmTsPlayer_enableADMix(mPlayer);
            else
                ret = AmTsPlayer_disableADMix(mPlayer);
            break;
        }

        case AML_MP_PLAYER_PARAMETER_AD_MIX_LEVEL:
        {
            Aml_MP_ADVolume* ADVolume = (Aml_MP_ADVolume*)parameter;
            //MLOGI("trace setParameter, AML_MP_PLAYER_PARAMETER_AD_MIX_LEVEL, AML_MP_PLAYER_PARAMETER_AUDIO_OUTPUT_MODE, value is master %d, slave %d", ADVolume->masterVolume, ADVolume->slaveVolume);
            ret = AmTsPlayer_setADMixLevel(mPlayer, ADVolume->masterVolume, ADVolume->slaveVolume);
            break;
        }

        case AML_MP_PLAYER_PARAMETER_VENDOR_ID:
        {
            mVendorID = *(int*)parameter;
            break;
        }

        case AML_MP_PLAYER_PARAMETER_VIDEO_TUNNEL_ID:
        {
            mVideoTunnelId = *(int*)parameter;
            ret = AmTsPlayer_setSurface(mPlayer, &mVideoTunnelId);
            break;
        }

        case AML_MP_PLAYER_PARAMETER_SURFACE_HANDLE:
        {
            MLOGI("PVR set surface handle: %p", parameter);
#if ANDROID_PLATFORM_SDK_VERSION >= 30
            // this is video tunnel id
            mVideoTunnelId = (int)(long)parameter;
            ret = AmTsPlayer_setSurface(mPlayer, &mVideoTunnelId);
#else
            void* surface = parameter;
            ret = AmTsPlayer_setSurface(mPlayer, surface);
#endif

            break;
        }

        case AML_MP_PLAYER_PARAMETER_USE_TIF:
        {
            mUseTif = *(bool*)parameter;
            break;
        }

        default:
            ret = AM_TSPLAYER_ERROR_INVALID_PARAMS;
    }

    if (ret != AM_TSPLAYER_OK) {
        return -1;
    }

    return 0;
}

int AmlDVRPlayer::getParameter(Aml_MP_PlayerParameterKey key, void* parameter)
{
    am_tsplayer_result ret = AM_TSPLAYER_ERROR_INVALID_PARAMS;
    am_tsplayer_handle mPlayer = mTsPlayerHandle;

    MLOGI("Call getParameter, key is %s, player:%#x", mpPlayerParameterKey2Str(key), mPlayer);
    if (!parameter) {
        return -1;
    }

    switch (key) {
        case AML_MP_PLAYER_PARAMETER_VIDEO_INFO:
            am_tsplayer_video_info videoInfo;
            ret = AmTsPlayer_getVideoInfo(mPlayer, &videoInfo);
            convertToMpVideoInfo((Aml_MP_VideoInfo*)parameter, &videoInfo);
            //MLOGI("trace getParameter, AML_MP_PLAYER_PARAMETER_VIDEO_INFO, width: %d, height: %d, framerate: %d, bitrate: %d, ratio64: %llu", videoInfo.width, videoInfo.height, videoInfo.framerate, videoInfo.bitrate, videoInfo.ratio64);
            break;
        case AML_MP_PLAYER_PARAMETER_VIDEO_DECODE_STAT:
            ret = AmTsPlayer_getVideoStat(mPlayer, (am_tsplayer_vdec_stat*)parameter);
            //am_tsplayer_vdec_stat* vdec_stat;
            //vdec_stat = (am_tsplayer_vdec_stat*)parameter;
            //MLOGI("trace getParameter, AML_MP_PLAYER_PARAMETER_VIDEO_DECODE_STAT, frame_width: %d, frame_height: %d, frame_rate: %d", vdec_stat->frame_width, vdec_stat->frame_height, vdec_stat->frame_rate);
            break;
        case AML_MP_PLAYER_PARAMETER_AUDIO_INFO:
            ret = AmTsPlayer_getAudioInfo(mPlayer, (am_tsplayer_audio_info*)parameter);
            //am_tsplayer_audio_info* audioInfo;
            //audioInfo = (am_tsplayer_audio_info*)parameter;
            //MLOGI("trace getParameter, AML_MP_PLAYER_PARAMETER_AUDIO_INFO, sample_rate: %d, channels: %d, channel_mask: %d, bitrate: %d", audioInfo->sample_rate, audioInfo->channels, audioInfo->channel_mask, audioInfo->bitrate);
            break;
        case AML_MP_PLAYER_PARAMETER_AUDIO_DECODE_STAT:
            ret = AmTsPlayer_getAudioStat(mPlayer, (am_tsplayer_adec_stat*) parameter);
            //am_tsplayer_adec_stat* adec_stat;
            //adec_stat = (am_tsplayer_adec_stat*)parameter;
            //MLOGI("trace getParameter, AML_MP_PLAYER_PARAMETER_AUDIO_DECODE_STAT, frame_count: %d, error_frame_count: %d, drop_frame_count: %d", adec_stat->frame_count, adec_stat->error_frame_count, adec_stat->drop_frame_count);
            break;
        case AML_MP_PLAYER_PARAMETER_SUBTITLE_INFO:
            break;

        case AML_MP_PLAYER_PARAMETER_SUBTITLE_DECODE_STAT:
            break;

        case AML_MP_PLAYER_PARAMETER_AD_INFO:
            ret = AmTsPlayer_getADInfo(mPlayer, (am_tsplayer_audio_info*)parameter);
            //am_tsplayer_audio_info* adInfo;
            //adInfo = (am_tsplayer_audio_info*)parameter;
            //MLOGI("trace getParameter, AML_MP_PLAYER_PARAMETER_AUDIO_INFO, sample_rate: %d, channels: %d, channel_mask: %d, bitrate: %d", adInfo->sample_rate, adInfo->channels, adInfo->channel_mask, adInfo->bitrate);
            break;
        case AML_MP_PLAYER_PARAMETER_AD_DECODE_STAT:
            ret = AmTsPlayer_getADStat(mPlayer, (am_tsplayer_adec_stat*)parameter);
            //am_tsplayer_adec_stat* ad_stat;
            //ad_stat = (am_tsplayer_adec_stat*)parameter;
            //MLOGI("trace getParameter, AML_MP_PLAYER_PARAMETER_AUDIO_DECODE_STAT, frame_count: %d, error_frame_count: %d, drop_frame_count: %d", ad_stat->frame_count, ad_stat->error_frame_count, ad_stat->drop_frame_count);
            break;
        case AML_MP_PLAYER_PARAMETER_INSTANCE_ID:
            ret = AmTsPlayer_getInstansNo(mPlayer, (uint32_t*)parameter);
            break;
        #ifdef ANDROID
        case AML_MP_PLAYER_PARAMETER_SYNC_ID:
            ret = AmTsPlayer_getSyncInstansNo(mPlayer, (int32_t*)parameter);
            break;
        #endif
        default:
            ret = AM_TSPLAYER_ERROR_INVALID_PARAMS;
    }

    if (ret != AM_TSPLAYER_OK) {
        return -1;
    }
    return 0;
}

int AmlDVRPlayer::setANativeWindow(ANativeWindow* nativeWindow) {
    MLOG();
    #ifdef ANDROID
    mNativeWindow = nativeWindow;
    MLOGI("PVR setAnativeWindow: %p, mNativewindow: %p", nativeWindow, mNativeWindow.get());
    #endif
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
int AmlDVRPlayer::setBasicParams(Aml_MP_DVRPlayerBasicParams* basicParams)
{
    mPlaybackOpenParams.dmx_dev_id = basicParams->demuxId;
    memcpy(&(mPlaybackOpenParams.location), &(basicParams->location), DVR_MAX_LOCATION_SIZE);
    mPlaybackOpenParams.block_size = basicParams->blockSize;
    mPlaybackOpenParams.is_timeshift = basicParams->isTimeShift;

    return 0;
}

int AmlDVRPlayer::setDecryptParams(Aml_MP_DVRPlayerDecryptParams * decryptParams)
{
    mPlaybackOpenParams.crypto_fn = (DVR_CryptoFunction_t)decryptParams->cryptoFn;
    mPlaybackOpenParams.crypto_data = decryptParams->cryptoData;

    mSecureBuffer = decryptParams->secureBuffer;
    mSecureBufferSize = decryptParams->secureBufferSize;

    MLOGI("mSecureBuffer:%p, mSecureBufferSize:%d", mSecureBuffer, mSecureBufferSize);

    return 0;
}

int AmlDVRPlayer::createTsPlayerIfNeeded()
{
    if (mTsPlayerHandle != 0) {
        return 0;
    }

    am_tsplayer_init_params tsPlayerInitParam{};

    tsPlayerInitParam.source = TS_MEMORY;
    tsPlayerInitParam.drmmode = (am_tsplayer_input_buffer_type)mIsEncryptStream;
    MLOGI("drmMode: %d", tsPlayerInitParam.drmmode);
    tsPlayerInitParam.dmx_dev_id = mPlaybackOpenParams.dmx_dev_id;
    tsPlayerInitParam.event_mask = 0;


    am_tsplayer_handle tsPlayerHandle;
    am_tsplayer_result ret = AmTsPlayer_create(tsPlayerInitParam, &tsPlayerHandle);
    if (ret != AM_TSPLAYER_OK) {
        MLOGE("Create tsplayer fail");
    } else {
       mTsPlayerHandle = tsPlayerHandle;
    }

    return ret;
}

DVR_Result_t AmlDVRPlayer::eventHandlerLibDVR(DVR_PlaybackEvent_t event, void* params)
{
    DVR_Result_t ret = DVR_SUCCESS;
    Aml_MP_DVRPlayerStatus mpStatus;

    convertToMpDVRPlayerStatus(&mpStatus, (DVR_WrapperPlaybackStatus_t*)params);

    switch (event) {
    case DVR_PLAYBACK_EVENT_ERROR:
        if (mEventCb) mEventCb(mEventUserData, AML_MP_DVRPLAYER_EVENT_ERROR, (int64_t)&mpStatus);
        break;

    case DVR_PLAYBACK_EVENT_TRANSITION_OK:
        if (mEventCb) mEventCb(mEventUserData, AML_MP_DVRPLAYER_EVENT_TRANSITION_OK, (int64_t)&mpStatus);
        break;

    case DVR_PLAYBACK_EVENT_TRANSITION_FAILED:
        if (mEventCb) mEventCb(mEventUserData, AML_MP_DVRPLAYER_EVENT_TRANSITION_FAILED, (int64_t)&mpStatus);
        break;

    case DVR_PLAYBACK_EVENT_KEY_FAILURE:
        if (mEventCb) mEventCb(mEventUserData, AML_MP_DVRPLAYER_EVENT_KEY_FAILURE, (int64_t)&mpStatus);
        break;

    case DVR_PLAYBACK_EVENT_NO_KEY:
        if (mEventCb) mEventCb(mEventUserData, AML_MP_DVRPLAYER_EVENT_NO_KEY, (int64_t)&mpStatus);
        break;

    case DVR_PLAYBACK_EVENT_REACHED_BEGIN:
        if (mEventCb) mEventCb(mEventUserData, AML_MP_DVRPLAYER_EVENT_REACHED_BEGIN, (int64_t)&mpStatus);
        break;

    case DVR_PLAYBACK_EVENT_REACHED_END:
        if (mEventCb) mEventCb(mEventUserData, AML_MP_DVRPLAYER_EVENT_REACHED_END, (int64_t)&mpStatus);
        break;

    case DVR_PLAYBACK_EVENT_NOTIFY_PLAYTIME:
        if (mEventCb) mEventCb(mEventUserData, AML_MP_DVRPLAYER_EVENT_NOTIFY_PLAYTIME, (int64_t)&mpStatus);
        break;

    case DVR_PLAYBACK_EVENT_FIRST_FRAME:
        if (mEventCb) mEventCb(mEventUserData, AML_MP_PLAYER_EVENT_FIRST_FRAME, (int64_t)&mpStatus);
        break;

    default:
        MLOG("unhandled event:%d", event);
        break;
    }

    return ret;
}

DVR_Result_t AmlDVRPlayer::eventHandlerPlayer(am_tsplayer_event* event)
{
    if (mEventCb == NULL) {
        return DVR_FAILURE;
    }
    switch (event->type) {
    case AM_TSPLAYER_EVENT_TYPE_VIDEO_CHANGED:
    {
        MLOGE("[evt] AML_MP_PLAYER_EVENT_VIDEO_CHANGED");

        Aml_MP_PlayerEventVideoFormat videoFormatEvent;
        videoFormatEvent.frame_width = event->event.video_format.frame_width;
        videoFormatEvent.frame_height = event->event.video_format.frame_height;
        videoFormatEvent.frame_rate = event->event.video_format.frame_rate;
        videoFormatEvent.frame_aspectratio = event->event.video_format.frame_aspectratio;

        mEventCb(mEventUserData, AML_MP_PLAYER_EVENT_VIDEO_CHANGED, (int64_t)&videoFormatEvent);
    }
    break;

    case AM_TSPLAYER_EVENT_TYPE_AUDIO_CHANGED:
        mEventCb(mEventUserData, AML_MP_PLAYER_EVENT_AUDIO_CHANGED, 0);
        break;

    case AM_TSPLAYER_EVENT_TYPE_FIRST_FRAME:
        MLOGE("[evt] AM_TSPLAYER_EVENT_TYPE_FIRST_FRAME\n");

        mEventCb(mEventUserData, AML_MP_PLAYER_EVENT_FIRST_FRAME, 0);
        break;

    case AM_TSPLAYER_EVENT_TYPE_AV_SYNC_DONE:
        MLOGE("[evt] AML_MP_PLAYER_EVENT_AV_SYNC_DONE");

        mEventCb(mEventUserData, AML_MP_PLAYER_EVENT_AV_SYNC_DONE, 0);
        break;

    case AM_TSPLAYER_EVENT_TYPE_DATA_LOSS:
        mEventCb(mEventUserData, AML_MP_PLAYER_EVENT_DATA_LOSS, 0);
        break;

    case AM_TSPLAYER_EVENT_TYPE_DATA_RESUME:
        mEventCb(mEventUserData, AML_MP_PLAYER_EVENT_DATA_RESUME, 0);
        break;

    case AM_TSPLAYER_EVENT_TYPE_SCRAMBLING:
    {
        Aml_MP_PlayerEventScrambling scrambling;
        scrambling.scramling = 1;
        scrambling.type = convertToAmlMPStreamType(event->event.scramling.stream_type);

        mEventCb(mEventUserData, AML_MP_PLAYER_EVENT_SCRAMBLING, (int64_t)&scrambling);
    }
    break;

    case AM_TSPLAYER_EVENT_TYPE_USERDATA_AFD:
    {
        Aml_MP_PlayerEventMpegUserData userData;
        userData.data = event->event.mpeg_user_data.data;
        userData.len = event->event.mpeg_user_data.len;

        mEventCb(mEventUserData, AML_MP_PLAYER_EVENT_USERDATA_AFD, (int64_t)&userData);
    }
    break;

    case AM_TSPLAYER_EVENT_TYPE_USERDATA_CC:
    {
        Aml_MP_PlayerEventMpegUserData userData;
        userData.data = event->event.mpeg_user_data.data;
        userData.len = event->event.mpeg_user_data.len;

        mEventCb(mEventUserData, AML_MP_PLAYER_EVENT_USERDATA_CC, (int64_t)&userData);
    }
    break;

    default:
        MLOGE("unhandled event:%d", event->type);
        break;
    }

    return DVR_SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////
Aml_MP_DVRPlayerState convertToMpDVRPlayerState(DVR_PlaybackPlayState_t state)
{
    switch (state) {
    case DVR_PLAYBACK_STATE_START:
        return AML_MP_DVRPLAYER_STATE_START;
        break;

    case DVR_PLAYBACK_STATE_STOP:
        return AML_MP_DVRPLAYER_STATE_STOP;
        break;

    case DVR_PLAYBACK_STATE_PAUSE:
        return AML_MP_DVRPLAYER_STATE_PAUSE;
        break;

    case DVR_PLAYBACK_STATE_FF:
        return AML_MP_DVRPLAYER_STATE_FF;
        break;

    case DVR_PLAYBACK_STATE_FB:
        return AML_MP_DVRPLAYER_STATE_FB;
        break;
    }
}

Aml_MP_DVRPlayerSegmentFlag convertToMpDVRSegmentFlag(DVR_PlaybackSegmentFlag_t flag)
{
    switch (flag) {
    case DVR_PLAYBACK_SEGMENT_ENCRYPTED:
        return AML_MP_DVRPLAYER_SEGMENT_ENCRYPTED;
        break;

    case DVR_PLAYBACK_SEGMENT_DISPLAYABLE:
        return AML_MP_DVRPLAYER_SEGMENT_DISPLAYABLE;
        break;

    case DVR_PLAYBACK_SEGMENT_CONTINUOUS:
        return AML_MP_DVRPLAYER_SEGMENT_CONTINUOUS;
        break;
    }
}

static void convertToMpDVRPlayerStatus(Aml_MP_DVRPlayerStatus* mpStatus, DVR_WrapperPlaybackStatus_t* dvrStatus)
{
    mpStatus->state = convertToMpDVRPlayerState(dvrStatus->state);
    convertToMpDVRSourceInfo(&mpStatus->infoCur, &dvrStatus->info_cur);
    convertToMpDVRSourceInfo(&mpStatus->infoFull, &dvrStatus->info_full);

    convertToMpDVRStream(&mpStatus->pids.streams[AML_MP_DVR_VIDEO_INDEX], &dvrStatus->pids.video);
    convertToMpDVRStream(&mpStatus->pids.streams[AML_MP_DVR_AUDIO_INDEX], &dvrStatus->pids.audio);
    convertToMpDVRStream(&mpStatus->pids.streams[AML_MP_DVR_AD_INDEX], &dvrStatus->pids.ad);
    convertToMpDVRStream(&mpStatus->pids.streams[AML_MP_DVR_SUBTITLE_INDEX], &dvrStatus->pids.subtitle);
    convertToMpDVRStream(&mpStatus->pids.streams[AML_MP_DVR_PCR_INDEX], &dvrStatus->pids.pcr);

    mpStatus->pids.nbStreams = AML_MP_DVR_STREAM_NB;
    mpStatus->speed = dvrStatus->speed;
    mpStatus->flags = convertToMpDVRSegmentFlag(dvrStatus->flags);
    convertToMpDVRSourceInfo(&mpStatus->infoObsolete, &dvrStatus->info_obsolete);
}

}
