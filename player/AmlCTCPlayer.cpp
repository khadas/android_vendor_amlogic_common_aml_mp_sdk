/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#define LOG_TAG "AmlCTCPlayer"
#include <utils/Log.h>
#include "AmlCTCPlayer.h"
#include <CTC_MediaProcessor.h>
#include <utils/AmlMpUtils.h>
#include <system/window.h>
#include <amlogic/am_gralloc_ext.h>

namespace aml_mp {

vformat_t ctcVideoCodecConvert(Aml_MP_CodecID aml_MP_VideoCodec) {
    switch (aml_MP_VideoCodec) {
        case AML_MP_VIDEO_CODEC_MPEG12:
            return VFORMAT_MPEG12;
        case AML_MP_VIDEO_CODEC_MPEG4:
            return VFORMAT_MPEG4;
        case AML_MP_VIDEO_CODEC_H264:
            return VFORMAT_H264;
        case AML_MP_VIDEO_CODEC_VC1:
            return VFORMAT_VC1;
        case AML_MP_VIDEO_CODEC_AVS:
            return VFORMAT_AVS;
        case AML_MP_VIDEO_CODEC_VP9:
            return VFORMAT_VP9;
        case AML_MP_VIDEO_CODEC_HEVC:
            return VFORMAT_HEVC;
        case AML_MP_VIDEO_CODEC_AVS2:
            return VFORMAT_AVS2;
        default:
            return VFORMAT_UNKNOWN;
    }
}

aformat_t ctcAudioCodecConvert(Aml_MP_CodecID aml_MP_AudioCodec) {
    switch (aml_MP_AudioCodec) {
        case AML_MP_AUDIO_CODEC_MP2:
            return AFORMAT_MPEG2;
        case AML_MP_AUDIO_CODEC_MP3:
            return AFORMAT_MPEG;
        case AML_MP_AUDIO_CODEC_AC3:
            return AFORMAT_AC3;
        case AML_MP_AUDIO_CODEC_EAC3:
            return AFORMAT_EAC3;
        case AML_MP_AUDIO_CODEC_DTS:
            return AFORMAT_DTS;
        case AML_MP_AUDIO_CODEC_AAC:
            return AFORMAT_AAC;
        case AML_MP_AUDIO_CODEC_LATM:
            return AFORMAT_AAC_LATM;
        case AML_MP_AUDIO_CODEC_PCM:
            /*AFORMAT_PCM_S16LE = 1,//how to chose pcm format for ctc
            AFORMAT_PCM_S16BE = 7,
            AFORMAT_PCM_U8 = 10,
            AFORMAT_ADPCM = 11,
            AFORMAT_PCM_BLURAY = 16,
            AFORMAT_PCM_WIFIDISPLAY = 22,
            AFORMAT_PCM_S24LE = 30*/
            return AFORMAT_PCM_S16LE;
        default:
            return AFORMAT_UNKNOWN;
    }
}

CTC_SyncSource ctcSyncSourceTypeConvert(Aml_MP_AVSyncSource avSyncSource) {
    switch (avSyncSource) {
        case AML_MP_AVSYNC_SOURCE_VIDEO:
            return CTC_SYNCSOURCE_VIDEO;
        case AML_MP_AVSYNC_SOURCE_AUDIO:
            return CTC_SYNCSOURCE_AUDIO;
        case AML_MP_AVSYNC_SOURCE_PCR:
            return CTC_SYNCSOURCE_PCR;
        default:
            return CTC_SYNCSOURCE_AUDIO;
    }
}

PLAYER_STREAMTYPE_E ctcStreamTypeConvert(Aml_MP_StreamType streamType) {
    switch (streamType) {
        case AML_MP_STREAM_TYPE_VIDEO:
            return PLAYER_STREAMTYPE_VIDEO;
        case AML_MP_STREAM_TYPE_AUDIO:
            return PLAYER_STREAMTYPE_AUDIO;
        case AML_MP_STREAM_TYPE_SUBTITLE:
            return PLAYER_STREAMTYPE_SUBTITLE;
        default:
            return PLAYER_STREAMTYPE_NULL;
    }
}

aml::ABALANCE_E ctcAudioBalanceConvert(Aml_MP_AudioBalance balance)
{
    switch (balance) {
    case AML_MP_AUDIO_BALANCE_STEREO:
        return aml::ABALANCE_STEREO;

    case AML_MP_AUDIO_BALANCE_LEFT:
        return aml::ABALANCE_LEFT;

    case AML_MP_AUDIO_BALANCE_RIGHT:
        return aml::ABALANCE_RIGHT;

    default:
        return aml::ABALANCE_MAX;
    }
}


AmlCTCPlayer::AmlCTCPlayer(Aml_MP_PlayerCreateParams* createParams, int instanceId)
: aml_mp::AmlPlayerBase(createParams, instanceId)
{
    snprintf(mName, sizeof(mName), "%s_%d", LOG_TAG, instanceId);

    memset(mVideoPara, 0, sizeof(mVideoPara));
    mVideoPara[0].pid = AML_MP_INVALID_PID;
    memset(mAudioPara, 0, sizeof(mAudioPara));
    mAudioPara[0].pid = AML_MP_INVALID_PID;
    mVideoParaSeted = false;
    mAudioParaSeted = false;
    mIsPause = false;
    RETURN_VOID_IF(createParams == nullptr);

    aml::CTC_InitialParameter params;
    MLOGI("%s:%d instanceId=%d\n", __FUNCTION__, __LINE__, instanceId);
    params.userId = createParams->channelId;
    params.useOmx = 1; //0:ctc 1:pip
    params.isEsSource = 0; //0:TS source 1:ES source
    params.flags = 0;
    params.extensionSize = 0;
    mCtcPlayer = aml::GetMediaProcessor(&params);

    mCtcPlayer->playerback_register_evt_cb([](void* user_data, aml::IPTV_PLAYER_EVT_E event, uint32_t param1, uint32_t param2) {
        static_cast<AmlCTCPlayer*>(user_data)->eventCtcCallback(event,param1,param2);
    }, this);
}

int AmlCTCPlayer::initCheck() const
{
    return 0;
}

AmlCTCPlayer::~AmlCTCPlayer()
{
    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);
    if (mCtcPlayer != nullptr) {
        delete mCtcPlayer;
        mCtcPlayer = nullptr;
    }
}

