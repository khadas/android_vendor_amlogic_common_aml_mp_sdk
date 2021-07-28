/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _AML_CTC_PLAYER_H_
#define _AML_CTC_PLAYER_H_

#include "AmlPlayerBase.h"
#include <CTC_MediaProcessor.h>

struct ANativeWindow;

namespace aml_mp {
using namespace aml;

typedef enum {
    CTC_STAT_ASTOP_VSTOP,
    CTC_STAT_ASTOP_VSTART,
    CTC_STAT_ASTART_VSTOP,
    CTC_STAT_ASTART_VSTART,
} Ctc_Player_Stat;

class AmlCTCPlayer : public aml_mp::AmlPlayerBase
{
public:
    AmlCTCPlayer(Aml_MP_PlayerCreateParams* createParams, int instanceId);
    ~AmlCTCPlayer();

    int setANativeWindow(ANativeWindow* nativeWindow);

    int initCheck() const override;
    int setVideoParams(const Aml_MP_VideoParams* params) override;
    int setAudioParams(const Aml_MP_AudioParams* params) override;
    int start() override;
    int stop() override;
    int pause() override;
    int resume() override;
    int flush() override;
    int setPlaybackRate(float rate) override;
    int switchAudioTrack(const Aml_MP_AudioParams* params) override;
    int writeData(const uint8_t* buffer, size_t size) override;
    int writeEsData(Aml_MP_StreamType type, const uint8_t* buffer, size_t size, int64_t pts) override;
    int getCurrentPts(Aml_MP_StreamType type, int64_t* pts) override;
    int getBufferStat(Aml_MP_BufferStat* bufferStat) override;
    int setVideoWindow(int x, int y, int width, int height) override;
    int setVolume(float volume) override;
    int getVolume(float* volume) override;
    int showVideo() override;
    int hideVideo() override;
    int setParameter(Aml_MP_PlayerParameterKey key, void* parameter) override;
    int getParameter(Aml_MP_PlayerParameterKey key, void* parameter) override;
    int setAVSyncSource(Aml_MP_AVSyncSource syncSource) override;
    int setPcrPid(int pid) override;

    int startVideoDecoding() override;
    int stopVideoDecoding() override;
    int pauseVideoDecoding();
    int resumeVideoDecoding();

    int startAudioDecoding() override;
    int stopAudioDecoding() override;
    int pauseAudioDecoding();
    int resumeAudioDecoding();

    int setADParams(const Aml_MP_AudioParams* params, bool enableMix) override;


private:
    char mName[50];
    aml::CTC_MediaProcessor* mCtcPlayer = nullptr;
    void eventCtcCallback(aml::IPTV_PLAYER_EVT_E event, uint32_t param1, uint32_t param2);
    Ctc_Player_Stat mPlayerStat = CTC_STAT_ASTOP_VSTOP;
    aml::VIDEO_PARA_T mVideoPara[MAX_VIDEO_PARAM_SIZE];
    aml::AUDIO_PARA_T mAudioPara[MAX_AUDIO_PARAM_SIZE];
    bool mIsPause;
    bool mVideoParaSeted;
    bool mAudioParaSeted;

private:
    AmlCTCPlayer(const AmlCTCPlayer&) = delete;
    AmlCTCPlayer& operator= (const AmlCTCPlayer&) = delete;
};

}

#endif
