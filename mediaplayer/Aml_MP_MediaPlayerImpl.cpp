/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_NDEBUG 0
#define LOG_TAG "AmlMpMediaPlayerImpl"
#include "Aml_MP_MediaPlayerImpl.h"
#include <player/Aml_MP_PlayerImpl.h>
#include <utils/AmlMpLog.h>
#include <utils/AmlMpUtils.h>
#include <utils/AmlMpConfig.h>
#include <utils/AmlMpBuffer.h>
#include <sstream>
#include <mutex>
#include <condition_variable>

#ifdef ANDROID
#include <media/stagefright/foundation/ADebug.h>
#ifndef __ANDROID_VNDK__
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#endif
#endif
#include <utils/AmlMpEventLooper.h>

namespace aml_mp {
#define FAST_PLAY_THRESHOLD     2.0f

/////////////////////////////////////////////////////////////////////////////////
AmlMpMediaPlayerImpl::AmlMpMediaPlayerImpl()
: mInstanceId(0)
{
    //default use = 0
    snprintf(mName, sizeof(mName), "%s_%d", LOG_TAG, mInstanceId);
    MLOGI("mName:%s", mName);
    setState_l(STATE_IDLE);

    mPlayer = AmlMediaPlayerBase::create(NULL, mInstanceId);
}

AmlMpMediaPlayerImpl::~AmlMpMediaPlayerImpl()
{
    MLOG();

    if (mPlayer != nullptr) {
        delete mPlayer;
        mPlayer = nullptr;
    }
}

int AmlMpMediaPlayerImpl::setDataSource(const char *uri)
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);
    MLOG();

    return setDataSource_l(uri);
}

int AmlMpMediaPlayerImpl::setDataSource_l(const char *uri)
{
    int ret = 0;
    RETURN_IF(-1, mPlayer == nullptr);

    if (STATE_IDLE != mState) {
        MLOGE("%s, mState failed! mState:%s", __FUNCTION__, stateString(mState));
        return -1;
    }

    ret = mPlayer->setDataSource(uri);
    if (ret < 0) {
        return ret;
    }

    setState_l(STATE_INITED);

    return ret;
}

int AmlMpMediaPlayerImpl::setANativeWindow(ANativeWindow* nativeWindow)
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);

#ifdef ANDROID
    mNativeWindow = nativeWindow;
    MLOGI("setAnativeWindow: %p, mNativewindow: %p", nativeWindow, mNativeWindow.get());

    if (mState == STATE_RUNNING || mState == STATE_PAUSED) {
        RETURN_IF(-1, mPlayer == nullptr);
        if (mNativeWindow != nullptr) {
            mPlayer->setANativeWindow(mNativeWindow.get());
        }
    }
#endif
    return 0;
}

int AmlMpMediaPlayerImpl::setVideoWindow(int32_t x, int32_t y, int32_t width, int32_t height)
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);

    if (width < 0 || height < 0) {
        MLOGI("Invalid windowsize: %dx%d, return fail", width, height);
        return -1;
    }
#ifdef ANDROID
#ifndef __ANDROID_VNDK__
    if (mNativeWindow == nullptr) {
        MLOGI("Nativewindow is null, create it");
        mComposerClient = new android::SurfaceComposerClient;
        mComposerClient->initCheck();

        mSurfaceControl = mComposerClient->createSurface(android::String8("AmlMpPlayer"), width, height, android::PIXEL_FORMAT_RGB_565, 0);
        if (mSurfaceControl->isValid()) {
            mSurface = mSurfaceControl->getSurface();
            android::SurfaceComposerClient::Transaction()
                .setFlags(mSurfaceControl, android::layer_state_t::eLayerOpaque, android::layer_state_t::eLayerOpaque)
                .show(mSurfaceControl)
                .apply();
        }
        mNativeWindow = mSurface;
    }

    if (mSurfaceControl != nullptr) {
        MLOGI("Set video window size: x %d, y %d, width: %d, height: %d", x, y, width, height);
        auto transcation = android::SurfaceComposerClient::Transaction();
        if (x >= 0 && y >= 0) {
            transcation.setPosition(mSurfaceControl, x, y);
        }

        transcation.setSize(mSurfaceControl, width, height);
        transcation.setCrop_legacy(mSurfaceControl, android::Rect(width, height));
        transcation.setLayer(mSurfaceControl, mZorder);

        transcation.apply();
    }