int AmlCTCPlayer::setANativeWindow(ANativeWindow* pSurface) {
    RETURN_IF(-1, pSurface == nullptr);
    RETURN_IF(-1, mCtcPlayer == nullptr);

    MLOGI("%s:%d ANativeWindow: %p\n", __FUNCTION__, __LINE__, pSurface);
    mCtcPlayer->SetSurface_ANativeWindow(pSurface);

    return 0;
}

int AmlCTCPlayer::setVideoParams(const Aml_MP_VideoParams* params) {
    RETURN_IF(-1, params == nullptr);
    RETURN_IF(-1, mCtcPlayer == nullptr);

    MLOGI("%s:%d videoCodec: %s vpid: 0x%x width: %d height: %d\n", __FUNCTION__, __LINE__,
        mpCodecId2Str(params->videoCodec), params->pid, params->width, params->height);
    if (params->pid == AML_MP_INVALID_PID) {
        MLOGI("Video pid invalid, return -1\n");
        return -1;
    }
    if (params->pid == mVideoPara[0].pid) {
        MLOGI("This params is already seted skip");
        return -1;
    }
    mVideoPara[0].pid = params->pid;
    if (params->width > 0 && params->height > 0) {
        mVideoPara[0].nVideoWidth = params->width;
        mVideoPara[0].nVideoHeight = params->height;
    } else {
        mVideoPara[0].nVideoWidth = 0;
        mVideoPara[0].nVideoHeight = 0;
    }
    mVideoPara[0].nFrameRate = params->frameRate;
    mVideoPara[0].vFmt = ctcVideoCodecConvert(params->videoCodec);
    mVideoPara[0].nExtraSize = params->extraDataSize;
    mVideoPara[0].pExtraData = (uint8_t *)params->extraData;
    mVideoParaSeted = true;

    return 0;
}

