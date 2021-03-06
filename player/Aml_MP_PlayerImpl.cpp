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
#include <utils/AmlMpBuffer.h>
#include <sstream>
#include <mutex>
#include <condition_variable>
#include "AmlPlayerBase.h"

#ifdef ANDROID
#include <media/stagefright/foundation/ADebug.h>
#ifndef __ANDROID_VNDK__
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#endif
#endif
#include <utils/AmlMpEventLooper.h>


namespace aml_mp {

#define TS_BUFFER_SIZE          (2 * 1024 * 1024)
#define TEMP_BUFFER_SIZE        (188 * 100)

#define START_ALL_PENDING       (1 << 0)
#define START_VIDEO_PENDING     (1 << 1)
#define START_AUDIO_PENDING     (1 << 2)
#define START_SUBTITLE_PENDING  (1 << 3)

#define FAST_PLAY_THRESHOLD     2.0f

///////////////////////////////////////////////////////////////////////////////
AmlMpPlayerImpl::AmlMpPlayerImpl(const Aml_MP_PlayerCreateParams* createParams)
: mInstanceId(AmlMpPlayerRoster::instance().registerPlayer(this))
, mCreateParams(*createParams)
{
    snprintf(mName, sizeof(mName), "%s_%d", LOG_TAG, mInstanceId);

#ifdef ANDROID
    int sdkVersion = ANDROID_PLATFORM_SDK_VERSION;
    const char* platform =
#ifndef __ANDROID_VNDK__
        "system";
#else
        "vendor";
#endif
    MLOG("sdk:%d, platform:%s", sdkVersion, platform);
#else
    MLOG("drmMode:%s, sourceType:%s", mpInputStreamType2Str(createParams->drmMode), mpInputSourceType2Str(createParams->sourceType));
#endif

    memset(&mVideoParams, 0, sizeof(mVideoParams));
    mVideoParams.pid = AML_MP_INVALID_PID;
    mVideoParams.videoCodec = AML_MP_CODEC_UNKNOWN;
    memset(&mAudioParams, 0, sizeof(mAudioParams));
    mAudioParams.pid = AML_MP_INVALID_PID;
    mAudioParams.audioCodec = AML_MP_CODEC_UNKNOWN;
    memset(&mSubtitleParams, 0, sizeof(mSubtitleParams));
    mSubtitleParams.pid = AML_MP_INVALID_PID;
    mSubtitleParams.subtitleCodec = AML_MP_CODEC_UNKNOWN;
    memset(&mADParams, 0, sizeof(mADParams));
    mADParams.pid = AML_MP_INVALID_PID;
    mADParams.audioCodec = AML_MP_CODEC_UNKNOWN;
    memset(&mIptvCasParams, 0, sizeof(mIptvCasParams));

    mWorkMode = AML_MP_PLAYER_MODE_NORMAL;
    mAudioBalance = AML_MP_AUDIO_BALANCE_STEREO;

    AmlMpConfig::instance().init();
    mWaitingEcmMode = (WaitingEcmMode)AmlMpConfig::instance().mWaitingEcmMode;
    MLOGI("mWaitingEcmMode:%d", mWaitingEcmMode);

    mTsBuffer.init(AmlMpConfig::instance().mWriteBufferSize * 1024 * 1024);

    mWriteBuffer = new AmlMpBuffer(TEMP_BUFFER_SIZE);
    mWriteBuffer->setRange(0, 0);
    mZorder = kZorderBase + mInstanceId;

    mPlayer = AmlPlayerBase::create(&mCreateParams, mInstanceId);
}

AmlMpPlayerImpl::~AmlMpPlayerImpl()
{
    MLOG();

    CHECK(mState == STATE_IDLE);
    CHECK(mStreamState == 0);

    AmlMpPlayerRoster::instance().unregisterPlayer(mInstanceId);
}

int AmlMpPlayerImpl::registerEventCallback(Aml_MP_PlayerEventCallback cb, void* userData)
{
    MLOG();
    AML_MP_TRACE(10);

    {
        std::unique_lock<std::mutex> _l(mLock);
        if (mState != STATE_IDLE) {
            MLOGW("can't registerEventCallback now!");
            return -1;
        }
    }

    std::unique_lock<std::mutex> _eventLock(mEventLock);
    mEventCb = cb;
    mUserData = userData;

    return 0;
}

int AmlMpPlayerImpl::setVideoParams(const Aml_MP_VideoParams* params)
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);

    if (getStreamState_l(AML_MP_STREAM_TYPE_VIDEO) != STREAM_STATE_STOPPED) {
        MLOGE("video started already!");
        return -1;
    }

    mVideoParams.pid = params->pid;
    mVideoParams.videoCodec = params->videoCodec;
    mVideoParams.width = params->width;
    mVideoParams.height = params->height;
    mVideoParams.frameRate = params->frameRate;
    memcpy(mVideoParams.extraData, params->extraData, sizeof(mVideoParams.extraData));
    mVideoParams.extraDataSize = params->extraDataSize;

    MLOGI("setVideoParams vpid: 0x%x, fmt: %s", params->pid, mpCodecId2Str(params->videoCodec));

    return 0;
}

int AmlMpPlayerImpl::setAudioParams(const Aml_MP_AudioParams* params)
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);
    if (getStreamState_l(AML_MP_STREAM_TYPE_AUDIO) != STREAM_STATE_STOPPED) {
        MLOGE("audio started already!");
        return -1;
    }

    return setAudioParams_l(params);
}

int AmlMpPlayerImpl::setAudioParams_l(const Aml_MP_AudioParams* params)
{
    mAudioParams.pid = params->pid;
    mAudioParams.audioCodec = params->audioCodec;
    mAudioParams.nChannels = params->nChannels;
    mAudioParams.nSampleRate = params->nSampleRate;
    memcpy(mAudioParams.extraData, params->extraData, sizeof(mAudioParams.extraData));
    mAudioParams.extraDataSize = params->extraDataSize;
    mAudioParams.secureLevel = params->secureLevel;

    MLOGI("setAudioParams apid: 0x%x, fmt: %s", params->pid, mpCodecId2Str(params->audioCodec));

    return 0;
}

int AmlMpPlayerImpl::setSubtitleParams(const Aml_MP_SubtitleParams* params)
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);

    if (getStreamState_l(AML_MP_STREAM_TYPE_SUBTITLE) != STREAM_STATE_STOPPED) {
        MLOGE("subtitle started already!");
        return -1;
    }

    return setSubtitleParams_l(params);
}

int AmlMpPlayerImpl::setSubtitleParams_l(const Aml_MP_SubtitleParams* params)
{
    memcpy(&mSubtitleParams, params, sizeof(Aml_MP_SubtitleParams));

    MLOGI("setSubtitleParams spid: 0x%x, fmt:%s", params->pid, mpCodecId2Str(params->subtitleCodec));
    return 0;
}