#endif
#endif
    mVideoWindow = {x, y, width, height};
    return 0;
}

int AmlMpMediaPlayerImpl::prepare()
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);
    MLOG();

    return prepare_l();
}

int AmlMpMediaPlayerImpl::prepare_l()
{
    int ret = 0;
    RETURN_IF(-1, mPlayer == nullptr);

    if (STATE_INITED != mState && STATE_STOPPED != mState) {
        MLOGE("%s, mState failed! mState:%s", __FUNCTION__, stateString(mState));
        return -1;
    }

    mPlayer->registerEventCallBack([](void* userData, Aml_MP_MediaPlayerEventType event, int64_t param) {
        AmlMpMediaPlayerImpl* thiz = static_cast<AmlMpMediaPlayerImpl*>(userData);
        return thiz->notifyListener(event, param);
    }, this);

    setState_l(STATE_PREPARING);
    ret = mPlayer->prepare();
    if (ret < 0) {
        MLOGE("%s failed!", __FUNCTION__);
        return ret;
    }

    setState_l(STATE_PREPARED);

    return ret;
}

int AmlMpMediaPlayerImpl::prepareAsync()
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);
    MLOG();

    return prepareAsync_l();
}

int AmlMpMediaPlayerImpl::prepareAsync_l()
{
    int ret = 0;
    RETURN_IF(-1, mPlayer == nullptr);

    if (STATE_INITED != mState && STATE_STOPPED != mState) {
        MLOGE("%s, mState failed! mState:%s", __FUNCTION__, stateString(mState));
        return -1;
    }

    mPlayer->registerEventCallBack([](void* userData, Aml_MP_MediaPlayerEventType event, int64_t param) {
        AmlMpMediaPlayerImpl* thiz = static_cast<AmlMpMediaPlayerImpl*>(userData);
        return thiz->notifyListener(event, param);
    }, this);

    setState_l(STATE_PREPARING);
    ret = mPlayer->prepareAsync();
    if (ret < 0) {
        MLOGE("%s failed!", __FUNCTION__);
        return ret;
    }
    setState_l(STATE_PREPARED);//now set here

    return ret;
}

int AmlMpMediaPlayerImpl::start()
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);
    MLOG();

    return start_l();
}

int AmlMpMediaPlayerImpl::start_l()
{
    int ret = 0;
    RETURN_IF(-1, mPlayer == nullptr);

    //allow call start() when in STATE_RUNNING because after playing complete is still in STATE_RUNNING, but it`s in complete state at amldrmplayer
    //must call prepare() when in STATE_STOPPED
    if (mState == STATE_IDLE || mState == STATE_INITED || mState == STATE_PREPARING || mState == STATE_PAUSED || mState == STATE_STOPPED) {
        MLOGE("%s, mState failed! mState:%s", __FUNCTION__, stateString(mState));

        return -1;
    }

//    if (mState == STATE_INITED) {
//        if (prepare_l() < 0) {
//            MLOGE("prepare failed! mState:%s", stateString(mState));
//            return -1;
//        }
//    }

    //set windowsize
    MLOGI("windowsize: %dx%d\n", mVideoWindow.width, mVideoWindow.height);
    if (mVideoWindow.width > 0 && mVideoWindow.height > 0) {
        mPlayer->setVideoWindow(mVideoWindow.x, mVideoWindow.y, mVideoWindow.width, mVideoWindow.height);
    }

    //we allow call start() in STATE_PAUSED because it will call resume in actual
    ret = mPlayer->start();
    if (ret < 0) {
        MLOGE("%s failed!", __FUNCTION__);
        return ret;
    }

    setState_l(STATE_RUNNING);
    return ret;
}

