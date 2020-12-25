/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _AML_MP_PLAYER_IMPL_H_
#define _AML_MP_PLAYER_IMPL_H_

#include <Aml_MP/Aml_MP.h>
#include <utils/RefBase.h>
#include <utils/AmlMpHandle.h>
#include <system/window.h>
#include <mutex>
#include <condition_variable>
#include "cas/AmlCasBase.h"
#ifndef __ANDROID_VNDK__
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#endif

namespace aml_mp {
using android::RefBase;
class AmlPlayerBase;
class AmlMpConfig;

struct AmlMpPlayerRoster final : public RefBase
{
    static constexpr int kPlayerInstanceMax = 9;

    static AmlMpPlayerRoster& instance();
    int registerPlayer(void* player);
    void unregisterPlayer(int id);
    void signalAmTsPlayerId(int id);
    bool isAmTsPlayerExist() const;

private:
    static AmlMpPlayerRoster* sAmlPlayerRoster;
    mutable std::mutex mLock;
    std::condition_variable mcond;
    void* mPlayers[kPlayerInstanceMax];
    int mPlayerNum = 0;
    int mAmtsPlayerId = -1;

    AmlMpPlayerRoster();
    ~AmlMpPlayerRoster();

    AmlMpPlayerRoster(const AmlMpPlayerRoster&) = delete;
    AmlMpPlayerRoster& operator= (const AmlMpPlayerRoster&) = delete;
};

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
    enum State {
        STATE_IDLE,
        STATE_PREPARED,
        STATE_RUNNING, //audio,video,subtitle at least one is running
        STATE_PAUSED, //all paused
    };

    enum StreamState {
        ALL_STREAMS_STOPPED     = 0,
        AUDIO_STARTED           = 1 << 0,
        VIDEO_STARTED           = 1 << 1,
        SUBTITLE_STARTED        = 1 << 2,
    };

    struct WindowSize {
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
    };

    const char* stateString(State state);
    std::string streamStateString(int streamState);
    void setState(State state);
    int prepare();
    void setParams();
    int resetIfNeeded();
    int reset();

    const int mInstanceId;
    char mName[50];
    State mState{STATE_IDLE};
    int mStreamState{ALL_STREAMS_STOPPED};

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

    Aml_MP_PlayerWorkMode mWorkmode;

    int mZorder = -2;

#ifndef __ANDROID_VNDK__
    sp<android::SurfaceComposerClient> mComposerClient;
    sp<android::SurfaceControl> mSurfaceControl;
    sp<android::Surface> mSurface;
#endif

private:
    AmlMpPlayerImpl(const AmlMpPlayerImpl&) = delete;
    AmlMpPlayerImpl& operator= (const AmlMpPlayerImpl&) = delete;
};

}


#endif