int AmlCTCPlayer::setAudioParams(const Aml_MP_AudioParams* params) {
    RETURN_IF(-1, params == nullptr);
    RETURN_IF(-1, mCtcPlayer == nullptr);

    MLOGI("%s:%d audioCodec: %s apid: 0x%x ch: %d samp: %d\n", __FUNCTION__, __LINE__,
        mpCodecId2Str(params->audioCodec), params->pid, params->nChannels, params->nSampleRate);
    if (params->pid == AML_MP_INVALID_PID) {
        MLOGI("Audio pid invalid, return -1\n");
        return -1;
    }
    if (params->pid == mAudioPara[0].pid) {
        MLOGI("This params is already seted skip");
        return -1;
    }
    mAudioPara[0].pid = params->pid;
    mAudioPara[0].nChannels = params->nChannels;
    mAudioPara[0].nSampleRate = params->nSampleRate;
    mAudioPara[0].aFmt = ctcAudioCodecConvert(params->audioCodec);
    mAudioPara[0].block_align = 0;
    mAudioPara[0].bit_per_sample = 0;
    mAudioPara[0].nExtraSize = params->extraDataSize;
    mAudioPara[0].pExtraData = (uint8_t *)params->extraData;
    mAudioParaSeted = true;

    return 0;
}

int AmlCTCPlayer::start() {
    RETURN_IF(-1, mCtcPlayer == nullptr);

    bool ret = true;
    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);
    if (mVideoParaSeted) {
        mCtcPlayer->InitVideo(mVideoPara);
    } else {
        MLOGI("Video para not seted");
    }
    if (mAudioParaSeted) {
        mCtcPlayer->InitAudio(mAudioPara);
    } else {
        MLOGI("Audio para not seted");
    }
    ret = mCtcPlayer->StartPlay();
    AmlPlayerBase::start();
    mPlayerStat = CTC_STAT_ASTART_VSTART;
    mIsPause = false;

    return ret?0:-1;
}

int AmlCTCPlayer::stop() {
    RETURN_IF(-1, mCtcPlayer == nullptr);

    bool ret = true;
    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);
    AmlPlayerBase::stop();
    ret = mCtcPlayer->Stop();
    mPlayerStat = CTC_STAT_ASTOP_VSTOP;

    return ret?0:-1;
}

int AmlCTCPlayer::pause() {
    RETURN_IF(-1, mCtcPlayer == nullptr);

    bool ret = true;
    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);
    ret = mCtcPlayer->Pause();

    return ret?0:-1;
}

int AmlCTCPlayer::resume() {
    RETURN_IF(-1, mCtcPlayer == nullptr);

    bool ret = true;
    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);
    ret = mCtcPlayer->Resume();

    return ret?0:-1;
}

int AmlCTCPlayer::flush() {
    RETURN_IF(-1, mCtcPlayer == nullptr);
    bool ret = true;

    ret = mCtcPlayer->Seek();

    AmlPlayerBase::flush();
    return ret?0:-1;
}

int AmlCTCPlayer::setPlaybackRate(float rate) {
    RETURN_IF(-1, mCtcPlayer == nullptr);

    bool ret = true;
    MLOGI("%s:%d rate=%f\n", __FUNCTION__, __LINE__, rate);
    if (rate == 1.0f) {
        ret = mCtcPlayer->StopFast();
    } else {
        aml::CTC_PLAYSPEED_OPT_S params;
        int rateInt = rate * 100;
        params.enPlaySpeedDirect = (rate > 0.0f)?(aml::CTC_PLAYSPEED_DIRECT_FORWARD):(aml::CTC_PLAYSPEED_DIRECT_BACKWARD);
        params.u32SpeedInteger = rateInt / 100;
        params.u32SpeedDecimal = rateInt % 100;
        mCtcPlayer->SetParameter(aml::CTC_KEY_PARAMETER_PLAYSPEED, &params);
        ret = mCtcPlayer->Fast();
    }

    return ret?0:-1;
}

int AmlCTCPlayer::switchAudioTrack(const Aml_MP_AudioParams* params){
    RETURN_IF(-1, params == nullptr);
    RETURN_IF(-1, mCtcPlayer == nullptr);

    MLOGI("%s:%d audioCodec: %s apid: 0x%x ch: %d samp: %d\n", __FUNCTION__, __LINE__,
        mpCodecId2Str(params->audioCodec), params->pid, params->nChannels, params->nSampleRate);
    mAudioPara[0].pid = params->pid;
    mAudioPara[0].nChannels = params->nChannels;
    mAudioPara[0].nSampleRate = params->nSampleRate;
    mAudioPara[0].aFmt = ctcAudioCodecConvert(params->audioCodec);
    mAudioPara[0].block_align = 0;
    mAudioPara[0].bit_per_sample = 0;
    mAudioPara[0].nExtraSize = params->extraDataSize;
    mAudioPara[0].pExtraData = (uint8_t *)params->extraData;
    mCtcPlayer->SwitchAudioTrack(mAudioPara[0].pid , &mAudioPara[0]);

    return 0;
}

