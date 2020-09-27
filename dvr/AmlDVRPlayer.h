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

#ifndef _AML_DVR_PLAYER_H_
#define _AML_DVR_PLAYER_H_

#include <Aml_MP/Dvr.h>
#include <utils/RefBase.h>
#include <utils/AmlMpHandle.h>
#include <utils/AmlMpUtils.h>
#include "AmTsPlayer.h"

extern "C" int dvr_wrapper_set_playback_secure_buffer (DVR_WrapperPlayback_t playback,  uint8_t *p_secure_buf, uint32_t len);

namespace aml_mp {

class AmlDVRPlayer final : public AmlMpHandle
{
public:
    AmlDVRPlayer(Aml_MP_DVRPlayerBasicParams* basicParams, Aml_MP_DVRPlayerDecryptParams* decryptParams = nullptr);
    ~AmlDVRPlayer();
    int registerEventCallback(Aml_MP_DVRPlayerEventCallback cb, void* userData);
    int setStreams(Aml_MP_DVRStreamArray* streams);
    int start(bool initialPaused);
    int stop();
    int pause();
    int resume();
    int seek(int timeOffset);
    int setPlaybackRate(float rate);
    int getStatus(Aml_MP_DVRPlayerStatus* status);
    int showVideo();
    int hideVideo();
    int setVolume(float volume);
    int getVolume(float* volume);
    int setParameter(Aml_MP_PlayerParameterKey key, void* parameter);
    int getParameter(Aml_MP_PlayerParameterKey key, void* parameter);

private:
    char mName[50];

    DVR_WrapperPlayback_t mDVRPlayerHandle{0};
    DVR_WrapperPlaybackOpenParams_t mPlaybackOpenParams{};
    DVR_PlaybackPids_t mPlayPids{};

    bool mIsEncryptStream;
    uint8_t* mSecureBuffer = nullptr;
    size_t mSecureBufferSize = 0;

    int setBasicParams(Aml_MP_DVRPlayerBasicParams* basicParams);
    int setDecryptParams(Aml_MP_DVRPlayerDecryptParams* decryptParams);

private:
    AmlDVRPlayer(const AmlDVRPlayer&) = delete;
    AmlDVRPlayer& operator=(const AmlDVRPlayer&) = delete;
};

}
#endif