int AmlMpPlayerImpl::setADParams(Aml_MP_AudioParams* params)
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);

    if (getStreamState_l(AML_MP_STREAM_TYPE_AD) != STREAM_STATE_STOPPED) {
        MLOGE("AD started already!");
        return -1;
    }

    mADParams.pid = params->pid;
    mADParams.audioCodec = params->audioCodec;
    mADParams.nChannels = params->nChannels;
    mADParams.nSampleRate = params->nSampleRate;
    memcpy(mADParams.extraData, params->extraData, sizeof(mADParams.extraData));
    mADParams.extraDataSize = params->extraDataSize;
    mADParams.secureLevel = params->secureLevel;

    MLOGI("setADParams apid: 0x%x, fmt:%s", params->pid, mpCodecId2Str(params->audioCodec));

    return 0;
}

int AmlMpPlayerImpl::setCasSession(AML_MP_CASSESSION casSession)
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);
    MLOG();
    sptr<AmlCasBase> casBase = aml_handle_cast<AmlCasBase>(casSession);
    RETURN_IF(-1, casBase == nullptr);

    if (casBase->serviceType() < AML_MP_CAS_SERVICE_TYPE_IPTV) {
        MLOGE("do not support bind DVB caseSession!");
    } else if (casBase->serviceType() == AML_MP_CAS_SERVICE_VERIMATRIX_WEB) {
        MLOGW("verimatrix web casSession don't need bind!");
    }

    mCasHandle = casBase;
    casBase->getEcmPids(mEcmPids);
    mIsStandaloneCas = true;

    return 0;
}

int AmlMpPlayerImpl::setIptvCASParams(Aml_MP_CASServiceType serviceType, const Aml_MP_IptvCASParams* params)
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);
    MLOG();

    mCasServiceType = serviceType;
    mIptvCasParams = *params;

    return 0;
}

int AmlMpPlayerImpl::start()
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);
    MLOG();

    return start_l();
}

int AmlMpPlayerImpl::start_l()
{
    MLOGI("AmlMpPlayerImpl start_l\n");
    if (mState == STATE_IDLE) {
        if (prepare_l() < 0) {
            MLOGE("prepare failed!");
            return -1;
        }
    }

    if (mState == STATE_PREPARING) {
        setStreamState_l(AML_MP_STREAM_TYPE_AUDIO, STREAM_STATE_START_PENDING);
        setStreamState_l(AML_MP_STREAM_TYPE_VIDEO, STREAM_STATE_START_PENDING);
        setStreamState_l(AML_MP_STREAM_TYPE_SUBTITLE, STREAM_STATE_START_PENDING);
        setStreamState_l(AML_MP_STREAM_TYPE_AD, STREAM_STATE_START_PENDING);

        return 0;
    }

    if (mVideoParams.pid != AML_MP_INVALID_PID) {
        mPlayer->setVideoParams(&mVideoParams);
    }

    if (mAudioParams.pid != AML_MP_INVALID_PID) {
        mPlayer->setAudioParams(&mAudioParams);
        if (mAudioPresentationId > 0) {
            mPlayer->setParameter(AML_MP_PLAYER_PARAMETER_AUDIO_PRESENTATION_ID, &mAudioPresentationId);
        }
    }

    if (mSubtitleParams.subtitleCodec != AML_MP_CODEC_UNKNOWN) {
        mPlayer->setSubtitleParams(&mSubtitleParams);
    }

    int ret = 0;
    if (mVideoParams.pid != AML_MP_INVALID_PID ||
        mAudioParams.pid != AML_MP_INVALID_PID ||
        mSubtitleParams.subtitleCodec != AML_MP_CODEC_UNKNOWN) {
        ret = mPlayer->start();
        if (ret < 0) {
            MLOGE("%s failed!", __FUNCTION__);
            return ret;
        }
    }
    setState_l(STATE_RUNNING);

    //CHECK: assume start always be success if param exist
    if (mAudioParams.pid != AML_MP_INVALID_PID) {
        setStreamState_l(AML_MP_STREAM_TYPE_AUDIO, STREAM_STATE_STARTED);
    }

    if (mVideoParams.pid != AML_MP_INVALID_PID) {
        setStreamState_l(AML_MP_STREAM_TYPE_VIDEO, STREAM_STATE_STARTED);
    }

    if (mSubtitleParams.subtitleCodec != AML_MP_CODEC_UNKNOWN) {
        setStreamState_l(AML_MP_STREAM_TYPE_SUBTITLE, STREAM_STATE_STARTED);
    }

    startADDecoding_l();
    MLOGI("<<<< AmlMpPlayerImpl start_l\n");
    MLOG("end");
    return ret;
}

int AmlMpPlayerImpl::stop()
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> lock(mLock);
    MLOG();
    RETURN_IF(-1, mPlayer == nullptr);

    return stop_l(lock);
}

int AmlMpPlayerImpl::stop_l(std::unique_lock<std::mutex>& lock)
{
    if (mState == STATE_RUNNING || mState == STATE_PAUSED) {
        if (mPlayer) {
            lock.unlock();
            mPlayer->stop();
            lock.lock();

            Aml_MP_AudioParams dummyAudioParam{AML_MP_INVALID_PID, AML_MP_CODEC_UNKNOWN};
            mPlayer->setAudioParams(&dummyAudioParam);
        }
    }

    // ensure stream stopped if mState isn't running or paused
    setStreamState_l(AML_MP_STREAM_TYPE_AUDIO, STREAM_STATE_STOPPED);
    setStreamState_l(AML_MP_STREAM_TYPE_VIDEO, STREAM_STATE_STOPPED);
    setStreamState_l(AML_MP_STREAM_TYPE_SUBTITLE, STREAM_STATE_STOPPED);
    stopADDecoding_l(lock);

    int ret = resetIfNeeded_l(lock);

    MLOG("end");
    return ret;
}

int AmlMpPlayerImpl::pause()
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);
    MLOG();

    return pause_l();
}

int AmlMpPlayerImpl::pause_l() {
    if (mState != STATE_RUNNING) {
        return 0;
    }

    RETURN_IF(-1, mPlayer == nullptr);

    if (mPlayer->pause() < 0) {
        return -1;
    }

    setState_l(STATE_PAUSED);
    return 0;
}

int AmlMpPlayerImpl::resume()
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);

    return resume_l();
}

int AmlMpPlayerImpl::resume_l() {
    if (mState != STATE_PAUSED) {
        return 0;
    }

    RETURN_IF(-1, mPlayer == nullptr);

    if (mPlayer->resume() < 0) {
        return -1;
    }

    setState_l(STATE_RUNNING);
    return 0;
}