int AmlMpMediaPlayerImpl::seekTo(int msec)
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);
    MLOG();

    return seekTo_l(msec);
}

int AmlMpMediaPlayerImpl::seekTo_l(int msec)
{
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->seekTo(msec, SEEK_CLOSEST_SYNC);//default seeking to closeset keyframes
}

int AmlMpMediaPlayerImpl::pause()
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);
    MLOG();

    return pause_l();
}

int AmlMpMediaPlayerImpl::pause_l()
{
    int ret = 0;
    RETURN_IF(-1, mPlayer == nullptr);

    if (STATE_RUNNING != mState) {
        MLOGE("%s, mState failed! mState:%s", __FUNCTION__, stateString(mState));
        return -1;
    }

    if (mPlayer->pause() < 0) {
        MLOGE("%s failed!", __FUNCTION__);
        return -1;
    }
    setState_l(STATE_PAUSED);

    return ret;
}

int AmlMpMediaPlayerImpl::resume()
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);
    MLOG();

    return resume_l();
}

int AmlMpMediaPlayerImpl::resume_l()
{
    int ret = 0;
    RETURN_IF(-1, mPlayer == nullptr);

    if (STATE_PAUSED != mState) {
        MLOGE("%s, mState failed! mState:%s", __FUNCTION__, stateString(mState));
        return -1;
    }

    if (mPlayer->resume() < 0) {
        MLOGE("%s failed!", __FUNCTION__);
        return -1;
    }

    setState_l(STATE_RUNNING);

    return ret;
}

int AmlMpMediaPlayerImpl::stop()
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);
    MLOG();

    return stop_l();
}

int AmlMpMediaPlayerImpl::stop_l()
{
    int ret = 0;
    RETURN_IF(-1, mPlayer == nullptr);

    if (STATE_STOPPED == mState || STATE_IDLE == mState || STATE_INITED == mState || STATE_PAUSED == mState) {
        MLOGE("%s, mState failed! mState:%s", __FUNCTION__, stateString(mState));
        return -1;
    }

    if (mPlayer->stop() < 0) {
        MLOGE("%s failed!", __FUNCTION__);
        return -1;
    }

    setState_l(STATE_STOPPED);

    return ret;
}

int AmlMpMediaPlayerImpl::reset()
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);
    MLOG();

    return reset_l();
}

int AmlMpMediaPlayerImpl::reset_l()
{
    int ret = 0;
    RETURN_IF(-1, mPlayer == nullptr);

    if (mState == STATE_IDLE) {
        MLOGE("%s, mState failed! mState:%s", __FUNCTION__, stateString(mState));
        return -1;
    }

    if (mPlayer->reset() < 0) {
        MLOGE("%s failed!", __FUNCTION__);
        return -1;
    }
    setState_l(STATE_STOPPED);

    if (mPlayer->release() < 0) {
        MLOGE("%s failed!", __FUNCTION__);
        return -1;
    }

    setState_l(STATE_IDLE);

    return ret;
}

int AmlMpMediaPlayerImpl::release()
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);
    MLOG();

    return release_l();
}

int AmlMpMediaPlayerImpl::release_l()
{
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->release();
}

int AmlMpMediaPlayerImpl::getCurrentPosition(int* msec)
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);
    MLOG();

    return getCurrentPosition_l(msec);
}

int AmlMpMediaPlayerImpl::getCurrentPosition_l(int* msec)
{
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->getCurrentPosition(msec);
}

int AmlMpMediaPlayerImpl::getDuration(int* msec)
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);
    MLOG();

    return getDuration_l(msec);
}

