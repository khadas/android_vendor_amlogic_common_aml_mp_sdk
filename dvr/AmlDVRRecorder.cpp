/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_TAG "AmlDVRRecorder"
#define KEEP_ALOGX
#include <utils/AmlMpLog.h>
#include "AmlDVRRecorder.h"
#include <Aml_MP/Dvr.h>
#include <utils/AmlMpHandle.h>
#include <cutils/properties.h>
#include <dvr_utils.h>
#include <utils/AmlMpUtils.h>

namespace aml_mp {
static Aml_MP_DVRRecorderState convertToMpDVRRecordState(DVR_RecordState_t state);

///////////////////////////////////////////////////////////////////////////////
AmlDVRRecorder::AmlDVRRecorder(Aml_MP_DVRRecorderBasicParams* basicParams, Aml_MP_DVRRecorderTimeShiftParams* timeShiftParams, Aml_MP_DVRRecorderEncryptParams* encryptParams)
{
    snprintf(mName, sizeof(mName), "%s", LOG_TAG);
    MLOG();

    memset(&mRecOpenParams, 0, sizeof(DVR_WrapperRecordOpenParams_t));
    memset(&mRecStartParams, 0, sizeof(DVR_WrapperRecordStartParams_t));
    setBasicParams(basicParams);

    mIsEncryptStream = basicParams->flags & AML_MP_DVRRECORDER_SCRAMBLED;

    if (basicParams->isTimeShift) {
        setTimeShiftParams(timeShiftParams);
    }

    if (mIsEncryptStream) {
        setEncryptParams(encryptParams);
    }
}

AmlDVRRecorder::~AmlDVRRecorder()
{
    MLOG();
}

int AmlDVRRecorder::registerEventCallback(Aml_MP_DVRRecorderEventCallback cb, void* userData)
{
    AML_MP_TRACE(10);

    mEventCb = cb;
    mEventUserData = userData;

    return 0;
}

int AmlDVRRecorder::setStreams(Aml_MP_DVRStreamArray* streams)
{
    ALOGI("Recording PIDs:");

    int count = 0;
    DVR_StreamPid_t pids[DVR_MAX_RECORD_PIDS_COUNT];
    for (int i = 0; i < streams->nbStreams; ++i) {
        ALOGI("streamType:%d(%s), codecId:%d(%s), pid:%d", streams->streams[i].type,
                mpStreamType2Str(streams->streams[i].type),
                streams->streams[i].codecId,
                mpCodecId2Str(streams->streams[i].codecId),
                streams->streams[i].pid);

        switch (streams->streams[i].type) {
        case AML_MP_STREAM_TYPE_VIDEO:
            pids[count].type =  (DVR_StreamType_t)(convertToDVRStreamType(streams->streams[i].type) << 24 |
                convertToDVRVideoFormat(streams->streams[i].codecId));
            pids[count].pid = streams->streams[i].pid;
            count++;
            ALOGI("  VIDEO %d", streams->streams[i].pid);
            break;

        case AML_MP_STREAM_TYPE_AUDIO:
        case AML_MP_STREAM_TYPE_AD:
            pids[count].type =  (DVR_StreamType_t)(convertToDVRStreamType(streams->streams[i].type) << 24 |
                convertToDVRAudioFormat(streams->streams[i].codecId));
            pids[count].pid = streams->streams[i].pid;
            count++;
            ALOGI("  AUDIO %d", streams->streams[i].pid);
            break;

        case AML_MP_STREAM_TYPE_SUBTITLE:
            pids[count].type = (DVR_StreamType_t)(convertToDVRStreamType(streams->streams[i].type) << 24);
            pids[count].pid = streams->streams[i].pid;
            count++;
            ALOGI("  SUBTITLE %d", streams->streams[i].pid);
            break;

        case AML_MP_STREAM_TYPE_TELETEXT:
            pids[count].type = (DVR_StreamType_t)(convertToDVRStreamType(streams->streams[i].type) << 24);
            pids[count].pid = streams->streams[i].pid;
            count++;
            ALOGI("  TELETEXT %d", streams->streams[i].pid);
            break;

        case AML_MP_STREAM_TYPE_SECTION:
            pids[count].type = (DVR_StreamType_t)(convertToDVRStreamType(streams->streams[i].type) << 24);
            pids[count].pid = streams->streams[i].pid;
            count++;
            ALOGI("  SECTION %d", streams->streams[i].pid);
            break;

        default:
            ALOGI("  Not recording %d, type %s", streams->streams[i].pid, mpStreamType2Str(streams->streams[i].type));
            break;
        }
    }

    MLOG("request nbStreams:%d, actual count:%d", streams->nbStreams, count);

    //start play
    if (!mRecoderHandle) {
        mRecStartParams.pids_info.nb_pids = count;
        memcpy(mRecStartParams.pids_info.pids, pids, sizeof(mRecStartParams.pids_info.pids));
    } else {
        DVR_WrapperUpdatePidsParams_t updatePidParams;
        updatePidParams.nb_pids = count;
        memcpy(updatePidParams.pids, pids, sizeof(updatePidParams.pids));
        for (int i = 0; i < updatePidParams.nb_pids; ++i) {
            updatePidParams.pid_action[i] = DVR_RECORD_PID_CREATE;
        }

        for (int i = 0; i < mRecordPids.nb_pids; ++i) {
            bool found = false;

            for (int j = 0; j < updatePidParams.nb_pids; ++j) {
                if (updatePidParams.pids[j].pid == mRecordPids.pids[i].pid) {
                    found = true;
                    updatePidParams.pid_action[j] = DVR_RECORD_PID_KEEP;
                    ALOGI("keep %d", mRecordPids.pids[i].pid);

                    break;
                }
            }

            if (!found) {
                //not found this pid ,so need del this pid.
                if (updatePidParams.nb_pids < AML_MP_DVR_STREAMS_COUNT-1) {
                    updatePidParams.pid_action[updatePidParams.nb_pids] = DVR_RECORD_PID_CLOSE;
                    updatePidParams.pids[updatePidParams.nb_pids].pid = mRecordPids.pids[i].pid;
                    updatePidParams.pids[updatePidParams.nb_pids].type = mRecordPids.pids[i].type;
                    updatePidParams.nb_pids++;
                    ALOGI("close %d", mRecordPids.pids[i].pid);
                }
            }
        }

        int ret =dvr_wrapper_update_record_pids(mRecoderHandle, &updatePidParams);
        if (ret < 0) {
            ALOGE("Update record pids fail");

            //if update pids failed, we don't update mRecordPids.
            return -1;
        }
    }

    //save current pids info
    mRecordPids.nb_pids = count;
    memcpy(mRecordPids.pids, pids, sizeof(mRecordPids.pids));

    return 0;
}

int AmlDVRRecorder::start()
{
    ALOGI("Call DVRRecorderStart");

    mRecOpenParams.event_fn = [] (DVR_RecordEvent_t event, void* params, void* userData) {
        AmlDVRRecorder* recorder = static_cast<AmlDVRRecorder*>(userData);
        return recorder->eventHandler(event, params);
    };
    mRecOpenParams.event_userdata = this;

    int ret = dvr_wrapper_open_record(&mRecoderHandle, &mRecOpenParams);
    if (ret < 0) {
        ALOGE("Open dvr record fail");
        return -1;
    }

    if (mIsEncryptStream) {
        ALOGI("set secureBuffer:%p, secureBufferSize:%d", mSecureBuffer, mSecureBufferSize);
        dvr_wrapper_set_record_secure_buffer(mRecoderHandle, mSecureBuffer, mSecureBufferSize);
    }

    if (mRecStartParams.pids_info.nb_pids > 0) {
        ret = dvr_wrapper_start_record(mRecoderHandle, &mRecStartParams);
        if (ret < 0) {
            dvr_wrapper_close_record(mRecoderHandle);
            ALOGE("Failed to start recording.");
            return -1;
        }
    } else {
        ALOGE("Need set start params before start");
        ret = 0;
    }

    return 0;
}

int AmlDVRRecorder::stop()
{
    ALOGI("Call DVRRecorderStop");

    int ret = dvr_wrapper_stop_record(mRecoderHandle);
    if (ret) {
        ALOGI("Stop recoder fail");
    }
    //Add support cas
    ret = dvr_wrapper_close_record(mRecoderHandle);

    return ret;
}

int AmlDVRRecorder::getStatus(Aml_MP_DVRRecorderStatus* status)
{
    DVR_WrapperRecordStatus_t dvrStatus;
    int ret = dvr_wrapper_get_record_status(mRecoderHandle, &dvrStatus);
    if (ret < 0) {
        ALOGW("get record status failed!");
        return -1;
    }

    status->state = convertToMpDVRRecordState(dvrStatus.state);
    convertToMpDVRSourceInfo(&status->info, &dvrStatus.info);
    status->streams.nbStreams = dvrStatus.pids.nb_pids;
    for (int i = 0; i < status->streams.nbStreams; i++) {
        convertToMpDVRStream(&status->streams.streams[i], &dvrStatus.pids.pids[i]);
    }
    convertToMpDVRSourceInfo(&status->infoObsolete, &dvrStatus.info_obsolete);

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
int AmlDVRRecorder::setBasicParams(Aml_MP_DVRRecorderBasicParams* basicParams)
{
    mRecOpenParams.fend_dev_id = basicParams->fend_dev_id;
    mRecOpenParams.dmx_dev_id = basicParams->demuxId;
    memcpy(&(mRecOpenParams.location), &(basicParams->location), DVR_MAX_LOCATION_SIZE);
    mRecOpenParams.segment_size = basicParams->segmentSize;
    mRecOpenParams.flags = (DVR_RecordFlag_t)basicParams->flags;
    mRecOpenParams.flush_size = basicParams->bufferSize;
    ALOGI("location:%s", basicParams->location);

    return 0;
}

int AmlDVRRecorder::setTimeShiftParams(Aml_MP_DVRRecorderTimeShiftParams* timeShiftParams)
{
    mRecOpenParams.max_size = timeShiftParams->maxSize;
    mRecOpenParams.max_time = timeShiftParams->maxTime;
    mRecOpenParams.is_timeshift = DVR_TRUE;

    return 0;
}

int AmlDVRRecorder::setEncryptParams(Aml_MP_DVRRecorderEncryptParams* encryptParams)
{
    mRecOpenParams.crypto_period.interval_bytes = encryptParams->intervalBytes;
    mRecOpenParams.crypto_period.notify_clear_periods = encryptParams->notifyClearPeriods;
    mRecOpenParams.crypto_fn = (DVR_CryptoFunction_t)encryptParams->cryptoFn;
    mRecOpenParams.crypto_data = encryptParams->cryptoData;

    mSecureBuffer = encryptParams->secureBuffer;
    mSecureBufferSize = encryptParams->secureBufferSize;

    return 0;
}

DVR_Result_t AmlDVRRecorder::eventHandler(DVR_RecordEvent_t event, void* params)
{
    DVR_Result_t ret = DVR_SUCCESS;

    switch (event) {
    case DVR_RECORD_EVENT_ERROR:
        if (mEventCb)  mEventCb(mEventUserData, AML_MP_DVRRECORDER_EVENT_ERROR, (int64_t)params);
        break;

    case DVR_RECORD_EVENT_STATUS:
        if (mEventCb)  mEventCb(mEventUserData, AML_MP_DVRRECORDER_EVENT_STATUS, (int64_t)params);
        break;

    case DVR_RECORD_EVENT_SYNC_END:
        if (mEventCb)  mEventCb(mEventUserData, AML_MP_DVRRECORDER_EVENT_SYNC_END, (int64_t)params);
        break;

    case DVR_RECORD_EVENT_CRYPTO_STATUS:
        if (mEventCb)  mEventCb(mEventUserData, AML_MP_DVRRECORDER_EVENT_CRYPTO_STATUS, (int64_t)params);
        break;

    case DVR_RECORD_EVENT_WRITE_ERROR:
        if (mEventCb)  mEventCb(mEventUserData, AML_MP_DVRRECORDER_EVENT_WRITE_ERROR, (int64_t)params);
        break;

    default:
        MLOG("unhandled event:%d", event);
        break;
    }

    return ret;
}

///////////////////////////////////////////////////////////////////////////////
Aml_MP_DVRRecorderState convertToMpDVRRecordState(DVR_RecordState_t state)
{
    switch (state) {
    case DVR_RECORD_STATE_OPENED:
        return AML_MP_DVRRECORDER_STATE_OPENED;

    case DVR_RECORD_STATE_STARTED:
        return AML_MP_DVRRECORDER_STATE_STARTED;

    case DVR_RECORD_STATE_STOPPED:
        return AML_MP_DVRRECORDER_STATE_STOPPED;

    case DVR_RECORD_STATE_CLOSED:
    default:
        return AML_MP_DVRRECORDER_STATE_CLOSED;
    }
}


}