int AmlMpPlayerImpl::flush()
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);
    MLOG();
    RETURN_IF(-1, mPlayer == nullptr);

    if (mState != STATE_RUNNING && mState != STATE_PAUSED) {
        MLOGI("State is %s, can't call flush when not play", stateString(mState));
        return 0;
    }

    int ret;

    ret = mPlayer->flush();

    if (ret != AML_MP_ERROR_DEAD_OBJECT) {
        return ret;
    }

    ret = mPlayer->stop();
    if (ret < 0) {
        MLOGI("[%s, %d] stop play fail ret: %d", __FUNCTION__, __LINE__, ret);
    }
    if (getStreamState_l(AML_MP_STREAM_TYPE_VIDEO) == STREAM_STATE_STARTED) {
        mPlayer->setVideoParams(&mVideoParams);
    }
    if (getStreamState_l(AML_MP_STREAM_TYPE_AUDIO) == STREAM_STATE_STARTED) {
        mPlayer->setAudioParams(&mAudioParams);
        if (mAudioPresentationId > 0) {
            mPlayer->setParameter(AML_MP_PLAYER_PARAMETER_AUDIO_PRESENTATION_ID, &mAudioPresentationId);
        }
    }
    if (getStreamState_l(AML_MP_STREAM_TYPE_AD) == STREAM_STATE_STARTED) {
        mPlayer->setADParams(&mADParams, true);
    }
    ret = mPlayer->start();
    if (ret < 0) {
        MLOGI("[%s, %d] start play fail ret: %d", __FUNCTION__, __LINE__, ret);
    }

    if (mState == STATE_PAUSED) {
        ret = mPlayer->pause();
    }

    return ret;
}

int AmlMpPlayerImpl::setPlaybackRate(float rate)
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
            switchDecodeMode_l(newDecodeMode, lock);
        }

        if (mVideoDecodeMode == AML_MP_VIDEO_DECODE_MODE_NONE) {
            ret = mPlayer->setPlaybackRate(rate);
        }
    }

    return ret;
}

int AmlMpPlayerImpl::switchAudioTrack(const Aml_MP_AudioParams* params)
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);
    MLOG("new apid: 0x%x, fmt:%s", params->pid, mpCodecId2Str(params->audioCodec));
    int ret = -1;

    setAudioParams_l(params);

    if (mState == STATE_RUNNING || mState == STATE_PAUSED) {
        RETURN_IF(-1, mPlayer == nullptr);
        ret = mPlayer->switchAudioTrack(params);

        if (ret != AML_MP_ERROR_DEAD_OBJECT) {
            return ret;
        }

        ret = mPlayer->stopAudioDecoding();
        ret += startAudioDecoding_l();
    }

    return ret;
}

int AmlMpPlayerImpl::switchSubtitleTrack(const Aml_MP_SubtitleParams* params)
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);
    MLOG("new spid: 0x%x, fmt:%s", params->pid, mpCodecId2Str(params->subtitleCodec));
    int ret = 0;

    setSubtitleParams_l(params);

    if (mState == STATE_RUNNING || mState == STATE_PAUSED) {
        RETURN_IF(-1, mPlayer == nullptr);
        ret = mPlayer->switchSubtitleTrack(params);
    }

    return ret;
}

int AmlMpPlayerImpl::writeData(const uint8_t* buffer, size_t size)
{
    std::unique_lock<std::mutex> _l(mLock);
    RETURN_IF(-1, mPlayer == nullptr);

    int written = 0;

    bool needBuffering = false;
    if (mCasHandle && mCreateParams.drmMode == AML_MP_INPUT_STREAM_ENCRYPTED && mWaitingEcmMode == kWaitingEcmSynchronous && !mFirstEcmWritten) {
        size_t ecmOffset = size;
        size_t ecmSize = 188;
        ecmOffset = findEcmPacket(buffer, size, mEcmPids, &ecmSize);
        if (ecmSize > 0) {
            mCasHandle->processEcm(false, 0, buffer + ecmOffset, ecmSize);
            mFirstEcmWritten = true;
            MLOGI("first ECM written, offset:%d", mTsBuffer.size() + ecmOffset);
        } else {
            needBuffering = true;
        }
    }

    if (mState == STATE_PREPARING || needBuffering) {
        //is waiting for start_delay, writeData into mTsBuffer
        if (mTsBuffer.space() < size) {
            MLOGW("mTsBuffer full!");
            return -1;
        }
        written = mTsBuffer.put(buffer, size); //TODO: check buffer overflow
        if (mParser != nullptr) {
            written = mParser->writeData(buffer, size);
        }
    } else {
        //already start, need move data from mTsBuffer to player
        if (!mTsBuffer.empty() || mWriteBuffer->size() != 0) {
            if (drainDataFromBuffer_l() != 0) {
                return -1;
            }
        }

        written = doWriteData_l(buffer, size);
    }

    if (written == 0) {
        written = -1;
    }

    if (written > 0) {
        statisticWriteDataRate_l(written);
    }

    return written;
}

int AmlMpPlayerImpl::drainDataFromBuffer_l()
{
    int written = 0;
    int retry = 0;
    do {
        if (mWriteBuffer->size() == 0) {
            size_t readSize = mTsBuffer.get(mWriteBuffer->base(), mWriteBuffer->capacity());
            mWriteBuffer->setRange(0, readSize);
        }

        written = doWriteData_l(mWriteBuffer->data(), mWriteBuffer->size());

        if (written > 0) {
            mWriteBuffer->setRange(mWriteBuffer->offset()+written, mWriteBuffer->size()-written);
        } else {
            if (retry >= 4) {
                break;
            }
            retry++;
            usleep(50 * 1000);
        }
    } while (!mTsBuffer.empty() || mWriteBuffer->size() != 0);

    if (mTsBuffer.empty() && mWriteBuffer->size() == 0) {
        MLOGI("writeData from buffer done");
        return 0;
    }

    return -EAGAIN;
}

int AmlMpPlayerImpl::doWriteData_l(const uint8_t* buffer, size_t size)
{
    int written = 0;
    if (mCreateParams.drmMode == AML_MP_INPUT_STREAM_ENCRYPTED) {
        if (mCasHandle == nullptr || mWaitingEcmMode == kWaitingEcmASynchronous) {
            written = mPlayer->writeData(buffer, size);
        } else {
            size_t totalSize = size;
            size_t ecmOffset = size;
            size_t ecmSize = 188;
            int ecmCount = 0;

            while (size) {
                ecmOffset = findEcmPacket(buffer, size, mEcmPids, &ecmSize);
                ecmCount += ecmSize != 0;

                size_t partialSize = ecmOffset;
                int ret = 0;
                int retryCount = 0;
                do {
                    ret = mPlayer->writeData(buffer, partialSize);
                    if (ret <= 0) {
                        if (written == 0) {
                            goto exit;
                        }
                        usleep(50 * 1000);

                        ++retryCount;
                        if (retryCount%40 == 0) {
                            MLOGI("writeData %d/%d(%d), ecmOffset:%d(%d), return:%d", written, totalSize, size, ecmOffset, ecmCount, ret);
                        }
                    } else {
                        buffer += ret;
                        partialSize -= ret;
                        written += ret;
                        size -= ret;
                    }
                } while (partialSize);

                if (ecmSize > 0) {
                    mCasHandle->processEcm(false, 0, buffer, ecmSize);
                    buffer += ecmSize;
                    written += ecmSize;
                    size -= ecmSize;
                }
            }
        }
    } else if (mCreateParams.drmMode == AML_MP_INPUT_STREAM_SECURE_MEMORY) {
        written = mPlayer->writeData(buffer, size);
    } else {
        //normal stream
        written = mPlayer->writeData(buffer, size);
    }

exit:
    return written;
}

