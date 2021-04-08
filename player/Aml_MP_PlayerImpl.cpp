/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_NDEBUG 0
#define LOG_TAG "AmlMpPlayerImpl"
#include "Aml_MP_PlayerImpl.h"
#include <utils/AmlMpLog.h>
#include <utils/AmlMpUtils.h>
#include <utils/AmlMpConfig.h>
#include <sstream>
#include <mutex>
#include <condition_variable>
#include <media/stagefright/foundation/ADebug.h>
#include "AmlPlayerBase.h"
#ifndef __ANDROID_VNDK__
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#endif

namespace aml_mp {

///////////////////////////////////////////////////////////////////////////////
AmlMpPlayerImpl::AmlMpPlayerImpl(const Aml_MP_PlayerCreateParams* createParams)
: mInstanceId(AmlMpPlayerRoster::instance().registerPlayer(this))
, mCreateParams(*createParams)
, mTsBuffer(TS_BUFFER_SIZE)
{
    snprintf(mName, sizeof(mName), "%s_%d", LOG_TAG, mInstanceId);

    MLOG();

    memset(&mVideoParams, 0, sizeof(mVideoParams));
    mVideoParams.pid = AML_MP_INVALID_PID;
    mVideoParams.videoCodec = AML_MP_CODEC_UNKNOWN;
    memset(&mAudioParams, 0, sizeof(mAudioParams));
    mAudioParams.pid = AML_MP_INVALID_PID;
    mAudioParams.audioCodec = AML_MP_CODEC_UNKNOWN;
    memset(&mSubtitleParams, 0, sizeof(mSubtitleParams));
    mSubtitleParams.pid = AML_MP_INVALID_PID;
    mSubtitleParams.subtitleCodec = AML_MP_CODEC_UNKNOWN;
    memset(&mCASParams, 0, sizeof(mCASParams));
    memset(&mADParams, 0, sizeof(mADParams));
    mADParams.pid = AML_MP_INVALID_PID;

    mWorkMode = AML_MP_PLAYER_MODE_NORMAL;
    mAudioBalance = AML_MP_AUDIO_BALANCE_STEREO;

    AmlMpConfig::instance().init();

    mPlayer = AmlPlayerBase::create(&mCreateParams, mInstanceId);
    mParser = new Parser(mCreateParams.demuxId, mCreateParams.sourceType == AML_MP_INPUT_SOURCE_TS_DEMOD, true);
    mTempBuffer = new uint8_t[TEMP_BUFFER_SIZE];
}

AmlMpPlayerImpl::~AmlMpPlayerImpl()
{
    MLOG();

    if (mTempBuffer) {
        delete[] mTempBuffer;
    }
    CHECK(mState == STATE_IDLE);
    CHECK(mStreamState == ALL_STREAMS_STOPPED);

    AmlMpPlayerRoster::instance().unregisterPlayer(mInstanceId);
}

int AmlMpPlayerImpl::registerEventCallback(Aml_MP_PlayerEventCallback cb, void* userData)
{
    AML_MP_TRACE(10);

    if (mState != STATE_IDLE) {
        return -1;
    }

    mEventCb = cb;
    mUserData = userData;

    return 0;
}

int AmlMpPlayerImpl::setVideoParams(const Aml_MP_VideoParams* params)
{
    AML_MP_TRACE(10);

    if (mStreamState & VIDEO_STARTED) {
        ALOGE("video started already!");
        return -1;
    }

    mVideoParams.pid = params->pid;
    mVideoParams.videoCodec = params->videoCodec;
    if (mParser && (mVideoParams.videoCodec == AML_MP_CODEC_UNKNOWN)) {
        sptr<ProgramInfo> programInfo = mParser->getProgramInfo();
        if (programInfo) {
            for (auto it : programInfo->videoStreams) {
                if (it.pid == mVideoParams.pid) {
                    mVideoParams.videoCodec = it.codecId;
                }
            }
        }
    }
    mVideoParams.width = params->width;
    mVideoParams.height = params->height;
    mVideoParams.frameRate = params->frameRate;
    memcpy(mVideoParams.extraData, params->extraData, sizeof(mVideoParams.extraData));
    mVideoParams.extraDataSize = params->extraDataSize;

    return 0;
}

int AmlMpPlayerImpl::setAudioParams(const Aml_MP_AudioParams* params)
{
    AML_MP_TRACE(10);

    if (mStreamState & AUDIO_STARTED) {
        ALOGE("audio started already!");
        return -1;
    }

    mAudioParams.pid = params->pid;
    mAudioParams.audioCodec = params->audioCodec;
    if (mParser && (mAudioParams.audioCodec == AML_MP_CODEC_UNKNOWN)) {
        sptr<ProgramInfo> programInfo = mParser->getProgramInfo();
        if (programInfo) {
            for (auto it : programInfo->audioStreams) {
                if (it.pid == mAudioParams.pid) {
                    mAudioParams.audioCodec = it.codecId;
                }
            }
        }
    }
    mAudioParams.nChannels = params->nChannels;
    mAudioParams.nSampleRate = params->nSampleRate;
    memcpy(mAudioParams.extraData, params->extraData, sizeof(mAudioParams.extraData));
    mAudioParams.extraDataSize = params->extraDataSize;

    return 0;
}

int AmlMpPlayerImpl::setSubtitleParams(const Aml_MP_SubtitleParams* params)
{
    AML_MP_TRACE(10);

    if (mStreamState & SUBTITLE_STARTED) {
        ALOGE("subtitle started already!");
        return -1;
    }

    memcpy(&mSubtitleParams, params, sizeof(Aml_MP_SubtitleParams));
    if (mParser && (mSubtitleParams.subtitleCodec == AML_MP_CODEC_UNKNOWN)) {
        sptr<ProgramInfo> programInfo = mParser->getProgramInfo();
        if (programInfo) {
            for (auto it : programInfo->subtitleStreams) {
                if (it.pid == mSubtitleParams.pid) {
                    mSubtitleParams.subtitleCodec = it.codecId;
                }
            }
        }
    }

    return 0;
}

int AmlMpPlayerImpl::setIptvCASParams(const Aml_MP_IptvCasParams* params)
{
    AML_MP_TRACE(10);

    mCASParams = *params;

    return 0;
}

int AmlMpPlayerImpl::start()
{
    AML_MP_TRACE(10);

    if (mState == STATE_IDLE) {
        if (prepare() < 0) {
            ALOGE("prepare failed!");
            return -1;
        }
    }

    if ((mVideoParams.videoCodec == AML_MP_CODEC_UNKNOWN && mVideoParams.pid != AML_MP_INVALID_PID) ||
        (mAudioParams.audioCodec == AML_MP_CODEC_UNKNOWN && mAudioParams.pid != AML_MP_INVALID_PID) ||
        (mSubtitleParams.subtitleCodec == AML_MP_CODEC_UNKNOWN && mSubtitleParams.pid != AML_MP_INVALID_PID)) {
        // need parse ts stream and find format info
        mStartDelayFlag |= START_ALL_DELAY;
        return 0;
    } else {
        mStartDelayFlag &= ~START_ALL_DELAY;
    }

    setParams();
    int ret = mPlayer->start();
    if (ret < 0) {
        ALOGE("%s failed!", __FUNCTION__);
    }

    setState(STATE_RUNNING);

    //CHECK: assume start always be success if param exist
    if (mAudioParams.pid != AML_MP_INVALID_PID) {
        mStreamState |= AUDIO_STARTED;
    }

    if (mVideoParams.pid != AML_MP_INVALID_PID) {
        mStreamState |= VIDEO_STARTED;
    }

    if (mSubtitleParams.pid != AML_MP_INVALID_PID) {
        mStreamState |= SUBTITLE_STARTED;
    }

    if (mADParams.pid != AML_MP_INVALID_PID) {
        mStreamState |= AD_STARTED;
    }

    return ret;
}

int AmlMpPlayerImpl::stop()
{
    AML_MP_TRACE(10);

    if (mState == STATE_RUNNING || mState == STATE_PAUSED) {
        if (mPlayer) {
            mPlayer->stop();
        }

        mStreamState &= ~(AUDIO_STARTED | VIDEO_STARTED | SUBTITLE_STARTED);
    }

    return resetIfNeeded();
}

int AmlMpPlayerImpl::pause()
{
    AML_MP_TRACE(10);

    if (mState != STATE_RUNNING) {
        return 0;
    }

    RETURN_IF(-1, mPlayer == nullptr);

    if (mPlayer->pause() < 0) {
        return -1;
    }

    setState(STATE_PAUSED);
    return 0;
}

int AmlMpPlayerImpl::resume()
{
    AML_MP_TRACE(10);

    if (mState != STATE_PAUSED) {
        return 0;
    }

    RETURN_IF(-1, mPlayer == nullptr);

    if (mPlayer->resume() < 0) {
        return -1;
    }

    setState(STATE_RUNNING);
    return 0;
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
    int ret = 0;

    if (mState == STATE_RUNNING || mState == STATE_PAUSED) {
        RETURN_IF(-1, mPlayer == nullptr);
        ret = mPlayer->setPlaybackRate(rate);
    }

    return ret;
}

int AmlMpPlayerImpl::switchAudioTrack(const Aml_MP_AudioParams* params)
{
    AML_MP_TRACE(10);
    int ret = 0;

    if (mState == STATE_RUNNING || mState == STATE_PAUSED) {
        RETURN_IF(-1, mPlayer == nullptr);
        ret = mPlayer->switchAudioTrack(params);
    }

    return ret;
}

int AmlMpPlayerImpl::switchSubtitleTrack(const Aml_MP_SubtitleParams* params)
{
    AML_MP_TRACE(10);
    int ret = 0;

    if (mState == STATE_RUNNING || mState == STATE_PAUSED) {
        RETURN_IF(-1, mPlayer == nullptr);
        ret = mPlayer->switchSubtitleTrack(params);
    }

    return ret;
}

int AmlMpPlayerImpl::writeData(const uint8_t* buffer, size_t size)
{
    RETURN_IF(-1, mPlayer == nullptr);

    int writeLen = 0;
    if (mParserEnable) {
        std::lock_guard<std::mutex> _l(mLock);
        if (mStartDelayFlag > 0) {
            //is waiting for start_delay, writeData into mTsBuffer
            writeLen = mTsBuffer.put(buffer, size);
            mParser->writeData(buffer, writeLen);
        } else {
            //already start, need move data from mTsBuffer to player
            if (!mTsBuffer.empty()) {
                writeDataFromBuffer();
            }
            if (mTsBuffer.empty() && mTempBufferSize == 0) {
                //normal write data
                if (mCasHandle != nullptr) {
                    mCasHandle->processEcm(buffer, size);
                }
                if (mPlayer != nullptr) {
                    writeLen = mPlayer->writeData(buffer, size);
                }
            } else {
                //mTsBuffer or mTempBuffer still has data left to writeData
                writeLen = mTsBuffer.put(buffer, size);
            }
        }
    } else {
        if (mCasHandle != nullptr) {
            mCasHandle->processEcm(buffer, size);
        }
        if (mPlayer != nullptr) {
            writeLen = mPlayer->writeData(buffer, size);
        }
    }
    if (writeLen == 0) {
        writeLen = -1;
    }
    return writeLen;
}

void AmlMpPlayerImpl::writeDataFromBuffer()
{
    if (mTempBufferSize != 0) {
        //if writeData failed lasttime, then writeData again
        mTempBufferSize -= mPlayer->writeData(mTempBuffer, mTempBufferSize);
    }
    while (!mTsBuffer.empty() && mTempBufferSize == 0) {
        mTempBufferSize = mTsBuffer.get(mTempBuffer, 188 * 100);
        if (mCasHandle != nullptr) {
            mCasHandle->processEcm(mTempBuffer, mTempBufferSize);
        }
        if (mPlayer != nullptr) {
            mTempBufferSize -= mPlayer->writeData(mTempBuffer, mTempBufferSize);
        }
    }
    if (mTsBuffer.empty() && mTempBufferSize == 0) {
        ALOGI("writeData from buffer done");
    }
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

int AmlMpPlayerImpl::setANativeWindow(ANativeWindow* nativeWindow)
{
    AML_MP_TRACE(10);
    mNativeWindow = nativeWindow;
    ALOGI("setAnativeWindow: %p, mNativewindow: %p", nativeWindow, mNativeWindow.get());

    if (mState == STATE_RUNNING || mState == STATE_PAUSED) {
        RETURN_IF(-1, mPlayer == nullptr);
        if (mNativeWindow != nullptr) {
            mPlayer->setANativeWindow(mNativeWindow.get());
        }
    }

    return 0;
}

int AmlMpPlayerImpl::setVideoWindow(int x, int y, int width, int height)
{
    AML_MP_TRACE(10);
    if (width < 0 || height < 0) {
        ALOGI("Invalid windowsize: %dx%d, return fail", width, height);
        return -1;
    }
#ifndef __ANDROID_VNDK__
    if (mNativeWindow == nullptr) {
        ALOGI("Nativewindow is null, create it");
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
        ALOGI("Set video window size: x %d, y %d, width: %d, height: %d", x, y, width, height);
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
    mVideoWindow = {x, y, width, height};
    return 0;
}

int AmlMpPlayerImpl::setVolume(float volume)
{
    AML_MP_TRACE(10);
    int ret = 0;
    if (volume < 0) {
        ALOGI("volume is %f, set to 0.0", volume);
        volume = 0.0;
    }
    mVolume = volume;

    if (mState == STATE_RUNNING || mState == STATE_PAUSED) {
        RETURN_IF(-1, mPlayer == nullptr);
        ret = mPlayer->setVolume(volume);
    }

    return ret;
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
    case AML_MP_PLAYER_PARAMETER_VIDEO_DISPLAY_MODE:
        RETURN_IF(-1, parameter == nullptr);
        mVideoDisplayMode = *(Aml_MP_VideoDisplayMode*)parameter;
        break;

    case AML_MP_PLAYER_PARAMETER_BLACK_OUT:
        RETURN_IF(-1, parameter == nullptr);
        mBlackOut = *(bool*)parameter;
        break;

    case AML_MP_PLAYER_PARAMETER_VIDEO_DECODE_MODE:
        RETURN_IF(-1, parameter == nullptr);
        mVideoDecodeMode = *(Aml_MP_VideoDecodeMode*)parameter;
        break;

    case AML_MP_PLAYER_PARAMETER_VIDEO_PTS_OFFSET:
        RETURN_IF(-1, parameter == nullptr);
        mVideoPtsOffset = *(int*)parameter;
        break;

    case AML_MP_PLAYER_PARAMETER_AUDIO_OUTPUT_MODE:
        RETURN_IF(-1, parameter == nullptr);
        mAudioOutputMode = *(Aml_MP_AudioOutputMode*)parameter;
        break;

    case AML_MP_PLAYER_PARAMETER_AUDIO_OUTPUT_DEVICE:
        RETURN_IF(-1, parameter == nullptr);
        mAudioOutputDevice = *(Aml_MP_AudioOutputDevice*)parameter;
        break;

    case AML_MP_PLAYER_PARAMETER_AUDIO_PTS_OFFSET:
        RETURN_IF(-1, parameter == nullptr);
        mAudioPtsOffset = *(int*)parameter;
        break;

    case AML_MP_PLAYER_PARAMETER_AUDIO_BALANCE:
        RETURN_IF(-1, parameter == nullptr);
        mAudioBalance = *(Aml_MP_AudioBalance*)parameter;
        break;

    case AML_MP_PLAYER_PARAMETER_AUDIO_MUTE:
        RETURN_IF(-1, parameter == nullptr);
        mAudioMute = *(bool*)parameter;
        break;

    case AML_MP_PLAYER_PARAMETER_NETWORK_JITTER:
        RETURN_IF(-1, parameter == nullptr);
        mNetworkJitter = *(int*)parameter;
        break;

    case AML_MP_PLAYER_PARAMETER_AD_STATE:
        RETURN_IF(-1, parameter == nullptr);
        mADState = *(int*)parameter;
        break;

    case AML_MP_PLAYER_PARAMETER_AD_MIX_LEVEL:
        RETURN_IF(-1, parameter == nullptr);
        mADMixLevel = *(int*)parameter;
        break;

    case AML_MP_PLAYER_PARAMETER_WORK_MODE:
    {
        mWorkMode = *(Aml_MP_PlayerWorkMode*)parameter;
        ALOGI("Set work mode: %d", mWorkMode);
    }
    break;

    case AML_MP_PLAYER_PARAMETER_VIDEO_WINDOW_ZORDER:
    {
        mZorder = *(int*)parameter;
        ALOGI("Set zorder: %d", mZorder);

#ifndef __ANDROID_VNDK__
        if (mSurfaceControl != nullptr) {
            auto transcation = android::SurfaceComposerClient::Transaction();

            transcation.setLayer(mSurfaceControl, mZorder);

            transcation.apply();
        }
#endif
    }
    break;

    case AML_MP_PLAYER_PARAMETER_CREATE_PARAMS:
    {
        RETURN_IF(-1, parameter == nullptr);
        mCreateParams = *(Aml_MP_PlayerCreateParams *)parameter;
        ALOGI("Set mCreateParams drmmode:%d", mCreateParams.drmMode);
        return 0;
    }
    break;

    case AML_MP_PLAYER_PARAMETER_VIDEO_TUNNEL_ID:
    {
        mVideoTunnelId = *(int*)parameter;
    }
    break;

    case AML_MP_PLAYER_PARAMETER_SURFACE_HANDLE:
    {
        ALOGI("set surface handle: %p", parameter);
        mSurfaceHandle = parameter;
        break;
    }

    default:
        ALOGW("unhandled key:%#x", key);
        break;
    }

    if (mState == STATE_RUNNING || mState == STATE_PAUSED) {
        RETURN_IF(-1, mPlayer == nullptr);
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

    if (mState == STATE_IDLE) {
        if (prepare() < 0) {
            ALOGE("prepare failed!");
            return -1;
        }
    }

    if ((mVideoParams.videoCodec == AML_MP_CODEC_UNKNOWN && mVideoParams.pid != AML_MP_INVALID_PID)) {
        // need parse ts stream and find format info
        mStartDelayFlag |= START_VIDEO_DELAY;
        return 0;
    } else {
        mStartDelayFlag &= ~START_VIDEO_DELAY;
    }

    setParams();
    int ret = mPlayer->startVideoDecoding();
    if (ret < 0) {
        ALOGE("%s failed!", __FUNCTION__);
        return -1;
    }

    setState(STATE_RUNNING);
    if (mVideoParams.pid != AML_MP_INVALID_PID) {
        mStreamState |= VIDEO_STARTED;
    }

    return ret;
}

int AmlMpPlayerImpl::stopVideoDecoding()
{
    AML_MP_TRACE(10);
    RETURN_IF(-1, mPlayer == nullptr);

    if (mState == STATE_RUNNING || mState == STATE_PAUSED) {
        if (mPlayer) {
            mPlayer->stopVideoDecoding();
            mStartDelayFlag &= ~START_VIDEO_DELAY;
        }

        mStreamState &= ~VIDEO_STARTED;
    }

    return resetIfNeeded();
}

int AmlMpPlayerImpl::startAudioDecoding()
{
    AML_MP_TRACE(10);
    MLOG();

    if (mState == STATE_IDLE) {
        if (prepare() < 0) {
            ALOGE("prepare failed!");
            return -1;
        }
    }

    if (mAudioParams.audioCodec == AML_MP_CODEC_UNKNOWN && mAudioParams.pid != AML_MP_INVALID_PID) {
        // need parse ts stream and find format info
        mStartDelayFlag |= START_AUDIO_DELAY;
        return 0;
    } else {
        mStartDelayFlag &= ~START_AUDIO_DELAY;
    }
    setParams();
    int ret = mPlayer->startAudioDecoding();
    if (ret < 0) {
        ALOGE("%s failed!", __FUNCTION__);
        return -1;
    }

    setState(STATE_RUNNING);
    if (mAudioParams.pid != AML_MP_INVALID_PID) {
        mStreamState |= AUDIO_STARTED;
    }

    return ret;
}

int AmlMpPlayerImpl::stopAudioDecoding()
{
    AML_MP_TRACE(10);
    RETURN_IF(-1, mPlayer == nullptr);

    if (mState == STATE_RUNNING || mState == STATE_PAUSED) {
        if (mPlayer) {
            mPlayer->stopAudioDecoding();
            mStartDelayFlag &= ~START_AUDIO_DELAY;
        }

        mStreamState &= ~AUDIO_STARTED;
    }

    return resetIfNeeded();
}

int AmlMpPlayerImpl::startSubtitleDecoding()
{
    AML_MP_TRACE(10);
    RETURN_IF(-1, mPlayer == nullptr);

    if (mState == STATE_IDLE) {
        if (prepare() < 0) {
            ALOGE("prepare failed!");
            return -1;
        }
    }

    if (mSubtitleParams.subtitleCodec == AML_MP_CODEC_UNKNOWN && mSubtitleParams.pid != AML_MP_INVALID_PID) {
        // need parse ts stream and find format info
        mStartDelayFlag |= START_SUBTITLE_DELAY;
        return 0;
    } else {
        mStartDelayFlag &= ~START_SUBTITLE_DELAY;
    }
    setParams();
    int ret = mPlayer->startSubtitleDecoding();
    if (ret < 0) {
        ALOGE("%s failed!", __FUNCTION__);
        return -1;
    }

    setState(STATE_RUNNING);
    if (mSubtitleParams.pid != AML_MP_INVALID_PID) {
        mStreamState |= SUBTITLE_STARTED;
    }

    return ret;
}

int AmlMpPlayerImpl::stopSubtitleDecoding()
{
    AML_MP_TRACE(10);
    RETURN_IF(-1, mPlayer == nullptr);

    if (mState == STATE_RUNNING || mState == STATE_PAUSED) {
        if (mPlayer) {
            mPlayer->stopSubtitleDecoding();
            mStartDelayFlag &= ~START_SUBTITLE_DELAY;
        }

        mStreamState &= ~SUBTITLE_STARTED;
    }

    return resetIfNeeded();
}

int AmlMpPlayerImpl::setSubtitleWindow(int x, int y, int width, int height)
{
    AML_MP_TRACE(10);
    mSubtitleWindow = {x, y, width, height};
    int ret = 0;

    if (mState == STATE_RUNNING || mState == STATE_PAUSED) {
        ret = mPlayer->setSubtitleWindow(x, y, width, height);
    }

    return ret;
}

//internal function
int AmlMpPlayerImpl::startDescrambling()
{
    AML_MP_TRACE(10);

    ALOGI("encrypted stream!, mCreateParams.sourceType=%d", mCreateParams.sourceType);
    if (mCASParams.type == AML_MP_CAS_UNKNOWN) {
        ALOGE("unknown cas type!");
        return -1;
    }

    mCasHandle = AmlCasBase::create(&mCASParams, mInstanceId);
    if (mCasHandle == nullptr) {
        ALOGE("create CAS handle failed!");
        return -1;
    }

    mCasHandle->openSession();

    return 0;
}

//internal function
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

    mADParams.pid = params->pid;
    mADParams.audioCodec = params->audioCodec;
    mADParams.nChannels = params->nChannels;
    mADParams.nSampleRate = params->nSampleRate;
    memcpy(mADParams.extraData, params->extraData, sizeof(mADParams.extraData));
    mADParams.extraDataSize = params->extraDataSize;

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
const char* AmlMpPlayerImpl::stateString(State state)
{
    switch (state) {
    case STATE_IDLE:
        return "STATE_IDLE";
    case STATE_PREPARED:
        return "STATE_PREPARED";
    case STATE_RUNNING:
        return "STATE_RUNNING";
    case STATE_PAUSED:
        return "STATE_PAUSED";
    }
}

std::string AmlMpPlayerImpl::streamStateString(int streamState)
{
    std::stringstream ss;
    bool hasValue = false;

    if (streamState & AUDIO_STARTED) {
        if (hasValue) {
            ss << "|";
        }
        ss << "AUDIO_STARTED";
        hasValue = true;
    }

    if (streamState & VIDEO_STARTED) {
        if (hasValue) {
            ss << "|";
        }
        ss << "VIDEO_STARTED";
        hasValue = true;
    }

    if (streamState & SUBTITLE_STARTED) {
        if (hasValue) {
            ss << "|";
        }
        ss << "SUBTITLE_STARTED";
        hasValue = true;
    }

    if (!hasValue) {
        ss << "STREAM_STOPPED";
    }

    return ss.str();
}

void AmlMpPlayerImpl::setState(State state)
{
    if (mState != state) {
        ALOGI("%s -> %s", stateString(mState), stateString(state));
        mState = state;
    }
}

int AmlMpPlayerImpl::prepare()
{
    MLOG();

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

    if (mParser == nullptr) {
        mParser = new Parser(mCreateParams.demuxId, mCreateParams.sourceType == AML_MP_INPUT_SOURCE_TS_DEMOD, true);
    }

    ALOGI("mWorkMode: %d", mWorkMode);
    mPlayer->setParameter(AML_MP_PLAYER_PARAMETER_WORK_MODE, &mWorkMode);

    mPlayer->registerEventCallback(mEventCb, mUserData);

    if ((mSubtitleWindow.width > 0) && (mSubtitleWindow.height > 0)) {
        mPlayer->setSubtitleWindow(mSubtitleWindow.x, mSubtitleWindow.y, mSubtitleWindow.width, mSubtitleWindow.height);
    }

    if (mSurfaceHandle != nullptr) {
        mPlayer->setParameter(AML_MP_PLAYER_PARAMETER_SURFACE_HANDLE, mSurfaceHandle);
    }

    ALOGI("mNativeWindow:%p", mNativeWindow.get());
    if (mNativeWindow != nullptr) {
        mPlayer->setANativeWindow(mNativeWindow.get());
    }

    if (!mNativeWindow.get() && mVideoWindow.width > 0 && mVideoWindow.height > 0) {
        mPlayer->setVideoWindow(mVideoWindow.x, mVideoWindow.y, mVideoWindow.width, mVideoWindow.height);
    }

    if (mVideoTunnelId >= 0) {
        mPlayer->setParameter(AML_MP_PLAYER_PARAMETER_VIDEO_TUNNEL_ID, &mVideoTunnelId);
    }

    if (mSyncSource == AML_MP_AVSYNC_SOURCE_DEFAULT) {
        mSyncSource = AML_MP_AVSYNC_SOURCE_PCR;
    }

    mPlayer->setAVSyncSource(mSyncSource);

    if (mSyncSource == AML_MP_AVSYNC_SOURCE_PCR && mPcrPid != AML_MP_INVALID_PID) {
        mPlayer->setPcrPid(mPcrPid);
    }

    mPlayer->setPlaybackRate(mPlaybackRate);

    if (mVolume >= 0) {
        mPlayer->setVolume(mVolume);
    }

    applyParameters();


    if ((mVideoParams.videoCodec == AML_MP_CODEC_UNKNOWN && mVideoParams.pid != AML_MP_INVALID_PID) ||
        (mAudioParams.audioCodec == AML_MP_CODEC_UNKNOWN && mAudioParams.pid != AML_MP_INVALID_PID) ||
        (mSubtitleParams.subtitleCodec == AML_MP_CODEC_UNKNOWN && mSubtitleParams.pid != AML_MP_INVALID_PID)) {
        // need parse ts stream and find format info
        mParser->setProgram(mVideoParams.pid, mAudioParams.pid);
        mParser->setEventCallback([this] (Parser::ProgramEventType event, int param1, int param2, void* data) {
                return programEventCallback(event, param1, param2, data);
        });
        mParser->open();
        mParserEnable = true;
    }

    setState(STATE_PREPARED);
    return 0;
}

void AmlMpPlayerImpl::setParams()
{
    if (mVideoParams.pid != AML_MP_INVALID_PID) {
        mPlayer->setVideoParams(&mVideoParams);
    }

    if (mAudioParams.pid != AML_MP_INVALID_PID) {
        mPlayer->setAudioParams(&mAudioParams);
    }

    if (mSubtitleParams.subtitleCodec != AML_MP_CODEC_UNKNOWN) {
        mPlayer->setSubtitleParams(&mSubtitleParams);
    }

    if (mADParams.pid != AML_MP_INVALID_PID) {
        mPlayer->setADParams(&mADParams);
    }
}

void AmlMpPlayerImpl::programEventCallback(Parser::ProgramEventType event, int param1, int param2, void* data)
{
    switch (event)
    {
        case Parser::ProgramEventType::EVENT_PROGRAM_PARSED:
        {
            ProgramInfo* programInfo = (ProgramInfo*)data;
            ALOGI("programEventCallback: program(programNumber=%d,pid=%d) parsed", programInfo->programNumber, programInfo->pmtPid);
            programInfo->debugLog();
            std::lock_guard<std::mutex> _l(mLock);
            for (auto it : programInfo->videoStreams) {
                if (it.pid == mVideoParams.pid) {
                    mVideoParams.videoCodec = it.codecId;
                }
            }
            for (auto it : programInfo->audioStreams) {
                if (it.pid == mAudioParams.pid) {
                    mAudioParams.audioCodec = it.codecId;
                }
            }
            for (auto it : programInfo->subtitleStreams) {
                if (it.pid == mSubtitleParams.pid) {
                    mSubtitleParams.subtitleCodec = it.codecId;
                }
            }
            if (mStartDelayFlag & START_ALL_DELAY) {
                start();
            }
            if (mStartDelayFlag & START_VIDEO_DELAY) {
                startVideoDecoding();
            }
            if (mStartDelayFlag & START_AUDIO_DELAY) {
                startAudioDecoding();
            }
            if (mStartDelayFlag & START_SUBTITLE_DELAY) {
                startSubtitleDecoding();
            }
            break;
        }
        case Parser::ProgramEventType::EVENT_AV_PID_CHANGED:
        {
            Aml_MP_PlayerEventPidChangeInfo* info = (Aml_MP_PlayerEventPidChangeInfo*)data;
            ALOGI("programEventCallback: program(programNumber=%d,pid=%d) pidchangeInfo: oldPid:%d --> newPid:%d",
                info->programNumber, info->programPid, info->oldStreamPid, info->newStreamPid);
            if (mEventCb) {
                mEventCb(mUserData, AML_MP_PLAYER_EVENT_PID_CHANGED, (uint64_t)data);
            }
            break;
        }
        case Parser::ProgramEventType::EVENT_ECM_DATA_PARSED:
        {
            ALOGI("programEventCallback: ecmData parsed, size:%d", param2);
            uint8_t* ecmData = (uint8_t*)data;
            std::string ecmDataStr;
            char hex[3];
            for (int i = 0; i < param2; i++) {
                 snprintf(hex, sizeof(hex), "%02X", ecmData[i]);
                 ecmDataStr.append(hex);
                 ecmDataStr.append(" ");
            }
            ALOGI("programEventCallback: ecmData: size:%d, hexStr:%s", param2, ecmDataStr.c_str());

            if (mCasHandle) {
                mCasHandle->processEcm(ecmData, param2);
            }
        }
    }
}

int AmlMpPlayerImpl::resetIfNeeded()
{
    MLOG();

    if (mState == STATE_RUNNING || mState == STATE_PAUSED) {
        if (mStreamState == ALL_STREAMS_STOPPED) {
            setState(STATE_PREPARED);
        } else {
            ALOGE("current streamState:%s", streamStateString(mStreamState).c_str());
        }
    }

    if (mState == STATE_PREPARED) {
        reset();
    }

    return 0;
}

int AmlMpPlayerImpl::reset()
{
    MLOG();

    if (mCasHandle) {
        stopDescrambling();
    }

    if (mParserEnable) {
        std::lock_guard<std::mutex> _l(mLock);
        mStartDelayFlag = 0;
        mTsBuffer.reset();
        mTempBufferSize = 0;
        mParser->close();
        mParserEnable = false;
    }

    mParser.clear();
    mPlayer.clear();

    setState(STATE_IDLE);

    return 0;
}

int AmlMpPlayerImpl::applyParameters()
{
    mPlayer->setParameter(AML_MP_PLAYER_PARAMETER_VIDEO_DISPLAY_MODE, &mVideoDisplayMode);
    mPlayer->setParameter(AML_MP_PLAYER_PARAMETER_BLACK_OUT, &mBlackOut);
    mPlayer->setParameter(AML_MP_PLAYER_PARAMETER_VIDEO_DECODE_MODE, &mVideoDecodeMode);
    mPlayer->setParameter(AML_MP_PLAYER_PARAMETER_VIDEO_PTS_OFFSET, &mVideoPtsOffset);
    mPlayer->setParameter(AML_MP_PLAYER_PARAMETER_AUDIO_OUTPUT_MODE, &mAudioOutputMode);
    mPlayer->setParameter(AML_MP_PLAYER_PARAMETER_AUDIO_OUTPUT_DEVICE, &mAudioOutputDevice);
    mPlayer->setParameter(AML_MP_PLAYER_PARAMETER_AUDIO_PTS_OFFSET, &mAudioPtsOffset);
    mPlayer->setParameter(AML_MP_PLAYER_PARAMETER_AUDIO_BALANCE, &mAudioBalance);
    mPlayer->setParameter(AML_MP_PLAYER_PARAMETER_AUDIO_MUTE, &mAudioMute);
    mPlayer->setParameter(AML_MP_PLAYER_PARAMETER_NETWORK_JITTER, &mNetworkJitter);

    if (mADState != -1) {
        mPlayer->setParameter(AML_MP_PLAYER_PARAMETER_AD_STATE, &mADState);
    }

    if (mADMixLevel != -1) {
        mPlayer->setParameter(AML_MP_PLAYER_PARAMETER_AD_MIX_LEVEL, &mADMixLevel);
    }

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

void AmlMpPlayerRoster::signalAmTsPlayerId(int id)
{
    std::lock_guard<std::mutex> _l(mLock);
    mAmtsPlayerId = id;
}

bool AmlMpPlayerRoster::isAmTsPlayerExist() const
{
    std::lock_guard<std::mutex> _l(mLock);
    return mAmtsPlayerId != -1;
}

}
