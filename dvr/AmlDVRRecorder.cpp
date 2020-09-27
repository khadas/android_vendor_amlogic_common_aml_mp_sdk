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
static DVR_RecordPidAction_t convertToDVRPidAction(Aml_MP_DVRRecorderStreamAction action);
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

    if (timeShiftParams != nullptr) {
        setTimeShiftParams(timeShiftParams);
    }

    if (encryptParams != nullptr) {
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

    mRecOpenParams.event_fn = (DVR_RecordEventFunction_t)cb;
    mRecOpenParams.event_userdata = userData;

    return 0;
}

int AmlDVRRecorder::setStreams(Aml_MP_DVRRecorderStreams* streams)
{
    MLOG("nbStreams:%d", streams->streamArray.nbStreams);

    DVR_StreamPid_t pids[DVR_MAX_RECORD_PIDS_COUNT];
    for (int i = 0; i < streams->streamArray.nbStreams; ++i) {
        ALOGI("streamType:%d(%s), codecId:%d(%s), pid:%d", streams->streamArray.streams[i].type,
                mpStreamType2Str(streams->streamArray.streams[i].type),
                streams->streamArray.streams[i].codecId,
                mpCodecId2Str(streams->streamArray.streams[i].codecId),
                streams->streamArray.streams[i].pid);

        switch (streams->streamArray.streams[i].type) {
        case AML_MP_STREAM_TYPE_VIDEO:
            pids[i].type =  (DVR_StreamType_t)(convertToDVRStreamType(streams->streamArray.streams[i].type) << 24 |
                convertToDVRVideoFormat(streams->streamArray.streams[i].codecId));
            break;

        case AML_MP_STREAM_TYPE_AUDIO:
        case AML_MP_STREAM_TYPE_AD:
            pids[i].type =  (DVR_StreamType_t)(convertToDVRStreamType(streams->streamArray.streams[i].type) << 24 |
                convertToDVRAudioFormat(streams->streamArray.streams[i].codecId));
            break;

        default:
            pids[i].type = (DVR_StreamType_t)(convertToDVRStreamType(streams->streamArray.streams[i].type) << 24);
            break;
        }

        pids[i].pid = streams->streamArray.streams[i].pid;
    }

    if (!mRecoderHandle) {
        //start play
        mRecStartParams.pids_info.nb_pids = streams->streamArray.nbStreams;
        memcpy(mRecStartParams.pids_info.pids, pids, sizeof(mRecStartParams.pids_info.pids));
    } else {
        DVR_WrapperUpdatePidsParams_t updatePidParams;
        updatePidParams.nb_pids = streams->streamArray.nbStreams;
        memcpy(updatePidParams.pids, pids, sizeof(updatePidParams.pids));

        for (int i = 0; i < updatePidParams.nb_pids; ++i) {
            updatePidParams.pid_action[i] = convertToDVRPidAction(streams->streamActions[i]);
        }

        int ret =dvr_wrapper_update_record_pids(mRecoderHandle, &updatePidParams);
        if (ret < 0) {
            ALOGE("Update record pids fail");
            return -1;
        }
    }

    return 0;
}

int AmlDVRRecorder::start()
{
    ALOGI("Call DVRRecorderStart");
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

///////////////////////////////////////////////////////////////////////////////
DVR_RecordPidAction_t convertToDVRPidAction(Aml_MP_DVRRecorderStreamAction action)
{
    switch (action) {
    case AML_MP_DVRRECORDER_STREAM_CREATE:
        return DVR_RECORD_PID_CREATE;

    case AML_MP_DVRRECORDER_STREAM_KEEP:
        return DVR_RECORD_PID_KEEP;

    case AML_MP_DVRRECORDER_STREAM_CLOSE:
        return DVR_RECORD_PID_CLOSE;
    }
}

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