int AmlCTCPlayer::writeData(const uint8_t* buffer, size_t size) {
    RETURN_IF(-1, mCtcPlayer == nullptr);

    bool ret = true;
    ret = mCtcPlayer->WriteData((uint8_t*)buffer, (uint32_t)size);
    //MLOGI("AmlCTCPlayer->writeData, buffer:%p, size:%d, ret:%d", buffer, size, ret);

    return ret?0:-1;
}

int AmlCTCPlayer::writeEsData(Aml_MP_StreamType type, const uint8_t* buffer, size_t size, int64_t pts)
{
    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);

    RETURN_IF(-1, mCtcPlayer == nullptr);
    uint8_t* buf = const_cast<uint8_t *>(buffer);

    bool ret = true;
    ret = mCtcPlayer->WriteData(ctcStreamTypeConvert(type), buf, size, pts);

    return ret?0:-1;
}

int AmlCTCPlayer::getCurrentPts(Aml_MP_StreamType type, int64_t* pts) {
    RETURN_IF(-1, mCtcPlayer == nullptr);

    int64_t ctcPts = mCtcPlayer->GetCurrentPts(ctcStreamTypeConvert(type));
    MLOGI("%s:%d type: %s ctcPts: 0x%llx \n", __FUNCTION__, __LINE__, mpStreamType2Str(type), ctcPts);
    pts = &ctcPts;
    return 0;
}

int AmlCTCPlayer::getBufferStat(Aml_MP_BufferStat* bufferStat) {
    RETURN_IF(-1, mCtcPlayer == nullptr);

    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);
    bool ret = true;
    aml::AVBUF_STATUS buffer_stat;
    ret = mCtcPlayer->GetBufferStatus(&buffer_stat);
    bufferStat->audioBuffer.size = buffer_stat.abuf_size;
    bufferStat->audioBuffer.dataLen = buffer_stat.abuf_data_len;
    bufferStat->audioBuffer.bufferedMs = buffer_stat.abuf_ms;
    bufferStat->videoBuffer.size = buffer_stat.vbuf_size;
    bufferStat->videoBuffer.dataLen = buffer_stat.vbuf_data_len;
    bufferStat->videoBuffer.bufferedMs = buffer_stat.vbuf_ms;

    return ret?0:-1;
}

int AmlCTCPlayer::setVideoWindow(int x, int y, int width, int height) {
    RETURN_IF(-1, mCtcPlayer == nullptr);

    MLOGI("%s:%d x: %d y: %d width: %d height: %d\n", __FUNCTION__, __LINE__, x, y, width, height);
    mCtcPlayer->SetVideoWindow(x, y, width, height);

    return 0;
}

int AmlCTCPlayer::setVolume(float volume) {
    RETURN_IF(-1, mCtcPlayer == nullptr);

    bool ret = true;
    MLOGI("%s:%d vol=%f\n", __FUNCTION__, __LINE__, volume);
    ret = mCtcPlayer->SetVolume((int) volume);

    return ret?0:-1;
}

int AmlCTCPlayer::getVolume(float* volume) {
    RETURN_IF(-1, mCtcPlayer == nullptr);

    int tempVol = 0;
    tempVol = mCtcPlayer->GetVolume();
    MLOGI("%s:%d vol: %d\n", __FUNCTION__, __LINE__, tempVol);
    *volume = (float) tempVol;

    return 0;
}

int AmlCTCPlayer::showVideo() {
    RETURN_IF(-1, mCtcPlayer == nullptr);

    bool ret = true;
    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);
    ret = mCtcPlayer->VideoShow();

    return ret?0:-1;
}

int AmlCTCPlayer::hideVideo() {
    RETURN_IF(-1, mCtcPlayer == nullptr);

    bool ret = true;
    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);
    ret = mCtcPlayer->VideoHide();

    return ret?0:-1;
}