int AmlMpMediaPlayerImpl::getDuration_l(int* msec)
{
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->getDuration(msec);
}

int AmlMpMediaPlayerImpl::setVolume(float volume)
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);
    MLOG();

    return setVolume_l(volume);
}

int AmlMpMediaPlayerImpl::setVolume_l(float volume)
{
    int ret = 0;
    if (volume < 0) {
        MLOGI("volume is %f, set to 0.0", volume);
        volume = 0.0;
    }
    mVolume = volume;

    if (mState == STATE_RUNNING || mState == STATE_PAUSED) {
        RETURN_IF(-1, mPlayer == nullptr);
        ret = mPlayer->setVolume(volume);
    }

    return ret;
}

int AmlMpMediaPlayerImpl::getVolume(float* volume)
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);
    MLOG();

    return getVolume_l(volume);
}

int AmlMpMediaPlayerImpl::getVolume_l(float* volume)
{
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->getVolume(volume);
}

int AmlMpMediaPlayerImpl::showVideo()
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);
    MLOG();

    return showVideo_l();
}

int AmlMpMediaPlayerImpl::showVideo_l()
{
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->showVideo();
}

int AmlMpMediaPlayerImpl::hideVideo()
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);
    MLOG();

    return hideVideo_l();
}

int AmlMpMediaPlayerImpl::hideVideo_l()
{
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->hideVideo();
}

int AmlMpMediaPlayerImpl::showSubtitle()
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);
    MLOG();

    return showSubtitle_l();
}

int AmlMpMediaPlayerImpl::showSubtitle_l()
{
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->showSubtitle();
}

int AmlMpMediaPlayerImpl::hideSubtitle()
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);
    MLOG();

    return hideSubtitle_l();
}

int AmlMpMediaPlayerImpl::hideSubtitle_l()
{
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->hideSubtitle();
}

int AmlMpMediaPlayerImpl::setSubtitleWindow(int x, int y, int width, int height)
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);
    MLOG();

    return setSubtitleWindow_l(x, y, width, height);
}

int AmlMpMediaPlayerImpl::setSubtitleWindow_l(int x, int y, int width, int height)
{
    int ret = 0;
    mSubtitleWindow = {x, y, width, height};

    if (mState == STATE_RUNNING || mState == STATE_PAUSED) {
        RETURN_IF(-1, mPlayer == nullptr);
        ret = mPlayer->setSubtitleWindow(x, y, width, height);
    }

    return ret;
}

int AmlMpMediaPlayerImpl::invoke(Aml_MP_MediaPlayerInvokeRequest* request, Aml_MP_MediaPlayerInvokeReply* reply)
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);
    MLOG();

    return invoke_l(request, reply);
}

int AmlMpMediaPlayerImpl::invoke_l(Aml_MP_MediaPlayerInvokeRequest* request, Aml_MP_MediaPlayerInvokeReply* reply)
{
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->invoke(request, reply);
}

int AmlMpMediaPlayerImpl::setAVSyncSource(Aml_MP_AVSyncSource syncSource)
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);
    MLOG();

    return setAVSyncSource_l(syncSource);
}

int AmlMpMediaPlayerImpl::setAVSyncSource_l(Aml_MP_AVSyncSource syncSource)
{
    int ret = 0;
    mSyncSource = syncSource;

    return ret;
}

int AmlMpMediaPlayerImpl::setParameter(Aml_MP_MediaPlayerParameterKey key, void* parameter)
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);
    MLOG();

    return setParameter_l(key, parameter);
}

int AmlMpMediaPlayerImpl::setParameter_l(Aml_MP_MediaPlayerParameterKey key, void* parameter)
{
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->setParameter(key, parameter);
}

int AmlMpMediaPlayerImpl::getParameter(Aml_MP_MediaPlayerParameterKey key, void* parameter)
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);
    MLOG();

    return getParameter_l(key, parameter);
}

