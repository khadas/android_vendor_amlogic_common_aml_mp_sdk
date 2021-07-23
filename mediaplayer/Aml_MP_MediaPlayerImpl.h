/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _AML_MP_MEDIAPLAYER_IMPL_H_
#define _AML_MP_MEDIAPLAYER_IMPL_H_

#include <Aml_MP/Aml_MediaPlayer.h>
#include <utils/AmlMpRefBase.h>
#include <utils/AmlMpHandle.h>
#include <utils/AmlMpPlayerRoster.h>
#ifdef ANDROID
#include <system/window.h>
#endif
#include <mutex>
#include <map>
#include "utils/AmlMpChunkFifo.h"
#include <condition_variable>
#include "cas/AmlCasBase.h"
#include "demux/AmlTsParser.h"
#ifdef ANDROID
#ifndef __ANDROID_VNDK__
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#endif
#endif

#ifdef ANDROID
#include <utils/RefBase.h>
#endif
#include "AmlMediaPlayerBase.h"


namespace aml_mp {
#ifdef ANDROID
using android::RefBase;
#endif
class AmlMediaPlayerBase;
class AmlMpConfig;

class AmlMpMediaPlayerImpl final : public AmlMpHandle
{
public:
    explicit AmlMpMediaPlayerImpl();
    ~AmlMpMediaPlayerImpl();
    int setDataSource(const char *uri);
    int setANativeWindow(ANativeWindow* nativeWindow);
    int setVideoWindow(int32_t x, int32_t y, int32_t width, int32_t height);
    int prepare();
    int prepareAsync();
    int start();
    int seekTo(int msec);
    int pause();
    int resume();
    int stop();
    int reset();
    int release();
    int getCurrentPosition(int* msec);
    int getDuration(int* msec);
    int setVolume(float volume);
    int getVolume(float* volume);
    int showVideo();
    int hideVideo();
    int showSubtitle();
    int hideSubtitle();
    int setSubtitleWindow(int x, int y, int width, int height);
    int invoke(Aml_MP_MediaPlayerInvokeRequest* request, Aml_MP_MediaPlayerInvokeReply* reply);
    int setAVSyncSource(Aml_MP_AVSyncSource syncSource);
    int setParameter(Aml_MP_MediaPlayerParameterKey key, void* parameter);
    int getParameter(Aml_MP_MediaPlayerParameterKey key, void* parameter);
    bool isPlaying();
    int registerEventCallBack(Aml_MP_MediaPlayerEventCallback cb, void* userData);
    int setPlaybackRate(float rate);

private:
    enum State {
        STATE_IDLE,
        STATE_INITED,
        STATE_PREPARING,
        STATE_PREPARED,
        STATE_RUNNING,
        STATE_PAUSED,
        STATE_STOPPED,
        STATE_COMPLETED,
    };

    struct WindowSize {
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
    };

private:
    int setDataSource_l(const char * uri);
    int prepare_l();
    int prepareAsync_l();
    int start_l();
    int seekTo_l(int msec);
    int pause_l();
    int resume_l();
    int stop_l();
    int reset_l();
    int release_l();;
    int getCurrentPosition_l(int* msec);
    int getDuration_l(int* msec);
    int setVolume_l(float volume);
    int getVolume_l(float* volume);
    int showVideo_l();
    int hideVideo_l();
    int showSubtitle_l();
    int hideSubtitle_l();
    int setSubtitleWindow_l(int x, int y, int width, int height);
    int invoke_l(Aml_MP_MediaPlayerInvokeRequest* request, Aml_MP_MediaPlayerInvokeReply* reply);
    int setAVSyncSource_l(Aml_MP_AVSyncSource syncSource);
    int setParameter_l(Aml_MP_MediaPlayerParameterKey key, void* parameter);
    int getParameter_l(Aml_MP_MediaPlayerParameterKey key, void* parameter);
    bool isPlaying_l();
    void notifyListener(Aml_MP_MediaPlayerEventType eventType, int64_t param);

    const char* stateString(State state);
    void setState_l(State state);
private:
    const int mInstanceId;
    char mName[50];

    AmlMediaPlayerBase* mPlayer;

    //playbcak
    float mPlaybackRate = 1.0f;
    Aml_MP_VideoDecodeMode mVideoDecodeMode{AML_MP_VIDEO_DECODE_MODE_NONE};
    float mVolume = 1.0f;

    mutable std::mutex mLock;
    State mState{STATE_IDLE};

    //event
    std::mutex mEventLock;
    Aml_MP_MediaPlayerEventCallback mEventCb = nullptr;
    void* mUserData = nullptr;

    //surface
#ifdef ANDROID
    android::sp<ANativeWindow> mNativeWindow;
    int mZorder;
#ifndef __ANDROID_VNDK__
    android::sp<android::SurfaceComposerClient> mComposerClient;
    android::sp<android::SurfaceControl> mSurfaceControl;
    android::sp<android::Surface> mSurface = nullptr;
#endif
#endif
    WindowSize mVideoWindow;

    //subtitle
    WindowSize mSubtitleWindow;

    //syncSource
    Aml_MP_AVSyncSource mSyncSource = AML_MP_AVSYNC_SOURCE_DEFAULT;

private:
    AmlMpMediaPlayerImpl(const AmlMpMediaPlayerImpl&) = delete;
    AmlMpMediaPlayerImpl& operator= (const AmlMpMediaPlayerImpl&) = delete;
};

}

#endif