int AmlCTCPlayer::setParameter(Aml_MP_PlayerParameterKey key, void* parameter) {
    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);

    RETURN_IF(-1, mCtcPlayer == nullptr);
    bool ret = true;
    aml::CTC_AVOFFEST_OPT_S param;
    switch (key) {
        case AML_MP_PLAYER_PARAMETER_AUDIO_MUTE:
            aml::CTC_MEDIACONFIG_AUDIOMUTE_S request;
            request.mute = *((bool*)parameter);
            mCtcPlayer->SetParameter(aml::CTC_KEY_PARAMETER_SET_AUDIOMUTE, &request);
            break;
        case AML_MP_PLAYER_PARAMETER_VIDEO_PTS_OFFSET:
            param.u64offest = (*((int*)parameter));
            mCtcPlayer->SetParameter(aml::CTC_KEY_PARAMETER_VIDEO_DELAY, &param);
            break;
        case AML_MP_PLAYER_PARAMETER_AUDIO_PTS_OFFSET:
            param.u64offest = (*((int*)parameter));
            mCtcPlayer->SetParameter(aml::CTC_KEY_PARAMETER_AUDIO_DELAY, &param);
            break;
        case AML_MP_PLAYER_PARAMETER_AUDIO_BALANCE:
            ret = mCtcPlayer->SetAudioBalance(ctcAudioBalanceConvert(*(Aml_MP_AudioBalance*)parameter));
            break;

        case AML_MP_PLAYER_PARAMETER_TELETEXT_CONTROL:
        {
            ret = (AmlPlayerBase::setParameter(key, parameter) == AML_MP_OK) ? true : false;
            break;
        }

        default:
            MLOGI("Not support parameter key: %s", mpPlayerParameterKey2Str(key));
            break;
    }
    return ret?0:-1;
}

int AmlCTCPlayer::getParameter(Aml_MP_PlayerParameterKey key, void* parameter) {
    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);
    MLOGD("Not support parameter key: %s", mpPlayerParameterKey2Str(key));
    AML_MP_UNUSED(key);
    AML_MP_UNUSED(parameter);
    return 0;
}

int AmlCTCPlayer::setAVSyncSource(Aml_MP_AVSyncSource syncSource) {
    RETURN_IF(-1, mCtcPlayer == nullptr);

    bool ret = true;
    MLOGI("%s:%d syncmode:%s\n", __FUNCTION__, __LINE__, mpAVSyncSource2Str(syncSource));
    CTC_SyncSource ctcSyncSource;
    ctcSyncSource = ctcSyncSourceTypeConvert(syncSource);
    ret = mCtcPlayer->initSyncSource(ctcSyncSource);

    return ret?0:-1;
}

int AmlCTCPlayer::setPcrPid(int pid) {
    RETURN_IF(-1, mCtcPlayer == nullptr);

    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);
    aml::CTC_DEMUX_PCRPID_OPT_S para;
    para.pcrpid = pid;
    mCtcPlayer->SetParameter(aml::CTC_KEY_PARAMETER_DEMUX_PCRPID, &para);
    return 0;
}

int AmlCTCPlayer::startVideoDecoding() {
    RETURN_IF(-1, mCtcPlayer == nullptr);

    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);
    switch (mPlayerStat) {
        case CTC_STAT_ASTOP_VSTOP:
            if (!mVideoParaSeted) {
                MLOGI("Video param not seted!!!");
                return -1;
            }
            start();
            if (mAudioParaSeted) {
                mPlayerStat = CTC_STAT_ASTART_VSTART;
            } else {
                mPlayerStat = CTC_STAT_ASTOP_VSTART;
            }
            break;
        case CTC_STAT_ASTART_VSTOP:
            if (!mVideoParaSeted) {
                MLOGI("Video param not seted!!!");
                return -1;
            }
            mCtcPlayer->SwitchVideoTrack(mVideoPara[0].pid, &mVideoPara[0]);
            mPlayerStat = CTC_STAT_ASTART_VSTART;
            break;
        case CTC_STAT_ASTART_VSTART:
        case CTC_STAT_ASTOP_VSTART:
            MLOGI("Video is already start");
            break;
    }
    mIsPause = false;
    return 0;
}