int AmlMpPlayerImpl::writeEsData(Aml_MP_StreamType type, const uint8_t* buffer, size_t size, int64_t pts)
{
    std::unique_lock<std::mutex> _l(mLock);
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->writeEsData(type, buffer, size, pts);
}

int AmlMpPlayerImpl::getCurrentPts(Aml_MP_StreamType streamType, int64_t* pts)
{
    std::unique_lock<std::mutex> _l(mLock);
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->getCurrentPts(streamType, pts);
}

int AmlMpPlayerImpl::getBufferStat(Aml_MP_BufferStat* bufferStat)
{
    std::unique_lock<std::mutex> _l(mLock);
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->getBufferStat(bufferStat);
}

int AmlMpPlayerImpl::setANativeWindow(ANativeWindow* nativeWindow)
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

int AmlMpPlayerImpl::setVideoWindow(int x, int y, int width, int height)
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

int AmlMpPlayerImpl::setVolume(float volume)
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);

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

int AmlMpPlayerImpl::getVolume(float* volume)
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);

    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->getVolume(volume);
}

int AmlMpPlayerImpl::showVideo()
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);

    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->showVideo();
}

int AmlMpPlayerImpl::hideVideo()
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);

    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->hideVideo();
}

int AmlMpPlayerImpl::showSubtitle()
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);

    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->showSubtitle();
}

int AmlMpPlayerImpl::hideSubtitle()
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);

    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->hideSubtitle();
}

int AmlMpPlayerImpl::setParameter(Aml_MP_PlayerParameterKey key, void* parameter)
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> lock(mLock);

    return setParameter_l(key, parameter, lock);
}

int AmlMpPlayerImpl::setParameter_l(Aml_MP_PlayerParameterKey key, void* parameter, std::unique_lock<std::mutex>& lock) {
    int ret = 0;

    switch (key) {
    case AML_MP_PLAYER_PARAMETER_VIDEO_DISPLAY_MODE:
        RETURN_IF(-1, parameter == nullptr);
        mVideoDisplayMode = *(Aml_MP_VideoDisplayMode*)parameter;
        break;

    case AML_MP_PLAYER_PARAMETER_BLACK_OUT:
        RETURN_IF(-1, parameter == nullptr);
        mBlackOut = *(bool*)parameter?1:0;
        break;

    case AML_MP_PLAYER_PARAMETER_VIDEO_DECODE_MODE:
        RETURN_IF(-1, parameter == nullptr);
        switchDecodeMode_l(*(Aml_MP_VideoDecodeMode*)parameter, lock);
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
    {
        RETURN_IF(-1, parameter == nullptr);
        int ADState = *(int*)parameter;
        if (ADState) {
            startADDecoding_l();
        } else {
            stopADDecoding_l(lock);
        }
        return 0;
    }

    case AML_MP_PLAYER_PARAMETER_AD_MIX_LEVEL:
        RETURN_IF(-1, parameter == nullptr);
        mADMixLevel = *(int*)parameter;
        break;

    case AML_MP_PLAYER_PARAMETER_WORK_MODE:
    {
        mWorkMode = *(Aml_MP_PlayerWorkMode*)parameter;
        MLOGI("Set work mode: %s", mpPlayerWorkMode2Str(mWorkMode));
    }
    break;

    case AML_MP_PLAYER_PARAMETER_VIDEO_WINDOW_ZORDER:
    {
        mZorder = *(int*)parameter;
        MLOGI("Set zorder: %d", mZorder);
#ifdef ANDROID
#ifndef __ANDROID_VNDK__
        if (mSurfaceControl != nullptr) {
            auto transcation = android::SurfaceComposerClient::Transaction();

            transcation.setLayer(mSurfaceControl, mZorder);

            transcation.apply();
        }
#endif
#endif
    }
    break;

    case AML_MP_PLAYER_PARAMETER_CREATE_PARAMS:
    {
        RETURN_IF(-1, parameter == nullptr);
        mCreateParams = *(Aml_MP_PlayerCreateParams *)parameter;
        MLOGI("Set mCreateParams drmmode:%s", mpInputStreamType2Str(mCreateParams.drmMode));
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
        MLOGI("set surface handle: %p", parameter);
        mSurfaceHandle = parameter;
    }
    break;

    case AML_MP_PLAYER_PARAMETER_AUDIO_PRESENTATION_ID:
    {
        mAudioPresentationId = *(int*)parameter;
    }
    break;

    case AML_MP_PLAYER_PARAMETER_USE_TIF:
    {
        RETURN_IF(-1, parameter == nullptr);
        mUseTif = *(bool*)parameter;
    }
    break;

    default:
        MLOGW("unhandled key: %s", mpPlayerParameterKey2Str(key));
        return ret;
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
    std::unique_lock<std::mutex> _l(mLock);

    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->getParameter(key, parameter);
}

int AmlMpPlayerImpl::setAVSyncSource(Aml_MP_AVSyncSource syncSource)
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);

    mSyncSource = syncSource;

    return 0;
}

int AmlMpPlayerImpl::setPcrPid(int pid)
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);

    mPcrPid = pid;

    return 0;
}

int AmlMpPlayerImpl::startVideoDecoding()
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);

    return startVideoDecoding_l();
}

int AmlMpPlayerImpl::startVideoDecoding_l()
{
    MLOG();

    if (mState == STATE_IDLE) {
        if (prepare_l() < 0) {
            MLOGE("prepare failed!");
            return -1;
        }
    }

    if (mState == STATE_PREPARING) {
        setStreamState_l(AML_MP_STREAM_TYPE_VIDEO, STREAM_STATE_START_PENDING);
        return 0;
    }

    if (mVideoParams.pid != AML_MP_INVALID_PID) {
        mPlayer->setVideoParams(&mVideoParams);
    }

    int ret = mPlayer->startVideoDecoding();
    if (ret < 0) {
        MLOGE("%s failed!", __FUNCTION__);
        return -1;
    }

    setState_l(STATE_RUNNING);
    if (mVideoParams.pid != AML_MP_INVALID_PID) {
        setStreamState_l(AML_MP_STREAM_TYPE_VIDEO, STREAM_STATE_STARTED);
    }

    MLOG("end");
    return ret;
}

