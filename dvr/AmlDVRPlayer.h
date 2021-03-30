/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _AML_DVR_PLAYER_H_
#define _AML_DVR_PLAYER_H_

#include <system/window.h>
#include <Aml_MP/Dvr.h>
#include <utils/AmlMpRefBase.h>
#include <utils/AmlMpHandle.h>
#include <utils/AmlMpUtils.h>
#include "AmTsPlayer.h"
#ifdef ANDROID
#include <utils/RefBase.h>
#include <utils/StrongPointer.h>
#endif

namespace android {
class NativeHandle;
}

extern "C" int dvr_wrapper_set_playback_secure_buffer (DVR_WrapperPlayback_t playback,  uint8_t *p_secure_buf, uint32_t len);

namespace aml_mp {
using android::NativeHandle;

class AmlDVRPlayer final : public AmlMpHandle
{
public:
    AmlDVRPlayer(Aml_MP_DVRPlayerBasicParams* basicParams, Aml_MP_DVRPlayerDecryptParams* decryptParams = nullptr);
    ~AmlDVRPlayer();
    int registerEventCallback(Aml_MP_PlayerEventCallback cb, void* userData);
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
    int setANativeWindow(void* nativeWindow);

private:
    char mName[50];

    DVR_WrapperPlayback_t mDVRPlayerHandle{0};
    am_tsplayer_handle mTsPlayerHandle{0};
    DVR_WrapperPlaybackOpenParams_t mPlaybackOpenParams{};
    DVR_PlaybackPids_t mPlayPids{};
    int mVendorID{};

    bool mIsEncryptStream;
    uint8_t* mSecureBuffer = nullptr;
    size_t mSecureBufferSize = 0;

    Aml_MP_PlayerEventCallback mEventCb = nullptr;
    void* mEventUserData = nullptr;

    int setBasicParams(Aml_MP_DVRPlayerBasicParams* basicParams);
    int setDecryptParams(Aml_MP_DVRPlayerDecryptParams* decryptParams);
    int createTsPlayerIfNeeded();
    DVR_Result_t eventHandlerLibDVR(DVR_PlaybackEvent_t event, void* params);
    DVR_Result_t eventHandlerPlayer(am_tsplayer_event* event);
#ifdef ANDROID
    android::sp<ANativeWindow> mNativeWindow = nullptr;
    android::sp<NativeHandle> mSidebandHandle;
#endif

private:
    AmlDVRPlayer(const AmlDVRPlayer&) = delete;
    AmlDVRPlayer& operator=(const AmlDVRPlayer&) = delete;
};

}
#endif
