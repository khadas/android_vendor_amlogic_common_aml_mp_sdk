/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_TAG "AmlMpUtils"
#include <utils/Log.h>
#include "AmlMpUtils.h"
#include <unistd.h>
#ifdef HAVE_SUBTITLE
#include <SubtitleNativeAPI.h>
#endif

#ifdef ANDROID
#include <amlogic/am_gralloc_ext.h>
#include <system/window.h>
#include <video_tunnel.h>
#include <hardware/gralloc1.h>
#endif

#ifdef ANDROID
#ifndef __ANDROID_VNDK__
#include <gui/Surface.h>
#endif
#endif

static const char* mName = LOG_TAG;

namespace aml_mp {
#define ENUM_TO_STR(e) case e: return TO_STR(e); break
#define TO_STR(v) #v

const char* mpCodecId2Str(Aml_MP_CodecID codecId)
{
    switch (codecId) {
        ENUM_TO_STR(AML_MP_CODEC_UNKNOWN);
        ENUM_TO_STR(AML_MP_VIDEO_CODEC_BASE);
        ENUM_TO_STR(AML_MP_VIDEO_CODEC_MPEG12);
        ENUM_TO_STR(AML_MP_VIDEO_CODEC_MPEG4);
        ENUM_TO_STR(AML_MP_VIDEO_CODEC_H264);
        ENUM_TO_STR(AML_MP_VIDEO_CODEC_VC1);
        ENUM_TO_STR(AML_MP_VIDEO_CODEC_AVS);
        ENUM_TO_STR(AML_MP_VIDEO_CODEC_HEVC);
        ENUM_TO_STR(AML_MP_VIDEO_CODEC_VP9);
        ENUM_TO_STR(AML_MP_VIDEO_CODEC_AVS2);

        ENUM_TO_STR(AML_MP_AUDIO_CODEC_BASE);
        ENUM_TO_STR(AML_MP_AUDIO_CODEC_MP2);
        ENUM_TO_STR(AML_MP_AUDIO_CODEC_MP3);
        ENUM_TO_STR(AML_MP_AUDIO_CODEC_AC3);
        ENUM_TO_STR(AML_MP_AUDIO_CODEC_EAC3);
        ENUM_TO_STR(AML_MP_AUDIO_CODEC_DTS);
        ENUM_TO_STR(AML_MP_AUDIO_CODEC_AAC);
        ENUM_TO_STR(AML_MP_AUDIO_CODEC_LATM);
        ENUM_TO_STR(AML_MP_AUDIO_CODEC_PCM);
        ENUM_TO_STR(AML_MP_AUDIO_CODEC_AC4);

        ENUM_TO_STR(AML_MP_SUBTITLE_CODEC_BASE);
        ENUM_TO_STR(AML_MP_SUBTITLE_CODEC_CC);
        ENUM_TO_STR(AML_MP_SUBTITLE_CODEC_SCTE27);
        ENUM_TO_STR(AML_MP_SUBTITLE_CODEC_DVB);
        ENUM_TO_STR(AML_MP_SUBTITLE_CODEC_TELETEXT);
        default: return "unknown codec Id";
    }
}

const char* mpStreamType2Str(Aml_MP_StreamType streamType)
{
    switch (streamType) {
        case AML_MP_STREAM_TYPE_VIDEO: return "VIDEO";
        case AML_MP_STREAM_TYPE_AUDIO: return "AUDIO";
        case AML_MP_STREAM_TYPE_AD: return "AD";
        case AML_MP_STREAM_TYPE_SUBTITLE: return "SUBTITLE";
        case AML_MP_STREAM_TYPE_TELETEXT: return "TELETEXT";
        case AML_MP_STREAM_TYPE_ECM: return "ECM";
        case AML_MP_STREAM_TYPE_EMM: return "EMM";
        case AML_MP_STREAM_TYPE_SECTION: return "SECTION";
        case AML_MP_STREAM_TYPE_UNKNOWN:
        default:
            return "UNKNOWN";
    }
}

const char* mpDemuxId2Str(Aml_MP_DemuxId demuxId) {
    switch (demuxId) {
        ENUM_TO_STR(AML_MP_DEMUX_ID_DEFAULT);
        ENUM_TO_STR(AML_MP_HW_DEMUX_ID_0);
        ENUM_TO_STR(AML_MP_HW_DEMUX_ID_1);
        ENUM_TO_STR(AML_MP_HW_DEMUX_ID_2);
        ENUM_TO_STR(AML_MP_HW_DEMUX_ID_3);
        ENUM_TO_STR(AML_MP_HW_DEMUX_ID_4);
        ENUM_TO_STR(AML_MP_HW_DEMUX_ID_5);
        ENUM_TO_STR(AML_MP_HW_DEMUX_ID_6);
        ENUM_TO_STR(AML_MP_HW_DEMUX_ID_7);
        ENUM_TO_STR(AML_MP_HW_DEMUX_ID_MAX);
        default:
            return "unknown demux Id";
    }
}

const char* mpPlayerParameterKey2Str(Aml_MP_PlayerParameterKey playerParamKey) {
    switch (playerParamKey) {
        //set/get
        ENUM_TO_STR(AML_MP_PLAYER_PARAMETER_SET_BASE);
        ENUM_TO_STR(AML_MP_PLAYER_PARAMETER_VIDEO_DISPLAY_MODE);
        ENUM_TO_STR(AML_MP_PLAYER_PARAMETER_BLACK_OUT);
        ENUM_TO_STR(AML_MP_PLAYER_PARAMETER_VIDEO_DECODE_MODE);
        ENUM_TO_STR(AML_MP_PLAYER_PARAMETER_VIDEO_PTS_OFFSET);
        ENUM_TO_STR(AML_MP_PLAYER_PARAMETER_AUDIO_OUTPUT_MODE);
        ENUM_TO_STR(AML_MP_PLAYER_PARAMETER_AUDIO_OUTPUT_DEVICE);
        ENUM_TO_STR(AML_MP_PLAYER_PARAMETER_AUDIO_PTS_OFFSET);
        ENUM_TO_STR(AML_MP_PLAYER_PARAMETER_AUDIO_BALANCE);
        ENUM_TO_STR(AML_MP_PLAYER_PARAMETER_AUDIO_MUTE);
        ENUM_TO_STR(AML_MP_PLAYER_PARAMETER_CREATE_PARAMS);
        ENUM_TO_STR(AML_MP_PLAYER_PARAMETER_NETWORK_JITTER);
        ENUM_TO_STR(AML_MP_PLAYER_PARAMETER_AD_STATE);
        ENUM_TO_STR(AML_MP_PLAYER_PARAMETER_AD_MIX_LEVEL);
        ENUM_TO_STR(AML_MP_PLAYER_PARAMETER_WORK_MODE);
        ENUM_TO_STR(AML_MP_PLAYER_PARAMETER_VIDEO_WINDOW_ZORDER);
        ENUM_TO_STR(AML_MP_PLAYER_PARAMETER_TELETEXT_CONTROL);
        ENUM_TO_STR(AML_MP_PLAYER_PARAMETER_VENDOR_ID);
        ENUM_TO_STR(AML_MP_PLAYER_PARAMETER_VIDEO_TUNNEL_ID);
        ENUM_TO_STR(AML_MP_PLAYER_PARAMETER_SURFACE_HANDLE);
        ENUM_TO_STR(AML_MP_PLAYER_PARAMETER_AUDIO_PRESENTATION_ID);
        ENUM_TO_STR(AML_MP_PLAYER_PARAMETER_USE_TIF);
        ENUM_TO_STR(AML_MP_PLAYER_PARAMETER_SPDIF_PROTECTION);
        //get only
        ENUM_TO_STR(AML_MP_PLAYER_PARAMETER_GET_BASE);
        ENUM_TO_STR(AML_MP_PLAYER_PARAMETER_VIDEO_INFO);
        ENUM_TO_STR(AML_MP_PLAYER_PARAMETER_VIDEO_DECODE_STAT);
        ENUM_TO_STR(AML_MP_PLAYER_PARAMETER_AUDIO_INFO);
        ENUM_TO_STR(AML_MP_PLAYER_PARAMETER_AUDIO_DECODE_STAT);
        ENUM_TO_STR(AML_MP_PLAYER_PARAMETER_SUBTITLE_INFO);
        ENUM_TO_STR(AML_MP_PLAYER_PARAMETER_SUBTITLE_DECODE_STAT);
        ENUM_TO_STR(AML_MP_PLAYER_PARAMETER_AD_INFO);
        ENUM_TO_STR(AML_MP_PLAYER_PARAMETER_AD_DECODE_STAT);
        ENUM_TO_STR(AML_MP_PLAYER_PARAMETER_INSTANCE_ID);
        ENUM_TO_STR(AML_MP_PLAYER_PARAMETER_SYNC_ID);

        default:
            return "unknown player parameter key";
    }
}

const char* mpAVSyncSource2Str(Aml_MP_AVSyncSource syncSource) {
    switch (syncSource) {
        ENUM_TO_STR(AML_MP_AVSYNC_SOURCE_DEFAULT);
        ENUM_TO_STR(AML_MP_AVSYNC_SOURCE_VIDEO);
        ENUM_TO_STR(AML_MP_AVSYNC_SOURCE_AUDIO);
        ENUM_TO_STR(AML_MP_AVSYNC_SOURCE_PCR);
        default:
            return "unknown av sync source";
    }
}

const char* mpPlayerEventType2Str(Aml_MP_PlayerEventType eventType) {
    switch (eventType) {
        ENUM_TO_STR(AML_MP_EVENT_UNKNOWN);
        ENUM_TO_STR(AML_MP_PLAYER_EVENT_FIRST_FRAME);
        ENUM_TO_STR(AML_MP_PLAYER_EVENT_AV_SYNC_DONE);
        ENUM_TO_STR(AML_MP_PLAYER_EVENT_DATA_LOSS);
        ENUM_TO_STR(AML_MP_PLAYER_EVENT_DATA_RESUME);
        ENUM_TO_STR(AML_MP_PLAYER_EVENT_SCRAMBLING);
        ENUM_TO_STR(AML_MP_PLAYER_EVENT_USERDATA_AFD);
        ENUM_TO_STR(AML_MP_PLAYER_EVENT_USERDATA_CC);
        ENUM_TO_STR(AML_MP_PLAYER_EVENT_PID_CHANGED);
        // DVR player
        ENUM_TO_STR(AML_MP_DVRPLAYER_EVENT_ERROR);
        ENUM_TO_STR(AML_MP_DVRPLAYER_EVENT_TRANSITION_OK);
        ENUM_TO_STR(AML_MP_DVRPLAYER_EVENT_TRANSITION_FAILED);
        ENUM_TO_STR(AML_MP_DVRPLAYER_EVENT_KEY_FAILURE);
        ENUM_TO_STR(AML_MP_DVRPLAYER_EVENT_NO_KEY);
        ENUM_TO_STR(AML_MP_DVRPLAYER_EVENT_REACHED_BEGIN);
        ENUM_TO_STR(AML_MP_DVRPLAYER_EVENT_REACHED_END);
        ENUM_TO_STR(AML_MP_DVRPLAYER_EVENT_NOTIFY_PLAYTIME);
        // Video event
        ENUM_TO_STR(AML_MP_PLAYER_EVENT_VIDEO_BASE);
        ENUM_TO_STR(AML_MP_PLAYER_EVENT_VIDEO_CHANGED);
        ENUM_TO_STR(AML_MP_PLAYER_EVENT_VIDEO_DECODE_FIRST_FRAME);
        ENUM_TO_STR(AML_MP_PLAYER_EVENT_VIDEO_FIRST_FRAME);
        ENUM_TO_STR(AML_MP_PLAYER_EVENT_VIDEO_OVERFLOW);
        ENUM_TO_STR(AML_MP_PLAYER_EVENT_VIDEO_UNDERFLOW);
        ENUM_TO_STR(AML_MP_PLAYER_EVENT_VIDEO_INVALID_TIMESTAMP);
        ENUM_TO_STR(AML_MP_PLAYER_EVENT_VIDEO_INVALID_DATA);
        // Audio event
        ENUM_TO_STR(AML_MP_PLAYER_EVENT_AUDIO_BASE);
        ENUM_TO_STR(AML_MP_PLAYER_EVENT_AUDIO_CHANGED);
        ENUM_TO_STR(AML_MP_PLAYER_EVENT_AUDIO_DECODE_FIRST_FRAME);
        ENUM_TO_STR(AML_MP_PLAYER_EVENT_AUDIO_FIRST_FRAME);
        ENUM_TO_STR(AML_MP_PLAYER_EVENT_AUDIO_OVERFLOW);
        ENUM_TO_STR(AML_MP_PLAYER_EVENT_AUDIO_UNDERFLOW);
        ENUM_TO_STR(AML_MP_PLAYER_EVENT_AUDIO_INVALID_TIMESTAMP);
        ENUM_TO_STR(AML_MP_PLAYER_EVENT_AUDIO_INVALID_DATA);
        // Subtitle event
        ENUM_TO_STR(AML_MP_PLAYER_EVENT_SUBTITLE_BASE);
        ENUM_TO_STR(AML_MP_PLAYER_EVENT_SUBTITLE_DATA);
        ENUM_TO_STR(AML_MP_PLAYER_EVENT_SUBTITLE_AVAIL);
        ENUM_TO_STR(AML_MP_PLAYER_EVENT_SUBTITLE_DIMENSION);
        ENUM_TO_STR(AML_MP_PLAYER_EVENT_SUBTITLE_AFD_EVENT);
        ENUM_TO_STR(AML_MP_PLAYER_EVENT_SUBTITLE_CHANNEL_UPDATE);
        ENUM_TO_STR(AML_MP_PLAYER_EVENT_SUBTITLE_LANGUAGE);
        ENUM_TO_STR(AML_MP_PLAYER_EVENT_SUBTITLE_INFO);
        ENUM_TO_STR(AML_MP_PLAYER_EVENT_SUBTITLE_LOSEDATA);
        ENUM_TO_STR(AML_MP_PLAYER_EVENT_SUBTITLE_TIMEOUT);
        ENUM_TO_STR(AML_MP_PLAYER_EVENT_SUBTITLE_INVALID_TIMESTAMP);
        ENUM_TO_STR(AML_MP_PLAYER_EVENT_SUBTITLE_INVALID_DATA);
        default:
            return "unknown player event type";
    }
}

const char* mpPlayerWorkMode2Str(Aml_MP_PlayerWorkMode workmode) {
    switch (workmode) {
        ENUM_TO_STR(AML_MP_PLAYER_MODE_NORMAL);
        ENUM_TO_STR(AML_MP_PLAYER_MODE_CACHING_ONLY);
        ENUM_TO_STR(AML_MP_PLAYER_MODE_DECODE_ONLY);
        default:
            return "unknown player work mode";
    }
}

const char* mpInputStreamType2Str(Aml_MP_InputStreamType inputStreamType) {
    switch (inputStreamType) {
        ENUM_TO_STR(AML_MP_INPUT_STREAM_NORMAL);
        ENUM_TO_STR(AML_MP_INPUT_STREAM_ENCRYPTED);
        ENUM_TO_STR(AML_MP_INPUT_STREAM_SECURE_MEMORY);
        default:
            return "unknown input stream type";
    }
}

const char* mpInputSourceType2Str(Aml_MP_InputSourceType inputSourceType) {
    switch (inputSourceType) {
        ENUM_TO_STR(AML_MP_INPUT_SOURCE_TS_DEMOD);
        ENUM_TO_STR(AML_MP_INPUT_SOURCE_TS_MEMORY);
        ENUM_TO_STR(AML_MP_INPUT_SOURCE_ES_MEMORY);
        default:
            return "unknown input source type";
    }
}

const char* mpCASType2Str(Aml_MP_CASType casType) {
    switch (casType) {
        ENUM_TO_STR(AML_MP_CAS_UNKNOWN);
        ENUM_TO_STR(AML_MP_CAS_VERIMATRIX_IPTV);
        ENUM_TO_STR(AML_MP_CAS_WIDEVINE);
        ENUM_TO_STR(AML_MP_CAS_VERIMATRIX_WEB);
        default:
            return "unknown CAS type";
    }
}

const char* mpVideoDecideMode2Str(Aml_MP_VideoDecodeMode decodeMode) {
    switch (decodeMode) {
        ENUM_TO_STR(AML_MP_VIDEO_DECODE_MODE_NONE);
        ENUM_TO_STR(AML_MP_VIDEO_DECODE_MODE_IONLY);
        default:
            return "unknown video decode mode";
    }
}

const char* mpCASServiceType2Str(Aml_MP_CASServiceType serviceType)
{
    switch (serviceType) {
    case AML_MP_CAS_SERVICE_LIVE_PLAY: return "live play";
    case AML_MP_CAS_SERVICE_PVR_RECORDING: return "pvr recording";
    case AML_MP_CAS_SERVICE_PVR_PLAY: return "pvr playback";
    case AML_MP_CAS_SERVICE_VERIMATRIX_IPTV: return "verimatrix IPTV";
    case AML_MP_CAS_SERVICE_VERIMATRIX_WEB: return "verimatrix WEB";
    case AML_MP_CAS_SERVICE_WIDEVINE: return "widevine";
    }

    return TO_STR(AML_MP_CAS_SERVICE_TYPE_INVALID);
}

////////////////////////////////////////////////////////////////////////

vformat_t convertToVFormat(Aml_MP_CodecID videoCodec)
{
    vformat_t vFormat = VFORMAT_UNKNOWN;

    switch (videoCodec) {
    case AML_MP_VIDEO_CODEC_MPEG12:
        vFormat = VFORMAT_MPEG12;
        break;

    case AML_MP_VIDEO_CODEC_MPEG4:
        vFormat = VFORMAT_MPEG4;
        break;

    case AML_MP_VIDEO_CODEC_H264:
        vFormat = VFORMAT_H264;
        break;

    case AML_MP_VIDEO_CODEC_VC1:
        vFormat = VFORMAT_VC1;
        break;

    case AML_MP_VIDEO_CODEC_AVS:
        vFormat = VFORMAT_AVS;
        break;

    case AML_MP_VIDEO_CODEC_HEVC:
        vFormat = VFORMAT_HEVC;
        break;

    case AML_MP_VIDEO_CODEC_VP9:
        vFormat = VFORMAT_VP9;
        break;

#if ANDROID_PLATFORM_SDK_VERSION <= 29
    case AML_MP_VIDEO_CODEC_AVS2:
        vFormat = VFORMAT_AVS2;
        break;
#endif

    default:
        MLOGE("unknown videoCodec:%d", videoCodec);
        break;
    }

    return vFormat;

}

aformat_t convertToAForamt(Aml_MP_CodecID audioCodec)
{
    aformat_t aFormat = AFORMAT_UNKNOWN;

    switch (audioCodec) {
    case AML_MP_AUDIO_CODEC_MP2:
        aFormat = AFORMAT_MPEG2;
        break;

    case AML_MP_AUDIO_CODEC_MP3:
        aFormat = AFORMAT_MPEG;
        break;

    case AML_MP_AUDIO_CODEC_AC3:
        aFormat = AFORMAT_AC3;
        break;

    case AML_MP_AUDIO_CODEC_EAC3:
        aFormat = AFORMAT_EAC3;
        break;

    case AML_MP_AUDIO_CODEC_DTS:
        aFormat = AFORMAT_DTS;
        break;

    case AML_MP_AUDIO_CODEC_AAC:
        aFormat = AFORMAT_AAC;
        break;

    case AML_MP_AUDIO_CODEC_LATM:
        aFormat = AFORMAT_AAC_LATM;
        break;

    case AML_MP_AUDIO_CODEC_PCM:
        aFormat = AFORMAT_PCM_S16LE;
        break;

    default:
        MLOGE("unknown audioCodec:%d", audioCodec);
        break;
    }

    return aFormat;
}

Aml_MP_StreamType convertToMpStreamType(DVR_StreamType_t streamType)
{
    switch (streamType) {
        case DVR_STREAM_TYPE_VIDEO:
            return AML_MP_STREAM_TYPE_VIDEO;
        case DVR_STREAM_TYPE_AUDIO:
            return AML_MP_STREAM_TYPE_AUDIO;
        case DVR_STREAM_TYPE_AD:
            return AML_MP_STREAM_TYPE_AD;
        case DVR_STREAM_TYPE_SUBTITLE:
            return AML_MP_STREAM_TYPE_SUBTITLE;
        case DVR_STREAM_TYPE_TELETEXT:
            return AML_MP_STREAM_TYPE_TELETEXT;
        case DVR_STREAM_TYPE_ECM:
            return AML_MP_STREAM_TYPE_ECM;
        case DVR_STREAM_TYPE_EMM:
            return AML_MP_STREAM_TYPE_EMM;
        default:
            return AML_MP_STREAM_TYPE_UNKNOWN;
    }
}

DVR_StreamType_t convertToDVRStreamType(Aml_MP_StreamType streamType)
{
    switch (streamType) {
        case AML_MP_STREAM_TYPE_VIDEO:
            return DVR_STREAM_TYPE_VIDEO;
        case AML_MP_STREAM_TYPE_AUDIO:
            return DVR_STREAM_TYPE_AUDIO;
        case AML_MP_STREAM_TYPE_AD:
            return DVR_STREAM_TYPE_AD;
        case AML_MP_STREAM_TYPE_SUBTITLE:
            return DVR_STREAM_TYPE_SUBTITLE;
        case AML_MP_STREAM_TYPE_TELETEXT:
            return DVR_STREAM_TYPE_TELETEXT;
        case AML_MP_STREAM_TYPE_ECM:
            return DVR_STREAM_TYPE_ECM;
        case AML_MP_STREAM_TYPE_EMM:
            return DVR_STREAM_TYPE_EMM;
        default:
            return DVR_STREAM_TYPE_OTHER;
    }
}

Aml_MP_CodecID convertToMpCodecId(DVR_VideoFormat_t fmt)
{
    Aml_MP_CodecID codecId = AML_MP_CODEC_UNKNOWN;

    switch (fmt) {
    case DVR_VIDEO_FORMAT_MPEG1:
    case DVR_VIDEO_FORMAT_MPEG2:
        codecId = AML_MP_VIDEO_CODEC_MPEG12;
        break;

    case DVR_VIDEO_FORMAT_H264:
        codecId = AML_MP_VIDEO_CODEC_H264;
        break;

    case DVR_VIDEO_FORMAT_HEVC:
        codecId = AML_MP_VIDEO_CODEC_HEVC;
        break;

    case DVR_VIDEO_FORMAT_VP9:
        codecId = AML_MP_VIDEO_CODEC_VP9;
        break;

    default:
        MLOGW("unknown video codec:%d", fmt);
        break;
    }

    return codecId;
}

Aml_MP_CodecID convertToMpCodecId(DVR_AudioFormat_t fmt)
{
    Aml_MP_CodecID codecId = AML_MP_CODEC_UNKNOWN;

    switch (fmt) {
    case DVR_AUDIO_FORMAT_MPEG:
        codecId = AML_MP_AUDIO_CODEC_MP3;
        break;

    case DVR_AUDIO_FORMAT_AC3:
        codecId = AML_MP_AUDIO_CODEC_AC3;
        break;

    case DVR_AUDIO_FORMAT_EAC3:
        codecId = AML_MP_AUDIO_CODEC_EAC3;
        break;

    case DVR_AUDIO_FORMAT_DTS:
        codecId = AML_MP_AUDIO_CODEC_DTS;
        break;

    case DVR_AUDIO_FORMAT_AAC:
    case DVR_AUDIO_FORMAT_HEAAC:
        codecId = AML_MP_AUDIO_CODEC_AAC;
        break;

    case DVR_AUDIO_FORMAT_LATM:
        codecId = AML_MP_AUDIO_CODEC_LATM;
        break;

    case DVR_AUDIO_FORMAT_PCM:
        codecId = AML_MP_AUDIO_CODEC_PCM;
        break;

    case DVR_AUDIO_FORMAT_AC4:
        codecId = AML_MP_AUDIO_CODEC_AC4;
        break;

    default:
        MLOGW("unknown audio codec:%d", fmt);
        break;
    }

    return codecId;
}

DVR_VideoFormat_t convertToDVRVideoFormat(Aml_MP_CodecID codecId)
{
    switch (codecId) {
        case AML_MP_VIDEO_CODEC_MPEG12:
            return DVR_VIDEO_FORMAT_MPEG2;
        case AML_MP_VIDEO_CODEC_H264:
            return DVR_VIDEO_FORMAT_H264;
        case AML_MP_VIDEO_CODEC_HEVC:
            return DVR_VIDEO_FORMAT_HEVC;
        case AML_MP_VIDEO_CODEC_VP9:
            return DVR_VIDEO_FORMAT_VP9;
        default:
            MLOGE("unknown video codecId:%d", codecId);
            return DVR_VIDEO_FORMAT_MPEG2;
    }
}

DVR_AudioFormat_t convertToDVRAudioFormat(Aml_MP_CodecID codecId)
{
    switch (codecId) {
        case AML_MP_AUDIO_CODEC_MP2:
            return DVR_AUDIO_FORMAT_MPEG;
        case AML_MP_AUDIO_CODEC_AC3:
            return DVR_AUDIO_FORMAT_AC3;
        case AML_MP_AUDIO_CODEC_EAC3:
            return DVR_AUDIO_FORMAT_EAC3;
        case AML_MP_AUDIO_CODEC_DTS:
            return DVR_AUDIO_FORMAT_DTS;
        case AML_MP_AUDIO_CODEC_AAC:
            return DVR_AUDIO_FORMAT_AAC;
        case AML_MP_AUDIO_CODEC_LATM:
            return DVR_AUDIO_FORMAT_LATM;
        case AML_MP_AUDIO_CODEC_PCM:
            return DVR_AUDIO_FORMAT_PCM;
        case AML_MP_AUDIO_CODEC_AC4:
            return DVR_AUDIO_FORMAT_AC4;
        default:
            MLOGE("unknown audio codecId:%d", codecId);
            return DVR_AUDIO_FORMAT_MPEG;
    }
}

void convertToMpDVRStream(Aml_MP_DVRStream* mpDvrStream, DVR_StreamPid_t* dvrStream)
{
    DVR_StreamType_t streamType = (DVR_StreamType_t)(dvrStream->type>>24 & 0x0F);
    int format = dvrStream->type & 0xFFFFFF;

    mpDvrStream->pid = dvrStream->pid;
    mpDvrStream->type = convertToMpStreamType(streamType);

    switch (streamType) {
    case DVR_STREAM_TYPE_VIDEO:
        mpDvrStream->codecId = convertToMpCodecId((DVR_VideoFormat_t)format);
        break;

    case DVR_STREAM_TYPE_AUDIO:
        mpDvrStream->codecId = convertToMpCodecId((DVR_AudioFormat_t)format);
        break;

    default:
        break;
    }
}

void convertToMpDVRStream(Aml_MP_DVRStream* mpDvrStream, DVR_StreamInfo_t* dvrStreamInfo)
{
    mpDvrStream->type = convertToMpStreamType(dvrStreamInfo->type);
    mpDvrStream->pid = dvrStreamInfo->pid;
    switch (mpDvrStream->type) {
    case AML_MP_STREAM_TYPE_VIDEO:
        mpDvrStream->codecId = convertToMpCodecId((DVR_VideoFormat_t)dvrStreamInfo->format);
        break;

    case AML_MP_STREAM_TYPE_AUDIO:
        mpDvrStream->codecId = convertToMpCodecId((DVR_AudioFormat_t)dvrStreamInfo->format);
        break;

    default:
        break;
    }
}

void convertToMpDVRSourceInfo(Aml_MP_DVRSourceInfo* dest, DVR_WrapperInfo_t* source)
{
    dest->time = source->time;
    dest->size = source->size;
    dest->pkts = source->pkts;
}

am_tsplayer_video_match_mode convertToTsPlayerVideoMatchMode(Aml_MP_VideoDisplayMode videoDisplayMode)
{
    switch (videoDisplayMode) {
    case AML_MP_VIDEO_DISPLAY_MODE_NORMAL:
        return AV_VIDEO_MATCH_MODE_NONE;
    case AML_MP_VIDEO_DISPLAY_MODE_FULLSCREEN:
        return AV_VIDEO_MATCH_MODE_FULLSCREEN;
    case AML_MP_VIDEO_DISPLAY_MODE_LETTER_BOX:
        return AV_VIDEO_MATCH_MODE_LETTER_BOX;
    case AML_MP_VIDEO_DISPLAY_MODE_PAN_SCAN:
        return AV_VIDEO_MATCH_MODE_PAN_SCAN;
    case AML_MP_VIDEO_DISPLAY_MODE_COMBINED:
        return AV_VIDEO_MATCH_MODE_COMBINED;
    case AML_MP_VIDEO_DISPLAY_MODE_WIDTHFULL:
        return AV_VIDEO_MATCH_MODE_WIDTHFULL;
    case AML_MP_VIDEO_DISPLAY_MODE_HEIGHTFULL:
        return AV_VIDEO_MATCH_MODE_HEIGHFULL;
#ifdef ANDROID
    case AML_MP_VIDEO_DISPLAY_MODE_4_3_LETTER_BOX:
        return AV_VIDEO_WIDEOPTION_4_3_LETTER_BOX;
    case AML_MP_VIDEO_DISPLAY_MODE_4_3_PAN_SCAN:
        return AV_VIDEO_WIDEOPTION_4_3_PAN_SCAN;
    case AML_MP_VIDEO_DISPLAY_MODE_4_3_COMBINED:
        return AV_VIDEO_WIDEOPTION_4_3_COMBINED;
    case AML_MP_VIDEO_DISPLAY_MODE_16_9_IGNORE:
        return AV_VIDEO_WIDEOPTION_16_9_IGNORE;
    case AML_MP_VIDEO_DISPLAY_MODE_16_9_LETTER_BOX:
        return AV_VIDEO_WIDEOPTION_16_9_LETTER_BOX;
    case AML_MP_VIDEO_DISPLAY_MODE_16_9_PAN_SCAN:
        return AV_VIDEO_WIDEOPTION_16_9_PAN_SCAN;
    case AML_MP_VIDEO_DISPLAY_MODE_16_9_COMBINED:
        return AV_VIDEO_WIDEOPTION_16_9_COMBINED;
    case AML_MP_VIDEO_DISPLAY_MODE_CUSTOM:
        return AV_VIDEO_WIDEOPTION_CUSTOM;
#endif
    default:
        return AV_VIDEO_MATCH_MODE_NONE;
    }
}

am_tsplayer_video_trick_mode convertToTsplayerVideoTrickMode(Aml_MP_VideoDecodeMode videoDecodeMode)
{
    switch (videoDecodeMode) {
    case AML_MP_VIDEO_DECODE_MODE_NONE:
        return AV_VIDEO_TRICK_MODE_NONE;
    case AML_MP_VIDEO_DECODE_MODE_IONLY:
        return AV_VIDEO_TRICK_MODE_IONLY;
    }
}

am_tsplayer_audio_out_mode convertToTsPlayerAudioOutMode(Aml_MP_AudioOutputMode audioOutputMode)
{
    switch (audioOutputMode) {
    case AML_MP_AUDIO_OUTPUT_PCM:
        return AV_AUDIO_OUT_PCM;
    case AML_MP_AUDIO_OUTPUT_PASSTHROUGH:
        return AV_AUDIO_OUT_PASSTHROUGH;
    default:
        return AV_AUDIO_OUT_AUTO;
    }
}

void convertToMpVideoInfo(Aml_MP_VideoInfo* mpVideoInfo, am_tsplayer_video_info* tsVideoInfo)
{
    mpVideoInfo->width      = tsVideoInfo->width;
    mpVideoInfo->height     = tsVideoInfo->height;
    mpVideoInfo->frameRate  = tsVideoInfo->framerate;
    mpVideoInfo->bitrate    = tsVideoInfo->bitrate;
    mpVideoInfo->ratio64    = tsVideoInfo->ratio64;
}

am_tsplayer_audio_stereo_mode convertToTsPlayerAudioStereoMode(Aml_MP_AudioBalance audioBalance)
{
    switch (audioBalance) {
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

void convertToMpPlayerEventAudioFormat(Aml_MP_PlayerEventAudioFormat* dest, am_tsplayer_audio_format_t* source) {
    dest->sample_rate = source->sample_rate;
    dest->channels = source->channels;
}


DVB_DemuxSource_t convertToDVBDemuxSource(Aml_MP_DemuxSource source)
{
    switch (source) {
    case AML_MP_DEMUX_SOURCE_TS0:
       return DVB_DEMUX_SOURCE_TS0;

    case AML_MP_DEMUX_SOURCE_TS1:
       return DVB_DEMUX_SOURCE_TS1;

    case AML_MP_DEMUX_SOURCE_TS2:
       return DVB_DEMUX_SOURCE_TS2;

    case AML_MP_DEMUX_SOURCE_TS3:
       return DVB_DEMUX_SOURCE_TS3;

    case AML_MP_DEMUX_SOURCE_TS4:
       return DVB_DEMUX_SOURCE_TS4;

    case AML_MP_DEMUX_SOURCE_TS5:
       return DVB_DEMUX_SOURCE_TS5;

    case AML_MP_DEMUX_SOURCE_TS6:
       return DVB_DEMUX_SOURCE_TS6;

    case AML_MP_DEMUX_SOURCE_TS7:
       return DVB_DEMUX_SOURCE_TS7;

    case AML_MP_DEMUX_SOURCE_DMA0:
       return DVB_DEMUX_SOURCE_DMA0;

    case AML_MP_DEMUX_SOURCE_DMA1:
       return DVB_DEMUX_SOURCE_DMA1;

    case AML_MP_DEMUX_SOURCE_DMA2:
       return DVB_DEMUX_SOURCE_DMA2;

    case AML_MP_DEMUX_SOURCE_DMA3:
       return DVB_DEMUX_SOURCE_DMA3;

    case AML_MP_DEMUX_SOURCE_DMA4:
       return DVB_DEMUX_SOURCE_DMA4;

    case AML_MP_DEMUX_SOURCE_DMA5:
       return DVB_DEMUX_SOURCE_DMA5;

    case AML_MP_DEMUX_SOURCE_DMA6:
       return DVB_DEMUX_SOURCE_DMA6;

    case AML_MP_DEMUX_SOURCE_DMA7:
       return DVB_DEMUX_SOURCE_DMA7;
    }
}

Aml_MP_DemuxSource convertToMpDemuxSource(DVB_DemuxSource_t source)
{
    switch (source) {
    case DVB_DEMUX_SOURCE_TS0:
        return AML_MP_DEMUX_SOURCE_TS0;

    case DVB_DEMUX_SOURCE_TS1:
        return AML_MP_DEMUX_SOURCE_TS1;

    case DVB_DEMUX_SOURCE_TS2:
        return AML_MP_DEMUX_SOURCE_TS2;

    case DVB_DEMUX_SOURCE_TS3:
        return AML_MP_DEMUX_SOURCE_TS3;

    case DVB_DEMUX_SOURCE_TS4:
        return AML_MP_DEMUX_SOURCE_TS4;

    case DVB_DEMUX_SOURCE_TS5:
        return AML_MP_DEMUX_SOURCE_TS5;

    case DVB_DEMUX_SOURCE_TS6:
        return AML_MP_DEMUX_SOURCE_TS6;

    case DVB_DEMUX_SOURCE_TS7:
        return AML_MP_DEMUX_SOURCE_TS7;

    case DVB_DEMUX_SOURCE_DMA0:
        return AML_MP_DEMUX_SOURCE_DMA0;

    case DVB_DEMUX_SOURCE_DMA1:
        return AML_MP_DEMUX_SOURCE_DMA1;

    case DVB_DEMUX_SOURCE_DMA2:
        return AML_MP_DEMUX_SOURCE_DMA2;

    case DVB_DEMUX_SOURCE_DMA3:
        return AML_MP_DEMUX_SOURCE_DMA3;

    case DVB_DEMUX_SOURCE_DMA4:
        return AML_MP_DEMUX_SOURCE_DMA4;

    case DVB_DEMUX_SOURCE_DMA5:
        return AML_MP_DEMUX_SOURCE_DMA5;

    case DVB_DEMUX_SOURCE_DMA6:
        return AML_MP_DEMUX_SOURCE_DMA6;

    case DVB_DEMUX_SOURCE_DMA7:
        return AML_MP_DEMUX_SOURCE_DMA7;
    }
    return AML_MP_DEMUX_SOURCE_TS0;
}

am_tsplayer_stream_type convertToTsplayerStreamType(Aml_MP_StreamType streamType) {
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

Aml_MP_StreamType convertToAmlMPStreamType(am_tsplayer_stream_type streamType) {
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

#ifdef HAVE_SUBTITLE
AmlTeletextCtrlParam convertToTeletextCtrlParam(AML_MP_TeletextCtrlParam* teletextCtrlParam) {
    AmlTeletextCtrlParam params;
    params.magazine = teletextCtrlParam->magazine;
    params.page = teletextCtrlParam->page;
    params.event = convertToTeletextEvent(teletextCtrlParam->event);

    return params;
}

AmlTeletextEvent convertToTeletextEvent(Aml_MP_TeletextEvent teletextEvent) {
    switch (teletextEvent) {
        case AML_MP_TT_EVENT_QUICK_NAVIGATE_RED:
            return TT_EVENT_QUICK_NAVIGATE_RED;
        case AML_MP_TT_EVENT_QUICK_NAVIGATE_GREEN:
            return TT_EVENT_QUICK_NAVIGATE_GREEN;
        case AML_MP_TT_EVENT_QUICK_NAVIGATE_YELLOW:
            return TT_EVENT_QUICK_NAVIGATE_YELLOW;
        case AML_MP_TT_EVENT_QUICK_NAVIGATE_BLUE:
            return TT_EVENT_QUICK_NAVIGATE_BLUE;
        case AML_MP_TT_EVENT_0:
            return TT_EVENT_0;
        case AML_MP_TT_EVENT_1:
            return TT_EVENT_1;
        case AML_MP_TT_EVENT_2:
            return TT_EVENT_2;
        case AML_MP_TT_EVENT_3:
            return TT_EVENT_3;
        case AML_MP_TT_EVENT_4:
            return TT_EVENT_4;
        case AML_MP_TT_EVENT_5:
            return TT_EVENT_5;
        case AML_MP_TT_EVENT_6:
            return TT_EVENT_6;
        case AML_MP_TT_EVENT_7:
            return TT_EVENT_7;
        case AML_MP_TT_EVENT_8:
            return TT_EVENT_8;
        case AML_MP_TT_EVENT_9:
            return TT_EVENT_9;
        case AML_MP_TT_EVENT_INDEXPAGE:
            return TT_EVENT_INDEXPAGE;
        case AML_MP_TT_EVENT_NEXTPAGE:
            return TT_EVENT_NEXTPAGE;
        case AML_MP_TT_EVENT_PREVIOUSPAGE:
            return TT_EVENT_PREVIOUSPAGE;
        case AML_MP_TT_EVENT_NEXTSUBPAGE:
            return TT_EVENT_NEXTSUBPAGE;
        case AML_MP_TT_EVENT_PREVIOUSSUBPAGE:
            return TT_EVENT_PREVIOUSSUBPAGE;
        case AML_MP_TT_EVENT_BACKPAGE:
            return TT_EVENT_BACKPAGE;
        case AML_MP_TT_EVENT_FORWARDPAGE:
            return TT_EVENT_FORWARDPAGE;
        case AML_MP_TT_EVENT_HOLD:
            return TT_EVENT_HOLD;
        case AML_MP_TT_EVENT_REVEAL:
            return TT_EVENT_REVEAL;
        case AML_MP_TT_EVENT_CLEAR:
            return TT_EVENT_CLEAR;
        case AML_MP_TT_EVENT_CLOCK:
            return TT_EVENT_CLOCK;
        case AML_MP_TT_EVENT_MIX_VIDEO:
            return TT_EVENT_MIX_VIDEO;
        case AML_MP_TT_EVENT_DOUBLE_HEIGHT:
            return TT_EVENT_DOUBLE_HEIGHT;
        case AML_MP_TT_EVENT_DOUBLE_SCROLL_UP:
            return TT_EVENT_DOUBLE_SCROLL_UP;
        case AML_MP_TT_EVENT_DOUBLE_SCROLL_DOWN:
            return TT_EVENT_DOUBLE_SCROLL_DOWN;
        case AML_MP_TT_EVENT_TIMER:
            return TT_EVENT_TIMER;
        case AML_MP_TT_EVENT_GO_TO_PAGE:
            return TT_EVENT_GO_TO_PAGE;
        case AML_MP_TT_EVENT_GO_TO_SUBTITLE:
            return TT_EVENT_GO_TO_SUBTITLE;
        default:
            return TT_EVENT_INVALID;
    }
}

AML_MP_SubtitleDataType convertToMpSubtitleDataType(AmlSubDataType subDataType) {
    switch (subDataType) {
        case SUB_DATA_TYPE_STRING:
            return AML_MP_SUB_DATA_TYPE_STRING;
        case SUB_DATA_TYPE_CC_JSON:
            return AML_MP_SUB_DATA_TYPE_CC_JSON;
        case SUB_DATA_TYPE_BITMAP:
            return AML_MP_SUB_DATA_TYPE_BITMAP;
        case SUB_DATA_TYPE_POSITON_BITMAP:
            return AML_MP_SUB_DATA_TYPE_POSITON_BITMAP;
        default:
            return AML_MP_SUB_DATA_TYPE_UNKNOWN;
    }
}

#endif

bool isSupportMultiHwDemux()
{
    return access("/sys/module/dvb_demux/", F_OK) == 0;
}

NativeWindowHelper::NativeWindowHelper()
{
}

NativeWindowHelper::~NativeWindowHelper()
{
    #ifdef ANDROID
    if (mMesonVtFd >= 0) {
        if (mTunnelId >= 0) {
            MLOGI("~NativeWindowHelper: free Id:%d", mTunnelId);
            meson_vt_free_id(mMesonVtFd, mTunnelId);
        }
        meson_vt_close(mMesonVtFd);
        mTunnelId = -1;
        mMesonVtFd = -1;
    }
    #endif
}

int NativeWindowHelper::setSiebandTunnelMode(ANativeWindow* nativeWindow)
{
    int ret = -1;

    if (nativeWindow == nullptr) {
        return ret;
    }
    #ifdef ANDROID
    native_handle_t* sidebandHandle = am_gralloc_create_sideband_handle(AM_TV_SIDEBAND, AM_VIDEO_DEFAULT);
    mSidebandHandle = android::NativeHandle::create(sidebandHandle, true);

    MLOG("setAnativeWindow:%p, sidebandHandle:%p", nativeWindow, sidebandHandle);

    ret = native_window_set_sideband_stream(nativeWindow, sidebandHandle);
    #endif
    if (ret < 0) {
        MLOGE("set sideband stream failed!");
    }

    return ret;
}

int NativeWindowHelper::setSidebandNonTunnelMode(ANativeWindow* nativeWindow, int& videoTunnelId)
{
    int ret = -1;

    if (nativeWindow == nullptr) {
        return ret;
    }

    if (videoTunnelId != -1) {
        MLOGE("setANativeWindow mVideoTunnelId:%d", videoTunnelId);
    }
    #ifdef ANDROID
    if (mMesonVtFd >= 0) {
        if (mTunnelId >= 0) {
            meson_vt_free_id(mMesonVtFd, mTunnelId);
            MLOGI("setAnativeWindow: free Id %d, before sideband", mTunnelId);
        }
        meson_vt_close(mMesonVtFd);
        mTunnelId = -1;
        mMesonVtFd = -1;
    }
    if (nativeWindow != nullptr) {
        int type = AM_FIXED_TUNNEL;
        mMesonVtFd = meson_vt_open();
        if (mMesonVtFd < 0) {
            MLOGI("meson_vt_open failed!");
            return ret;
        }
        if (meson_vt_alloc_id(mMesonVtFd, &videoTunnelId) < 0) {
            MLOGI("meson_vt_alloc_id failed!");
            meson_vt_close(mMesonVtFd);
            mMesonVtFd = -1;
            return ret;
        }
        MLOGI("setAnativeWindow: allocId: %d", videoTunnelId);
        mTunnelId = videoTunnelId;

        native_handle_t* sidebandHandle = am_gralloc_create_sideband_handle(type, videoTunnelId);
        mSidebandHandle = android::NativeHandle::create(sidebandHandle, true);

        MLOG("setAnativeWindow:%p, sidebandHandle:%p", nativeWindow, sidebandHandle);

        ret = native_window_set_sideband_stream(nativeWindow, sidebandHandle);
        if (ret < 0) {
            MLOGE("set sideband stream failed!");
            return ret;
        }
    }
    #endif
    return ret;
}
}
