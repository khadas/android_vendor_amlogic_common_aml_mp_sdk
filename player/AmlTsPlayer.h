/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _AML_TS_PLAYER_H_
#define _AML_TS_PLAYER_H_

#include "AmlPlayerBase.h"
#include <AmTsPlayer.h>
#include <utils/AmlMpUtils.h>

namespace android {
class NativeHandle;
}

struct ANativeWindow;

namespace aml_mp {
using android::NativeHandle;

class AmlTsPlayer : public aml_mp::AmlPlayerBase
{
public:
    AmlTsPlayer(Aml_MP_PlayerCreateParams* createParams, int instanceId);
    ~AmlTsPlayer();

    int setANativeWindow(ANativeWindow* nativeWindow);

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
    int startADDecoding() override;
    int stopADDecoding() override;
    int pauseAudioDecoding();
    int resumeAudioDecoding();

    int setADParams(const Aml_MP_AudioParams* params, bool enableMix) override;


private:
    void eventCallback(am_tsplayer_event* event);

    char mName[50];
    am_tsplayer_handle mPlayer{0};
    am_tsplayer_init_params init_param = {TS_MEMORY, TS_INPUT_BUFFER_TYPE_NORMAL, 0, 0};
    const int kRwTimeout = 30000;
    int mVideoTunnelId = -1;

    ANativeWindow* mNativewindow = nullptr;
    int mBlackOut = 0;
    NativeWindowHelper mNativeWindowHelper;

private:
    AmlTsPlayer(const AmlTsPlayer&) = delete;
    AmlTsPlayer& operator= (const AmlTsPlayer&) = delete;
};

}

#endif
