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

AmlCTCPlayer::AmlCTCPlayer(Aml_MP_PlayerCreateParams* createParams, int instanceId)
: aml_mp::AmlPlayerBase(instanceId)
{
    RETURN_VOID_IF(createParams == nullptr);

    aml::CTC_InitialParameter params;
    ALOGI("%s:%d instanceId=%d\n", __FUNCTION__, __LINE__, instanceId);
    params.userId = createParams->userId;
    params.useOmx = 1; //0:ctc 1:pip
    params.isEsSource = 0; //0:TS source 1:ES source
    params.flags = 0;
    params.extensionSize = 0;
    mCtcPlayer = aml::GetMediaProcessor(&params);

    mCtcPlayer->playerback_register_evt_cb([](void* user_data, aml::IPTV_PLAYER_EVT_E event, uint32_t param1, uint32_t param2) {
        static_cast<AmlCTCPlayer*>(user_data)->eventCtcCallback(event,param1,param2);
    }, this);
}

AmlCTCPlayer::~AmlCTCPlayer()
{
    ALOGI("%s:%d\n", __FUNCTION__, __LINE__);
    if (mCtcPlayer != nullptr) {
        delete mCtcPlayer;
        mCtcPlayer = nullptr;
    }
}

int AmlCTCPlayer::setANativeWindow(ANativeWindow* pSurface) {
    RETURN_IF(-1, pSurface == nullptr);
    RETURN_IF(-1, mCtcPlayer == nullptr);

    ALOGI("%s:%d ANativeWindow: %p\n", __FUNCTION__, __LINE__, pSurface);
    mCtcPlayer->SetSurface_ANativeWindow(pSurface);

    return 0;
}

int AmlCTCPlayer::setVideoParams(const Aml_MP_VideoParams* params) {
    RETURN_IF(-1, params == nullptr);
    RETURN_IF(-1, mCtcPlayer == nullptr);

    aml::VIDEO_PARA_T videoPara[MAX_VIDEO_PARAM_SIZE];
    memset(videoPara, 0, sizeof(videoPara));
    ALOGI("%s:%d videoCodec: %d vpid: %d width: %d height: %d\n", __FUNCTION__, __LINE__,
        params->videoCodec, params->pid, params->width, params->height);
    videoPara[0].pid = params->pid;
    videoPara[0].nVideoWidth = params->width;
    videoPara[0].nVideoHeight = params->height;
    videoPara[0].nFrameRate = params->frameRate;
    videoPara[0].vFmt = ctcVideoCodecConvert(params->videoCodec);
    videoPara[0].nExtraSize = params->extraDataSize;
    videoPara[0].pExtraData = (uint8_t *)params->extraData;
    mCtcPlayer->InitVideo(videoPara);

    return 0;
}

int AmlCTCPlayer::setAudioParams(const Aml_MP_AudioParams* params) {
    RETURN_IF(-1, params == nullptr);
    RETURN_IF(-1, mCtcPlayer == nullptr);

    aml::AUDIO_PARA_T audioPara[MAX_AUDIO_PARAM_SIZE];
    memset(audioPara, 0, sizeof(audioPara));
    ALOGI("%s:%d audioCodec: %d apid: %d ch: %d samp: %d\n", __FUNCTION__, __LINE__,
        params->audioCodec, params->pid, params->nChannels, params->nSampleRate);
    audioPara[0].pid = params->pid;
    audioPara[0].nChannels = params->nChannels;
    audioPara[0].nSampleRate = params->nSampleRate;
    audioPara[0].aFmt = ctcAudioCodecConvert(params->audioCodec);
    audioPara[0].block_align = 0;
    audioPara[0].bit_per_sample = 0;
    audioPara[0].nExtraSize = params->extraDataSize;
    audioPara[0].pExtraData = (uint8_t *)params->extraData;
    mCtcPlayer->InitAudio(audioPara);

    return 0;
}

int AmlCTCPlayer::start() {
    RETURN_IF(-1, mCtcPlayer == nullptr);

    int ret = 0;
    ALOGI("%s:%d\n", __FUNCTION__, __LINE__);
    ret = mCtcPlayer->StartPlay();
    AmlPlayerBase::start();

    return ret;
}

int AmlCTCPlayer::stop() {
    RETURN_IF(-1, mCtcPlayer == nullptr);

    int ret = 0;
    ALOGI("%s:%d\n", __FUNCTION__, __LINE__);
    AmlPlayerBase::stop();
    ret = mCtcPlayer->Stop();

    return ret;
}

