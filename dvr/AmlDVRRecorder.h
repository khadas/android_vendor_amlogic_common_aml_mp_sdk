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

#ifndef _AML_DVR_RECODER_H_
#define _AML_DVR_RECODER_H_

#include <Aml_MP/Dvr.h>
#include <utils/RefBase.h>
#include <utils/AmlMpHandle.h>
#include <utils/AmlMpUtils.h>

namespace aml_mp {

class AmlDVRRecorder final : public AmlMpHandle
{
public:
    AmlDVRRecorder(Aml_MP_DVRRecorderBasicParams* basicParams, Aml_MP_DVRRecorderTimeShiftParams* timeShiftParams = nullptr, Aml_MP_DVRRecorderEncryptParams* encryptParams = nullptr);
    ~AmlDVRRecorder();
    int registerEventCallback(Aml_MP_DVRRecorderEventCallback cb, void* userData);
    int setStreams(Aml_MP_DVRStreamArray* streams);
    int start();
    int stop();
    int getStatus(Aml_MP_DVRRecorderStatus* status);

private:
    int setBasicParams(Aml_MP_DVRRecorderBasicParams* basicParams);
    int setTimeShiftParams(Aml_MP_DVRRecorderTimeShiftParams* timeShiftParams);
    int setEncryptParams(Aml_MP_DVRRecorderEncryptParams* encryptParams);

    DVR_Result_t eventHandler(DVR_RecordEvent_t event, void* params);

    char mName[50];
    DVR_WrapperRecordOpenParams_t mRecOpenParams{};
    DVR_WrapperRecordStartParams_t mRecStartParams{};
    DVR_WrapperRecord_t mRecoderHandle = nullptr;

    bool mIsEncryptStream;
    uint8_t* mSecureBuffer = nullptr;
    size_t mSecureBufferSize = 0;

    DVR_WrapperPidsInfo_t mRecordPids;

    Aml_MP_DVRRecorderEventCallback mEventCb = nullptr;
    void* mEventUserData;

private:
    AmlDVRRecorder(const AmlDVRRecorder&) = delete;
    AmlDVRRecorder& operator= (const AmlDVRRecorder&) = delete;
};

}
#endif
