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

#define LOG_TAG "AmlDVRPlayer"
#include "AmlDVRPlayer.h"
#include <Aml_MP/Dvr.h>
#include <utils/AmlMpHandle.h>
#include <utils/AmlMpUtils.h>
#define KEEP_ALOGX
#include <utils/AmlMpLog.h>

namespace aml_mp {

static Aml_MP_DVRPlayerState convertToMpDVRPlayerState(DVR_PlaybackPlayState_t state);
static Aml_MP_DVRPlayerSegmentFlag convertToMpDVRSegmentFlag(DVR_PlaybackSegmentFlag_t flag);

///////////////////////////////////////////////////////////////////////////////
AmlDVRPlayer::AmlDVRPlayer(Aml_MP_DVRPlayerBasicParams* basicParams, Aml_MP_DVRPlayerDecryptParams* decryptParams)
{
    snprintf(mName, sizeof(mName), "%s", LOG_TAG);
    MLOG();

    memset(&mPlaybackOpenParams, 0, sizeof(DVR_WrapperPlaybackOpenParams_t));
    setBasicParams(basicParams);

    mIsEncryptStream = basicParams->drmMode != AML_MP_INPUT_STREAM_NORMAL;

    if (decryptParams != nullptr) {
        setDecryptParams(decryptParams);
    }
}

AmlDVRPlayer::~AmlDVRPlayer()
{
    MLOG();
}

int AmlDVRPlayer::registerEventCallback(Aml_MP_DVRPlayerEventCallback cb, void* userData)
{
    AML_MP_TRACE(10);

    mPlaybackOpenParams.event_fn = (DVR_PlaybackEventFunction_t)cb;
    mPlaybackOpenParams.event_userdata = userData;

    return 0;
}

int AmlDVRPlayer::setStreams(Aml_MP_DVRStreamArray* streams)
{
    int ret = 0;
    Aml_MP_DVRStream* videoStream   = &streams->streams[AML_MP_DVR_VIDEO_INDEX];
    Aml_MP_DVRStream* audioStream   = &streams->streams[AML_MP_DVR_AUDIO_INDEX];
    Aml_MP_DVRStream* adStream      = &streams->streams[AML_MP_DVR_AD_INDEX];

    MLOG("video:pid %#x, codecId:%d, audio:pid %#x, codecId:%d, ad:pid:%#x, codecId:%d", videoStream->pid, videoStream->codecId,
            audioStream->pid, audioStream->codecId, adStream->pid, adStream->codecId);

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

    if (mDVRPlayerHandle) {
        ret = dvr_wrapper_update_playback(mDVRPlayerHandle, &mPlayPids);
        if (ret < 0) {
            ALOGE("update playback failed!");
        }
    }

    return ret;
}

int AmlDVRPlayer::start(bool initialPaused)
{
    MLOG();

    am_tsplayer_init_params tsPlayerInitParam{};

    tsPlayerInitParam.source = TS_MEMORY;
    tsPlayerInitParam.drmmode = (am_tsplayer_input_buffer_type)mIsEncryptStream;
    ALOGI("drmMode:%d", tsPlayerInitParam.drmmode);
    tsPlayerInitParam.dmx_dev_id = mPlaybackOpenParams.dmx_dev_id;
    tsPlayerInitParam.event_mask = 0;


    am_tsplayer_handle tsPlayerHandle;
    am_tsplayer_result ret = AmTsPlayer_create(tsPlayerInitParam, &tsPlayerHandle);
    if (ret != AM_TSPLAYER_OK) {
        ALOGI("Create tsplayer fail");
        return -1;
    }

    ret = AmTsPlayer_setWorkMode(tsPlayerHandle, TS_PLAYER_MODE_NORMAL);
    ALOGI("TsPlayer set Workmode NORMAL %s, result(%d)", (ret)? "FAIL" : "OK", ret);
    ret = AmTsPlayer_setSyncMode(tsPlayerHandle, TS_SYNC_PCRMASTER );
    ALOGI(" TsPlayer set Syncmode PCRMASTER %s, result(%d)", (ret)? "FAIL" : "OK", ret);

    mPlaybackOpenParams.playback_handle = (Playback_DeviceHandle_t)tsPlayerHandle;

    int error = dvr_wrapper_open_playback(&mDVRPlayerHandle, &mPlaybackOpenParams);
    if (error < 0) {
        ALOGE("open playback failed!");
        return error;
    }

    if (mIsEncryptStream) {
        dvr_wrapper_set_playback_secure_buffer(mDVRPlayerHandle, mSecureBuffer, mSecureBufferSize);
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
    AmTsPlayer_release((am_tsplayer_handle)mPlaybackOpenParams.playback_handle);
    return 0;
}

int AmlDVRPlayer::pause()
{
    MLOG();

    int ret = 0;

    ret = dvr_wrapper_pause_playback(mDVRPlayerHandle);
    if (ret < 0) {
        ALOGE("pause playback failed!");
    }

    return ret;
}

int AmlDVRPlayer::resume()
{
    MLOG();

    int ret = 0;

    ret = dvr_wrapper_resume_playback(mDVRPlayerHandle);
    if (ret < 0) {
        ALOGE("resume playback failed!");
    }

    return ret;
}

int AmlDVRPlayer::seek(int timeOffset)
{
    MLOG("timeOffset:%d", timeOffset);

    int ret = dvr_wrapper_seek_playback(mDVRPlayerHandle, timeOffset);
    if (ret < 0) {
        ALOGE("seek playback %d failed!", timeOffset);
    }

    return ret;
}

int AmlDVRPlayer::setPlaybackRate(float rate)
{
    MLOG("rate:%f", rate);

    int ret = 0;

    ret = dvr_wrapper_set_playback_speed(mDVRPlayerHandle, rate);
    if (ret < 0) {
        ALOGE("set playback speed failed!");
    }

    return 0;
}

int AmlDVRPlayer::getStatus(Aml_MP_DVRPlayerStatus* status)
{
    MLOG();

    DVR_WrapperPlaybackStatus_t dvrStatus;
    int ret = dvr_wrapper_get_playback_status(mDVRPlayerHandle, &dvrStatus);
    if (ret < 0) {
        ALOGE("get playback status failed!");
        return ret;
    }

    status->state = convertToMpDVRPlayerState(dvrStatus.state);
    convertToMpDVRSourceInfo(&status->infoCur, &dvrStatus.info_cur);
    convertToMpDVRSourceInfo(&status->infoFull, &dvrStatus.info_full);

    convertToMpDVRStream(&status->pids.streams[AML_MP_DVR_VIDEO_INDEX], &dvrStatus.pids.video);
    convertToMpDVRStream(&status->pids.streams[AML_MP_DVR_AUDIO_INDEX], &dvrStatus.pids.audio);
    convertToMpDVRStream(&status->pids.streams[AML_MP_DVR_AD_INDEX], &dvrStatus.pids.ad);
    convertToMpDVRStream(&status->pids.streams[AML_MP_DVR_SUBTITLE_INDEX], &dvrStatus.pids.subtitle);
    convertToMpDVRStream(&status->pids.streams[AML_MP_DVR_PCR_INDEX], &dvrStatus.pids.pcr);

    status->pids.nbStreams = AML_MP_DVR_STREAM_NB;
    status->speed = dvrStatus.speed;
    status->flags = convertToMpDVRSegmentFlag(dvrStatus.flags);
    convertToMpDVRSourceInfo(&status->infoObsolete, &dvrStatus.info_obsolete);

    return 0;
}

int AmlDVRPlayer::showVideo()
{
    MLOG();

    am_tsplayer_result ret;
    ret = AmTsPlayer_showVideo((am_tsplayer_handle)mPlaybackOpenParams.playback_handle);

    return ret;
}

int AmlDVRPlayer::hideVideo()
{
    MLOG();

    am_tsplayer_result ret;
    ret = AmTsPlayer_hideVideo((am_tsplayer_handle)mPlaybackOpenParams.playback_handle);

    return ret;
}

int AmlDVRPlayer::setVolume(float volume)
{
    MLOG("volume:%f", volume);

    am_tsplayer_result ret;
    ret = AmTsPlayer_setAudioVolume((am_tsplayer_handle)mPlaybackOpenParams.playback_handle, volume);

    return ret;
}

int AmlDVRPlayer::getVolume(float* volume)
{
    MLOG();

    am_tsplayer_result ret;
    int32_t tsVolume;
    ret = AmTsPlayer_getAudioVolume((am_tsplayer_handle)mPlaybackOpenParams.playback_handle, &tsVolume);
    *volume = tsVolume;

    return ret;
}

int AmlDVRPlayer::setParameter(Aml_MP_PlayerParameterKey key, void* parameter)
{
    am_tsplayer_result ret = AM_TSPLAYER_ERROR_INVALID_PARAMS;
    Aml_MP_ADVolume* ADVolume;
    am_tsplayer_handle mPlayer = (am_tsplayer_handle)mPlaybackOpenParams.playback_handle;

    ALOGI("Call setParameter, key is %#x", key);
    switch (key) {
        case AML_MP_PLAYER_PARAMETER_VIDEO_DISPLAY_MODE:
            //ALOGI("trace setParameter, AML_MP_PLAYER_PARAMETER_VIDEO_DISPLAY_MODE, value is %d", *(am_tsplayer_video_match_mode*)parameter);
            ret = AmTsPlayer_setVideoMatchMode(mPlayer, *(am_tsplayer_video_match_mode*)parameter);
            break;
        case AML_MP_PLAYER_PARAMETER_BLACK_OUT:
            //ALOGI("trace setParameter, AML_MP_PLAYER_PARAMETER_BLACK_OUT, value is %d", *(bool_t*)parameter);
            ret = AmTsPlayer_setVideoBlackOut(mPlayer, *(bool_t*)parameter);
            break;
        case AML_MP_PLAYER_PARAMETER_AUDIO_OUTPUT_MODE:
            //ALOGI("trace setParameter, AML_MP_PLAYER_PARAMETER_AUDIO_OUTPUT_MODE, value is %d", *(Aml_MP_AudioOutputMode*)parameter);
            ret = AmTsPlayer_setAudioOutMode(mPlayer, convertToTsPlayerAudioOutMode(*(Aml_MP_AudioOutputMode*)parameter));
            break;

        case AML_MP_PLAYER_PARAMETER_AUDIO_BALANCE:
            ret = AM_TSPLAYER_OK;
            break;

        case AML_MP_PLAYER_PARAMETER_AD_MIX_LEVEL:
            ADVolume = (Aml_MP_ADVolume*)parameter;
            //ALOGI("trace setParameter, AML_MP_PLAYER_PARAMETER_AD_MIX_LEVEL, AML_MP_PLAYER_PARAMETER_AUDIO_OUTPUT_MODE, value is master %d, slave %d", ADVolume->masterVolume, ADVolume->slaveVolume);
            ret = AmTsPlayer_setADMixLevel(mPlayer, ADVolume->masterVolume, ADVolume->slaveVolume);
            break;
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
    am_tsplayer_handle mPlayer = (am_tsplayer_handle)mPlaybackOpenParams.playback_handle;

    ALOGI("Call getParameter, key is %d", key);
    if (!parameter) {
        return -1;
    }

    switch (key) {
        case AML_MP_PLAYER_PARAMETER_VIDEO_INFO:
            am_tsplayer_video_info videoInfo;
            ret = AmTsPlayer_getVideoInfo(mPlayer, &videoInfo);
            convertToMpVideoInfo((Aml_MP_VideoInfo*)parameter, &videoInfo);
            //ALOGI("trace getParameter, AML_MP_PLAYER_PARAMETER_VIDEO_INFO, width: %d, height: %d, framerate: %d, bitrate: %d, ratio64: %llu", videoInfo.width, videoInfo.height, videoInfo.framerate, videoInfo.bitrate, videoInfo.ratio64);
            break;
        case AML_MP_PLAYER_PARAMETER_VIDEO_DECODE_STAT:
            ret = AmTsPlayer_getVideoStat(mPlayer, (am_tsplayer_vdec_stat*)parameter);
            //am_tsplayer_vdec_stat* vdec_stat;
            //vdec_stat = (am_tsplayer_vdec_stat*)parameter;
            //ALOGI("trace getParameter, AML_MP_PLAYER_PARAMETER_VIDEO_DECODE_STAT, frame_width: %d, frame_height: %d, frame_rate: %d", vdec_stat->frame_width, vdec_stat->frame_height, vdec_stat->frame_rate);
            break;
        case AML_MP_PLAYER_PARAMETER_AUDIO_INFO:
            ret = AmTsPlayer_getAudioInfo(mPlayer, (am_tsplayer_audio_info*)parameter);
            //am_tsplayer_audio_info* audioInfo;
            //audioInfo = (am_tsplayer_audio_info*)parameter;
            //ALOGI("trace getParameter, AML_MP_PLAYER_PARAMETER_AUDIO_INFO, sample_rate: %d, channels: %d, channel_mask: %d, bitrate: %d", audioInfo->sample_rate, audioInfo->channels, audioInfo->channel_mask, audioInfo->bitrate);
            break;
        case AML_MP_PLAYER_PARAMETER_AUDIO_DECODE_STAT:
            ret = AmTsPlayer_getAudioStat(mPlayer, (am_tsplayer_adec_stat*) parameter);
            //am_tsplayer_adec_stat* adec_stat;
            //adec_stat = (am_tsplayer_adec_stat*)parameter;
            //ALOGI("trace getParameter, AML_MP_PLAYER_PARAMETER_AUDIO_DECODE_STAT, frame_count: %d, error_frame_count: %d, drop_frame_count: %d", adec_stat->frame_count, adec_stat->error_frame_count, adec_stat->drop_frame_count);
            break;
        case AML_MP_PLAYER_PARAMETER_AD_INFO:
            ret = AmTsPlayer_getADInfo(mPlayer, (am_tsplayer_audio_info*)parameter);
            //am_tsplayer_audio_info* adInfo;
            //adInfo = (am_tsplayer_audio_info*)parameter;
            //ALOGI("trace getParameter, AML_MP_PLAYER_PARAMETER_AUDIO_INFO, sample_rate: %d, channels: %d, channel_mask: %d, bitrate: %d", adInfo->sample_rate, adInfo->channels, adInfo->channel_mask, adInfo->bitrate);
            break;
        case AML_MP_PLAYER_PARAMETER_AD_DECODE_STAT:
            ret = AmTsPlayer_getADStat(mPlayer, (am_tsplayer_adec_stat*)parameter);
            //am_tsplayer_adec_stat* ad_stat;
            //ad_stat = (am_tsplayer_adec_stat*)parameter;
            //ALOGI("trace getParameter, AML_MP_PLAYER_PARAMETER_AUDIO_DECODE_STAT, frame_count: %d, error_frame_count: %d, drop_frame_count: %d", ad_stat->frame_count, ad_stat->error_frame_count, ad_stat->drop_frame_count);
            break;
        default:
            ret = AM_TSPLAYER_ERROR_INVALID_PARAMS;
    }
    if (ret != AM_TSPLAYER_OK) {
        return -1;
    }
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

    ALOGI("mSecureBuffer:%p, mSecureBufferSize:%d", mSecureBuffer, mSecureBufferSize);

    return 0;
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


}