int AmlCTCPlayer::pause() {
    RETURN_IF(-1, mCtcPlayer == nullptr);

    int ret = 0;
    ALOGI("%s:%d\n", __FUNCTION__, __LINE__);
    ret = mCtcPlayer->Pause();

    return ret;
}

int AmlCTCPlayer::resume() {
    RETURN_IF(-1, mCtcPlayer == nullptr);

    int ret = 0;
    ALOGI("%s:%d\n", __FUNCTION__, __LINE__);
    ret = mCtcPlayer->Resume();

    return ret;
}

int AmlCTCPlayer::flush() {return -1;}

int AmlCTCPlayer::setPlaybackRate(float rate) {
    RETURN_IF(-1, mCtcPlayer == nullptr);

    int ret = 0;
    ALOGI("%s:%d rate=%f\n", __FUNCTION__, __LINE__, rate);
    if (rate == 1.0f) {
        ret = mCtcPlayer->StopFast();
    } else {
        ret = mCtcPlayer->Fast();
    }

    return ret;
}

int AmlCTCPlayer::switchAudioTrack(const Aml_MP_AudioParams* params){
    RETURN_IF(-1, params == nullptr);
    RETURN_IF(-1, mCtcPlayer == nullptr);

    aml::AUDIO_PARA_T audioPara;
    memset(&audioPara, 0, sizeof(audioPara));
    ALOGI("%s:%d audioCodec: %d apid: %d ch: %d samp: %d\n", __FUNCTION__, __LINE__,
        params->pid, params->audioCodec, params->nChannels, params->nSampleRate);
    audioPara.pid = params->pid;
    audioPara.nChannels = params->nChannels;
    audioPara.nSampleRate = params->nSampleRate;
    audioPara.aFmt = ctcAudioCodecConvert(params->audioCodec);
    audioPara.block_align = 0;
    audioPara.bit_per_sample = 0;
    audioPara.nExtraSize = params->extraDataSize;
    audioPara.pExtraData = (uint8_t *)params->extraData;
    mCtcPlayer->SwitchAudioTrack(audioPara.pid , &audioPara);

    return 0;
}

int AmlCTCPlayer::writeData(const uint8_t* buffer, size_t size) {
    RETURN_IF(-1, mCtcPlayer == nullptr);

    int ret = 0;
    ret = mCtcPlayer->WriteData((uint8_t*)buffer, (uint32_t)size);
    //ALOGI("AmlCTCPlayer->writeData, buffer:%p, size:%d, ret:%d", buffer, size, ret);

    return ret;
}

int AmlCTCPlayer::writeEsData(Aml_MP_StreamType type, const uint8_t* buffer, size_t size, int64_t pts)
{
    ALOGI("%s:%d\n", __FUNCTION__, __LINE__);

    AML_MP_UNUSED(type);
    AML_MP_UNUSED(buffer);
    AML_MP_UNUSED(size);
    AML_MP_UNUSED(pts);

    return -1;
}

int AmlCTCPlayer::getCurrentPts(Aml_MP_StreamType type, int64_t* pts) {
    RETURN_IF(-1, mCtcPlayer == nullptr);

    int64_t ctcPts = mCtcPlayer->GetCurrentPts(ctcStreamTypeConvert(type));
    ALOGI("%s:%d type: %d ctcPts: 0x%llx \n", __FUNCTION__, __LINE__, type, ctcPts);
    pts = &ctcPts;
    return 0;
}

int AmlCTCPlayer::getBufferStat(Aml_MP_BufferStat* bufferStat) {
    RETURN_IF(-1, mCtcPlayer == nullptr);

    ALOGI("%s:%d\n", __FUNCTION__, __LINE__);
    int ret = 0;
    aml::AVBUF_STATUS buffer_stat;
    ret = mCtcPlayer->GetBufferStatus(&buffer_stat);
    bufferStat->audioBuffer.size = buffer_stat.abuf_size;
    bufferStat->audioBuffer.dataLen = buffer_stat.abuf_data_len;
    bufferStat->audioBuffer.bufferedMs = buffer_stat.abuf_ms;
    bufferStat->videoBuffer.size = buffer_stat.vbuf_size;
    bufferStat->videoBuffer.dataLen = buffer_stat.vbuf_data_len;
    bufferStat->videoBuffer.bufferedMs = buffer_stat.vbuf_ms;

    return ret;
}

