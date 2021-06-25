/*
 * Copyright (c) 2021 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_TAG "AmlMpPlayerDemo_DVRRecord"
#include <utils/AmlMpLog.h>
#include "DVRRecord.h"
#include <Aml_MP/Dvr.h>
#include "TestUtils.h"


static const char* mName = LOG_TAG;

namespace aml_mp {
DVRRecord::DVRRecord(bool cryptoMode, Aml_MP_DemuxId demuxId, const sptr<ProgramInfo>& programInfo)
: mCryptoMode(cryptoMode)
, mDemuxId(demuxId)
, mProgramInfo(programInfo)
{
    Aml_MP_DVRRecorderCreateParams createParams;
    memset(&createParams, 0, sizeof(createParams));
    createParams.basicParams.demuxId = mDemuxId;
    snprintf(createParams.basicParams.location, AML_MP_MAX_PATH_SIZE, AML_MP_TEST_SUPPORTER_RECORD_FILE);
    createParams.basicParams.segmentSize = 100 * 1024 * 1024;
    createParams.basicParams.isTimeShift = false;

    MLOGI("mCryptoMode:%d", mCryptoMode);
    if (mCryptoMode) {
        initDVREncryptRecord(createParams.encryptParams);
        createParams.basicParams.flags = Aml_MP_DVRRecorderFlag(createParams.basicParams.flags | AML_MP_DVRRECORDER_SCRAMBLED);
    }

    int ret = Aml_MP_DVRRecorder_Create(&createParams, &mRecorder);
    if (ret < 0) {
        MLOGE("create dvr recorder failed! ret=%d", ret);
    }
}

DVRRecord::~DVRRecord()
{
    stop();

    if (mRecorder != AML_MP_INVALID_HANDLE) {
        Aml_MP_DVRRecorder_Destroy(mRecorder);
        mRecorder = AML_MP_INVALID_HANDLE;
    }

    MLOGI("dtor dvr record end!");
}

int DVRRecord::start()
{
    Aml_MP_DVRStreamArray streams;
    memset(&streams, 0, sizeof(streams));
    int index = 0;
    Aml_MP_DVRStream* st = nullptr;

    st = &streams.streams[index++];
    if (mProgramInfo->videoPid != AML_MP_INVALID_PID) {
        st->type = AML_MP_STREAM_TYPE_VIDEO;
        st->pid = mProgramInfo->videoPid;
        st->codecId = mProgramInfo->videoCodec;
    }

    st = &streams.streams[index++];
    if (mProgramInfo->audioPid != AML_MP_INVALID_PID) {
        st->type = AML_MP_STREAM_TYPE_AUDIO;
        st->pid = mProgramInfo->audioPid;
        st->codecId = mProgramInfo->audioCodec;
    }

    streams.nbStreams = index;

    int ret = Aml_MP_DVRRecorder_SetStreams(mRecorder, &streams);
    if (ret < 0) {
        MLOGE("set streams failed with %d", ret);
        return ret;
    }

    ret = Aml_MP_DVRRecorder_Start(mRecorder);
    if (ret < 0) {
        MLOGE("start recorder failed with %d", ret);
    }

    return 0;
}

int DVRRecord::stop()
{
    MLOG();

    if (mRecorder != AML_MP_INVALID_HANDLE) {
        Aml_MP_DVRRecorder_Stop(mRecorder);
    }

    if (mCryptoMode) {
        uninitDVREncryptRecord();
    }

    return 0;
}

void DVRRecord::signalQuit()
{
    MLOGI("signalQuit!");
}

int DVRRecord::initDVREncryptRecord(Aml_MP_DVRRecorderEncryptParams& encryptParams)
{
    #ifdef ANDROID
    MLOG();

    Aml_MP_CAS_Initialize();

    int ret = Aml_MP_CAS_OpenSession(&mCasSession, AML_MP_CAS_SERVICE_PVR_RECORDING);
    if (ret < 0) {
        MLOGE("openSession failed! ret:%d", ret);
        return ret;
    }

    Aml_MP_CASServiceInfo casServiceInfo;
    memset(&casServiceInfo, 0, sizeof(casServiceInfo));
    casServiceInfo.service_id = mProgramInfo->serviceNum;
    casServiceInfo.dmx_dev = mDemuxId;
    casServiceInfo.serviceMode = AML_MP_CAS_SERVICE_DVB;
    casServiceInfo.serviceType = AML_MP_CAS_SERVICE_PVR_RECORDING;
    casServiceInfo.ecm_pid = mProgramInfo->ecmPid[0];
    casServiceInfo.stream_pids[0] = mProgramInfo->audioPid;
    casServiceInfo.stream_pids[1] = mProgramInfo->videoPid;
    casServiceInfo.stream_num = 2;
    casServiceInfo.ca_private_data_len = 0;
    ret = Aml_MP_CAS_StartDVRRecord(mCasSession, &casServiceInfo);
    if (ret < 0) {
        MLOGE("start DVRRecord failed with %d", ret);
        return ret;
    }

    encryptParams.cryptoData = mCasSession;
    encryptParams.cryptoFn = [](Aml_MP_CASCryptoParams* params, void* userdata) {
        AML_MP_CASSESSION casSession = (AML_MP_CASSESSION)userdata;
        return Aml_MP_CAS_DVREncrypt(casSession, params);
    };

    uint8_t* secBuf = nullptr;
    uint32_t secBufSize;
    mSecMem = Aml_MP_CAS_CreateSecmem(mCasSession, AML_MP_CAS_SERVICE_PVR_RECORDING, (void**)&secBuf, &secBufSize);
    if (mSecMem == nullptr) {
        MLOGE("create secMem failed");
        return -1;
    }

    encryptParams.secureBuffer = secBuf;
    encryptParams.secureBufferSize = secBufSize;
    #endif
    return 0;
}

int DVRRecord::uninitDVREncryptRecord()
{
    #ifdef ANDROID
    MLOG("mCasSession:%p", mCasSession);
    if (mCasSession == AML_MP_INVALID_HANDLE) {
        return 0;
    }

    Aml_MP_CAS_DestroySecmem(mCasSession, mSecMem);
    mSecMem = nullptr;

    Aml_MP_CAS_StopDVRRecord(mCasSession);

    Aml_MP_CAS_CloseSession(mCasSession);
    mCasSession = nullptr;
    #endif
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
static struct TestModule::Command g_commandTable[] = {

    {nullptr, 0, nullptr, nullptr}
};

const TestModule::Command* DVRRecord::getCommandTable() const
{
    return g_commandTable;
}

void* DVRRecord::getCommandHandle() const
{
    return mRecorder;
}


}