int AmlMpMediaPlayerImpl::getParameter_l(Aml_MP_MediaPlayerParameterKey key, void* parameter)
{
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->getParameter(key, parameter);
}

bool AmlMpMediaPlayerImpl::isPlaying()
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);
    MLOG();

    return isPlaying_l();
}

bool AmlMpMediaPlayerImpl::isPlaying_l()
{
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->isPlaying();
}

int AmlMpMediaPlayerImpl::registerEventCallBack(Aml_MP_MediaPlayerEventCallback cb, void* userData)
{
    MLOG();
    AML_MP_TRACE(10);

    {
        std::unique_lock<std::mutex> _l(mLock);
        //registerEventCallBack before prepare
        if (mState > STATE_PREPARING) {
            MLOGW("can't registerEventCallBack now!");
            return -1;
        }
    }

    std::unique_lock<std::mutex> _eventLock(mEventLock);
    mEventCb = cb;
    mUserData = userData;

    return 0;
}

void AmlMpMediaPlayerImpl::notifyListener(Aml_MP_MediaPlayerEventType eventType, int64_t param)
{
    std::unique_lock<std::mutex> _l(mEventLock);
    if (mEventCb) {
        if (eventType == AML_MP_MEDIAPLAYER_EVENT_PLAYBACK_COMPLETE) {
            setState_l(STATE_COMPLETED);
        }
        mEventCb(mUserData, eventType, param);
    } else {
        MLOGW("mEventCb is NULL, eventType: %s, param:%lld", mpPlayerEventType2Str(eventType), param);
    }
}

int AmlMpMediaPlayerImpl::setPlaybackRate(float rate)
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> lock(mLock);
    MLOG("rate:%f", rate);

    int ret = 0;
    Aml_MP_VideoDecodeMode newDecodeMode;
    bool decodeModeChanged = false;

    if (rate < 0.0f) {
        if (rate <= -FAST_PLAY_THRESHOLD) {
            mPlaybackRate = -rate;
        } else {
            return AML_MP_ERROR_BAD_VALUE;
        }
    } else {
        mPlaybackRate = rate;
    }

    newDecodeMode = (mPlaybackRate >= FAST_PLAY_THRESHOLD)?AML_MP_VIDEO_DECODE_MODE_IONLY:AML_MP_VIDEO_DECODE_MODE_NONE;
    decodeModeChanged = (newDecodeMode != mVideoDecodeMode);

    if (mState == STATE_RUNNING || mState == STATE_PAUSED) {
        RETURN_IF(-1, mPlayer == nullptr);

        if (mPlaybackRate == 0.0f) {
            ret = pause_l();
            return ret;
        }

        if (mState == STATE_PAUSED) {
            resume_l();
        }

        if (decodeModeChanged) {
            //switchDecodeMode_l(newDecodeMode, lock);//TODO!!!
        }

        if (mVideoDecodeMode == AML_MP_VIDEO_DECODE_MODE_NONE) {
            ret = mPlayer->setPlaybackRate(rate);
        }
    }

    return ret;
}

///////////////////////////////////////////////////////////////
const char* AmlMpMediaPlayerImpl::stateString(State state)
{
    switch (state) {
    case STATE_IDLE:
        return "STATE_IDLE";
    case STATE_INITED:
        return "STATE_INITED";
    case STATE_PREPARING:
        return "STATE_PREPARING";
    case STATE_PREPARED:
        return "STATE_PREPARED";
    case STATE_RUNNING:
        return "STATE_RUNNING";
    case STATE_PAUSED:
        return "STATE_PAUSED";
    case STATE_STOPPED:
        return "STATE_STOPPED";
    case STATE_COMPLETED:
        return "STATE_COMPLETED";
    default:
        return "unknow";
    }
}

void AmlMpMediaPlayerImpl::setState_l(State state)
{
    if (mState != state) {
        MLOGI("%s -> %s", stateString(mState), stateString(state));
        mState = state;
    }
}

}
