/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_TAG "AmlMpPlayerDemo_DVRPlayback"
#include <utils/AmlMpLog.h>
#include "DVRPlayback.h"

static const char* mName = LOG_TAG;

namespace aml_mp {
DVRPlayback::DVRPlayback(const std::string& url, bool cryptoMode, Aml_MP_DemuxId demuxId)
: mUrl(stripUrlIfNeeded(url))
, mCryptoMode(cryptoMode)
, mDemuxId(demuxId)
{
    MLOG();

    Aml_MP_DVRPlayerCreateParams createParams;
    memset(&createParams, 0, sizeof(createParams));
    createParams.basicParams.demuxId = mDemuxId;
    strncpy(createParams.basicParams.location, mUrl.c_str(), AML_MP_MAX_PATH_SIZE);
    createParams.basicParams.blockSize = 188 * 1024;
    createParams.basicParams.isTimeShift = false;
    createParams.basicParams.drmMode = AML_MP_INPUT_STREAM_NORMAL;

    MLOGI("mCryptoMode:%d", mCryptoMode);
    if (mCryptoMode) {
        if (initDVRDecryptPlayback(createParams.decryptParams) >= 0) {
            createParams.basicParams.blockSize = 256 * 1024;
            createParams.basicParams.drmMode = AML_MP_INPUT_STREAM_ENCRYPTED;
        }
    }

    int ret = Aml_MP_DVRPlayer_Create(&createParams, &mPlayer);
    if (ret < 0) {
        MLOGE("create dvr player failed!");
        return;
    }
}

DVRPlayback::~DVRPlayback()
{
    MLOG();

    if (mPlayer != AML_MP_INVALID_HANDLE) {
        Aml_MP_DVRPlayer_Destroy(mPlayer);
        mPlayer = AML_MP_INVALID_HANDLE;
    }

    MLOGI("dtor dvr playback end!");
}

#ifdef ANDROID

void DVRPlayback::setANativeWindow(const android::sp<ANativeWindow>& window)
{
    MLOG("setANativeWindow %p", window.get());
    Aml_MP_DVRPlayer_SetANativeWindow(mPlayer, window.get());
}
#endif

void DVRPlayback::registerEventCallback(Aml_MP_PlayerEventCallback cb, void* userData)
{
    mEventCallback = cb;
    mUserData = userData;
}

int DVRPlayback::start()
{
    Aml_MP_DVRPlayer_RegisterEventCallback(mPlayer, mEventCallback, mUserData);

    uint32_t segments;
    uint64_t* pSegmentIds = nullptr;
    int segmentIndex = 0;
    Aml_MP_DVRSegmentInfo segmentInfo;
    int ret = Aml_MP_DVRRecorder_GetSegmentList(mUrl.c_str(), &segments, &pSegmentIds);
    if (ret < 0) {
        MLOGE("getSegmentList for %s failed with %d", mUrl.c_str(), ret);
        return -1;
    }

    ret = Aml_MP_DVRRecorder_GetSegmentInfo(mUrl.c_str(), pSegmentIds[segmentIndex], &segmentInfo);
    if (ret < 0) {
        MLOGE("getSegmentInfo failed with %d", ret);
        goto exit;
    }

    Aml_MP_DVRStreamArray streams;
    memset(&streams, 0, sizeof(streams));
    for (int i = 0; i < segmentInfo.streams.nbStreams; ++i) {
        switch (segmentInfo.streams.streams[i].type) {
        case AML_MP_STREAM_TYPE_VIDEO:
            streams.streams[AML_MP_DVR_VIDEO_INDEX].type = AML_MP_STREAM_TYPE_VIDEO;
            streams.streams[AML_MP_DVR_VIDEO_INDEX].codecId = segmentInfo.streams.streams[i].codecId;
            streams.streams[AML_MP_DVR_VIDEO_INDEX].pid = segmentInfo.streams.streams[i].pid;
            break;

        case AML_MP_STREAM_TYPE_AUDIO:
            streams.streams[AML_MP_DVR_AUDIO_INDEX].type = AML_MP_STREAM_TYPE_AUDIO;
            streams.streams[AML_MP_DVR_AUDIO_INDEX].codecId = segmentInfo.streams.streams[i].codecId;
            streams.streams[AML_MP_DVR_AUDIO_INDEX].pid = segmentInfo.streams.streams[i].pid;
            break;

        default:
            break;
        }
    }

    ret = Aml_MP_DVRPlayer_SetStreams(mPlayer, &streams);
    if (ret < 0) {
        MLOGE("dvr player set streams failed with %d", ret);
        goto exit;
    }

    ret = Aml_MP_DVRPlayer_Start(mPlayer, false);
    if (ret < 0) {
        MLOGE("dvr player start failed with %d", ret);
        goto exit;
    }

exit:
    if (pSegmentIds) {
        ::free(pSegmentIds);
        pSegmentIds = nullptr;
    }
    return 0;
}

int DVRPlayback::stop()
{
    MLOG();

    if (mPlayer != AML_MP_INVALID_HANDLE) {
        Aml_MP_DVRPlayer_Stop(mPlayer);
    }

    if (mCryptoMode) {
        uninitDVRDecryptPlayback();
    }

    MLOG();
    return 0;
}

std::string DVRPlayback::stripUrlIfNeeded(const std::string& url) const
{
    std::string result = url;

    auto suffix = result.rfind(".ts");
    if (suffix != result.npos) {
        auto hyphen = result.find_last_of('-', suffix);
        if (hyphen != result.npos) {
            result = result.substr(0, hyphen);
        }
    }

    result.erase(0, strlen("dvr://"));
    //for (;;) {
        //auto it = ++result.begin();
        //if (*it != '/') {
            //break;
        //}
        //result.erase(it);
    //}

    MLOGI("result str:%s", result.c_str());
    return result;
}

int DVRPlayback::initDVRDecryptPlayback(Aml_MP_DVRPlayerDecryptParams& decryptParams)
{
    #ifdef ANDROID
    MLOG();

    Aml_MP_CAS_Initialize();

    int ret = Aml_MP_CAS_OpenSession(&mCasSession, AML_MP_CAS_SERVICE_PVR_PLAY);
    if (ret < 0) {
        MLOGE("openSession failed! ret:%d", ret);
        return ret;
    }

    Aml_MP_CASDVRReplayParams params;
    params.dmxDev = mDemuxId;

    ret = Aml_MP_CAS_StartDVRReplay(mCasSession, &params);
    if (ret < 0) {
        MLOGE("start dvr replay failed with %d", ret);
        return ret;
    }

    decryptParams.cryptoData = mCasSession;
    decryptParams.cryptoFn = [](Aml_MP_CASCryptoParams* params, void* userdata) {
        AML_MP_CASSESSION casSession = (AML_MP_CASSESSION)userdata;
        return Aml_MP_CAS_DVRDecrypt(casSession, params);
    };

    uint8_t* secBuf = nullptr;
    uint32_t secBufSize;
    mSecMem = Aml_MP_CAS_CreateSecmem(mCasSession, AML_MP_CAS_SERVICE_PVR_PLAY, (void**)&secBuf, &secBufSize);
    if (mSecMem == nullptr) {
        MLOGE("create secMem failed");
        return -1;
    }

    decryptParams.secureBuffer = secBuf;
    decryptParams.secureBufferSize = secBufSize;
    #endif
    return 0;
}

int DVRPlayback::uninitDVRDecryptPlayback()
{
    #ifdef ANDROID
    MLOG("mCasSession:%p", mCasSession);
    if (mCasSession == AML_MP_INVALID_HANDLE) {
        return 0;
    }

    Aml_MP_CAS_DestroySecmem(mCasSession, mSecMem);
    mSecMem = nullptr;

    Aml_MP_CAS_StopDVRReplay(mCasSession);

    Aml_MP_CAS_CloseSession(mCasSession);
    mCasSession = nullptr;
    #endif
    return 0;
}

void DVRPlayback::signalQuit()
{
    MLOGI("signalQuit!");
}

///////////////////////////////////////////////////////////////////////////////
static struct TestModule::Command g_commandTable[] = {

    {nullptr, 0, nullptr, nullptr}
};

const TestModule::Command* DVRPlayback::getCommandTable() const
{
    return g_commandTable;
}

void* DVRPlayback::getCommandHandle() const
{
    return nullptr;
}



}
