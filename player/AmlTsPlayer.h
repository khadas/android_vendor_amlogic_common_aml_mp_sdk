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

#ifndef _AML_TS_PLAYER_H_
#define _AML_TS_PLAYER_H_

#include "AmlPlayerBase.h"
#include <AmTsPlayer.h>

namespace aml_mp {

class AmlTsPlayer : public aml_mp::AmlPlayerBase
{
public:
    AmlTsPlayer(Aml_MP_PlayerCreateParams* createParams, int instanceId);
    ~AmlTsPlayer();

    int registerEventCallback(Aml_MP_PlayerEventCallback cb, void* userData);
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

    int setADParams(Aml_MP_AudioParams* params) override;


private:
    void eventCallback(am_tsplayer_event* event);
    am_tsplayer_handle mPlayer{0};
    am_tsplayer_init_params init_param = {TS_MEMORY, TS_INPUT_BUFFER_TYPE_NORMAL, 0, 0};
    const int kRwTimeout = 30000;

private:
    AmlTsPlayer(const AmlTsPlayer&) = delete;
    AmlTsPlayer& operator= (const AmlTsPlayer&) = delete;
};

}

#endif