int AmlMpPlayerImpl::stopVideoDecoding()
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> lock(mLock);

    MLOG();
    RETURN_IF(-1, mPlayer == nullptr);
    MLOGI("stopVideoDecoding\n");
    if (mState == STATE_RUNNING || mState == STATE_PAUSED) {
        if (mPlayer) {
            //release the lock in case of event handle thread try to acquire lock at stopping time.
            MLOGI("release lock!");
            lock.unlock();
            mPlayer->stopVideoDecoding();
            lock.lock();
            MLOGI("reacquire lock!");
        }
    }

    // ensure stream stopped if mState isn't running or paused
    setStreamState_l(AML_MP_STREAM_TYPE_VIDEO, STREAM_STATE_STOPPED);

    int ret = resetIfNeeded_l(lock);

    MLOG("end");
    return ret;
}

int AmlMpPlayerImpl::startAudioDecoding()
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);
    MLOGI("startAudioDecoding\n");
    return startAudioDecoding_l();
}

int AmlMpPlayerImpl::startAudioDecoding_l()
{
    MLOG();
    int ret;

    if (mState == STATE_IDLE) {
        if (prepare_l() < 0) {
            MLOGE("prepare failed!");
            return -1;
        }
    }

    if (mState == STATE_PREPARING) {
        setStreamState_l(AML_MP_STREAM_TYPE_AUDIO, STREAM_STATE_START_PENDING);
        return 0;
    }

    if (mAudioParams.pid == AML_MP_INVALID_PID) {
        return 0;
    }

    if (getStreamState_l(AML_MP_STREAM_TYPE_AD) == STREAM_STATE_STARTED) {
        resetADCodec_l(false);
    }

    mPlayer->setAudioParams(&mAudioParams);
    if (mAudioPresentationId > 0) {
        mPlayer->setParameter(AML_MP_PLAYER_PARAMETER_AUDIO_PRESENTATION_ID, &mAudioPresentationId);
    }

    ret = mPlayer->startAudioDecoding();
    if (ret < 0) {
        MLOGE("%s failed!", __FUNCTION__);
        return -1;
    }

    setState_l(STATE_RUNNING);

    setStreamState_l(AML_MP_STREAM_TYPE_AUDIO, STREAM_STATE_STARTED);

    MLOG("end");
    return ret;
}

int AmlMpPlayerImpl::stopAudioDecoding()
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> lock(mLock);

    return stopAudioDecoding_l(lock);
}

int AmlMpPlayerImpl::stopAudioDecoding_l(std::unique_lock<std::mutex>& lock)
{
    MLOG();
    int ret;
    RETURN_IF(-1, mPlayer == nullptr);

    mAudioPresentationId = 0;
    if (mState == STATE_RUNNING || mState == STATE_PAUSED) {
        if (mPlayer) {
            mPlayer->stopAudioDecoding();

            Aml_MP_AudioParams dummyAudioParam{AML_MP_INVALID_PID, AML_MP_CODEC_UNKNOWN};
            mPlayer->setAudioParams(&dummyAudioParam);
        }

        if (getStreamState_l(AML_MP_STREAM_TYPE_AD) == STREAM_STATE_STARTED) {
            resetADCodec_l(true);
        }
    }

    // ensure stream stopped if mState isn't running or paused
    setStreamState_l(AML_MP_STREAM_TYPE_AUDIO, STREAM_STATE_STOPPED);

    ret = resetIfNeeded_l(lock);

    MLOG("end");
    return ret;
}

int AmlMpPlayerImpl::startADDecoding()
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);

    return startADDecoding_l();
}

int AmlMpPlayerImpl::startADDecoding_l()
{
    MLOG();
    int ret;

    if (mState == STATE_IDLE) {
        if (prepare_l() < 0) {
            MLOGE("prepare failed!");
            return -1;
        }
    }

    if (mState == STATE_PREPARING) {
        setStreamState_l(AML_MP_STREAM_TYPE_AD, STREAM_STATE_START_PENDING);
        return 0;
    }

    if (mADParams.pid == AML_MP_INVALID_PID) {
        return 0;
    }

    if (getStreamState_l(AML_MP_STREAM_TYPE_AUDIO) == STREAM_STATE_STARTED) {
        resetAudioCodec_l(false);
    }

    mPlayer->setADParams(&mADParams, true);

    ret = mPlayer->startADDecoding();
    if (ret < 0) {
        MLOGE("%s failed!", __FUNCTION__);
        return -1;
    }

    setState_l(STATE_RUNNING);

    setStreamState_l(AML_MP_STREAM_TYPE_AD, STREAM_STATE_STARTED);

    MLOG("end");
    return ret;
}

int AmlMpPlayerImpl::stopADDecoding()
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> lock(mLock);

    return stopADDecoding_l(lock);
}

int AmlMpPlayerImpl::stopADDecoding_l(std::unique_lock<std::mutex>& lock)
{
    MLOG();
    RETURN_IF(-1, mPlayer == nullptr);
    int ret;

    if (mState == STATE_RUNNING || mState == STATE_PAUSED) {
        if (mPlayer) {
            mPlayer->stopADDecoding();

            Aml_MP_AudioParams dummyADParam{AML_MP_INVALID_PID, AML_MP_CODEC_UNKNOWN};
            mPlayer->setADParams(&dummyADParam, false);
        }

        if (getStreamState_l(AML_MP_STREAM_TYPE_AUDIO) == STREAM_STATE_STARTED) {
            resetAudioCodec_l(true);
        }
    }

    // ensure stream stopped if mState isn't running or paused
    setStreamState_l(AML_MP_STREAM_TYPE_AD, STREAM_STATE_STOPPED);

    ret = resetIfNeeded_l(lock);

    MLOG("end");
    return ret;
}

int AmlMpPlayerImpl::startSubtitleDecoding()
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);

    return startSubtitleDecoding_l();
}

int AmlMpPlayerImpl::startSubtitleDecoding_l()
{
    MLOG();
    RETURN_IF(-1, mPlayer == nullptr);

    if (mState == STATE_IDLE) {
        if (prepare_l() < 0) {
            MLOGE("prepare failed!");
            return -1;
        }
    }

    if (mState == STATE_PREPARING) {
        setStreamState_l(AML_MP_STREAM_TYPE_SUBTITLE, STREAM_STATE_START_PENDING);
        return 0;
    }

    if (mSubtitleParams.subtitleCodec != AML_MP_CODEC_UNKNOWN) {
        mPlayer->setSubtitleParams(&mSubtitleParams);
    }

    int ret = mPlayer->startSubtitleDecoding();
    if (ret < 0) {
        MLOGE("%s failed!", __FUNCTION__);
        return -1;
    }

    setState_l(STATE_RUNNING);
    if (mSubtitleParams.subtitleCodec != AML_MP_CODEC_UNKNOWN) {
        setStreamState_l(AML_MP_STREAM_TYPE_SUBTITLE, STREAM_STATE_STARTED);
    }

    MLOG("end");
    return ret;
}