int AmlCTCPlayer::setVideoWindow(int x, int y, int width, int height) {
    RETURN_IF(-1, mCtcPlayer == nullptr);

    ALOGI("%s:%d x: %d y: %d width: %d height: %d\n", __FUNCTION__, __LINE__, x, y, width, height);
    mCtcPlayer->SetVideoWindow(x, y, width, height);

    return 0;
}

int AmlCTCPlayer::setVolume(float volume) {
    RETURN_IF(-1, mCtcPlayer == nullptr);

    int ret = 0;
    ALOGI("%s:%d vol=%f\n", __FUNCTION__, __LINE__, volume);
    ret = mCtcPlayer->SetVolume((int) volume);

    return ret;
}

int AmlCTCPlayer::getVolume(float* volume) {
    RETURN_IF(-1, mCtcPlayer == nullptr);

    int tempVol = 0;
    tempVol = mCtcPlayer->GetVolume();
    ALOGI("%s:%d vol: %d\n", __FUNCTION__, __LINE__, tempVol);
    *volume = (float) tempVol;

    return 0;
}

int AmlCTCPlayer::showVideo() {
    RETURN_IF(-1, mCtcPlayer == nullptr);

    int ret = 0;
    ALOGI("%s:%d\n", __FUNCTION__, __LINE__);
    ret = mCtcPlayer->VideoShow();

    return ret;
}

int AmlCTCPlayer::hideVideo() {
    RETURN_IF(-1, mCtcPlayer == nullptr);

    int ret = 0;
    ALOGI("%s:%d\n", __FUNCTION__, __LINE__);
    ret = mCtcPlayer->VideoHide();

    return ret;
}

int AmlCTCPlayer::setParameter(Aml_MP_PlayerParameterKey key, void* parameter) {
    ALOGI("%s:%d\n", __FUNCTION__, __LINE__);
    AML_MP_UNUSED(key);
    AML_MP_UNUSED(parameter);
    return 0;
}

int AmlCTCPlayer::getParameter(Aml_MP_PlayerParameterKey key, void* parameter) {
    ALOGI("%s:%d\n", __FUNCTION__, __LINE__);
    AML_MP_UNUSED(key);
    AML_MP_UNUSED(parameter);
    return 0;
}

int AmlCTCPlayer::setAVSyncSource(Aml_MP_AVSyncSource syncSource) {
    RETURN_IF(-1, mCtcPlayer == nullptr);

    int ret = 0;
    ALOGI("%s:%d syncmode:%d\n", __FUNCTION__, __LINE__, syncSource);
    CTC_SyncSource ctcSyncSource;
    ctcSyncSource = ctcSyncSourceTypeConvert(syncSource);
    ret = mCtcPlayer->initSyncSource(ctcSyncSource);

    return ret;
}

int AmlCTCPlayer::setPcrPid(int pid) {
    ALOGI("%s:%d\n", __FUNCTION__, __LINE__);
    AML_MP_UNUSED(pid);
    return 0;
}

int AmlCTCPlayer::startVideoDecoding() {
    ALOGI("%s:%d\n", __FUNCTION__, __LINE__);
    return 0;
}

int AmlCTCPlayer::stopVideoDecoding() {
    ALOGI("%s:%d\n", __FUNCTION__, __LINE__);
    return 0;

}

int AmlCTCPlayer::pauseVideoDecoding() {
    ALOGI("%s:%d\n", __FUNCTION__, __LINE__);
    return 0;
}

int AmlCTCPlayer::resumeVideoDecoding() {
    ALOGI("%s:%d\n", __FUNCTION__, __LINE__);
    return 0;
}

int AmlCTCPlayer::startAudioDecoding() {
    ALOGI("%s:%d\n", __FUNCTION__, __LINE__);
    return 0;
}

int AmlCTCPlayer::stopAudioDecoding() {
    ALOGI("%s:%d\n", __FUNCTION__, __LINE__);
    return 0;
}

int AmlCTCPlayer::pauseAudioDecoding() {
    ALOGI("%s:%d\n", __FUNCTION__, __LINE__);
    return 0;
}

int AmlCTCPlayer::resumeAudioDecoding() {
    ALOGI("%s:%d\n", __FUNCTION__, __LINE__);
    return 0;
}

int AmlCTCPlayer::setADParams(Aml_MP_AudioParams* params) {
    AML_MP_UNUSED(params);
    return -1;
}

