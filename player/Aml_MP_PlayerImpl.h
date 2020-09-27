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

#ifndef _AML_MP_PLAYER_IMPL_H_
#define _AML_MP_PLAYER_IMPL_H_

#include <Aml_MP/Aml_MP.h>
#include <utils/RefBase.h>
#include <utils/AmlMpHandle.h>
#include <system/window.h>
#include "cas/AmlCasBase.h"

namespace aml_mp {
using android::RefBase;
class AmlPlayerBase;
class AmlMpConfig;

class AmlMpPlayerImpl final : public AmlMpHandle
{
public:
    explicit AmlMpPlayerImpl(const Aml_MP_PlayerCreateParams* createParams);
    ~AmlMpPlayerImpl();
    int registerEventCallback(Aml_MP_PlayerEventCallback cb, void* userData);
    int setVideoParams(const Aml_MP_VideoParams* params);
    int setAudioParams(const Aml_MP_AudioParams* params);
    int setSubtitleParams(const Aml_MP_SubtitleParams* params);
    int setCASParams(const Aml_MP_CASParams* params);
    int start();
    int stop();
    int pause();
    int resume();
    int flush();
    int setPlaybackRate(float rate);
    int switchAudioTrack(const Aml_MP_AudioParams* params);
    int switchSubtitleTrack(const Aml_MP_SubtitleParams* params);
    int writeData(const uint8_t* buffer, size_t size);
    int writeEsData(Aml_MP_StreamType type, const uint8_t* buffer, size_t size, int64_t pts);
    int getCurrentPts(Aml_MP_StreamType, int64_t* pts);
    int getBufferStat(Aml_MP_BufferStat* bufferStat);
    int setAnativeWindow(void* nativeWindow);
    int setVideoWindow(int x, int y, int width, int height);
    int setVolume(float volume);
    int getVolume(float* volume);
    int showVideo();
    int hideVideo();
    int showSubtitle();
    int hideSubtitle();
    int setParameter(Aml_MP_PlayerParameterKey key, void* parameter);
    int getParameter(Aml_MP_PlayerParameterKey key, void* parameter);

    int setAVSyncSource(Aml_MP_AVSyncSource syncSource);
    int setPcrPid(int pid);

    int startVideoDecoding();
    int stopVideoDecoding();

    int startAudioDecoding();
    int stopAudioDecoding();

    int startSubtitleDecoding();
    int stopSubtitleDecoding();

    int startDescrambling();
    int stopDescrambling();

    int setADParams(Aml_MP_AudioParams* params);
    int setSubtitleWindow(int x, int y, int width, int height);

private:
    struct WindowSize {
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
    };

    const int mInstanceId;
    char mName[50];
    Aml_MP_PlayerCreateParams mCreateParams;
    Aml_MP_PlayerEventCallback mEventCb = nullptr;
    void* mUserData = nullptr;
    Aml_MP_VideoParams mVideoParams;
    Aml_MP_AudioParams mAudioParams;
    Aml_MP_SubtitleParams mSubtitleParams;
    WindowSize mSubtitleWindow;
    Aml_MP_CASParams mCASParams;
    Aml_MP_AudioParams mADParams;

    Aml_MP_AudioBalance mAudioBalance;
    float mVolume = 100.0;

    float mPlaybackRate = 1.0f;
    sp<ANativeWindow> mNativeWindow;
    WindowSize mVideoWindow;

    Aml_MP_AVSyncSource mSyncSource = AML_MP_AVSYNC_SOURCE_DEFAULT;
    int mPcrPid = AML_MP_INVALID_PID;

    sp<AmlPlayerBase> mPlayer;

    sp<AmlCasBase> mCasHandle;

private:
    AmlMpPlayerImpl(const AmlMpPlayerImpl&) = delete;
    AmlMpPlayerImpl& operator= (const AmlMpPlayerImpl&) = delete;
};

}


#endif