int AmlMpPlayerImpl::stopSubtitleDecoding()
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> lock(mLock);

    return stopSubtitleDecoding_l(lock);
}

int AmlMpPlayerImpl::stopSubtitleDecoding_l(std::unique_lock<std::mutex>& lock) {
    MLOG();
    RETURN_IF(-1, mPlayer == nullptr);

    if (mState == STATE_RUNNING || mState == STATE_PAUSED) {
        if (mPlayer) {
            mPlayer->stopSubtitleDecoding();
        }
    }

    // ensure stream stopped if mState isn't running or paused
    setStreamState_l(AML_MP_STREAM_TYPE_SUBTITLE, STREAM_STATE_STOPPED);

    int ret = resetIfNeeded_l(lock);

    MLOG("end");
    return ret;
}

int AmlMpPlayerImpl::setSubtitleWindow(int x, int y, int width, int height)
{
    AML_MP_TRACE(10);
    std::unique_lock<std::mutex> _l(mLock);
    MLOG("subtitle window:(%d %d %d %d)", x, y, width, height);

    mSubtitleWindow = {x, y, width, height};
    int ret = 0;

    if (mState == STATE_RUNNING || mState == STATE_PAUSED) {
        ret = mPlayer->setSubtitleWindow(x, y, width, height);
    }

    return ret;
}

//internal function
int AmlMpPlayerImpl::startDescrambling_l()
{
    AML_MP_TRACE(10);

    if (mCasServiceType == AML_MP_CAS_SERVICE_TYPE_INVALID) {
        MLOGE("unknown cas type!");
        return -1;
    }

    if (mCasHandle == nullptr) {
        mCasHandle = AmlCasBase::create(mCasServiceType);
    }

    if (mCasHandle == nullptr) {
        MLOGE("create CAS handle failed!");
        return -1;
    }

    int ret = mCasHandle->startDescrambling(&mIptvCasParams);
    mCasHandle->getEcmPids(mEcmPids);

    return ret;
}

