/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_TAG "AmlTsPlayer"
#include <utils/Log.h>
#include "AmlTsPlayer.h"
#include <AmTsPlayer.h>
#include <utils/AmlMpUtils.h>

#define VOLUME_MULTIPLE 100.0;

namespace aml_mp {

am_tsplayer_video_codec videoCodecConvert(Aml_MP_CodecID aml_MP_VideoCodec) {
    switch (aml_MP_VideoCodec) {
        case AML_MP_VIDEO_CODEC_MPEG12:
            return AV_VIDEO_CODEC_MPEG2;
        case AML_MP_VIDEO_CODEC_MPEG4:
            return AV_VIDEO_CODEC_MPEG4;
        case AML_MP_VIDEO_CODEC_H264:
            return AV_VIDEO_CODEC_H264;
        case AML_MP_VIDEO_CODEC_AVS:
            return AV_VIDEO_CODEC_AVS;
        case AML_MP_VIDEO_CODEC_VP9:
            return AV_VIDEO_CODEC_VP9;
        case AML_MP_VIDEO_CODEC_HEVC:
            return AV_VIDEO_CODEC_H265;
        default:
            return AV_VIDEO_CODEC_AUTO;
    }
}

am_tsplayer_audio_codec audioCodecConvert(Aml_MP_CodecID aml_MP_AudioCodec) {
    switch (aml_MP_AudioCodec) {
        case AML_MP_AUDIO_CODEC_MP2:
            return AV_AUDIO_CODEC_MP2;
        case AML_MP_AUDIO_CODEC_MP3:
            return AV_AUDIO_CODEC_MP3;
        case AML_MP_AUDIO_CODEC_AC3:
            return AV_AUDIO_CODEC_AC3;
        case AML_MP_AUDIO_CODEC_EAC3:
            return AV_AUDIO_CODEC_EAC3;
        case AML_MP_AUDIO_CODEC_DTS:
            return AV_AUDIO_CODEC_DTS;
        case AML_MP_AUDIO_CODEC_AAC:
            return AV_AUDIO_CODEC_AAC;
        case AML_MP_AUDIO_CODEC_LATM:
            return AV_AUDIO_CODEC_LATM;
        case AML_MP_AUDIO_CODEC_PCM:
            return AV_AUDIO_CODEC_PCM;
        default:
            return AV_AUDIO_CODEC_AUTO;
    }
}

am_tsplayer_stream_type streamTypeConvert(Aml_MP_StreamType streamType) {
    switch (streamType) {
        case AML_MP_STREAM_TYPE_VIDEO:
            return TS_STREAM_VIDEO;
        case AML_MP_STREAM_TYPE_AUDIO:
            return TS_STREAM_AUDIO;
        case AML_MP_STREAM_TYPE_AD:
            return TS_STREAM_AD;
        case AML_MP_STREAM_TYPE_SUBTITLE:
            return TS_STREAM_SUB;
        default:
            break;
    }

    return (am_tsplayer_stream_type)-1;
}

Aml_MP_StreamType streamTypeConvert(am_tsplayer_stream_type streamType) {
    switch (streamType) {
        case TS_STREAM_VIDEO:
            return AML_MP_STREAM_TYPE_VIDEO;
        case TS_STREAM_AUDIO:
            return AML_MP_STREAM_TYPE_AUDIO;
        case TS_STREAM_AD:
            return AML_MP_STREAM_TYPE_AD;
        case TS_STREAM_SUB:
            return AML_MP_STREAM_TYPE_SUBTITLE;
    }

    return (AML_MP_STREAM_TYPE_UNKNOWN);
}

am_tsplayer_input_source_type sourceTypeConvert(Aml_MP_InputSourceType sourceType) {
    switch (sourceType) {
    case AML_MP_INPUT_SOURCE_TS_MEMORY:
        return TS_MEMORY;

    case AML_MP_INPUT_SOURCE_TS_DEMOD:
        return TS_DEMOD;
    }
}

am_tsplayer_input_buffer_type inputStreamTypeConvert(Aml_MP_InputStreamType streamType) {
    switch (streamType) {
    case AML_MP_INPUT_STREAM_NORMAL:
        return TS_INPUT_BUFFER_TYPE_NORMAL;

    case AML_MP_INPUT_STREAM_ENCRYPTED:
        return TS_INPUT_BUFFER_TYPE_TVP;
    }

    return TS_INPUT_BUFFER_TYPE_NORMAL;
}

am_tsplayer_audio_stereo_mode audioBalanceConvert(Aml_MP_AudioBalance balance)
{
    switch (balance) {
    case AML_MP_AUDIO_BALANCE_STEREO:
        return AV_AUDIO_STEREO;

    case AML_MP_AUDIO_BALANCE_LEFT:
        return AV_AUDIO_LEFT;

    case AML_MP_AUDIO_BALANCE_RIGHT:
        return AV_AUDIO_RIGHT;

    case AML_MP_AUDIO_BALANCE_SWAP:
        return AV_AUDIO_SWAP;

    case AML_MP_AUDIO_BALANCE_LRMIX:
        return AV_AUDIO_LRMIX;
    }
}

am_tsplayer_avsync_mode AVSyncSourceTypeConvert(Aml_MP_AVSyncSource avSyncSource) {
    switch (avSyncSource) {
        case AML_MP_AVSYNC_SOURCE_VIDEO:
            return TS_SYNC_VMASTER;
        case AML_MP_AVSYNC_SOURCE_AUDIO:
            return TS_SYNC_AMASTER;
        default:
            return TS_SYNC_PCRMASTER;
    }
}

AmlTsPlayer::AmlTsPlayer(Aml_MP_PlayerCreateParams* createParams, int instanceId)
: aml_mp::AmlPlayerBase(instanceId)
{
    if (createParams->demuxId == AML_MP_DEMUX_ID_DEFAULT) {
        createParams->demuxId = AML_MP_HW_DEMUX_ID_0;
    }

    if (createParams->sourceType == AML_MP_INPUT_SOURCE_TS_MEMORY) {
        ALOGI("set demux source AML_MP_DEMUX_SOURCE_DMA0");
        Aml_MP_SetDemuxSource(createParams->demuxId, AML_MP_DEMUX_SOURCE_DMA0);
    } else {
        Aml_MP_DemuxSource demuxSource = AML_MP_DEMUX_SOURCE_TS0;
        switch (createParams->demuxId) {
        case AML_MP_HW_DEMUX_ID_0:
            demuxSource = AML_MP_DEMUX_SOURCE_TS0;
            break;

        case AML_MP_HW_DEMUX_ID_1:
            demuxSource = AML_MP_DEMUX_SOURCE_TS1;
            break;

        case AML_MP_HW_DEMUX_ID_2:
            demuxSource = AML_MP_DEMUX_SOURCE_TS2;
            break;

        default:
            break;
        }

        Aml_MP_SetDemuxSource(createParams->demuxId, demuxSource);
    }

    init_param.source = sourceTypeConvert(createParams->sourceType);
    init_param.drmmode = inputStreamTypeConvert(createParams->drmMode);
    init_param.dmx_dev_id = createParams->demuxId;
    init_param.event_mask = 0;

    AmTsPlayer_create(init_param, &mPlayer);

    AmTsPlayer_registerCb(mPlayer, [](void *user_data, am_tsplayer_event *event) {
        static_cast<AmlTsPlayer*>(user_data)->eventCallback(event);
    }, this);
}

AmlTsPlayer::~AmlTsPlayer()
{
    ALOGI("%s:%d", __FUNCTION__, __LINE__);

    if (mPlayer) {
        AmTsPlayer_release(mPlayer);
        mPlayer = AML_MP_INVALID_HANDLE;
    }
}

int AmlTsPlayer::setVideoParams(const Aml_MP_VideoParams* params) {
    am_tsplayer_result ret;
    am_tsplayer_video_params video_params = {videoCodecConvert(params->videoCodec), params->pid};

    ALOGI("amtsplayer handle:%#x, video codec:%d, pid:%d", mPlayer, video_params.codectype, video_params.pid);
    ret = AmTsPlayer_setVideoParams(mPlayer, &video_params);
    if (ret != AM_TSPLAYER_OK) {
        return -1;
    }
    return 0;
}

int AmlTsPlayer::setAudioParams(const Aml_MP_AudioParams* params) {
    am_tsplayer_result ret;
    am_tsplayer_audio_params audio_params = {audioCodecConvert(params->audioCodec), params->pid};

    ALOGI("amtsplayer handle:%#x, audio codec:%d, pid:%d", mPlayer, audio_params.codectype, audio_params.pid);
    ret = AmTsPlayer_setAudioParams(mPlayer, &audio_params);
    if (ret != AM_TSPLAYER_OK) {
        return -1;
    }
    return 0;
}

int AmlTsPlayer::start() {
    int ret = 0;

    ALOGI("Call start");
    AmTsPlayer_setTrickMode(mPlayer, AV_VIDEO_TRICK_MODE_NONE);

    ret += startVideoDecoding();
    ret += startAudioDecoding();
    //ret += showVideo();

    AmlPlayerBase::start();

    return ret;
}

int AmlTsPlayer::stop() {
    int ret = 0;

    ALOGI("Call stop");
    AmlPlayerBase::stop();

    ret += stopVideoDecoding();
    ret += stopAudioDecoding();

    return ret;
}

int AmlTsPlayer::pause() {
    int ret = 0;

    ALOGI("Call pause");
    ret += pauseVideoDecoding();
    ret += pauseAudioDecoding();
    return ret;
}

int AmlTsPlayer::resume() {
    int ret = 0;

    ALOGI("Call resume");
    ret += resumeVideoDecoding();
    ret += resumeAudioDecoding();
    return ret;
}

int AmlTsPlayer::flush() {return -1;}

int AmlTsPlayer::setPlaybackRate(float rate){
    am_tsplayer_result ret = AM_TSPLAYER_ERROR_INVALID_PARAMS;
    ALOGI("setPlaybackRate, rate: %f", rate);
    if (rate == 1.0f) {
        ret = AmTsPlayer_stopFast(mPlayer);
    } else {
        ret = AmTsPlayer_startFast(mPlayer, rate);
    }
    if (ret != AM_TSPLAYER_OK)
        return -1;
    return 0;
}

int AmlTsPlayer::switchAudioTrack(const Aml_MP_AudioParams* params){
    int ret = 0;
    ret = stopAudioDecoding();
    if (ret) {
        ALOGE("Stop Audio Decoding fail");
        return -1;
    }
    ret = setAudioParams(params);
    if (ret) {
        ALOGE("Set Audio Params fail");
        return -1;
    }
    ret = startAudioDecoding();
    if (ret) {
        ALOGE("Strat Audio Decoding fail");
        return -1;
    }
    return 0;
}

int AmlTsPlayer::writeData(const uint8_t* buffer, size_t size) {
    //AML_MP_TRACE(10);
    am_tsplayer_result ret;
    am_tsplayer_input_buffer buf ={init_param.drmmode, (void*)buffer, (int32_t)size};

    ret = AmTsPlayer_writeData(mPlayer, &buf, kRwTimeout);
    //ALOGI("writedata, buffer:%p, size:%d, ret:%d", buffer, size, ret);
    if (ret != AM_TSPLAYER_OK) {
        return -1;
    }
    return 0;
}

int AmlTsPlayer::writeEsData(Aml_MP_StreamType type, const uint8_t* buffer, size_t size, int64_t pts)
{
    AML_MP_UNUSED(type);
    AML_MP_UNUSED(buffer);
    AML_MP_UNUSED(size);
    AML_MP_UNUSED(pts);

    return -1;
}

int AmlTsPlayer::getCurrentPts(Aml_MP_StreamType type, int64_t* pts) {
    am_tsplayer_result ret;

    ret = AmTsPlayer_getPts(mPlayer, streamTypeConvert(type), (uint64_t*)pts);
    ALOGI("getCurrentPts type: %d, pts: 0x%llx, ret: %d", type, *pts, ret);
    if (ret != AM_TSPLAYER_OK) {
        return -1;
    }
    return 0;
}

int AmlTsPlayer::getBufferStat(Aml_MP_BufferStat* bufferStat) {
    am_tsplayer_result ret;
    am_tsplayer_buffer_stat buffer_stat;

    ret = AmTsPlayer_getBufferStat(mPlayer, TS_STREAM_AUDIO, &buffer_stat);
    if (ret != AM_TSPLAYER_OK) {
        ALOGE("Get audio buffer error, ret = %d", ret);
        return -1;
    }
    bufferStat->audioBuffer.size = buffer_stat.size;
    bufferStat->audioBuffer.dataLen = buffer_stat.data_len;

    ret = AmTsPlayer_getBufferStat(mPlayer, TS_STREAM_VIDEO, &buffer_stat);
    bufferStat->videoBuffer.size = buffer_stat.size;
    bufferStat->videoBuffer.dataLen = buffer_stat.data_len;
    if (ret != AM_TSPLAYER_OK) {
        ALOGE("Get video buffer error, ret = %d", ret);
        return -1;
    }
    return 0;

}

int AmlTsPlayer::setVideoWindow(int x, int y, int width, int height) {
    am_tsplayer_result ret;

    ALOGI("setVideoWindow, x: %d, y: %d, width: %d, height: %d", x, y, width, height);
    ret = AmTsPlayer_setVideoWindow(mPlayer, x, y, width, height);
    if (ret != AM_TSPLAYER_OK) {
        return -1;
    }
    return 0;
}

int AmlTsPlayer::setVolume(float volume) {
    am_tsplayer_result ret;
    int32_t tsplayer_volume = volume * VOLUME_MULTIPLE;

    ALOGI("setVolume, tsplayer_volume: %d", tsplayer_volume);
    ret = AmTsPlayer_setAudioVolume(mPlayer, tsplayer_volume);
    if (ret != AM_TSPLAYER_OK) {
        return -1;
    }
    return 0;
}

int AmlTsPlayer::getVolume(float* volume) {
    am_tsplayer_result ret;
    int32_t tsplayer_volume;

    ret = AmTsPlayer_getAudioVolume(mPlayer, &tsplayer_volume);
    ALOGI("getVolume volume: %d, ret: %d", tsplayer_volume, ret);
    if (ret != AM_TSPLAYER_OK) {
        return -1;
    }
    *volume = tsplayer_volume / VOLUME_MULTIPLE;
    return 0;
}

int AmlTsPlayer::showVideo() {
    am_tsplayer_result ret;

    ret = AmTsPlayer_showVideo(mPlayer);
    if (ret != AM_TSPLAYER_OK) {
        return -1;
    }
    return 0;
}

int AmlTsPlayer::hideVideo() {
    am_tsplayer_result ret;

    ret = AmTsPlayer_hideVideo(mPlayer);
    if (ret != AM_TSPLAYER_OK) {
        return -1;
    }
    return 0;
}

int AmlTsPlayer::setParameter(Aml_MP_PlayerParameterKey key, void* parameter) {
    am_tsplayer_result ret = AM_TSPLAYER_ERROR_INVALID_PARAMS;
    Aml_MP_ADVolume* ADVolume;

    ALOGI("Call setParameter, key is %d", key);
    switch (key) {
        case AML_MP_PLAYER_PARAMETER_VIDEO_DISPLAY_MODE:
            //ALOGI("trace setParameter, AML_MP_PLAYER_PARAMETER_VIDEO_DISPLAY_MODE, value is %d", *(am_tsplayer_video_match_mode*)parameter);
            ret = AmTsPlayer_setVideoMatchMode(mPlayer, *(am_tsplayer_video_match_mode*)parameter);
            break;
        case AML_MP_PLAYER_PARAMETER_BLACK_OUT:
            //ALOGI("trace setParameter, AML_MP_PLAYER_PARAMETER_BLACK_OUT, value is %d", *(bool_t*)parameter);
            ret = AmTsPlayer_setVideoBlackOut(mPlayer, *(bool_t*)parameter);
            break;

        case AML_MP_PLAYER_PARAMETER_VIDEO_DECODE_MODE:
            break;

        case AML_MP_PLAYER_PARAMETER_VIDEO_PTS_OFFSET:
            break;

        case AML_MP_PLAYER_PARAMETER_AUDIO_OUTPUT_MODE:
            //ALOGI("trace setParameter, AML_MP_PLAYER_PARAMETER_AUDIO_OUTPUT_MODE, value is %d", *(Aml_MP_AudioOutputMode*)parameter);
            ret = AmTsPlayer_setAudioOutMode(mPlayer, convertToTsPlayerAudioOutMode(*(Aml_MP_AudioOutputMode*)parameter));
            break;

        case AML_MP_PLAYER_PARAMETER_AUDIO_OUTPUT_DEVICE:
            break;

        case AML_MP_PLAYER_PARAMETER_AUDIO_PTS_OFFSET:
            break;

        case AML_MP_PLAYER_PARAMETER_AUDIO_BALANCE:
            ret = AmTsPlayer_setAudioStereoMode(mPlayer, audioBalanceConvert(*(Aml_MP_AudioBalance*)parameter));
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

int AmlTsPlayer::getParameter(Aml_MP_PlayerParameterKey key, void* parameter) {
    am_tsplayer_result ret = AM_TSPLAYER_ERROR_INVALID_PARAMS;

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

int AmlTsPlayer::setAVSyncSource(Aml_MP_AVSyncSource syncSource)
{
    am_tsplayer_result ret = AM_TSPLAYER_OK;

    ALOGI("setsyncmode, syncSource %d!!!", syncSource);
    ALOGI("converted syncSoource is: %d", AVSyncSourceTypeConvert(syncSource));
    ret = AmTsPlayer_setSyncMode(mPlayer, AVSyncSourceTypeConvert(syncSource));
    if (ret != AM_TSPLAYER_OK) {
        return -1;
    }

    return 0;
}

int AmlTsPlayer::setPcrPid(int pid) {
    am_tsplayer_result ret = AM_TSPLAYER_ERROR_INVALID_PARAMS;

    ret = AmTsPlayer_setPcrPid(mPlayer, (uint32_t)pid);

    if (ret != AM_TSPLAYER_OK) {
        return -1;
    }
    return 0;
}

int AmlTsPlayer::startVideoDecoding() {
    am_tsplayer_result ret = AM_TSPLAYER_ERROR_INVALID_PARAMS;

    ret = AmTsPlayer_startVideoDecoding(mPlayer);

    if (ret != AM_TSPLAYER_OK) {
        return -1;
    }
    return 0;
}

int AmlTsPlayer::stopVideoDecoding() {
    am_tsplayer_result ret = AM_TSPLAYER_ERROR_INVALID_PARAMS;

    ret = AmTsPlayer_stopVideoDecoding(mPlayer);

    if (ret != AM_TSPLAYER_OK) {
        return -1;
    }
    return 0;

}

int AmlTsPlayer::pauseVideoDecoding() {
    am_tsplayer_result ret = AM_TSPLAYER_ERROR_INVALID_PARAMS;

    ret = AmTsPlayer_pauseVideoDecoding(mPlayer);

    if (ret != AM_TSPLAYER_OK) {
        return -1;
    }
    return 0;
}

int AmlTsPlayer::resumeVideoDecoding() {
    am_tsplayer_result ret = AM_TSPLAYER_ERROR_INVALID_PARAMS;

    ret = AmTsPlayer_resumeVideoDecoding(mPlayer);

    if (ret != AM_TSPLAYER_OK) {
        return -1;
    }
    return 0;
}

int AmlTsPlayer::startAudioDecoding() {
    am_tsplayer_result ret = AM_TSPLAYER_ERROR_INVALID_PARAMS;

    ret = AmTsPlayer_startAudioDecoding(mPlayer);

    if (ret != AM_TSPLAYER_OK) {
        return -1;
    }
    return 0;
}

int AmlTsPlayer::stopAudioDecoding() {
    am_tsplayer_result ret = AM_TSPLAYER_ERROR_INVALID_PARAMS;

    ret = AmTsPlayer_stopAudioDecoding(mPlayer);

    if (ret != AM_TSPLAYER_OK) {
        return -1;
    }
    return 0;
}

int AmlTsPlayer::pauseAudioDecoding() {
    am_tsplayer_result ret = AM_TSPLAYER_ERROR_INVALID_PARAMS;

    ret = AmTsPlayer_pauseAudioDecoding(mPlayer);

    if (ret != AM_TSPLAYER_OK) {
        return -1;
    }
    return 0;
}

int AmlTsPlayer::resumeAudioDecoding() {
    am_tsplayer_result ret = AM_TSPLAYER_ERROR_INVALID_PARAMS;

    ret = AmTsPlayer_resumeAudioDecoding(mPlayer);

    if (ret != AM_TSPLAYER_OK) {
        return -1;
    }
    return 0;
}


int AmlTsPlayer::setADParams(Aml_MP_AudioParams* params) {
    am_tsplayer_result ret = AM_TSPLAYER_ERROR_INVALID_PARAMS;
    am_tsplayer_audio_params audioParams;

    audioParams.pid = (int32_t)(params->pid);
    audioParams.codectype = audioCodecConvert(params->audioCodec);

    ret = AmTsPlayer_setADParams(mPlayer, &audioParams);

    if (ret != AM_TSPLAYER_OK) {
        return -1;
    }
    return 0;
}

void AmlTsPlayer::eventCallback(am_tsplayer_event* event)
{
    switch (event->type) {
    case AM_TSPLAYER_EVENT_TYPE_VIDEO_CHANGED:
    {
        ALOGE("[evt] AML_MP_PLAYER_EVENT_VIDEO_CHANGED");

        Aml_MP_PlayerEventVideoFormat videoFormatEvent;
        videoFormatEvent.frame_width = event->event.video_format.frame_width;
        videoFormatEvent.frame_height = event->event.video_format.frame_height;
        videoFormatEvent.frame_rate = event->event.video_format.frame_rate;
        videoFormatEvent.frame_aspectratio = event->event.video_format.frame_aspectratio;

        notifyListener(AML_MP_PLAYER_EVENT_VIDEO_CHANGED, (int64_t)&videoFormatEvent);
    }
    break;

    case AM_TSPLAYER_EVENT_TYPE_AUDIO_CHANGED:
        notifyListener(AML_MP_PLAYER_EVENT_AUDIO_CHANGED);
        break;

    case AM_TSPLAYER_EVENT_TYPE_FIRST_FRAME:
        ALOGE("[evt] AM_TSPLAYER_EVENT_TYPE_FIRST_FRAME\n");

        notifyListener(AML_MP_PLAYER_EVENT_FIRST_FRAME);
        break;

    case AM_TSPLAYER_EVENT_TYPE_AV_SYNC_DONE:
        ALOGE("[evt] AML_MP_PLAYER_EVENT_AV_SYNC_DONE");

        notifyListener(AML_MP_PLAYER_EVENT_AV_SYNC_DONE);
        break;

    case AM_TSPLAYER_EVENT_TYPE_DATA_LOSS:
        notifyListener(AML_MP_PLAYER_EVENT_DATA_LOSS);
        break;

    case AM_TSPLAYER_EVENT_TYPE_DATA_RESUME:
        notifyListener(AML_MP_PLAYER_EVENT_DATA_RESUME);
        break;

    case AM_TSPLAYER_EVENT_TYPE_SCRAMBLING:
    {
        Aml_MP_PlayerEventScrambling scrambling;
        scrambling.scramling = 1;
        scrambling.type = streamTypeConvert(event->event.scramling.stream_type);

        notifyListener(AML_MP_PLAYER_EVENT_SCRAMBLING, (int64_t)&scrambling);
    }
    break;

    case AM_TSPLAYER_EVENT_TYPE_USERDATA_AFD:
    {
        Aml_MP_PlayerEventMpegUserData userData;
        userData.data = event->event.mpeg_user_data.data;
        userData.len = event->event.mpeg_user_data.len;

        notifyListener(AML_MP_PLAYER_EVENT_USERDATA_AFD, (int64_t)&userData);
    }
    break;

    case AM_TSPLAYER_EVENT_TYPE_USERDATA_CC:
    {
        Aml_MP_PlayerEventMpegUserData userData;
        userData.data = event->event.mpeg_user_data.data;
        userData.len = event->event.mpeg_user_data.len;

        notifyListener(AML_MP_PLAYER_EVENT_USERDATA_CC, (int64_t)&userData);
    }
    break;

    default:
        ALOGE("unhandled event:%d", event->type);
        break;
    }
}

}

