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

#define LOG_NDEBUG 0
#define LOG_TAG "AmlMpPlayerImpl"
#include "Aml_MP_PlayerImpl.h"
#include <utils/AmlMpLog.h>
#include <utils/AmlMpUtils.h>
#include <utils/AmlMpConfig.h>
#include <mutex>
#include <condition_variable>
#include <media/stagefright/foundation/ADebug.h>
#include "AmlPlayerBase.h"

namespace aml_mp {

struct AmlMpPlayerRoster final : public RefBase
{
    static constexpr int kPlayerInstanceMax = 9;

    static AmlMpPlayerRoster& instance();
    int registerPlayer(void* player);
    void unregisterPlayer(int id);

private:
    static AmlMpPlayerRoster* sAmlPlayerRoster;
    std::mutex mLock;
    std::condition_variable mcond;
    void* mPlayers[kPlayerInstanceMax];
    int mPlayerNum = 0;

    AmlMpPlayerRoster();
    ~AmlMpPlayerRoster();

    AmlMpPlayerRoster(const AmlMpPlayerRoster&) = delete;
    AmlMpPlayerRoster& operator= (const AmlMpPlayerRoster&) = delete;
};

///////////////////////////////////////////////////////////////////////////////
AmlMpPlayerImpl::AmlMpPlayerImpl(const Aml_MP_PlayerCreateParams* createParams)
: mInstanceId(AmlMpPlayerRoster::instance().registerPlayer(this))
, mCreateParams(*createParams)
{
    snprintf(mName, sizeof(mName), "%s_%d", LOG_TAG, mInstanceId);

    MLOG();

    AmlMpConfig::instance().init();
}

AmlMpPlayerImpl::~AmlMpPlayerImpl()
{
    MLOG();

    AmlMpPlayerRoster::instance().unregisterPlayer(mInstanceId);
}

int AmlMpPlayerImpl::registerEventCallback(Aml_MP_PlayerEventCallback cb, void* userData)
{
    AML_MP_TRACE(10);

    mEventCb = cb;
    mUserData = userData;

    return 0;
}

int AmlMpPlayerImpl::setVideoParams(const Aml_MP_VideoParams* params)
{
    AML_MP_TRACE(10);

    mVideoParams.pid = params->pid;
    mVideoParams.videoCodec = params->videoCodec;
    mVideoParams.width = params->width;
    mVideoParams.height = params->height;
    mVideoParams.frameRate = params->frameRate;
    mVideoParams.extraDataSize = params->extraDataSize;
    mVideoParams.extraData =params->extraData;

    return 0;
}

int AmlMpPlayerImpl::setAudioParams(const Aml_MP_AudioParams* params)
{
    AML_MP_TRACE(10);

    mAudioParams.pid = params->pid;
    mAudioParams.audioCodec = params->audioCodec;
    mAudioParams.nChannels = params->nChannels;
    mAudioParams.nSampleRate = params->nSampleRate;
    mAudioParams.nExtraSize = params->nExtraSize;
    mAudioParams.extraData = params->extraData;

    return 0;
}

int AmlMpPlayerImpl::setSubtitleParams(const Aml_MP_SubtitleParams* params)
{
    AML_MP_TRACE(10);

    memcpy(&mSubtitleParams, params, sizeof(Aml_MP_SubtitleParams));

    return 0;
}

int AmlMpPlayerImpl::setCASParams(const Aml_MP_CASParams* params)
{
    AML_MP_TRACE(10);

    mCASParams = *params;

    return 0;
}

int AmlMpPlayerImpl::start()
{
    AML_MP_TRACE(10);

    if (mCreateParams.drmMode == AML_MP_INPUT_STREAM_ENCRYPTED) {
        startDescrambling();
    }

    if (mPlayer == nullptr) {
        mPlayer = AmlPlayerBase::create(&mCreateParams, mInstanceId);
    }

    if (mPlayer == nullptr) {
        ALOGE("AmlPlayerBase create failed!");
        return -1;
    }

    mPlayer->registerEventCallback(mEventCb, mUserData);

    if (mVideoParams.pid != AML_MP_INVALID_PID) {
        mPlayer->setVideoParams(&mVideoParams);
    }

    if (mAudioParams.pid != AML_MP_INVALID_PID) {
        mPlayer->setAudioParams(&mAudioParams);
    }

    if (mSubtitleParams.subtitleCodec != AML_MP_CODEC_UNKNOWN) {
        mPlayer->setSubtitleParams(&mSubtitleParams);
    }

    if ((mSubtitleWindow.width > 0) && (mSubtitleWindow.height > 0)) {
        mPlayer->setSubtitleWindow(mSubtitleWindow.x, mSubtitleWindow.y, mSubtitleWindow.width, mSubtitleWindow.height);
    }

    ALOGI("mNativeWindow:%p", mNativeWindow.get());
    if (mNativeWindow != nullptr) {
        mPlayer->setANativeWindow(mNativeWindow.get());
    } else {
        mPlayer->setVideoWindow(mVideoWindow.x, mVideoWindow.y, mVideoWindow.width, mVideoWindow.height);
    }

    if (mSyncSource == AML_MP_AVSYNC_SOURCE_DEFAULT) {
        mSyncSource = AML_MP_AVSYNC_SOURCE_PCR;
    }

    mPlayer->setAVSyncSource(mSyncSource);

    int ret = mPlayer->start();
    if (ret < 0) {
        ALOGE("start failed!");
    }

    return ret;
}

int AmlMpPlayerImpl::stop()
{
    AML_MP_TRACE(10);
    RETURN_IF(-1, mPlayer == nullptr);

    if (mCasHandle) {
        stopDescrambling();
    }

    mPlayer->stop();
    mPlayer.clear();

    return 0;
}

int AmlMpPlayerImpl::pause()
{
    AML_MP_TRACE(10);
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->pause();
}

int AmlMpPlayerImpl::resume()
{
    AML_MP_TRACE(10);
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->resume();
}

int AmlMpPlayerImpl::flush()
{
    AML_MP_TRACE(10);
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->flush();
}

int AmlMpPlayerImpl::setPlaybackRate(float rate)
{
    AML_MP_TRACE(10);
    mPlaybackRate = rate;
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->setPlaybackRate(rate);
}

int AmlMpPlayerImpl::switchAudioTrack(const Aml_MP_AudioParams* params)
{
    AML_MP_TRACE(10);
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->switchAudioTrack(params);
}

int AmlMpPlayerImpl::switchSubtitleTrack(const Aml_MP_SubtitleParams* params)
{
    AML_MP_TRACE(10);
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->switchSubtitleTrack(params);
}

int AmlMpPlayerImpl::writeData(const uint8_t* buffer, size_t size)
{
    RETURN_IF(-1, mPlayer == nullptr);

    if (mCasHandle != nullptr) {
        mCasHandle->processEcm(buffer, size);
    }

    return mPlayer->writeData(buffer, size);
}

int AmlMpPlayerImpl::writeEsData(Aml_MP_StreamType type, const uint8_t* buffer, size_t size, int64_t pts)
{
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->writeEsData(type, buffer, size, pts);
}

int AmlMpPlayerImpl::getCurrentPts(Aml_MP_StreamType streamType, int64_t* pts)
{
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->getCurrentPts(streamType, pts);
}

int AmlMpPlayerImpl::getBufferStat(Aml_MP_BufferStat* bufferStat)
{
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->getBufferStat(bufferStat);
}

int AmlMpPlayerImpl::setAnativeWindow(void* nativeWindow)
{
    AML_MP_TRACE(10);
    mNativeWindow = static_cast<ANativeWindow*>(nativeWindow);

    return 0;
}

int AmlMpPlayerImpl::setVideoWindow(int x, int y, int width, int height)
{
    AML_MP_TRACE(10);
    mVideoWindow = {x, y, width, height};
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->setVideoWindow(x, y, width, height);
}

int AmlMpPlayerImpl::setVolume(float volume)
{
    AML_MP_TRACE(10);
    mVolume = volume;
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->setVolume(volume);
}

int AmlMpPlayerImpl::getVolume(float* volume)
{
    AML_MP_TRACE(10);
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->getVolume(volume);
}

int AmlMpPlayerImpl::showVideo()
{
    AML_MP_TRACE(10);
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->showVideo();
}

int AmlMpPlayerImpl::hideVideo()
{
    AML_MP_TRACE(10);
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->hideVideo();
}

int AmlMpPlayerImpl::showSubtitle()
{
    AML_MP_TRACE(10);
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->showSubtitle();
}

int AmlMpPlayerImpl::hideSubtitle()
{
    AML_MP_TRACE(10);
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->hideSubtitle();
}

int AmlMpPlayerImpl::setParameter(Aml_MP_PlayerParameterKey key, void* parameter)
{
    AML_MP_TRACE(10);
    int ret = 0;

    switch (key) {
    case AML_MP_PLAYER_PARAMETER_AUDIO_BALANCE:
    {
        mAudioBalance = *(Aml_MP_AudioBalance*)parameter;
    }
    break;


    default:
        break;
    }

    if (mPlayer != nullptr) {
        ret = mPlayer->setParameter(key, parameter);
    }

    return ret;
}

int AmlMpPlayerImpl::getParameter(Aml_MP_PlayerParameterKey key, void* parameter)
{
    AML_MP_TRACE(10);
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->getParameter(key, parameter);
}

int AmlMpPlayerImpl::setAVSyncSource(Aml_MP_AVSyncSource syncSource)
{
    AML_MP_TRACE(10);
    mSyncSource = syncSource;

    return 0;
}

int AmlMpPlayerImpl::setPcrPid(int pid)
{
    AML_MP_TRACE(10);
    mPcrPid = pid;

    return 0;
}

int AmlMpPlayerImpl::startVideoDecoding()
{
    AML_MP_TRACE(10);
    MLOG();

    if (mPlayer == nullptr) {
        mPlayer = AmlPlayerBase::create(&mCreateParams, mInstanceId);
    }

    if (mPlayer == nullptr) {
        ALOGE("AmlPlayerBase create failed!");
        return -1;
    }

    mPlayer->registerEventCallback(mEventCb, mUserData);

    ALOGI("mVideoParams: pid:%d, fmt:%d", mVideoParams.pid, mVideoParams.videoCodec);
    if (mVideoParams.pid != AML_MP_INVALID_PID) {
        mPlayer->setVideoParams(&mVideoParams);
    }

    if (mSyncSource == AML_MP_AVSYNC_SOURCE_DEFAULT) {
        mSyncSource = AML_MP_AVSYNC_SOURCE_PCR;
    }

    mPlayer->setAVSyncSource(mSyncSource);

    return mPlayer->startVideoDecoding();
}


int AmlMpPlayerImpl::stopVideoDecoding()
{
    AML_MP_TRACE(10);
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->stopVideoDecoding();
}

int AmlMpPlayerImpl::startAudioDecoding()
{
    AML_MP_TRACE(10);

    if (mPlayer == nullptr) {
        mPlayer = AmlPlayerBase::create(&mCreateParams, mInstanceId);
    }

    if (mPlayer == nullptr) {
        ALOGE("AmlPlayerBase create failed!");
        return -1;
    }

    mPlayer->registerEventCallback(mEventCb, mUserData);

    ALOGI("mAudioParams.pid:%d", mAudioParams.pid);
    if (mAudioParams.pid != AML_MP_INVALID_PID) {
        mPlayer->setAudioParams(&mAudioParams);
    }

    if (mSyncSource == AML_MP_AVSYNC_SOURCE_DEFAULT) {
        mSyncSource = AML_MP_AVSYNC_SOURCE_PCR;
    }

    mPlayer->setAVSyncSource(mSyncSource);

    return mPlayer->startAudioDecoding();
}

int AmlMpPlayerImpl::stopAudioDecoding()
{
    AML_MP_TRACE(10);
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->stopAudioDecoding();
}

int AmlMpPlayerImpl::startSubtitleDecoding()
{
    AML_MP_TRACE(10);
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->startSubtitleDecoding();
}

int AmlMpPlayerImpl::stopSubtitleDecoding()
{
    AML_MP_TRACE(10);
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->stopSubtitleDecoding();
}

int AmlMpPlayerImpl::setSubtitleWindow(int x, int y, int width, int height)
{
    AML_MP_TRACE(10);
    mSubtitleWindow = {x, y, width, height};
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->setSubtitleWindow(x, y, width, height);
}

int AmlMpPlayerImpl::startDescrambling()
{
    AML_MP_TRACE(10);

    ALOGI("encrypted stream!");
    if (mCASParams.type == AML_MP_CAS_UNKNOWN) {
        ALOGE("cas params is invalid!");
        return -1;
    }

    mCasHandle = AmlCasBase::create(mCreateParams.sourceType, &mCASParams);
    if (mCasHandle == nullptr) {
        ALOGE("create CAS handle failed!");
        return -1;
    }

    mCasHandle->openSession();

    return 0;
}

int AmlMpPlayerImpl::stopDescrambling()
{
    AML_MP_TRACE(10);

    if (mCasHandle) {
        mCasHandle->closeSession();
        mCasHandle.clear();
    }

    return 0;
}

int AmlMpPlayerImpl::setADParams(Aml_MP_AudioParams* params)
{
    AML_MP_TRACE(10);
    RETURN_IF(-1, mPlayer == nullptr);

    mADParams.pid = params->pid;
    mADParams.audioCodec = params->audioCodec;
    mADParams.nChannels = params->nChannels;
    mADParams.nSampleRate = params->nSampleRate;
    mADParams.nExtraSize = params->nExtraSize;
    mADParams.extraData = params->extraData;

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
AmlMpPlayerRoster* AmlMpPlayerRoster::sAmlPlayerRoster = nullptr;

AmlMpPlayerRoster::AmlMpPlayerRoster()
: mPlayers{nullptr}
{

}

AmlMpPlayerRoster::~AmlMpPlayerRoster()
{

}

AmlMpPlayerRoster& AmlMpPlayerRoster::instance()
{
    static std::once_flag s_onceFlag;

    if (sAmlPlayerRoster == nullptr) {
        std::call_once(s_onceFlag, [] {
            sAmlPlayerRoster = new AmlMpPlayerRoster();
        });
    }

    return *sAmlPlayerRoster;
}

int AmlMpPlayerRoster::registerPlayer(void* player)
{
    if (player == nullptr) {
        return -1;
    }

    std::lock_guard<std::mutex> _l(mLock);
    for (size_t i = 0; i < kPlayerInstanceMax; ++i) {
        if (mPlayers[i] == nullptr) {
            mPlayers[i] = player;
            (void)++mPlayerNum;

            //ALOGI("register player id:%d(%p)", i, player);
            return i;
        }
    }

    return -1;
}

void AmlMpPlayerRoster::unregisterPlayer(int id)
{
    std::lock_guard<std::mutex> _l(mLock);

    CHECK(mPlayers[id]);
    mPlayers[id] = nullptr;
    (void)--mPlayerNum;
}



}