//internal function
int AmlMpPlayerImpl::stopDescrambling_l()
{
    AML_MP_TRACE(10);

    if (mCasHandle) {
        mCasHandle->stopDescrambling();
        mCasHandle.clear();
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
const char* AmlMpPlayerImpl::stateString(State state)
{
    switch (state) {
    case STATE_IDLE:
        return "STATE_IDLE";
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
    }
}

std::string AmlMpPlayerImpl::streamStateString(uint32_t streamState)
{
    AML_MP_UNUSED(streamState);
    std::stringstream ss;
    bool hasValue = false;

    for (size_t i = 0; i < AML_MP_STREAM_TYPE_NB; ++i) {
        int value = (mStreamState >> i*kStreamStateBits) & kStreamStateMask;

        if (value != STREAM_STATE_STOPPED) {
            if (hasValue) ss << "|";

            ss << mpStreamType2Str((Aml_MP_StreamType)i);
            switch (value) {
            case STREAM_STATE_START_PENDING:
                ss << "_START_PENDING";
                break;

            case STREAM_STATE_STARTED:
                ss << "_STARTED";
                break;

            default:
                break;
            }
            hasValue = true;
        }
    }

    return ss.str();
}

void AmlMpPlayerImpl::setState_l(State state)
{
    if (mState != state) {
        MLOGI("%s -> %s", stateString(mState), stateString(state));
        mState = state;
    }
}

void AmlMpPlayerImpl::setStreamState_l(Aml_MP_StreamType streamType, int state)
{
    int offset = streamType * kStreamStateBits;

    if (offset > sizeof(mStreamState)*8) {
        MLOGE("streamType(%s) is overflow!", mpStreamType2Str(streamType));
        return;
    }

    mStreamState &= ~(kStreamStateMask<<offset);
    mStreamState |= state<<offset;
}

AmlMpPlayerImpl::StreamState AmlMpPlayerImpl::getStreamState_l(Aml_MP_StreamType streamType)
{
    int offset = streamType * kStreamStateBits;

    return StreamState((mStreamState >> offset) & kStreamStateMask);
}

int AmlMpPlayerImpl::prepare_l()
{
    MLOG();

    if (mCreateParams.drmMode != AML_MP_INPUT_STREAM_NORMAL && !mIsStandaloneCas) {
        startDescrambling_l();
    }

    if (mPlayer == nullptr) {
        mPlayer = AmlPlayerBase::create(&mCreateParams, mInstanceId);
    }

    if (mPlayer == nullptr) {
        MLOGE("AmlPlayerBase create failed!");
        return -1;
    }

    MLOGI("mWorkMode: %s", mpPlayerWorkMode2Str(mWorkMode));
    mPlayer->setParameter(AML_MP_PLAYER_PARAMETER_WORK_MODE, &mWorkMode);

    mPlayer->registerEventCallback([](void* userData, Aml_MP_PlayerEventType event, int64_t param) {
        AmlMpPlayerImpl* thiz = static_cast<AmlMpPlayerImpl*>(userData);
        return thiz->notifyListener(event, param);
    }, this);

    if ((mSubtitleWindow.width > 0) && (mSubtitleWindow.height > 0)) {
        mPlayer->setSubtitleWindow(mSubtitleWindow.x, mSubtitleWindow.y, mSubtitleWindow.width, mSubtitleWindow.height);
    }

    if (mSurfaceHandle != nullptr) {
        mPlayer->setParameter(AML_MP_PLAYER_PARAMETER_SURFACE_HANDLE, mSurfaceHandle);
    }
    #ifdef ANDROID
    MLOGI("mNativeWindow:%p", mNativeWindow.get());
    if (mNativeWindow != nullptr) {
        mPlayer->setANativeWindow(mNativeWindow.get());
    }
    if (!mNativeWindow.get() && mVideoWindow.width > 0 && mVideoWindow.height > 0) {
        mPlayer->setVideoWindow(mVideoWindow.x, mVideoWindow.y, mVideoWindow.width, mVideoWindow.height);
    }
    #else
    //direct set in yocto
    if (mVideoWindow.width > 0 && mVideoWindow.height > 0) {
        mPlayer->setVideoWindow(mVideoWindow.x, mVideoWindow.y, mVideoWindow.width, mVideoWindow.height);
    }
    #endif
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

    if (mPlaybackRate > 0.0f && mPlaybackRate < FAST_PLAY_THRESHOLD) {
        mPlayer->setPlaybackRate(mPlaybackRate);
    }

    if (mVolume >= 0) {
        mPlayer->setVolume(mVolume);
    }

    applyParameters_l();

    mPrepareWaitingType = kPrepareWaitingNone;

    if (mCreateParams.sourceType == AML_MP_INPUT_SOURCE_TS_MEMORY &&
        mCreateParams.drmMode == AML_MP_INPUT_STREAM_ENCRYPTED &&
        mWaitingEcmMode == kWaitingEcmASynchronous &&
        mCasHandle) {
        mPrepareWaitingType |= kPrepareWaitingEcm;
    }

    if ((mVideoParams.videoCodec == AML_MP_CODEC_UNKNOWN && mVideoParams.pid != AML_MP_INVALID_PID) ||
        (mAudioParams.audioCodec == AML_MP_CODEC_UNKNOWN && mAudioParams.pid != AML_MP_INVALID_PID) ||
        (mSubtitleParams.subtitleCodec == AML_MP_CODEC_UNKNOWN && mSubtitleParams.pid != AML_MP_INVALID_PID) ||
        (mADParams.audioCodec == AML_MP_CODEC_UNKNOWN && mADParams.pid != AML_MP_INVALID_PID)) {
        mPrepareWaitingType |= kPrepareWaitingCodecId;
    }

    if (mPrepareWaitingType == kPrepareWaitingNone) {
        setState_l(STATE_PREPARED);
    } else {
        setState_l(STATE_PREPARING);
    }

    if (mParser == nullptr && mPrepareWaitingType != kPrepareWaitingNone) {
        mParser = new Parser(mCreateParams.demuxId, mCreateParams.sourceType == AML_MP_INPUT_SOURCE_TS_DEMOD, true);
        mParser->setProgram(mVideoParams.pid, mAudioParams.pid);
        mParser->setEventCallback([this] (Parser::ProgramEventType event, int param1, int param2, void* data) {
                return programEventCallback(event, param1, param2, data);
        });

        if (mPrepareWaitingType == kPrepareWaitingEcm) {
            mParser->open(false /*autoParsing*/);

            int lastEcmPid = AML_MP_INVALID_PID;
            for (size_t i = 0; i < mEcmPids.size(); ++i) {
                int ecmPid = mEcmPids[i];
                if (ecmPid <= 0 ||ecmPid >= AML_MP_INVALID_PID) {
                    continue;
                }

                if (ecmPid != lastEcmPid) {
                    mParser->addSectionFilter(ecmPid, Parser::ecmCb, false);
                    lastEcmPid = ecmPid;
                }
            }

            if (lastEcmPid == AML_MP_INVALID_PID) {
                MLOGE("no valid ecm pid!");
            }
        } else {
            mParser->open();
        }
    }

    return 0;
}

void AmlMpPlayerImpl::programEventCallback(Parser::ProgramEventType event, int param1, int param2, void* data)
{
    AML_MP_UNUSED(param1);
    switch (event)
    {
        case Parser::ProgramEventType::EVENT_PROGRAM_PARSED:
        {
            ProgramInfo* programInfo = (ProgramInfo*)data;
            MLOGI("programEventCallback: program(programNumber=%d,pid= 0x%x) parsed", programInfo->programNumber, programInfo->pmtPid);
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

            mPrepareWaitingType &= ~kPrepareWaitingCodecId;
            finishPreparingIfNeeded_l();

            break;
        }
        case Parser::ProgramEventType::EVENT_AV_PID_CHANGED:
        {
            Aml_MP_PlayerEventPidChangeInfo* info = (Aml_MP_PlayerEventPidChangeInfo*)data;
            MLOGI("programEventCallback: program(programNumber=%d,pid= 0x%x) pidchangeInfo: oldPid: 0x%x --> newPid: 0x%x",
                info->programNumber, info->programPid, info->oldStreamPid, info->newStreamPid);
            notifyListener(AML_MP_PLAYER_EVENT_PID_CHANGED, (uint64_t)data);
            break;
        }
        case Parser::ProgramEventType::EVENT_ECM_DATA_PARSED:
        {
            MLOGI("programEventCallback: ecmData parsed, size:%d", param2);
            uint8_t* ecmData = (uint8_t*)data;
            std::string ecmDataStr;
            char hex[3];
            for (int i = 0; i < param2; i++) {
                 snprintf(hex, sizeof(hex), "%02X", ecmData[i]);
                 ecmDataStr.append(hex);
                 ecmDataStr.append(" ");
            }
            MLOGI("programEventCallback: ecmPid: %d, ecmData: size:%d, hexStr:%s", param1, param2, ecmDataStr.c_str());

            {
                std::unique_lock<std::mutex> _l(mLock);
                if (mCasHandle && mWaitingEcmMode == kWaitingEcmASynchronous) {
                    mCasHandle->processEcm(true, param1, ecmData, param2);
                }

                mPrepareWaitingType &= ~kPrepareWaitingEcm;
                finishPreparingIfNeeded_l();
            }
        }
    }
}

int AmlMpPlayerImpl::finishPreparingIfNeeded_l()
{
    if (mState != STATE_PREPARING || mPrepareWaitingType != kPrepareWaitingNone) {
        return 0;
    }

    setState_l(STATE_PREPARED);

    if (getStreamState_l(AML_MP_STREAM_TYPE_VIDEO) == STREAM_STATE_START_PENDING) {
        startVideoDecoding_l();
    }

    if (getStreamState_l(AML_MP_STREAM_TYPE_AUDIO) == STREAM_STATE_START_PENDING) {
        startAudioDecoding_l();
    }

    if (getStreamState_l(AML_MP_STREAM_TYPE_SUBTITLE) == STREAM_STATE_START_PENDING) {
        startSubtitleDecoding_l();
    }

    if (getStreamState_l(AML_MP_STREAM_TYPE_AD) == STREAM_STATE_START_PENDING) {
        startADDecoding_l();
    }

    return 0;
}

int AmlMpPlayerImpl::resetIfNeeded_l(std::unique_lock<std::mutex>& lock)
{
    if (mStreamState == 0) {
        setState_l(STATE_STOPPED);
    } else {
        MLOGI("current streamState:%s", streamStateString(mStreamState).c_str());
    }

    if (mState == STATE_STOPPED) {
        reset_l(lock);
    }

    return 0;
}

int AmlMpPlayerImpl::reset_l(std::unique_lock<std::mutex>& lock)
{
    MLOG();
    mTsBuffer.reset();
    mWriteBuffer->setRange(0, 0);
    if (mParser) {
        lock.unlock();
        mParser->close();
        lock.lock();

    }

    mParser.clear();
    mPlayer.clear();

    if (!mIsStandaloneCas) {
        stopDescrambling_l();
    }
    mIsStandaloneCas = false;
    mCasHandle.clear();

    resetVariables_l();

    setState_l(STATE_IDLE);

    return 0;
}

int AmlMpPlayerImpl::applyParameters_l()
{
    mPlayer->setParameter(AML_MP_PLAYER_PARAMETER_VIDEO_DISPLAY_MODE, &mVideoDisplayMode);
    if (mBlackOut >= 0) {
        bool param = (0 == mBlackOut)?false:true;
        mPlayer->setParameter(AML_MP_PLAYER_PARAMETER_BLACK_OUT, &param);
    }
    mPlayer->setParameter(AML_MP_PLAYER_PARAMETER_VIDEO_DECODE_MODE, &mVideoDecodeMode);
    mPlayer->setParameter(AML_MP_PLAYER_PARAMETER_VIDEO_PTS_OFFSET, &mVideoPtsOffset);
    mPlayer->setParameter(AML_MP_PLAYER_PARAMETER_AUDIO_OUTPUT_MODE, &mAudioOutputMode);
    mPlayer->setParameter(AML_MP_PLAYER_PARAMETER_AUDIO_OUTPUT_DEVICE, &mAudioOutputDevice);
    mPlayer->setParameter(AML_MP_PLAYER_PARAMETER_AUDIO_PTS_OFFSET, &mAudioPtsOffset);
    mPlayer->setParameter(AML_MP_PLAYER_PARAMETER_AUDIO_BALANCE, &mAudioBalance);
    mPlayer->setParameter(AML_MP_PLAYER_PARAMETER_AUDIO_MUTE, &mAudioMute);
    mPlayer->setParameter(AML_MP_PLAYER_PARAMETER_NETWORK_JITTER, &mNetworkJitter);

    if (mADMixLevel != -1) {
        mPlayer->setParameter(AML_MP_PLAYER_PARAMETER_AD_MIX_LEVEL, &mADMixLevel);
    }

    if (mUseTif != -1) {
        mPlayer->setParameter(AML_MP_PLAYER_PARAMETER_USE_TIF, &mUseTif);
    }

    return 0;
}

void AmlMpPlayerImpl::notifyListener(Aml_MP_PlayerEventType eventType, int64_t param)
{
    std::unique_lock<std::mutex> _l(mEventLock);
    if (mEventCb) {
        mEventCb(mUserData, eventType, param);
    } else {
        MLOGW("mEventCb is NULL, eventType: %s, param:%lld", mpPlayerEventType2Str(eventType), param);
    }
}

int AmlMpPlayerImpl::resetADCodec_l(bool callStart)
{
    int ret = 0;
    MLOG("callStart:%d", callStart);

    ret = mPlayer->stopADDecoding();
    if (ret < 0) {
        MLOGW("stopADDecoding failed while resetADCodec");
    }

    if (mADParams.pid != AML_MP_INVALID_PID) {
        mPlayer->setADParams(&mADParams, true);
    }

    if (callStart) {
        ret = mPlayer->startADDecoding();
    }

    return ret;
}

int AmlMpPlayerImpl::resetAudioCodec_l(bool callStart)
{
    int ret = 0;
    MLOG("callStart:%d", callStart);

    ret = mPlayer->stopAudioDecoding();
    if (ret < 0) {
        MLOGW("stopAudioDecoding failed while resetAudioCodec");
    }

    if (mAudioParams.pid != AML_MP_INVALID_PID) {
        mPlayer->setAudioParams(&mAudioParams);
        if (mAudioPresentationId > 0) {
            mPlayer->setParameter(AML_MP_PLAYER_PARAMETER_AUDIO_PRESENTATION_ID, &mAudioPresentationId);
        }
    }

    if (callStart) {
        ret = mPlayer->startAudioDecoding();
    }

    return ret;
}

int AmlMpPlayerImpl::switchDecodeMode_l(Aml_MP_VideoDecodeMode decodeMode, std::unique_lock<std::mutex>& lock) {
    AML_MP_TRACE(10);
    int ret = 0;
    MLOG("decodeMode: %s", mpVideoDecideMode2Str(decodeMode));

    if (mVideoDecodeMode == decodeMode) {
        MLOGI("Decode mode not change, no need do process");
        return 0;
    }

    mVideoDecodeMode = decodeMode;

    if (mState != STATE_RUNNING) {
        MLOGI("Now not in play, no Need to do process");
        return 0;
    }

    ret = stop_l(lock);
    ret += mPlayer->setParameter(AML_MP_PLAYER_PARAMETER_VIDEO_DECODE_MODE, &mVideoDecodeMode);
    if (AML_MP_VIDEO_DECODE_MODE_IONLY == mVideoDecodeMode) {
        ret += startVideoDecoding_l();
    } else {
        ret += start_l();
    }

    return ret;
}

void AmlMpPlayerImpl::statisticWriteDataRate_l(size_t size)
{
    mLastBytesWritten += size;

    int64_t nowUs = AmlMpEventLooper::GetNowUs();
    if (mLastWrittenTimeUs == 0) {
        mLastWrittenTimeUs = nowUs;
    } else {
        int64_t diffUs = nowUs - mLastWrittenTimeUs;
        if (diffUs > 2 * 1000000ll) {
            int64_t bitrate = mLastBytesWritten * 1000000 / diffUs;
            MLOGI("writeData rate:%.2fKB/s", bitrate/1024.0);

            mLastWrittenTimeUs = nowUs;
            mLastBytesWritten = 0;

            collectBuffingInfos_l();
        }
    }
}

void AmlMpPlayerImpl::collectBuffingInfos_l()
{
    Aml_MP_BufferStat bufferStat;
    mPlayer->getBufferStat(&bufferStat);

    int64_t vpts, apts;
    mPlayer->getCurrentPts(AML_MP_STREAM_TYPE_VIDEO, &vpts);
    mPlayer->getCurrentPts(AML_MP_STREAM_TYPE_AUDIO, &apts);

    if (mVideoParams.pid != AML_MP_INVALID_PID) {
        MLOGI("Video(%#x) buffer stat:%d/%d, %.2fms, pts:%f", mVideoParams.pid, bufferStat.videoBuffer.dataLen, bufferStat.videoBuffer.size, bufferStat.videoBuffer.bufferedMs*1.0, vpts/1e6);
    }

    if (mAudioParams.pid != AML_MP_INVALID_PID) {
        MLOGI("Audio(%#x) buffer stat:%d/%d, %.2fms, pts:%f", mAudioParams.pid, bufferStat.audioBuffer.dataLen, bufferStat.audioBuffer.size, bufferStat.audioBuffer.bufferedMs*1.0, apts/1e6);
    }
}

void AmlMpPlayerImpl::resetVariables_l()
{
    mFirstEcmWritten = false;

    mLastBytesWritten = 0;
    mLastWrittenTimeUs = 0;
}

///////////////////////////////////////////////////////////////////////////////
}
