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

#ifndef _AML_PLAYER_BASE_H_
#define _AML_PLAYER_BASE_H_

#include <utils/RefBase.h>
#include <Aml_MP/Aml_MP.h>
#include <SubtitleNativeAPI.h>

namespace android {
class NativeHandle;
}

struct ANativeWindow;

namespace aml_mp {
using android::sp;
using android::RefBase;
using android::NativeHandle;

class AmlPlayerBase : public RefBase {
public:
    static sp<AmlPlayerBase> create(Aml_MP_PlayerCreateParams* createParams, int instanceId);
    virtual ~AmlPlayerBase();

    int registerEventCallback(Aml_MP_PlayerEventCallback cb, void* userData);
    int setANativeWindow(ANativeWindow* nativeWindow);

    int setSubtitleParams(const Aml_MP_SubtitleParams* params);
    int switchSubtitleTrack(const Aml_MP_SubtitleParams* params);
    int showSubtitle();
    int hideSubtitle();
    int startSubtitleDecoding();
    int stopSubtitleDecoding();
    int setSubtitleWindow(int x, int y, int width, int height);

    virtual int setVideoParams(const Aml_MP_VideoParams* params) = 0;
    virtual int setAudioParams(const Aml_MP_AudioParams* params) = 0;
    virtual int start();
    virtual int stop();
    virtual int pause() = 0;
    virtual int resume() = 0;
    virtual int flush() = 0;
    virtual int setPlaybackRate(float rate) = 0;
    virtual int switchAudioTrack(const Aml_MP_AudioParams* params) = 0;
    virtual int writeData(const uint8_t* buffer, size_t size) = 0;
    virtual int writeEsData(Aml_MP_StreamType type, const uint8_t* buffer, size_t size, int64_t pts) = 0;
    virtual int getCurrentPts(Aml_MP_StreamType type, int64_t* pts) = 0;
    virtual int getBufferStat(Aml_MP_BufferStat* bufferStat) = 0;
    virtual int setVideoWindow(int x, int y, int width, int height) = 0;
    virtual int setVolume(float volume) = 0;
    virtual int getVolume(float* volume) = 0;
    virtual int showVideo() = 0;
    virtual int hideVideo() = 0;
    virtual int setParameter(Aml_MP_PlayerParameterKey key, void* parameter) = 0;
    virtual int getParameter(Aml_MP_PlayerParameterKey key, void* parameter) = 0;

    virtual int setAVSyncSource(Aml_MP_AVSyncSource syncSource) = 0;
    virtual int setPcrPid(int pid) = 0;

    virtual int startVideoDecoding() = 0;
    virtual int stopVideoDecoding() = 0;

    virtual int startAudioDecoding() = 0;
    virtual int stopAudioDecoding() = 0;

    virtual int setADParams(Aml_MP_AudioParams* params) = 0;

protected:
    int mSubWindowX;
    int mSubWindowY;
    int mSubWindowWidth;
    int mSubWindowHeight;

    explicit AmlPlayerBase(int instanceId);
    void notifyListener(Aml_MP_PlayerEventType eventType, int64_t param = 0);

private:
    static bool constructAmlSubtitleParam(AmlSubtitleParam* amlSubParam, Aml_MP_SubtitleParams* params);

    char mName[50];

    Aml_MP_PlayerEventCallback mEventCb;
    void* mUserData;

    sp<NativeHandle> mSidebandHandle;

    Aml_MP_SubtitleParams mSubtitleParams;
    AmlSubtitleHnd mSubtitleHandle = nullptr;

    AmlPlayerBase(const AmlPlayerBase&) = delete;
    AmlPlayerBase& operator= (const AmlPlayerBase&) = delete;
};

}

#endif