void AmlCTCPlayer::eventCtcCallback(aml::IPTV_PLAYER_EVT_E event, uint32_t param1, uint32_t param2)
{
    AML_MP_UNUSED(param1);
    AML_MP_UNUSED(param2);

    switch (event) {
    case aml::IPTV_PLAYER_EVT_FIRST_PTS:
        ALOGI("[evt] CTC IPTV_PLAYER_EVT_FIRST_PTS\n");
        notifyListener(AML_MP_PLAYER_EVENT_FIRST_FRAME);
        break;
    case aml::IPTV_PLAYER_EVT_AUDIO_FIRST_DECODED:
        ALOGI("[evt] CTC IPTV_PLAYER_EVT_AUDIO_FIRST_DECODED\n");
        break;
    case aml::IPTV_PLAYER_EVT_USERDATA_READY:
        ALOGI("[evt] CTC IPTV_PLAYER_EVT_USERDATA_READY\n");
        break;
    case aml::IPTV_PLAYER_EVT_VID_BUFF_UNLOAD_START:
        ALOGI("[evt] CTC IPTV_PLAYER_EVT_VID_BUFF_UNLOAD_START\n");
        break;
    case aml::IPTV_PLAYER_EVT_VID_BUFF_UNLOAD_END:
        ALOGI("[evt] CTC IPTV_PLAYER_EVT_VID_BUFF_UNLOAD_END\n");
        break;
    case aml::IPTV_PLAYER_EVT_VID_MOSAIC_START:
        ALOGI("[evt] CTC IPTV_PLAYER_EVT_VID_MOSAIC_START\n");
        break;
    case aml::IPTV_PLAYER_EVT_VID_MOSAIC_END:
        ALOGI("[evt] CTC IPTV_PLAYER_EVT_VID_MOSAIC_END\n");
        break;
    case aml::IPTV_PLAYER_EVT_VID_FRAME_ERROR:
        ALOGI("[evt] CTC IPTV_PLAYER_EVT_VID_FRAME_ERROR\n");
        break;
    case aml::IPTV_PLAYER_EVT_VID_DISCARD_FRAME:
        ALOGI("[evt] CTC IPTV_PLAYER_EVT_VID_DISCARD_FRAME\n");
        break;
    case aml::IPTV_PLAYER_EVT_VID_DEC_UNDERFLOW:
        ALOGI("[evt] CTC IPTV_PLAYER_EVT_VID_DEC_UNDERFLOW\n");
        break;
    case aml::IPTV_PLAYER_EVT_VID_DEC_OVERFLOW:
        ALOGI("[evt] CTC IPTV_PLAYER_EVT_VID_DEC_OVERFLOW\n");
        break;
    case aml::IPTV_PLAYER_EVT_VID_PTS_ERROR:
        ALOGI("[evt] CTC PROBE_PLAYER_EVT_VID_PTS_ERROR\n");
        break;
    case aml::IPTV_PLAYER_EVT_VID_INVALID_DATA:
        ALOGI("[evt] CTC IPTV_PLAYER_EVT_VID_INVALID_DATA\n");
        break;
    case aml::IPTV_PLAYER_EVT_AUD_FRAME_ERROR:
        ALOGI("[evt] CTC IPTV_PLAYER_EVT_AUD_FRAME_ERROR\n");
        break;
    case aml::IPTV_PLAYER_EVT_AUD_DISCARD_FRAME:
        ALOGI("[evt] CTC IPTV_PLAYER_EVT_AUD_DISCARD_FRAME\n");
        break;
    case aml::IPTV_PLAYER_EVT_AUD_PTS_ERROR:
        ALOGI("[evt] CTC IPTV_PLAYER_EVT_AUD_PTS_ERROR\n");
        break;
    case aml::IPTV_PLAYER_EVT_AUD_DEC_UNDERFLOW:
        ALOGI("[evt] CTC IPTV_PLAYER_EVT_AUD_DEC_UNDERFLOW\n");
        break;
    case aml::IPTV_PLAYER_EVT_AUD_DEC_OVERFLOW:
        ALOGI("[evt] CTC IPTV_PLAYER_EVT_AUD_DEC_OVERFLOW\n");
        break;
    case aml::IPTV_PLAYER_EVT_AUD_INVALID_DATA:
        ALOGI("[evt] CTC IPTV_PLAYER_EVT_AUD_INVALID_DATA\n");
        break;
    default :
        ALOGE("unhandled event:%d", event);
        break;
    }
}

}