int AmlCTCPlayer::stopVideoDecoding() {
    RETURN_IF(-1, mCtcPlayer == nullptr);

    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);
    switch (mPlayerStat) {
        case CTC_STAT_ASTART_VSTART:
            mCtcPlayer->VideoHide();
            mPlayerStat = CTC_STAT_ASTART_VSTOP;
            break;
        case CTC_STAT_ASTOP_VSTART:
            mCtcPlayer->Stop();
            mPlayerStat = CTC_STAT_ASTOP_VSTOP;
            break;
        case CTC_STAT_ASTART_VSTOP:
        case CTC_STAT_ASTOP_VSTOP:
            MLOGI("Video is already stop");
            break;
    }
    return 0;
}

int AmlCTCPlayer::pauseVideoDecoding() {
    RETURN_IF(-1, mCtcPlayer == nullptr);

    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);
    int ret = -1;
    if (!mIsPause) {
        ret = mCtcPlayer->Pause();
        mIsPause = true;
    } else {
        MLOGI("Video is already paused");
    }
    return ret?0:-1;
}

int AmlCTCPlayer::resumeVideoDecoding() {
    RETURN_IF(-1, mCtcPlayer == nullptr);

    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);
    int ret = -1;
    if (mIsPause) {
        ret = mCtcPlayer->Resume();
        mIsPause = false;
    } else {
        MLOGI("Video is already resumed");
    }
    return ret?0:-1;
}

int AmlCTCPlayer::startAudioDecoding() {
    RETURN_IF(-1, mCtcPlayer == nullptr);

    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);
    switch (mPlayerStat) {
        case CTC_STAT_ASTOP_VSTOP:
            if (!mAudioParaSeted) {
                MLOGI("Audio param not seted!!!");
                return -1;
            }
            start();
            if (mVideoParaSeted) {
                mPlayerStat = CTC_STAT_ASTART_VSTART;
            } else {
                mPlayerStat = CTC_STAT_ASTART_VSTOP;
            }
            break;
        case CTC_STAT_ASTOP_VSTART:
            if (!mAudioParaSeted) {
                MLOGI("Audio param not seted!!!");
                return -1;
            }
            mCtcPlayer->SwitchAudioTrack(mAudioPara[0].pid, &mAudioPara[0]);
            mPlayerStat = CTC_STAT_ASTART_VSTART;
            break;
        case CTC_STAT_ASTART_VSTART:
        case CTC_STAT_ASTART_VSTOP:
            MLOGI("Audio is already start");
            break;
    }
    mIsPause = false;
    return 0;
}

int AmlCTCPlayer::stopAudioDecoding() {
    RETURN_IF(-1, mCtcPlayer == nullptr);

    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);

    bool tmp = true;
    switch (mPlayerStat) {
        case CTC_STAT_ASTART_VSTART:
            mCtcPlayer->SetParameter(aml::CTC_KEY_PARAMETER_SET_AUDIOMUTE, &tmp);
            mPlayerStat = CTC_STAT_ASTOP_VSTART;
            break;
        case CTC_STAT_ASTART_VSTOP:
            mCtcPlayer->Stop();
            mPlayerStat = CTC_STAT_ASTOP_VSTOP;
            break;
        case CTC_STAT_ASTOP_VSTART:
        case CTC_STAT_ASTOP_VSTOP:
            MLOGI("Audio is already stop");
            break;
    }
    return 0;
}

int AmlCTCPlayer::pauseAudioDecoding() {
    RETURN_IF(-1, mCtcPlayer == nullptr);

    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);
    int ret = -1;
    if (!mIsPause) {
        ret = mCtcPlayer->Pause();
        mIsPause = true;
    } else {
        MLOGI("Audio is already paused");
    }
    return ret?0:-1;
}

int AmlCTCPlayer::resumeAudioDecoding() {
    RETURN_IF(-1, mCtcPlayer == nullptr);

    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);
    int ret = -1;
    if (mIsPause) {
        ret = mCtcPlayer->Resume();
        mIsPause = false;
    } else {
        MLOGI("Audio is already resumed");
    }
    return ret?0:-1;
}

int AmlCTCPlayer::setADParams(const Aml_MP_AudioParams* params, bool enableMix) {
    AML_MP_UNUSED(params);
    AML_MP_UNUSED(enableMix);
    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);
    return -1;
}

void AmlCTCPlayer::eventCtcCallback(aml::IPTV_PLAYER_EVT_E event, uint32_t param1, uint32_t param2)
{
    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);
    AML_MP_UNUSED(param1);
    AML_MP_UNUSED(param2);

    switch (event) {
    case aml::IPTV_PLAYER_EVT_FIRST_PTS:
        MLOGI("[evt] CTC IPTV_PLAYER_EVT_FIRST_PTS\n");
        notifyListener(AML_MP_PLAYER_EVENT_FIRST_FRAME);
        break;
    case aml::IPTV_PLAYER_EVT_AUDIO_FIRST_DECODED:
        MLOGI("[evt] CTC IPTV_PLAYER_EVT_AUDIO_FIRST_DECODED\n");
        break;
    case aml::IPTV_PLAYER_EVT_USERDATA_READY:
        MLOGI("[evt] CTC IPTV_PLAYER_EVT_USERDATA_READY\n");
        break;
    case aml::IPTV_PLAYER_EVT_VID_BUFF_UNLOAD_START:
        MLOGI("[evt] CTC IPTV_PLAYER_EVT_VID_BUFF_UNLOAD_START\n");
        break;
    case aml::IPTV_PLAYER_EVT_VID_BUFF_UNLOAD_END:
        MLOGI("[evt] CTC IPTV_PLAYER_EVT_VID_BUFF_UNLOAD_END\n");
        break;
    case aml::IPTV_PLAYER_EVT_VID_MOSAIC_START:
        MLOGI("[evt] CTC IPTV_PLAYER_EVT_VID_MOSAIC_START\n");
        break;
    case aml::IPTV_PLAYER_EVT_VID_MOSAIC_END:
        MLOGI("[evt] CTC IPTV_PLAYER_EVT_VID_MOSAIC_END\n");
        break;
    case aml::IPTV_PLAYER_EVT_VID_FRAME_ERROR:
        MLOGI("[evt] CTC IPTV_PLAYER_EVT_VID_FRAME_ERROR\n");
        break;
    case aml::IPTV_PLAYER_EVT_VID_DISCARD_FRAME:
        MLOGI("[evt] CTC IPTV_PLAYER_EVT_VID_DISCARD_FRAME\n");
        break;
    case aml::IPTV_PLAYER_EVT_VID_DEC_UNDERFLOW:
        MLOGI("[evt] CTC IPTV_PLAYER_EVT_VID_DEC_UNDERFLOW\n");
        break;
    case aml::IPTV_PLAYER_EVT_VID_DEC_OVERFLOW:
        MLOGI("[evt] CTC IPTV_PLAYER_EVT_VID_DEC_OVERFLOW\n");
        break;
    case aml::IPTV_PLAYER_EVT_VID_PTS_ERROR:
        MLOGI("[evt] CTC PROBE_PLAYER_EVT_VID_PTS_ERROR\n");
        break;
    case aml::IPTV_PLAYER_EVT_VID_INVALID_DATA:
        MLOGI("[evt] CTC IPTV_PLAYER_EVT_VID_INVALID_DATA\n");
        break;
    case aml::IPTV_PLAYER_EVT_AUD_FRAME_ERROR:
        MLOGI("[evt] CTC IPTV_PLAYER_EVT_AUD_FRAME_ERROR\n");
        break;
    case aml::IPTV_PLAYER_EVT_AUD_DISCARD_FRAME:
        MLOGI("[evt] CTC IPTV_PLAYER_EVT_AUD_DISCARD_FRAME\n");
        break;
    case aml::IPTV_PLAYER_EVT_AUD_PTS_ERROR:
        MLOGI("[evt] CTC IPTV_PLAYER_EVT_AUD_PTS_ERROR\n");
        break;
    case aml::IPTV_PLAYER_EVT_AUD_DEC_UNDERFLOW:
        MLOGI("[evt] CTC IPTV_PLAYER_EVT_AUD_DEC_UNDERFLOW\n");
        break;
    case aml::IPTV_PLAYER_EVT_AUD_DEC_OVERFLOW:
        MLOGI("[evt] CTC IPTV_PLAYER_EVT_AUD_DEC_OVERFLOW\n");
        break;
    case aml::IPTV_PLAYER_EVT_AUD_INVALID_DATA:
        MLOGI("[evt] CTC IPTV_PLAYER_EVT_AUD_INVALID_DATA\n");
        break;
    default :
        MLOGE("unhandled event:%d", event);
        break;
    }
}

}
